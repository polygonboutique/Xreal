/*
=======================================================================================================================================
Parts of this file are based on the ETPub source code under GPL.
http:// trac2.assembla.com / etpub / browser / trunk / src / game / g_lua.h
rev 170 + rev 192
Code by quad and pheno

Ported to Xreal by Andrew "DerSaidin" Browne.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Spearmint Source Code.
If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms. You should have received a copy of these additional
terms immediately following the terms and conditions of the GNU General Public License. If not, please request a copy in writing from
id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
=======================================================================================================================================
*/
// g_lua.h--header game lua

#ifndef _G_LUA_H
#define _G_LUA_H

#include "g_local.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define LUA_NUM_VM 16
#define LUA_MAX_FSIZE 1024 * 1024	// 1MB

// define HOSTARCH and EXTENSION depending on host architecture
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__sun)

#define HOSTARCH	"UNIX"
#define EXTENSION	"so"

#elif defined WIN32

#define HOSTARCH	"WIN32"
#define EXTENSION	"dll"

#endif

// macros to register predefined constants
#define lua_registerglobal(L, n, v)(lua_pushstring(L, v), lua_setglobal(L, n))
#define lua_regconstinteger(L, n)(lua_pushstring(L, #n), lua_pushinteger(L, n), lua_settable(L, -3))
#define lua_regconststring(L, n)(lua_pushstring(L, #n), lua_pushstring(L, n), lua_settable(L, -3))

typedef enum {
	VMT_GAMESCRIPT,
	VMT_MAPSCRIPT
} vmType_t;

typedef struct {
	int id;
	char file_name[MAX_QPATH];
	char mod_name[MAX_CVAR_VALUE_STRING];
	char mod_signature[41];
	char *code;
	int code_size;
	int err;
	vmType_t type;
	lua_State      * L;
} lua_vm_t;
extern lua_vm_t *lVM[LUA_NUM_VM];

// API
void QDECL DEBUG_LUA(const char *fmt, ...);
void QDECL LOG(const char *fmt, ...);
qboolean G_LuaInit();
qboolean G_LuaCall(lua_vm_t *vm, char *func, int nargs, int nresults);
qboolean G_LuaGetNamedFunction(lua_vm_t *vm, char *name);
qboolean G_LuaStartVM(lua_vm_t *vm);
void G_LuaStopVM(lua_vm_t *vm);
void G_LuaShutdown();
void G_LuaStatus(gentity_t *ent);
lua_vm_t *G_LuaGetVM(lua_State * L);


// 
// lua_entity.c
// 
typedef struct {
	gentity_t *e;
} lua_Entity;

int luaopen_entity(lua_State * L);
void lua_pushentity(lua_State * L, gentity_t *ent);
lua_Entity     * lua_getentity(lua_State * L, int argNum);

// 
// lua_game.c
// 
int luaopen_game(lua_State * L);

// 
// lua_qmath.c
// 
int luaopen_qmath(lua_State * L);

// 
// lua_vector.c
// 
int luaopen_vector(lua_State * L);
void lua_pushvector(lua_State * L, vec3_t v);
vec_t *lua_getvector(lua_State * L, int argNum);
int lua_isvector(lua_State * L, int idx);
vec3_t *lua_getvectormisc(lua_State * L, int *idx);

// 
// lua_mover.c
// 
int luaopen_mover(lua_State * L);

// 
// lua_et.c
// 
int luaopen_et(lua_State * L);

#endif							/* ifndef _G_LUA_H */
