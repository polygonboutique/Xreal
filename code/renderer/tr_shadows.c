/*
=======================================================================================================================================
Copyright (C) 1999 - 2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

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

/*
=======================================================================================================================================

  For a projection shadow:

  point[x] += light vector * (z - shadow plane)
  point[y] +=
  point[z] = shadow plane

  1 0 light[x] / light[z]

=======================================================================================================================================
*/

shadowState_t shadowState;

/*
=======================================================================================================================================
RB_ProjectionShadowDeform
=======================================================================================================================================
*/
void RB_ProjectionShadowDeform(void) {
	float *xyz;
	int i;
	float h;
	vec3_t ground;
	vec3_t light;
	float groundDist;
	float d;
	vec3_t lightDir;

	xyz = (float *)tess.xyz;

	ground[0] = backEnd.orientation.axis[0][2];
	ground[1] = backEnd.orientation.axis[1][2];
	ground[2] = backEnd.orientation.axis[2][2];

	groundDist = backEnd.orientation.origin[2] - backEnd.currentEntity->e.shadowPlane;

	VectorCopy(backEnd.currentEntity->lightDir, lightDir);

	d = DotProduct(lightDir, ground);
	// don't let the shadows get too long or go negative
	if (d < 0.5) {
		VectorMA(lightDir, (0.5 - d), ground, lightDir);
		d = DotProduct(lightDir, ground);
	}

	d = 1.0 / d;

	light[0] = lightDir[0] * d;
	light[1] = lightDir[1] * d;
	light[2] = lightDir[2] * d;

	for (i = 0; i < tess.numVertexes; i++, xyz += 4) {
		h = DotProduct(xyz, ground) + groundDist;

		xyz[0] -= light[0] * h;
		xyz[1] -= light[1] * h;
		xyz[2] -= light[2] * h;
	}
}
