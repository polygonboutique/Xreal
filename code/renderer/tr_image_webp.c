/*
=======================================================================================================================================
Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>

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

#include "tr_local.h"
#ifdef USE_WEBP
#include <webp/decode.h>

/*
=======================================================================================================================================

LoadWEBP

=======================================================================================================================================
*/

void LoadWEBP(const char *filename, unsigned char **pic, int *width, int *height, byte alphaByte) {
	byte *out;
	int len;
	int stride;
	int size;
	union
	{
		byte *b;
		void *v;
	} fbuffer;

	/*read compressed data*/
	len = ri.FS_ReadFile((char *)filename, &fbuffer.v);

	if (!fbuffer.b || len < 0) {
		return;
	}

	/*validate data and query image size*/
	if (!WebPGetInfo(fbuffer.b, len, width, height))
		return;

	stride = *width * sizeof(color4ub_t);
	size = *height * stride;

	out = ri.Z_Malloc(size);

	if (!WebPDecodeRGBAInto(fbuffer.b, len, out, size, stride)){
		ri.Free(out);
		return;
	}

	ri.FS_FreeFile(fbuffer.v);
	*pic = out;
}
#endif
