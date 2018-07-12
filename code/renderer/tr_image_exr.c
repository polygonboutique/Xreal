/*
=======================================================================================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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

#ifdef __cplusplus
extern "C"
{
#endif
//#include "tr_local.h"
#include <q_shared.h>
#ifdef __cplusplus
}
#endif

//#include <OpenEXR / half.h>
#include "../openexr/half.h"

#ifdef __cplusplus
extern          "C"
{
#endif

void LoadRGBEToFloats(const char *name, float ** pic, int *width, int *height, qboolean doGamma, qboolean toneMap, qboolean compensate);

void LoadRGBEToHalfs(const char *name, unsigned short * * halfImage, int *width, int *height) {
	int i, j;
	int w, h;
	float *hdrImage;
	float *floatbuf;
	unsigned short * halfbuf;

#if 0
	w = h = 0;
	LoadRGBEToFloats(name, &hdrImage, &w, &h, qtrue, qtrue, qtrue);

	*width = w;
	*height = h;

	*ldrImage = ri.Malloc(w * h * 4);
	pixbuf = *ldrImage;

	floatbuf = hdrImage;

	for (i = 0; i < (w * h); i++) {
		for (j = 0; j < 3; j++) {
			sample[j] = *floatbuf++;
		}

		NormalizeColor(sample, sample);

		*pixbuf++ = (byte)(sample[0] * 255);
		*pixbuf++ = (byte)(sample[1] * 255);
		*pixbuf++ = (byte)(sample[2] * 255);
		*pixbuf++ = (byte)255;
	}
#else
	w = h = 0;
	LoadRGBEToFloats(name, &hdrImage, &w, &h, qtrue, qfalse, qtrue);

	*width = w;
	*height = h;

	*halfImage = (unsigned short *)Com_Allocate(w * h * 3 * 6);

	halfbuf = *halfImage;
	floatbuf = hdrImage;

	for (i = 0; i < (w * h); i++) {
		for (j = 0; j < 3; j++) {
			half sample(*floatbuf++);
			*halfbuf++ = sample.bits();
		}
	}
#endif

	Com_Dealloc(hdrImage);
}

#ifdef __cplusplus
}
#endif
