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

// null system driver to aid porting efforts

#include <errno.h>
#include <stdio.h>
#include "../qcommon/qcommon.h"

int sys_curtime;

/*
=======================================================================================================================================
Sys_BeginStreamedFile
=======================================================================================================================================
*/
void Sys_BeginStreamedFile(FILE *f, int readAhead) {

}

/*
=======================================================================================================================================
Sys_EndStreamedFile
=======================================================================================================================================
*/
void Sys_EndStreamedFile(FILE *f) {

}

/*
=======================================================================================================================================
Sys_StreamedRead
=======================================================================================================================================
*/
int Sys_StreamedRead(void *buffer, int size, int count, FILE *f) {
	return fread(buffer, size, count, f);
}

/*
=======================================================================================================================================
Sys_StreamSeek
=======================================================================================================================================
*/
void Sys_StreamSeek(FILE *f, int offset, int origin) {
	fseek(f, offset, origin);
}

/*
=======================================================================================================================================
Sys_mkdir
=======================================================================================================================================
*/
void Sys_mkdir(const char *path) {
}

/*
=======================================================================================================================================
Sys_Error
=======================================================================================================================================
*/
void Sys_Error(char *error, ...) {
	va_list argptr;

	printf("Sys_Error: ");
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);
	printf("\n");
	exit(1);
}

/*
=======================================================================================================================================
Sys_Quit
=======================================================================================================================================
*/
void Sys_Quit(void) {
	exit(0);
}

/*
=======================================================================================================================================
Sys_UnloadGame
=======================================================================================================================================
*/
void Sys_UnloadGame(void) {
}

/*
=======================================================================================================================================
Sys_GetGameAPI
=======================================================================================================================================
*/
void *Sys_GetGameAPI(void *parms) {
	return NULL;
}

/*
=======================================================================================================================================
Sys_GetClipboardData
=======================================================================================================================================
*/
char *Sys_GetClipboardData(void) {
	return NULL;
}

/*
=======================================================================================================================================
Sys_Milliseconds
=======================================================================================================================================
*/
int Sys_Milliseconds(void) {
	return 0;
}

/*
=======================================================================================================================================
Sys_Milliseconds
=======================================================================================================================================
*/
void Sys_Mkdir(char *path) {
}

/*
=======================================================================================================================================
Sys_FindFirst
=======================================================================================================================================
*/
char *Sys_FindFirst(char *path, unsigned musthave, unsigned canthave) {
	return NULL;
}

/*
=======================================================================================================================================
Sys_FindNext
=======================================================================================================================================
*/
char *Sys_FindNext(unsigned musthave, unsigned canthave) {
	return NULL;
}

/*
=======================================================================================================================================
Sys_FindClose
=======================================================================================================================================
*/
void Sys_FindClose(void) {

}

/*
=======================================================================================================================================
Sys_Init
=======================================================================================================================================
*/
void Sys_Init(void) {

}

/*
=======================================================================================================================================
Sys_EarlyOutput
=======================================================================================================================================
*/
void Sys_EarlyOutput(char *string) {
	printf("%s", string);
}

/*
=======================================================================================================================================
main
=======================================================================================================================================
*/
void main(int argc, char **argv) {

	Com_Init(argc, argv);

	while (1) {
		Com_Frame();
	}
}
