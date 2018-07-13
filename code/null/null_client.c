/*
=======================================================================================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

#include "../client/client.h"

cvar_t *cl_shownet;

/*
=======================================================================================================================================
CL_Shutdown
=======================================================================================================================================
*/
void CL_Shutdown(char *finalmsg) {

}

/*
=======================================================================================================================================
CL_Init
=======================================================================================================================================
*/
void CL_Init(void) {
	cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);
}

/*
=======================================================================================================================================
CL_MouseEvent
=======================================================================================================================================
*/
void CL_MouseEvent(int dx, int dy, int time) {

}

/*
=======================================================================================================================================
Key_WriteBindings
=======================================================================================================================================
*/
void Key_WriteBindings(fileHandle_t f) {

}

/*
=======================================================================================================================================
CL_Frame
=======================================================================================================================================
*/
void CL_Frame(int msec) {

}

/*
=======================================================================================================================================
CL_PacketEvent
=======================================================================================================================================
*/
void CL_PacketEvent(netadr_t from, msg_t *msg) {

}

/*
=======================================================================================================================================
CL_CharEvent
=======================================================================================================================================
*/
void CL_CharEvent(int key) {

}

/*
=======================================================================================================================================
CL_Disconnect
=======================================================================================================================================
*/
void CL_Disconnect(qboolean showMainMenu) {

}

/*
=======================================================================================================================================
CL_MapLoading
=======================================================================================================================================
*/
void CL_MapLoading(void) {

}

/*
=======================================================================================================================================
CL_GameCommand
=======================================================================================================================================
*/
qboolean CL_GameCommand(void) {
	return qfalse;
}

/*
=======================================================================================================================================
CL_KeyEvent
=======================================================================================================================================
*/
void CL_KeyEvent(int key, qboolean down, unsigned time) {

}

/*
=======================================================================================================================================
UI_GameCommand
=======================================================================================================================================
*/
qboolean UI_GameCommand(void) {
	return qfalse;
}

/*
=======================================================================================================================================
CL_ForwardCommandToServer
=======================================================================================================================================
*/
void CL_ForwardCommandToServer(const char *string) {

}

/*
=======================================================================================================================================
CL_ConsolePrint
=======================================================================================================================================
*/
void CL_ConsolePrint(char *txt) {

}

/*
=======================================================================================================================================
CL_JoystickEvent
=======================================================================================================================================
*/
void CL_JoystickEvent(int axis, int value, int time) {

}

/*
=======================================================================================================================================
CL_InitKeyCommands
=======================================================================================================================================
*/
void CL_InitKeyCommands(void) {

}

/*
=======================================================================================================================================
CL_FlushMemory
=======================================================================================================================================
*/
void CL_FlushMemory(void) {

}

/*
=======================================================================================================================================
CL_StartHunkUsers
=======================================================================================================================================
*/
void CL_StartHunkUsers(qboolean rendererOnly) {

}

/*
=======================================================================================================================================
CL_Snd_Restart
=======================================================================================================================================
*/
void CL_Snd_Restart(void) {

}

/*
=======================================================================================================================================
CL_ShutdownAll
=======================================================================================================================================
*/
void CL_ShutdownAll(void) {

}

