/*
=======================================================================================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

// all other sound mixing is portable

#include "../client/client.h"

/*
=======================================================================================================================================
SNDDMA_Init
=======================================================================================================================================
*/
qboolean SNDDMA_Init(void) {
	return qfalse;
}

/*
=======================================================================================================================================
SNDDMA_GetDMAPos
=======================================================================================================================================
*/
int SNDDMA_GetDMAPos(void) {
	return 0;
}

/*
=======================================================================================================================================
SNDDMA_Shutdown
=======================================================================================================================================
*/
void SNDDMA_Shutdown(void) {

}

/*
=======================================================================================================================================
SNDDMA_BeginPainting
=======================================================================================================================================
*/
void SNDDMA_BeginPainting(void) {

}

/*
=======================================================================================================================================
SNDDMA_Submit
=======================================================================================================================================
*/
void SNDDMA_Submit(void) {

}

/*
=======================================================================================================================================
S_RegisterSound
=======================================================================================================================================
*/
sfxHandle_t S_RegisterSound(const char *sample) {
	return 0;
}

/*
=======================================================================================================================================
S_StartLocalSound
=======================================================================================================================================
*/
void S_StartLocalSound(sfxHandle_t sfxHandle, int channelNum) {

}

/*
=======================================================================================================================================
S_ClearSoundBuffer
=======================================================================================================================================
*/
void S_ClearSoundBuffer(void) {

}
