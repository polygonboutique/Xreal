/*
===========================================================================
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// lua_entity.c -- entity library for Lua


#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "g_local.h"

static lua_Entity *entity_Get(lua_State * L)
{
	void           *ud;

	ud = luaL_checkudata(L, 1, "game.entity");
	luaL_argcheck(L, ud != NULL, 1, "`entity' expected");
	return (lua_Entity *) ud;
}

static int entity_Spawn(lua_State * L)
{
	lua_Entity     *lent;

	lent = lua_newuserdata(L, sizeof(lua_Entity));

	luaL_getmetatable(L, "game.entity");
	lua_setmetatable(L, -2);
	
	lent->e = G_Spawn();

	return 1;
}

static int entity_GetNumber(lua_State * L)
{
	lua_Entity     *lent;

	lent = entity_Get(L);
	lua_pushnumber(L, lent->e - g_entities);
//	lua_pushnumber(L, lent->e->s.number);

	return 1;
}

static int entity_IsClient(lua_State * L)
{
	lua_Entity     *lent;

	lent = entity_Get(L);
	lua_pushboolean(L, lent->e->client != NULL);

	return 1;
}

static int entity_GetClientName(lua_State * L)
{
	lua_Entity     *lent;

	lent = entity_Get(L);
	lua_pushstring(L, lent->e->client->pers.netname);

	return 1;
}

static int entity_Print(lua_State * L)
{
	lua_Entity     *lent;
	int             i;
	char            buf[MAX_STRING_CHARS];
	int             n = lua_gettop(L);	// number of arguments
	
	lent = entity_Get(L);
	if(!lent->e->client)
		return luaL_error(L, "`Print' must be used with a client entity");

	memset(buf, 0, sizeof(buf));

	lua_getglobal(L, "tostring");
	for(i = 2; i <= n; i++)
	{
		const char     *s;

		lua_pushvalue(L, -1);	// function to be called
		lua_pushvalue(L, i);	// value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);	// get result
		
		if(s == NULL)
			return luaL_error(L, "`tostring' must return a string to `print'");
		
		Q_strcat(buf, sizeof(buf), s);
		
		lua_pop(L, 1);			// pop result
	}
	
	trap_SendServerCommand(lent->e - g_entities, va("print \"%s\n\"", buf));
	
	return 0;
}

static int entity_CenterPrint(lua_State * L)
{
	lua_Entity     *lent;
	int             i;
	char            buf[MAX_STRING_CHARS];
	int             n = lua_gettop(L);	// number of arguments
	
	lent = entity_Get(L);
	if(!lent->e->client)
		return luaL_error(L, "`CenterPrint' must be used with a client entity");

	memset(buf, 0, sizeof(buf));

	lua_getglobal(L, "tostring");
	for(i = 2; i <= n; i++)
	{
		const char     *s;

		lua_pushvalue(L, -1);	// function to be called
		lua_pushvalue(L, i);	// value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);	// get result
		
		if(s == NULL)
			return luaL_error(L, "`tostring' must return a string to `print'");
		
		Q_strcat(buf, sizeof(buf), s);
		
		lua_pop(L, 1);			// pop result
	}
	
	trap_SendServerCommand(lent->e - g_entities, va("cp \"" S_COLOR_WHITE "%s\n\"", buf));
	
	return 0;
}

static int entity_GetClassName(lua_State * L)
{
	lua_Entity     *lent;

	lent = entity_Get(L);
	lua_pushstring(L, lent->e->classname);

	return 1;
}

static int entity_SetClassName(lua_State * L)
{
	lua_Entity     *lent;
//	char           *classname;

	lent = entity_Get(L);
	lent->e->classname = (char *) luaL_checkstring(L, 2);
	
//	lent->e->classname = classname;

	return 1;
}

static int entity_GetTargetName(lua_State * L)
{
	lua_Entity     *lent;

	lent = entity_Get(L);
	lua_pushstring(L, lent->e->targetname);

	return 1;
}

static int entity_GC(lua_State * L)
{
//	G_Printf("Lua says bye to entity = %p\n", entity_Get(L));
	
	return 0;
}

static int entity_ToString(lua_State * L)
{
	lua_Entity     *lent;
	gentity_t      *gent;
	char            buf[MAX_STRING_CHARS];
	
	lent = entity_Get(L);
	gent = lent->e;
	Com_sprintf(buf, sizeof(buf), "entity: class=%s name=%s id=%i pointer=%p\n", gent->classname, gent->targetname, gent - g_entities, gent);
	lua_pushstring(L, buf);
	
	return 1;
}

static const luaL_reg entity_ctor[] = {
	{"Spawn", entity_Spawn},
	{NULL, NULL}
};

static const luaL_reg entity_meta[] = {
	{"__gc", entity_GC},
	{"__tostring", entity_ToString},
	{"GetNumber", entity_GetNumber},
	{"IsClient", entity_IsClient},
	{"GetClientName", entity_GetClientName},
	{"Print", entity_Print},
	{"CenterPrint", entity_CenterPrint},
	{"GetClassName", entity_GetClassName},
	{"SetClassName", entity_SetClassName},
	{"GetTargetName", entity_GetTargetName},
	{NULL, NULL}
};

int luaopen_entity(lua_State * L)
{
	luaL_newmetatable(L, "game.entity");
	
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  // pushes the metatable
	lua_settable(L, -3);  // metatable.__index = metatable
	
	luaL_register(L, NULL, entity_meta);
	luaL_register(L, "entity", entity_ctor);

	return 1;
}
