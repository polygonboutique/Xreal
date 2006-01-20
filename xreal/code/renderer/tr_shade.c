/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

enum
{
	ATTR_INDEX_TEXCOORD0	= 8,
	ATTR_INDEX_TEXCOORD1	= 9,
	ATTR_INDEX_TEXCOORD2	= 10,
	ATTR_INDEX_TEXCOORD3	= 11,
	ATTR_INDEX_TANGENT		= 12,
	ATTR_INDEX_BINORMAL		= 13,
//  ATTR_INDEX_NORMAL       = 2,
	ATTR_INDEX_DELUXEL		= 14,
//  ATTR_INDEX_COLOR        = 3
};


static char    *RB_PrintInfoLog(GLhandleARB object)
{
	static char     msg[4096];
	int             maxLength = 0;

	qglGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);

	if(maxLength >= (int)sizeof(msg))
	{
		ri.Error(ERR_DROP, "RB_PrintInfoLog: max length >= sizeof(msg)");
		return NULL;
	}

	qglGetInfoLogARB(object, maxLength, &maxLength, msg);

	return msg;
}

char    *RB_PrintShaderSource(GLhandleARB object)
{
	static char     msg[4096];
	int             maxLength = 0;
	
	qglGetObjectParameterivARB(object, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &maxLength);

	if(maxLength >= (int)sizeof(msg))
	{
		ri.Error(ERR_DROP, "RB_PrintShaderSource: max length >= sizeof(msg)");
		return NULL;
	}

	qglGetShaderSourceARB(object, maxLength, &maxLength, msg);

	return msg;
}

static void RB_LoadGPUShader(GLhandleARB program, const char *name, GLenum shaderType)
{

	char            filename[MAX_QPATH];
	GLcharARB      *buffer = NULL;
	int             size;
	GLint           compiled;
	GLhandleARB     shader;

	if(shaderType == GL_VERTEX_SHADER_ARB)
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_vp.glsl", name);
	}
	else
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_fp.glsl", name);
	}

	ri.Printf(PRINT_ALL, "...loading '%s'\n", filename);
	size = ri.FS_ReadFile(filename, (void **)&buffer);
	if(!buffer)
	{
		ri.Error(ERR_DROP, "Couldn't load %s", filename);
	}

	shader = qglCreateShaderObjectARB(shaderType);
	qglShaderSourceARB(shader, 1, (const GLcharARB **)&buffer, &size);

	// compile shader
	qglCompileShaderARB(shader);

	// check if shader compiled
	qglGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
	if(!compiled)
	{
		ri.Error(ERR_DROP, "Couldn't compile %s", RB_PrintInfoLog(shader));
		ri.FS_FreeFile(buffer);
		return;
	}
//	ri.Printf(PRINT_ALL, "info log: %s\n", RB_PrintInfoLog(shader));
//	ri.Printf(PRINT_ALL, "%s\n", RB_PrintShaderSource(shader));

	// attach shader to program
	qglAttachObjectARB(program, shader);

	// delete shader, no longer needed
	qglDeleteObjectARB(shader);

	ri.FS_FreeFile(buffer);
}

static void RB_LinkProgram(GLhandleARB program)
{
	GLint           linked;

	qglLinkProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
	if(!linked)
	{
		ri.Error(ERR_DROP, "%s\nshaders failed to link", RB_PrintInfoLog(program));
	}
}

static void RB_ValidateProgram(GLhandleARB program)
{

	GLint           validated;

	qglValidateProgramARB(program);

	qglGetObjectParameterivARB(program, GL_OBJECT_VALIDATE_STATUS_ARB, &validated);
	if(!validated)
	{
		ri.Error(ERR_DROP, "%s\nshaders failed to validate", RB_PrintInfoLog(program));
	}
}

void RB_ShowProgramUniforms(GLhandleARB program)
{
	int				i, count, size, type;
	char			uniformName[1000];
	
	// install the executables in the program object as part of current state.
	qglUseProgramObjectARB(program);

	// check for GL Errors

	// query the number of active uniforms
	qglGetObjectParameterivARB(program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &count);

	// Loop over each of the active uniforms, and set their value
	for(i = 0; i < count; i++)
	{
		qglGetActiveUniformARB(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);
		
		ri.Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
	}
	
	qglUseProgramObjectARB(0);
}

static void RB_InitGPUShader(shaderProgram_t * program, const char *name, int attribs, qboolean fragmentShader)
{
	
	ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

	program->program = qglCreateProgramObjectARB();
	program->attribs = attribs;

	RB_LoadGPUShader(program->program, name, GL_VERTEX_SHADER_ARB);
	
	if(fragmentShader)
		RB_LoadGPUShader(program->program, name, GL_FRAGMENT_SHADER_ARB);

//  if( attribs & ATTRVERTEX )
//      qglBindAttribLocationARB( program->program, ATTR_INDEX_VERTEX, "attr_Vertex");

	if(attribs & GLCS_TEXCOORD0)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0");

	if(attribs & GLCS_TEXCOORD1)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1");
	
	if(attribs & GLCS_TEXCOORD2)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");
	
	if(attribs & GLCS_TEXCOORD3)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

	if(attribs & GLCS_TANGENT)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_TANGENT, "attr_Tangent");

	if(attribs & GLCS_BINORMAL)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_BINORMAL, "attr_Binormal");

//  if( attribs & GLCS_NORMAL )
//      qglBindAttribLocationARB( program->program, ATTR_INDEX_NORMAL, "attr_Color");

	if(attribs & GLCS_DELUXEL)
		qglBindAttribLocationARB(program->program, ATTR_INDEX_DELUXEL, "attr_Deluxel");

//  if( attribs & GLCS_COLOR )
//      qglBindAttribLocationARB( program->program, ATTR_INDEX_COLOR, "attr_Color");

	RB_LinkProgram(program->program);
	RB_ValidateProgram(program->program);
	RB_ShowProgramUniforms(program->program);
}

void RB_InitGPUShaders(void)
{
	ri.Printf(PRINT_ALL, "------- RB_InitGPUShaders -------\n");

	if(!glConfig2.shadingLanguage100Available)
		return;
	
	//
	// single texture rendering
	//
	RB_InitGPUShader(&tr.genericShader_single,
					  "generic_single",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_COLOR, qtrue);

	tr.genericShader_single.u_ColorMap =
			qglGetUniformLocationARB(tr.genericShader_single.program, "u_ColorMap");

	qglUseProgramObjectARB(tr.genericShader_single.program);
	qglUniform1iARB(tr.genericShader_single.u_ColorMap, 0);
	qglUseProgramObjectARB(0);

	//
	// directional lighting ( Q3A style )
	//
	RB_InitGPUShader(&tr.lightShader_D_direct,
					 "lighting_D_direct",
					 GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_NORMAL, qtrue);

	tr.lightShader_D_direct.u_DiffuseMap =
		qglGetUniformLocationARB(tr.lightShader_D_direct.program, "u_DiffuseMap");
	tr.lightShader_D_direct.u_AmbientColor =
		qglGetUniformLocationARB(tr.lightShader_D_direct.program, "u_AmbientColor");
	tr.lightShader_D_direct.u_LightDir =
		qglGetUniformLocationARB(tr.lightShader_D_direct.program, "u_LightDir");
	tr.lightShader_D_direct.u_LightColor =
		qglGetUniformLocationARB(tr.lightShader_D_direct.program, "u_LightColor");

	qglUseProgramObjectARB(tr.lightShader_D_direct.program);
	qglUniform1iARB(tr.lightShader_D_direct.u_DiffuseMap, 0);
	qglUseProgramObjectARB(0);

	//
	// directional bump mapping
	//
	RB_InitGPUShader(&tr.lightShader_DB_direct,
					 "lighting_DB_direct",
					 GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL, qtrue);

	tr.lightShader_DB_direct.u_DiffuseMap =
		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_DiffuseMap");
	tr.lightShader_DB_direct.u_NormalMap =
		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_NormalMap");
	tr.lightShader_DB_direct.u_AmbientColor =
		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_AmbientColor");
	tr.lightShader_DB_direct.u_LightDir =
		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_LightDir");
	tr.lightShader_DB_direct.u_LightColor =
		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_LightColor");
//	tr.lightShader_DB_direct.u_BumpScale =
//		qglGetUniformLocationARB(tr.lightShader_DB_direct.program, "u_BumpScale");

	qglUseProgramObjectARB(tr.lightShader_DB_direct.program);
	qglUniform1iARB(tr.lightShader_DB_direct.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DB_direct.u_NormalMap, 1);
	qglUseProgramObjectARB(0);
	
	//
	// directional specular bump mapping
	//
	RB_InitGPUShader(&tr.lightShader_DBS_direct,
					 "lighting_DBS_direct",
					 GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TEXCOORD2 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL,
					 qtrue);

	tr.lightShader_DBS_direct.u_DiffuseMap =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_DiffuseMap");
	tr.lightShader_DBS_direct.u_NormalMap =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_NormalMap");
	tr.lightShader_DBS_direct.u_SpecularMap =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_SpecularMap");
	tr.lightShader_DBS_direct.u_ViewOrigin =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_ViewOrigin");
	tr.lightShader_DBS_direct.u_AmbientColor =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_AmbientColor");
	tr.lightShader_DBS_direct.u_LightDir =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_LightDir");
	tr.lightShader_DBS_direct.u_LightColor =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_LightColor");
	tr.lightShader_DBS_direct.u_SpecularExponent =
		qglGetUniformLocationARB(tr.lightShader_DBS_direct.program, "u_SpecularExponent");

	qglUseProgramObjectARB(tr.lightShader_DBS_direct.program);
	qglUniform1iARB(tr.lightShader_DBS_direct.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DBS_direct.u_NormalMap, 1);
	qglUniform1iARB(tr.lightShader_DBS_direct.u_SpecularMap, 2);
	qglUseProgramObjectARB(0);
	
	//
	// omni-directional lighting ( Doom3 style )
	//
	RB_InitGPUShader(&tr.lightShader_D_omni,
					  "lighting_D_omni",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_NORMAL, qtrue);

	tr.lightShader_D_omni.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_D_omni.program, "u_DiffuseMap");
	tr.lightShader_D_omni.u_AttenuationMapXY =
			qglGetUniformLocationARB(tr.lightShader_D_omni.program, "u_AttenuationMapXY");
	tr.lightShader_D_omni.u_AttenuationMapZ =
			qglGetUniformLocationARB(tr.lightShader_D_omni.program, "u_AttenuationMapZ");
	tr.lightShader_D_omni.u_LightOrigin =
			qglGetUniformLocationARB(tr.lightShader_D_omni.program, "u_LightOrigin");
	tr.lightShader_D_omni.u_LightColor =
			qglGetUniformLocationARB(tr.lightShader_D_omni.program, "u_LightColor");

	qglUseProgramObjectARB(tr.lightShader_D_omni.program);
	qglUniform1iARB(tr.lightShader_D_omni.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_D_omni.u_AttenuationMapXY, 1);
	qglUniform1iARB(tr.lightShader_D_omni.u_AttenuationMapZ, 2);
	qglUseProgramObjectARB(0);
	
	//
	// omni-directional bump mapping ( Doom3 style )
	//
	RB_InitGPUShader(&tr.lightShader_DB_omni,
					  "lighting_DB_omni",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL, qtrue);

	tr.lightShader_DB_omni.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_DiffuseMap");
	tr.lightShader_DB_omni.u_NormalMap =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_NormalMap");
	tr.lightShader_DB_omni.u_AttenuationMapXY =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_AttenuationMapXY");
	tr.lightShader_DB_omni.u_AttenuationMapZ =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_AttenuationMapZ");
	tr.lightShader_DB_omni.u_LightOrigin =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_LightOrigin");
	tr.lightShader_DB_omni.u_LightColor =
			qglGetUniformLocationARB(tr.lightShader_DB_omni.program, "u_LightColor");

	qglUseProgramObjectARB(tr.lightShader_DB_omni.program);
	qglUniform1iARB(tr.lightShader_DB_omni.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DB_omni.u_NormalMap, 1);
	qglUniform1iARB(tr.lightShader_DB_omni.u_AttenuationMapXY, 2);
	qglUniform1iARB(tr.lightShader_DB_omni.u_AttenuationMapZ, 3);
	qglUseProgramObjectARB(0);
	
	//
	// omni-directional specular bump mapping ( Doom3 style )
	//
	RB_InitGPUShader(&tr.lightShader_DBS_omni,
					  "lighting_DBS_omni",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TEXCOORD2 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL, qtrue);

	tr.lightShader_DBS_omni.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_DiffuseMap");
	tr.lightShader_DBS_omni.u_NormalMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_NormalMap");
	tr.lightShader_DBS_omni.u_SpecularMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_SpecularMap");
	tr.lightShader_DBS_omni.u_AttenuationMapXY =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_AttenuationMapXY");
	tr.lightShader_DBS_omni.u_AttenuationMapZ =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_AttenuationMapZ");
	tr.lightShader_DBS_omni.u_ViewOrigin =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_ViewOrigin");
	tr.lightShader_DBS_omni.u_LightOrigin =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_LightOrigin");
	tr.lightShader_DBS_omni.u_LightColor =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_LightColor");
	tr.lightShader_DBS_omni.u_SpecularExponent =
			qglGetUniformLocationARB(tr.lightShader_DBS_omni.program, "u_SpecularExponent");

	qglUseProgramObjectARB(tr.lightShader_DBS_omni.program);
	qglUniform1iARB(tr.lightShader_DBS_omni.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DBS_omni.u_NormalMap, 1);
	qglUniform1iARB(tr.lightShader_DBS_omni.u_SpecularMap, 2);
	qglUniform1iARB(tr.lightShader_DBS_omni.u_AttenuationMapXY, 3);
	qglUniform1iARB(tr.lightShader_DBS_omni.u_AttenuationMapZ, 4);
	qglUseProgramObjectARB(0);
	
	//
	// radiosity lighting ( Q3A style )
	//
	RB_InitGPUShader(&tr.lightShader_D_radiosity,
					  "lighting_D_radiosity",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD3 | GLCS_NORMAL, qtrue);

	tr.lightShader_D_radiosity.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_D_radiosity.program, "u_DiffuseMap");
	tr.lightShader_D_radiosity.u_LightMap =
			qglGetUniformLocationARB(tr.lightShader_D_radiosity.program, "u_LightMap");

	qglUseProgramObjectARB(tr.lightShader_D_radiosity.program);
	qglUniform1iARB(tr.lightShader_D_radiosity.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_D_radiosity.u_LightMap, 1);
	qglUseProgramObjectARB(0);
	
	//
	// radiosity bump mapping
	//
	RB_InitGPUShader(&tr.lightShader_DB_radiosity,
					  "lighting_DB_radiosity",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TEXCOORD3 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL, qtrue);

	tr.lightShader_DB_radiosity.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_DB_radiosity.program, "u_DiffuseMap");
	tr.lightShader_DB_radiosity.u_NormalMap =
			qglGetUniformLocationARB(tr.lightShader_DB_radiosity.program, "u_NormalMap");
	tr.lightShader_DB_radiosity.u_LightMap =
			qglGetUniformLocationARB(tr.lightShader_DB_radiosity.program, "u_LightMap");
	tr.lightShader_DB_radiosity.u_DeluxeMap =
			qglGetUniformLocationARB(tr.lightShader_DB_radiosity.program, "u_DeluxeMap");

	qglUseProgramObjectARB(tr.lightShader_DB_radiosity.program);
	qglUniform1iARB(tr.lightShader_DB_radiosity.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DB_radiosity.u_NormalMap, 1);
	qglUniform1iARB(tr.lightShader_DB_radiosity.u_LightMap, 2);
	qglUniform1iARB(tr.lightShader_DB_radiosity.u_DeluxeMap, 3);
	qglUseProgramObjectARB(0);
	
	//
	// radiosity specular bump mapping
	//
	RB_InitGPUShader(&tr.lightShader_DBS_radiosity,
					  "lighting_DBS_radiosity",
					  GLCS_VERTEX | GLCS_TEXCOORD0 | GLCS_TEXCOORD1 | GLCS_TEXCOORD2 | GLCS_TEXCOORD3 | GLCS_TANGENT | GLCS_BINORMAL | GLCS_NORMAL, qtrue);

	tr.lightShader_DBS_radiosity.u_DiffuseMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_DiffuseMap");
	tr.lightShader_DBS_radiosity.u_NormalMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_NormalMap");
	tr.lightShader_DBS_radiosity.u_SpecularMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_SpecularMap");
	tr.lightShader_DBS_radiosity.u_LightMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_LightMap");
	tr.lightShader_DBS_radiosity.u_DeluxeMap =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_DeluxeMap");
	tr.lightShader_DBS_radiosity.u_ViewOrigin =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_ViewOrigin");
	tr.lightShader_DBS_radiosity.u_SpecularExponent =
			qglGetUniformLocationARB(tr.lightShader_DBS_radiosity.program, "u_SpecularExponent");

	qglUseProgramObjectARB(tr.lightShader_DBS_radiosity.program);
	qglUniform1iARB(tr.lightShader_DBS_radiosity.u_DiffuseMap, 0);
	qglUniform1iARB(tr.lightShader_DBS_radiosity.u_NormalMap, 1);
	qglUniform1iARB(tr.lightShader_DBS_radiosity.u_SpecularMap, 2);
	qglUniform1iARB(tr.lightShader_DBS_radiosity.u_LightMap, 3);
	qglUniform1iARB(tr.lightShader_DBS_radiosity.u_DeluxeMap, 4);
	qglUseProgramObjectARB(0);
	
	//
	// cubemap reflection for abitrary polygons
	//
	RB_InitGPUShader(&tr.reflectionShader_C,
					  "reflection_C",
					  GLCS_VERTEX | GLCS_NORMAL, qtrue);

	tr.reflectionShader_C.u_ColorMap =
			qglGetUniformLocationARB(tr.reflectionShader_C.program, "u_ColorMap");
	tr.reflectionShader_C.u_ViewOrigin =
			qglGetUniformLocationARB(tr.reflectionShader_C.program, "u_ViewOrigin");

	qglUseProgramObjectARB(tr.reflectionShader_C.program);
	qglUniform1iARB(tr.reflectionShader_C.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// cubemap refraction for abitrary polygons
	//
	RB_InitGPUShader(&tr.refractionShader_C,
					  "refraction_C",
					  GLCS_VERTEX | GLCS_NORMAL, qtrue);

	tr.reflectionShader_C.u_ColorMap =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_ColorMap");
	tr.refractionShader_C.u_ViewOrigin =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_ViewOrigin");
	tr.refractionShader_C.u_RefractionIndex =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_RefractionIndex");
	tr.refractionShader_C.u_FresnelPower =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_FresnelPower");
	tr.refractionShader_C.u_FresnelScale =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_FresnelScale");
	tr.refractionShader_C.u_FresnelBias =
			qglGetUniformLocationARB(tr.refractionShader_C.program, "u_FresnelBias");

	qglUseProgramObjectARB(tr.refractionShader_C.program);
	qglUniform1iARB(tr.refractionShader_C.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// cubemap dispersion for abitrary polygons
	//
	RB_InitGPUShader(&tr.dispersionShader_C,
					  "dispersion_C",
					  GLCS_VERTEX | GLCS_NORMAL, qtrue);

	tr.reflectionShader_C.u_ColorMap =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_ColorMap");
	tr.dispersionShader_C.u_ViewOrigin =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_ViewOrigin");
	tr.dispersionShader_C.u_EtaRatio =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_EtaRatio");
	tr.dispersionShader_C.u_FresnelPower =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_FresnelPower");
	tr.dispersionShader_C.u_FresnelScale =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_FresnelScale");
	tr.dispersionShader_C.u_FresnelBias =
			qglGetUniformLocationARB(tr.dispersionShader_C.program, "u_FresnelBias");

	qglUseProgramObjectARB(tr.dispersionShader_C.program);
	qglUniform1iARB(tr.dispersionShader_C.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// skybox drawing for abitrary polygons
	//
	RB_InitGPUShader(&tr.skyBoxShader,
					  "skybox",
					  GLCS_VERTEX | GLCS_NORMAL, qtrue);

	tr.skyBoxShader.u_ColorMap =
			qglGetUniformLocationARB(tr.skyBoxShader.program, "u_ColorMap");
	tr.skyBoxShader.u_ViewOrigin =
			qglGetUniformLocationARB(tr.skyBoxShader.program, "u_ViewOrigin");

	qglUseProgramObjectARB(tr.skyBoxShader.program);
	qglUniform1iARB(tr.skyBoxShader.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// heatHaze post process effect
	//
	RB_InitGPUShader(&tr.heatHazeShader,
					  "heatHaze",
					  GLCS_VERTEX | GLCS_TEXCOORD0, qtrue);

	tr.heatHazeShader.u_DeformMagnitude =
			qglGetUniformLocationARB(tr.heatHazeShader.program, "u_DeformMagnitude");
	tr.heatHazeShader.u_ColorMap =
			qglGetUniformLocationARB(tr.heatHazeShader.program, "u_ColorMap");
	tr.heatHazeShader.u_NormalMap =
			qglGetUniformLocationARB(tr.heatHazeShader.program, "u_NormalMap");
	tr.heatHazeShader.u_FBufScale =
			qglGetUniformLocationARB(tr.heatHazeShader.program, "u_FBufScale");
	tr.heatHazeShader.u_NPotScale =
			qglGetUniformLocationARB(tr.heatHazeShader.program, "u_NPotScale");

	qglUseProgramObjectARB(tr.heatHazeShader.program);
	qglUniform1iARB(tr.heatHazeShader.u_ColorMap, 0);
	qglUniform1iARB(tr.heatHazeShader.u_NormalMap, 1);
	qglUseProgramObjectARB(0);
	
	//
	// glow post process effect
	//
	RB_InitGPUShader(&tr.glowShader,
					  "glow",
					  GLCS_VERTEX | GLCS_TEXCOORD0, qtrue);

	tr.glowShader.u_ColorMap =
			qglGetUniformLocationARB(tr.glowShader.program, "u_ColorMap");
	tr.glowShader.u_FBufScale =
			qglGetUniformLocationARB(tr.glowShader.program, "u_FBufScale");
	tr.glowShader.u_NPotScale =
			qglGetUniformLocationARB(tr.glowShader.program, "u_NPotScale");
	tr.glowShader.u_BlurMagnitude =
			qglGetUniformLocationARB(tr.glowShader.program, "u_BlurMagnitude");

	qglUseProgramObjectARB(tr.glowShader.program);
	qglUniform1iARB(tr.glowShader.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// bloom post process effect
	//
	RB_InitGPUShader(&tr.bloomShader,
					  "bloom",
					  GLCS_VERTEX, qtrue);

	tr.bloomShader.u_ColorMap =
			qglGetUniformLocationARB(tr.bloomShader.program, "u_ColorMap");
	tr.bloomShader.u_ContrastMap =
			qglGetUniformLocationARB(tr.bloomShader.program, "u_ContrastMap");
	tr.bloomShader.u_FBufScale =
			qglGetUniformLocationARB(tr.bloomShader.program, "u_FBufScale");
	tr.bloomShader.u_NPotScale =
			qglGetUniformLocationARB(tr.bloomShader.program, "u_NPotScale");
	tr.bloomShader.u_BlurMagnitude =
			qglGetUniformLocationARB(tr.bloomShader.program, "u_BlurMagnitude");

	qglUseProgramObjectARB(tr.bloomShader.program);
	qglUniform1iARB(tr.bloomShader.u_ColorMap, 0);
	qglUniform1iARB(tr.bloomShader.u_ContrastMap, 1);
	qglUseProgramObjectARB(0);
	
	//
	// contrast post process effect
	//
	RB_InitGPUShader(&tr.contrastShader,
					  "contrast",
					  GLCS_VERTEX, qtrue);

	tr.contrastShader.u_ColorMap =
			qglGetUniformLocationARB(tr.contrastShader.program, "u_ColorMap");
	tr.contrastShader.u_FBufScale =
			qglGetUniformLocationARB(tr.contrastShader.program, "u_FBufScale");
	tr.contrastShader.u_NPotScale =
			qglGetUniformLocationARB(tr.contrastShader.program, "u_NPotScale");

	qglUseProgramObjectARB(tr.contrastShader.program);
	qglUniform1iARB(tr.contrastShader.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// blurX post process effect
	//
	RB_InitGPUShader(&tr.blurXShader,
					  "blurX",
					  GLCS_VERTEX, qtrue);

	tr.blurXShader.u_ColorMap =
			qglGetUniformLocationARB(tr.blurXShader.program, "u_ColorMap");
	tr.blurXShader.u_FBufScale =
			qglGetUniformLocationARB(tr.blurXShader.program, "u_FBufScale");
	tr.blurXShader.u_NPotScale =
			qglGetUniformLocationARB(tr.blurXShader.program, "u_NPotScale");

	qglUseProgramObjectARB(tr.blurXShader.program);
	qglUniform1iARB(tr.blurXShader.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
	
	//
	// blurY post process effect
	//
	RB_InitGPUShader(&tr.blurYShader,
					  "blurY",
					  GLCS_VERTEX, qtrue);

	tr.blurYShader.u_ColorMap =
			qglGetUniformLocationARB(tr.blurYShader.program, "u_ColorMap");
	tr.blurYShader.u_FBufScale =
			qglGetUniformLocationARB(tr.blurYShader.program, "u_FBufScale");
	tr.blurYShader.u_NPotScale =
			qglGetUniformLocationARB(tr.blurYShader.program, "u_NPotScale");

	qglUseProgramObjectARB(tr.blurYShader.program);
	qglUniform1iARB(tr.blurYShader.u_ColorMap, 0);
	qglUseProgramObjectARB(0);
}

void RB_ShutdownGPUShaders(void)
{
	ri.Printf(PRINT_ALL, "------- RB_ShutdownGPUShaders -------\n");

	if(!glConfig2.shadingLanguage100Available)
		return;
	
	GL_Program(0);
	
	if(tr.genericShader_single.program)
	{
		qglDeleteObjectARB(tr.genericShader_single.program);
		tr.genericShader_single.program = 0;
	}

	if(tr.lightShader_D_direct.program)
	{
		qglDeleteObjectARB(tr.lightShader_D_direct.program);
		tr.lightShader_D_direct.program = 0;
	}

	if(tr.lightShader_DB_direct.program)
	{
		qglDeleteObjectARB(tr.lightShader_DB_direct.program);
		tr.lightShader_DB_direct.program = 0;
	}
	
	if(tr.lightShader_DBS_direct.program)
	{
		qglDeleteObjectARB(tr.lightShader_DBS_direct.program);
		tr.lightShader_DBS_direct.program = 0;
	}
	
	if(tr.lightShader_D_omni.program)
	{
		qglDeleteObjectARB(tr.lightShader_D_omni.program);
		tr.lightShader_D_omni.program = 0;
	}
	
	if(tr.lightShader_DB_omni.program)
	{
		qglDeleteObjectARB(tr.lightShader_DB_omni.program);
		tr.lightShader_DB_omni.program = 0;
	}
	
	if(tr.lightShader_DBS_omni.program)
	{
		qglDeleteObjectARB(tr.lightShader_DBS_omni.program);
		tr.lightShader_DBS_omni.program = 0;
	}
	
	if(tr.lightShader_D_radiosity.program)
	{
		qglDeleteObjectARB(tr.lightShader_D_radiosity.program);
		tr.lightShader_D_radiosity.program = 0;
	}
	
	if(tr.lightShader_DB_radiosity.program)
	{
		qglDeleteObjectARB(tr.lightShader_DB_radiosity.program);
		tr.lightShader_DB_radiosity.program = 0;
	}
	
	if(tr.lightShader_DBS_radiosity.program)
	{
		qglDeleteObjectARB(tr.lightShader_DBS_radiosity.program);
		tr.lightShader_DBS_radiosity.program = 0;
	}
	
	if(tr.reflectionShader_C.program)
	{
		qglDeleteObjectARB(tr.reflectionShader_C.program);
		tr.reflectionShader_C.program = 0;
	}
	
	if(tr.refractionShader_C.program)
	{
		qglDeleteObjectARB(tr.refractionShader_C.program);
		tr.refractionShader_C.program = 0;
	}
	
	if(tr.dispersionShader_C.program)
	{
		qglDeleteObjectARB(tr.dispersionShader_C.program);
		tr.dispersionShader_C.program = 0;
	}
	
	if(tr.skyBoxShader.program)
	{
		qglDeleteObjectARB(tr.skyBoxShader.program);
		tr.skyBoxShader.program = 0;
	}
	
	if(tr.heatHazeShader.program)
	{
		qglDeleteObjectARB(tr.heatHazeShader.program);
		tr.heatHazeShader.program = 0;
	}
	
	if(tr.glowShader.program)
	{
		qglDeleteObjectARB(tr.glowShader.program);
		tr.glowShader.program = 0;
	}
	
	if(tr.bloomShader.program)
	{
		qglDeleteObjectARB(tr.bloomShader.program);
		tr.bloomShader.program = 0;
	}
	
	if(tr.contrastShader.program)
	{
		qglDeleteObjectARB(tr.contrastShader.program);
		tr.contrastShader.program = 0;
	}
	
	if(tr.blurXShader.program)
	{
		qglDeleteObjectARB(tr.blurXShader.program);
		tr.blurXShader.program = 0;
	}
	
	if(tr.blurYShader.program)
	{
		qglDeleteObjectARB(tr.blurYShader.program);
		tr.blurYShader.program = 0;
	}
}

void GL_ClientState(unsigned long stateBits)
{
	unsigned long   diff = stateBits ^ glState.glClientStateBits;

	if(!diff)
	{
		return;
	}
	
	/*
	if(diff & GLCS_VERTEX)
		qglEnableClientState(GL_VERTEX_ARRAY);
	else
		qglDisableClientState(GL_VERTEX_ARRAY);
	*/
	
	if(diff & GLCS_TEXCOORD0)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD0);

	if(diff & GLCS_TEXCOORD1)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD1);
	
	if(diff & GLCS_TEXCOORD2)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD2);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD2);
	
	if(diff & GLCS_TEXCOORD3)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD3);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_TEXCOORD3);

	if(diff & GLCS_TANGENT)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_TANGENT);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_TANGENT);

	if(diff & GLCS_BINORMAL)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_BINORMAL);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_BINORMAL);

	if(diff & GLCS_NORMAL)
		qglEnableClientState(GL_NORMAL_ARRAY);
	else
		qglDisableClientState(GL_NORMAL_ARRAY);

	if(diff & GLCS_DELUXEL)
		qglEnableVertexAttribArrayARB(ATTR_INDEX_DELUXEL);
	else
		qglDisableVertexAttribArrayARB(ATTR_INDEX_DELUXEL);

	if(diff & GLCS_COLOR)
		qglEnableClientState(GL_COLOR_ARRAY);
	else
		qglDisableClientState(GL_COLOR_ARRAY);
	
	glState.glClientStateBits = stateBits;
}


static void GL_SetVertexAttribs()
{
	/*
	   if( glConfig2.vertexBufferObjectAvailable && input->vbo_array_buffer ) {
	   TODO
	   }
	   else 


	   if(gl_config.arb_vertex_buffer_object) {
	   gl_state.current_vbo_array_buffer = 0;
	   gl_state.current_vbo_vertexes_ofs = 0;

	   qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	   }
	 */

//	if(attribs & GLCS_VERTEX)
//		qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);
	
	if(glState.glClientStateBits & GLCS_TEXCOORD0)
		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD0, 2, GL_FLOAT, 0, 0, tess.svars.texCoords[TB_COLORMAP]);

	if(glState.glClientStateBits & GLCS_TEXCOORD1)
		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD1, 2, GL_FLOAT, 0, 0, tess.svars.texCoords[TB_NORMALMAP]);
	
	if(glState.glClientStateBits & GLCS_TEXCOORD2)
		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD2, 2, GL_FLOAT, 0, 0, tess.svars.texCoords[TB_SPECULARMAP]);
	
	if(glState.glClientStateBits & GLCS_TEXCOORD3)
		qglVertexAttribPointerARB(ATTR_INDEX_TEXCOORD3, 2, GL_FLOAT, 0, 0, tess.svars.texCoords[TB_LIGHTMAP]);

	if(glState.glClientStateBits & GLCS_TANGENT)
		qglVertexAttribPointerARB(ATTR_INDEX_TANGENT, 3, GL_FLOAT, 0, 16, tess.tangents);

	if(glState.glClientStateBits & GLCS_BINORMAL)
		qglVertexAttribPointerARB(ATTR_INDEX_BINORMAL, 3, GL_FLOAT, 0, 16, tess.binormals);

	if(glState.glClientStateBits & GLCS_NORMAL)
		qglNormalPointer(GL_FLOAT, 16, tess.normals);

	if(glState.glClientStateBits & GLCS_DELUXEL)
		qglVertexAttribPointerARB(ATTR_INDEX_DELUXEL, 3, GL_FLOAT, 0, 16, tess.deluxels);

	if(glState.glClientStateBits & GLCS_COLOR)
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, tess.svars.colors);
}


/*
================
R_ArrayElementDiscrete

This is just for OpenGL conformance testing, it should never be the fastest
================
*/
static void APIENTRY R_ArrayElementDiscrete(GLint index)
{
	qglColor4ubv(tess.svars.colors[index]);
	if(glState.currenttmu)
	{
		qglMultiTexCoord2fARB(0, tess.svars.texCoords[0][index][0], tess.svars.texCoords[0][index][1]);
		qglMultiTexCoord2fARB(1, tess.svars.texCoords[1][index][0], tess.svars.texCoords[1][index][1]);
	}
	else
	{
		qglTexCoord2fv(tess.svars.texCoords[0][index]);
	}
	qglVertex3fv(tess.xyz[index]);
}


/*
===================
R_DrawStripElements

===================
*/
static int      c_vertexes;		// for seeing how long our average strips are
static int      c_begins;
static void R_DrawStripElements(int numIndexes, const glIndex_t * indexes, void (APIENTRY * element) (GLint))
{
	int             i;
	int             last[3] = { -1, -1, -1 };
	qboolean        even;

	c_begins++;

	if(numIndexes <= 0)
	{
		return;
	}

	qglBegin(GL_TRIANGLE_STRIP);

	// prime the strip
	element(indexes[0]);
	element(indexes[1]);
	element(indexes[2]);
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for(i = 3; i < numIndexes; i += 3)
	{
		// odd numbered triangle in potential strip
		if(!even)
		{
			// check previous triangle to see if we're continuing a strip
			if((indexes[i + 0] == last[2]) && (indexes[i + 1] == last[1]))
			{
				element(indexes[i + 2]);
				c_vertexes++;
				assert(indexes[i + 2] < tess.numVertexes);
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin(GL_TRIANGLE_STRIP);
				c_begins++;

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if((last[2] == indexes[i + 1]) && (last[0] == indexes[i + 0]))
			{
				element(indexes[i + 2]);
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin(GL_TRIANGLE_STRIP);
				c_begins++;

				element(indexes[i + 0]);
				element(indexes[i + 1]);
				element(indexes[i + 2]);
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i + 0];
		last[1] = indexes[i + 1];
		last[2] = indexes[i + 2];
	}

	qglEnd();
}




/*
==================
R_DrawElements

Optionally performs our own glDrawElements that looks for strip conditions
instead of using the single glDrawElements call that may be inefficient
without compiled vertex arrays.
==================
*/
static void R_DrawElements(int numIndexes, const glIndex_t * indexes)
{
	int             primitives;

	primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if(primitives == 0)
	{
		if(qglLockArraysEXT)
		{
			primitives = 2;
		}
		else
		{
			primitives = 1;
		}
	}


	if(primitives == 2)
	{
		qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, indexes);
		return;
	}

	if(primitives == 1)
	{
		R_DrawStripElements(numIndexes, indexes, qglArrayElement);
		return;
	}

	if(primitives == 3)
	{
		R_DrawStripElements(numIndexes, indexes, R_ArrayElementDiscrete);
		return;
	}

	// anything else will cause no drawing
}



/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;


/*
=================
R_BindAnimatedImage
=================
*/
static void R_BindAnimatedImage(textureBundle_t * bundle)
{
	int             index;

	if(bundle->isVideoMap)
	{
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if(bundle->numImageAnimations <= 1)
	{
		GL_Bind(bundle->image[0]);
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = myftol(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if(index < 0)
	{
		index = 0;				// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind(bundle->image[index]);
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris(shaderCommands_t * input)
{
	GL_Program(0);
	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
	
	switch (input->currentStageIteratorType)
	{
		case SIT_ZFILL:
			qglColor3f(1, 1, 1);
			break;
			
		case SIT_LIGHTING:
			qglColor3f(1, 0, 0);
			break;
			
		case SIT_FOG:
			qglColor3f(0, 0, 1);
			break;
		
		default:
		case SIT_DEFAULT:
			qglColor3f(1, 1, 1);
			break;
	}
	
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
	GL_ClientState(GLCS_VERTEX);
	qglDepthRange(0, 0);

	qglVertexPointer(3, GL_FLOAT, 16, input->xyz);	// padded for SIMD

	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}

	R_DrawElements(input->numIndexes, input->indexes);

	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}
	qglDepthRange(0, 1);
}


/*
================
DrawTangentSpaces

Draws vertex tangent spaces for debugging
================
*/
static void DrawTangentSpaces(shaderCommands_t * input)
{
	int             i;
	vec3_t          temp;

	GL_Program(0);
	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
	qglDepthRange(0, 0);		// never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	qglBegin(GL_LINES);
	for(i = 0; i < input->numVertexes; i++)
	{
		qglColor3f(1, 0, 0);
		qglVertex3fv(input->xyz[i]);
		VectorMA(input->xyz[i], 2, input->tangents[i], temp);
		qglVertex3fv(temp);

		qglColor3f(0, 1, 0);
		qglVertex3fv(input->xyz[i]);
		VectorMA(input->xyz[i], 2, input->binormals[i], temp);
		qglVertex3fv(temp);

		qglColor3f(0, 0, 1);
		qglVertex3fv(input->xyz[i]);
		VectorMA(input->xyz[i], 2, input->normals[i], temp);
		qglVertex3fv(temp);
	}
	qglEnd();

	qglDepthRange(0, 1);
}


/*
================
DrawDeluxels

Draws vertex deluxels for debugging
================
*/
static void DrawDeluxels(shaderCommands_t * input)
{
	int             i;
	vec3_t          temp;

	GL_Program(0);
	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
	qglDepthRange(0, 0);		// never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	qglBegin(GL_LINES);
	for(i = 0; i < input->numVertexes; i++)
	{
		qglColor3f(1, 1, 1);
		qglVertex3fv(input->xyz[i]);

		VectorMA(input->xyz[i], 2, input->deluxels[i], temp);
		qglColor3f(1, 1, 0);
		qglVertex3fv(temp);
	}
	qglEnd();

	qglDepthRange(0, 1);
}

/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals(shaderCommands_t * input)
{
	int             i;
	vec3_t          temp;

	GL_Program(0);
	GL_SelectTexture(0);
	GL_Bind(tr.whiteImage);
	qglColor3f(1, 1, 1);
	qglDepthRange(0, 0);		// never occluded
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	qglBegin(GL_LINES);
	for(i = 0; i < input->numVertexes; i++)
	{
		qglVertex3fv(input->xyz[i]);
		VectorMA(input->xyz[i], 2, input->normals[i], temp);
		qglVertex3fv(temp);
	}
	qglEnd();

	qglDepthRange(0, 1);
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface(shader_t * shader, int fogNum)
{
	shader_t       *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.xstages = state->stages;
	tess.numPasses = state->numStages;
	
	switch (tess.currentStageIteratorType)
	{
		case SIT_ZFILL:
			tess.currentStageIteratorFunc = RB_StageIteratorZFill;
			break;
			
		case SIT_LIGHTING:
			tess.currentStageIteratorFunc = RB_StageIteratorLighting;
			break;
			
		case SIT_TRANSLUCENT:
			tess.currentStageIteratorFunc = RB_StageIteratorTranslucent;
			break;
			
		case SIT_FOG:
			tess.currentStageIteratorFunc = RB_StageIteratorFog;
			break;
		
		default:
		case SIT_DEFAULT:
			tess.currentStageIteratorFunc = state->isSky ? RB_StageIteratorSky : RB_StageIteratorGeneric;
			break;
	}

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if(tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime)
	{
		tess.shaderTime = tess.shader->clampTime;
	}
}

static void Render_generic_single_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
	GL_Program(0);
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX | GLCS_COLOR);
	GL_SetVertexAttribs();
	
	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_COLORMAP]);

	R_BindAnimatedImage(&pStage->bundle[0]);

	R_DrawElements(tess.numIndexes, tess.indexes);
	
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

static void Render_zfill_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
#if 1
	qglColor3f(0, 0, 0);
#else
	qglColor3f(1, 1, 1);
#endif
	
	GL_Program(0);
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX);
	GL_SetVertexAttribs();
	
	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_DIFFUSEMAP]);
	
	if(pStage->stateBits & GLS_ATEST_BITS)
	{
		GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	}
	else
	{
		GL_Bind(tr.whiteImage);	
	}

	R_DrawElements(tess.numIndexes, tess.indexes);
	
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

/*
===================
DrawMultiTextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void Render_generic_multi_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
	GL_Program(0);
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX | GLCS_COLOR);
	GL_SetVertexAttribs();

	// base
	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_DIFFUSEMAP]);
	R_BindAnimatedImage(&pStage->bundle[0]);

	// lightmap/secondary pass
	GL_SelectTexture(1);
	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[1]);

	if(r_showLightMaps->integer)
	{
		GL_TexEnv(GL_REPLACE);
	}
	else
	{
		GL_TexEnv(tess.shader->collapseTextureEnv);
	}

	R_BindAnimatedImage(&pStage->bundle[1]);

	R_DrawElements(tess.numIndexes, tess.indexes);

	// clean up
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglDisable(GL_TEXTURE_2D);
	
	GL_SelectTexture(0);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

void Render_lighting_D_direct(int stage)
{
	shaderStage_t  *pStage;

	vec4_t          ambientLight;
	vec3_t          lightDir;
	vec4_t          directedLight;
	trRefEntity_t  *ent = backEnd.currentEntity;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_D_direct.program);
	GL_ClientState(tr.lightShader_D_direct.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorScale(ent->ambientLight, (1.0 / 255.0) * 0.3, ambientLight);
	ClampColor(ambientLight);
	VectorScale(ent->directedLight, 1.0 / 255.0, directedLight);
	ClampColor(directedLight);
	VectorCopy(ent->lightDir, lightDir);
	
	qglUniform3fARB(tr.lightShader_D_direct.u_AmbientColor, ambientLight[0], ambientLight[1], ambientLight[2]);
	qglUniform3fARB(tr.lightShader_D_direct.u_LightDir, lightDir[0], lightDir[1], lightDir[2]);
	qglUniform3fARB(tr.lightShader_D_direct.u_LightColor, directedLight[0], directedLight[1], directedLight[2]);

	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}


static void Render_lighting_DB_direct(int stage)
{
	shaderStage_t  *pStage;

	vec4_t          ambientLight;
	vec3_t          lightDir;
	vec4_t          directedLight;
	trRefEntity_t  *ent = backEnd.currentEntity;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_DB_direct.program);
	GL_ClientState(tr.lightShader_DB_direct.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorScale(ent->ambientLight, (1.0 / 255.0) * 0.3, ambientLight);
	ClampColor(ambientLight);
	VectorScale(ent->directedLight, 1.0 / 255.0, directedLight);
	ClampColor(directedLight);
	VectorCopy(ent->lightDir, lightDir);
	
	qglUniform3fARB(tr.lightShader_DB_direct.u_AmbientColor, ambientLight[0], ambientLight[1], ambientLight[2]);
	qglUniform3fARB(tr.lightShader_DB_direct.u_LightDir, lightDir[0], lightDir[1], lightDir[2]);
	qglUniform3fARB(tr.lightShader_DB_direct.u_LightColor, directedLight[0], directedLight[1], directedLight[2]);


	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	// bind normalmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_lighting_DBS_direct(int stage)
{
	shaderStage_t  *pStage;

	vec3_t			viewOrigin;
	vec4_t          ambientLight;
	vec3_t          lightDir;
	vec4_t          directedLight;
	trRefEntity_t  *ent = backEnd.currentEntity;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);

	// enable shader, set arrays
	GL_Program(tr.lightShader_DBS_direct.program);
	GL_ClientState(tr.lightShader_DBS_direct.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorCopy(backEnd.or.viewOrigin, viewOrigin);
	VectorScale(ent->ambientLight, (1.0 / 255.0) * 0.3, ambientLight);
	ClampColor(ambientLight);
	VectorScale(ent->directedLight, 1.0 / 255.0, directedLight);
	ClampColor(directedLight);
	VectorCopy(ent->lightDir, lightDir);
	
	qglUniform3fARB(tr.lightShader_DBS_direct.u_AmbientColor, ambientLight[0], ambientLight[1], ambientLight[2]);
	qglUniform3fARB(tr.lightShader_DBS_direct.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	qglUniform3fARB(tr.lightShader_DBS_direct.u_LightDir, lightDir[0], lightDir[1], lightDir[2]);
	qglUniform3fARB(tr.lightShader_DBS_direct.u_LightColor, directedLight[0], directedLight[1], directedLight[2]);
	qglUniform1fARB(tr.lightShader_DBS_direct.u_SpecularExponent, 32.0);


	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	// bind normalmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
	
	// bind specularmap
	GL_SelectTexture(2);
	GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_lighting_D_omni(	shaderStage_t * diffuseStage,
									shaderStage_t * attenuationXYStage,
									shaderStage_t * attenuationZStage,
									trRefDlight_t * dlight)
{
	vec3_t          lightOrigin;
	vec4_t          lightColor;	
//	trRefEntity_t  *ent = backEnd.currentEntity;
//	shaderStage_t  *pStage = tess.xstages[stage];
//	trRefDlight_t  *dl = &backEnd.refdef.dlights[dlight];

	// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
	// where they aren't rendered
#if 0
	if(!dl->additive)
	{
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
	}
	else
#endif
	{
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
	}
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_D_omni.program);
	GL_ClientState(tr.lightShader_D_omni.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorCopy(dlight->transformed, lightOrigin);
	VectorCopy(dlight->l.color, lightColor);
	ClampColor(lightColor);
	
	qglUniform3fARB(tr.lightShader_D_omni.u_LightOrigin, lightOrigin[0], lightOrigin[1], lightOrigin[2]);
	qglUniform3fARB(tr.lightShader_D_omni.u_LightColor, lightColor[0], lightColor[1], lightColor[2]);

	GL_SelectTexture(0);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(dlight->attenuationMatrix2);
	qglMatrixMode(GL_MODELVIEW);
	
	GL_SelectTexture(1);
	R_BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);
	
	GL_SelectTexture(2);
	R_BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_lighting_DB_omni(	shaderStage_t * diffuseStage,
										shaderStage_t * attenuationXYStage,
										shaderStage_t * attenuationZStage,
										trRefDlight_t * dlight)
{
	vec3_t          lightOrigin;
	vec4_t          lightColor;	

#if 0
	if(!dl->additive)
	{
		GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
	}
	else
#endif
	{
		GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
	}
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_DB_omni.program);
	GL_ClientState(tr.lightShader_DB_omni.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorCopy(dlight->transformed, lightOrigin);
	VectorCopy(dlight->l.color, lightColor);
	ClampColor(lightColor);
	
	qglUniform3fARB(tr.lightShader_DB_omni.u_LightOrigin, lightOrigin[0], lightOrigin[1], lightOrigin[2]);
	qglUniform3fARB(tr.lightShader_DB_omni.u_LightColor, lightColor[0], lightColor[1], lightColor[2]);

	GL_SelectTexture(0);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(dlight->attenuationMatrix2);
	qglMatrixMode(GL_MODELVIEW);
	
	GL_SelectTexture(1);
	GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);
	
	GL_SelectTexture(2);
	R_BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);
	
	GL_SelectTexture(3);
	R_BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_lighting_DBS_omni(	shaderStage_t * diffuseStage,
										shaderStage_t * attenuationXYStage,
										shaderStage_t * attenuationZStage,
										trRefDlight_t * dlight)
{
	vec3_t			viewOrigin;
	vec3_t          lightOrigin;
	vec4_t          lightColor;	

#if 0
	if(!dl->additive)
{
	GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
}
	else
#endif
{
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
}
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_DBS_omni.program);
	GL_ClientState(tr.lightShader_DBS_omni.attribs);
	GL_SetVertexAttribs();

	// set uniforms
	VectorCopy(backEnd.or.viewOrigin, viewOrigin);
	VectorCopy(dlight->transformed, lightOrigin);
	VectorCopy(dlight->l.color, lightColor);
	ClampColor(lightColor);
	
	qglUniform3fARB(tr.lightShader_DBS_omni.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	qglUniform3fARB(tr.lightShader_DBS_omni.u_LightOrigin, lightOrigin[0], lightOrigin[1], lightOrigin[2]);
	qglUniform3fARB(tr.lightShader_DBS_omni.u_LightColor, lightColor[0], lightColor[1], lightColor[2]);
	qglUniform1fARB(tr.lightShader_DBS_omni.u_SpecularExponent, 32.0);

	GL_SelectTexture(0);
	GL_Bind(diffuseStage->bundle[TB_DIFFUSEMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(dlight->attenuationMatrix2);
	qglMatrixMode(GL_MODELVIEW);
	
	GL_SelectTexture(1);
	GL_Bind(diffuseStage->bundle[TB_NORMALMAP].image[0]);
	
	GL_SelectTexture(2);
	GL_Bind(diffuseStage->bundle[TB_SPECULARMAP].image[0]);
	
	GL_SelectTexture(3);
	R_BindAnimatedImage(&attenuationXYStage->bundle[TB_COLORMAP]);
	
	GL_SelectTexture(4);
	R_BindAnimatedImage(&attenuationZStage->bundle[TB_COLORMAP]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

// Tr3B - r_showLightMaps development tool
static void Render_lightmap_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
	GL_Program(0);
	
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX);
	GL_SetVertexAttribs();

	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_LIGHTMAP]);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[0]);

	R_DrawElements(tess.numIndexes, tess.indexes);
	
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

// Tr3B - r_showDeluxeMaps development tool
static void Render_deluxemap_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
	if(!pStage->bundle[TB_LIGHTMAP].image[1])
		return;
	
	GL_Program(0);
	
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX);
	GL_SetVertexAttribs();

	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_LIGHTMAP]);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[1]);

	R_DrawElements(tess.numIndexes, tess.indexes);
	
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

static void Render_lighting_D_radiosity_FFP(int stage)
{
	shaderStage_t  *pStage;

	pStage = tess.xstages[stage];
	
	GL_Program(0);
	GL_SelectTexture(0);
	GL_State(pStage->stateBits);
	GL_ClientState(GLCS_VERTEX | GLCS_COLOR);
	GL_SetVertexAttribs();

	// base
	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	R_BindAnimatedImage(&pStage->bundle[TB_DIFFUSEMAP]);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_DIFFUSEMAP]);

	// lightmap/secondary pass
	GL_SelectTexture(1);
	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_LIGHTMAP]);
	R_BindAnimatedImage(&pStage->bundle[TB_LIGHTMAP]);

	R_DrawElements(tess.numIndexes, tess.indexes);

	// clean up
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	qglDisable(GL_TEXTURE_2D);
	
	GL_SelectTexture(0);
	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

static void Render_lighting_D_radiosity(int stage)
{
	shaderStage_t  *pStage;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_D_radiosity.program);
	GL_ClientState(tr.lightShader_D_radiosity.attribs);
	GL_SetVertexAttribs();

	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	// bind lightmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[0]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_lighting_DB_radiosity(int stage)
{
	shaderStage_t  *pStage;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_DB_radiosity.program);
	GL_ClientState(tr.lightShader_DB_radiosity.attribs);
	GL_SetVertexAttribs();

	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	// bind normalmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
	
	// bind lightmap
	GL_SelectTexture(2);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[0]);
	
	// bind deluxemap
	GL_SelectTexture(3);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[1]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_lighting_DBS_radiosity(int stage)
{
	vec3_t			viewOrigin;
	shaderStage_t  *pStage;
	
	pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.lightShader_DBS_radiosity.program);
	GL_ClientState(tr.lightShader_DBS_radiosity.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	VectorCopy(backEnd.or.viewOrigin, viewOrigin);
	
	qglUniform3fARB(tr.lightShader_DBS_radiosity.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	qglUniform1fARB(tr.lightShader_DBS_radiosity.u_SpecularExponent, 32.0);

	// bind diffusemap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_DIFFUSEMAP].image[0]);
	
	// bind normalmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_NORMALMAP].image[0]);
	
	// bind specular
	GL_SelectTexture(2);
	GL_Bind(pStage->bundle[TB_SPECULARMAP].image[0]);
	
	// bind lightmap
	GL_SelectTexture(3);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[0]);
	
	// bind deluxemap
	GL_SelectTexture(4);
	GL_Bind(pStage->bundle[TB_LIGHTMAP].image[1]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_reflection_C(int stage)
{
	vec3_t			viewOrigin;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.reflectionShader_C.program);
	GL_ClientState(tr.reflectionShader_C.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	VectorCopy(backEnd.viewParms.or.origin, viewOrigin);	// in world space
	qglUniform3fARB(tr.reflectionShader_C.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);

	// bind colormap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(backEnd.or.transformMatrix);
	qglMatrixMode(GL_MODELVIEW);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
//	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_refraction_C(int stage)
{
	vec3_t			viewOrigin;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.refractionShader_C.program);
	GL_ClientState(tr.refractionShader_C.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	VectorCopy(backEnd.viewParms.or.origin, viewOrigin);	// in world space
	qglUniform3fARB(tr.refractionShader_C.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	qglUniform1fARB(tr.refractionShader_C.u_RefractionIndex, RB_EvalExpression(&pStage->refractionIndexExp, 1.0));
	qglUniform1fARB(tr.refractionShader_C.u_FresnelPower, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0));
	qglUniform1fARB(tr.refractionShader_C.u_FresnelScale, RB_EvalExpression(&pStage->fresnelScaleExp, 2.0));
	qglUniform1fARB(tr.refractionShader_C.u_FresnelBias, RB_EvalExpression(&pStage->fresnelBiasExp, 1.0));

	// bind colormap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(backEnd.or.transformMatrix);
	qglMatrixMode(GL_MODELVIEW);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
//	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_dispersion_C(int stage)
{
	vec3_t			viewOrigin;
	shaderStage_t  *pStage = tess.xstages[stage];
	float			eta;
	float			etaDelta;
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.dispersionShader_C.program);
	GL_ClientState(tr.dispersionShader_C.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	VectorCopy(backEnd.viewParms.or.origin, viewOrigin);	// in world space
	eta	= RB_EvalExpression(&pStage->etaExp, 1.1);
	etaDelta = RB_EvalExpression(&pStage->etaDeltaExp, -0.02);
	
	qglUniform3fARB(tr.dispersionShader_C.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);
	qglUniform3fARB(tr.dispersionShader_C.u_EtaRatio, eta, eta + etaDelta, eta + (etaDelta * 2));
	qglUniform1fARB(tr.dispersionShader_C.u_FresnelPower, RB_EvalExpression(&pStage->fresnelPowerExp, 2.0));
	qglUniform1fARB(tr.dispersionShader_C.u_FresnelScale, RB_EvalExpression(&pStage->fresnelScaleExp, 2.0));
	qglUniform1fARB(tr.dispersionShader_C.u_FresnelBias, RB_EvalExpression(&pStage->fresnelBiasExp, 1.0));

	// bind colormap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(backEnd.or.transformMatrix);
	qglMatrixMode(GL_MODELVIEW);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
//	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_skybox(int stage)
{
	vec3_t			viewOrigin;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.skyBoxShader.program);
	GL_ClientState(tr.skyBoxShader.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	VectorCopy(backEnd.viewParms.or.origin, viewOrigin);	// in world space
	qglUniform3fARB(tr.skyBoxShader.u_ViewOrigin, viewOrigin[0], viewOrigin[1], viewOrigin[2]);

	// bind colormap
	GL_SelectTexture(0);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	qglMatrixMode(GL_TEXTURE);
	qglLoadMatrixf(backEnd.or.transformMatrix);
	qglMatrixMode(GL_MODELVIEW);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
//	GL_SelectTexture(0);
	qglMatrixMode(GL_TEXTURE);
	qglLoadIdentity();
	qglMatrixMode(GL_MODELVIEW);
}

static void Render_heatHaze(int stage)
{
	float           deformMagnitude;
	float			fbufWidthScale, fbufHeightScale;
	float			npotWidthScale, npotHeightScale;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.heatHazeShader.program);
	GL_ClientState(tr.heatHazeShader.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	deformMagnitude = RB_EvalExpression(&pStage->deformMagnitudeExp, 1.0);
	fbufWidthScale = Q_recip((float)glConfig.vidWidth);
	fbufHeightScale = Q_recip((float)glConfig.vidHeight);
	npotWidthScale = (float)glConfig.vidWidth / (float)tr.currentRenderImage->uploadWidth;
	npotHeightScale = (float)glConfig.vidHeight / (float)tr.currentRenderImage->uploadHeight;
	
	qglUniform1fARB(tr.heatHazeShader.u_DeformMagnitude, deformMagnitude);
	qglUniform2fARB(tr.heatHazeShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.heatHazeShader.u_NPotScale, npotWidthScale, npotHeightScale);

	// bind colormap
	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);
	
	// bind normalmap
	GL_SelectTexture(1);
	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_glow(int stage)
{
	float           blurMagnitude;
	float			fbufWidthScale, fbufHeightScale;
	float			npotWidthScale, npotHeightScale;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// enable shader, set arrays
	GL_Program(tr.glowShader.program);
	GL_ClientState(tr.glowShader.attribs);
	GL_SetVertexAttribs();
	
	// set uniforms
	blurMagnitude = RB_EvalExpression(&pStage->blurMagnitudeExp, 3.0);
	fbufWidthScale = Q_recip((float)glConfig.vidWidth);
	fbufHeightScale = Q_recip((float)glConfig.vidHeight);
	npotWidthScale = (float)glConfig.vidWidth / (float)tr.currentRenderImage->uploadWidth;
	npotHeightScale = (float)glConfig.vidHeight / (float)tr.currentRenderImage->uploadHeight;
	
	qglUniform1fARB(tr.glowShader.u_BlurMagnitude, blurMagnitude);
	qglUniform2fARB(tr.glowShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.glowShader.u_NPotScale, npotWidthScale, npotHeightScale);

	// bind colormap
	GL_SelectTexture(0);
//	GL_Bind(pStage->bundle[TB_COLORMAP].image[0]);
	GL_Bind(tr.currentRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_bloom(int stage)
{
	float           blurMagnitude;
	float			fbufWidthScale, fbufHeightScale;
	float			npotWidthScale, npotHeightScale;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// calc uniforms
	blurMagnitude = RB_EvalExpression(&pStage->blurMagnitudeExp, 3.0);
	fbufWidthScale = Q_recip((float)glConfig.vidWidth);
	fbufHeightScale = Q_recip((float)glConfig.vidHeight);
	npotWidthScale = (float)glConfig.vidWidth / (float)tr.currentRenderNearestImage->uploadWidth;
	npotHeightScale = (float)glConfig.vidHeight / (float)tr.currentRenderNearestImage->uploadHeight;
	
	// render contrast
	GL_Program(tr.contrastShader.program);
	GL_ClientState(tr.contrastShader.attribs);
	GL_SetVertexAttribs();

	qglUniform2fARB(tr.contrastShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.contrastShader.u_NPotScale, npotWidthScale, npotHeightScale);

	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderNearestImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderNearestImage->uploadWidth, tr.currentRenderNearestImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	// render bloom
	GL_Program(tr.bloomShader.program);
	GL_ClientState(tr.bloomShader.attribs);
	GL_SetVertexAttribs();
	
	qglUniform1fARB(tr.bloomShader.u_BlurMagnitude, blurMagnitude);
	qglUniform2fARB(tr.bloomShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.bloomShader.u_NPotScale, npotWidthScale, npotHeightScale);
	
	GL_SelectTexture(1);
	GL_Bind(tr.contrastRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.contrastRenderImage->uploadWidth, tr.contrastRenderImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void Render_bloom2(int stage)
{
	float           blurMagnitude;
	float			fbufWidthScale, fbufHeightScale;
	float			npotWidthScale, npotHeightScale;
	shaderStage_t  *pStage = tess.xstages[stage];
	
	GL_State(pStage->stateBits);
	
	// calc uniforms
	blurMagnitude = RB_EvalExpression(&pStage->blurMagnitudeExp, 3.0);
	fbufWidthScale = Q_recip((float)glConfig.vidWidth);
	fbufHeightScale = Q_recip((float)glConfig.vidHeight);
	npotWidthScale = (float)glConfig.vidWidth / (float)tr.currentRenderNearestImage->uploadWidth;
	npotHeightScale = (float)glConfig.vidHeight / (float)tr.currentRenderNearestImage->uploadHeight;
	
	// render contrast
	GL_Program(tr.contrastShader.program);
	GL_ClientState(tr.contrastShader.attribs);
	GL_SetVertexAttribs();

	qglUniform2fARB(tr.contrastShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.contrastShader.u_NPotScale, npotWidthScale, npotHeightScale);

	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderNearestImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderNearestImage->uploadWidth, tr.currentRenderNearestImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	// render blurX
	GL_Program(tr.blurXShader.program);
	GL_ClientState(tr.blurXShader.attribs);
	GL_SetVertexAttribs();

	qglUniform2fARB(tr.blurXShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.blurXShader.u_NPotScale, npotWidthScale, npotHeightScale);

	GL_Bind(tr.contrastRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.contrastRenderImage->uploadWidth, tr.contrastRenderImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	// render blurY
	GL_Program(tr.blurYShader.program);
	GL_ClientState(tr.blurYShader.attribs);
	GL_SetVertexAttribs();

	qglUniform2fARB(tr.blurYShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.blurYShader.u_NPotScale, npotWidthScale, npotHeightScale);

	GL_Bind(tr.contrastRenderImage);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.contrastRenderImage->uploadWidth, tr.contrastRenderImage->uploadHeight);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
	
	// render bloom
	GL_Program(tr.bloomShader.program);
	GL_ClientState(tr.bloomShader.attribs);
	GL_SetVertexAttribs();
	
	qglUniform1fARB(tr.bloomShader.u_BlurMagnitude, blurMagnitude);
	qglUniform2fARB(tr.bloomShader.u_FBufScale, fbufWidthScale, fbufHeightScale);
	qglUniform2fARB(tr.bloomShader.u_NPotScale, npotWidthScale, npotHeightScale);

	GL_SelectTexture(0);
	GL_Bind(tr.currentRenderNearestImage);
	
	GL_SelectTexture(1);
	GL_Bind(tr.contrastRenderImage);
	
	R_DrawElements(tess.numIndexes, tess.indexes);
}

static void RB_Render_fog()
{
	fog_t          *fog;
	int             i;

	fog = tr.world->fogs + tess.fogNum;

	for(i = 0; i < tess.numVertexes; i++)
	{
		*(int *)&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords((float *)tess.svars.texCoords[0]);

	GL_Program(0);
	GL_ClientState(GLCS_VERTEX | GLCS_COLOR);
	GL_SetVertexAttribs();

	GL_SelectTexture(0);
//	qglEnable(GL_TEXTURE_2D);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer(2, GL_FLOAT, 0, tess.svars.texCoords[TB_COLORMAP]);
	GL_Bind(tr.fogImage);

	if(tess.shader->fogPass == FP_EQUAL)
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL);
	}
	else
	{
		GL_State(GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}

	R_DrawElements(tess.numIndexes, tess.indexes);

	qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	qglDisable(GL_TEXTURE_2D);
}

/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
/*
void ProjectDlightTexture()
{
#if 0
	int             i, l;

#if idppc_altivec
	vec_t           origin0, origin1, origin2;
	float           texCoords0, texCoords1;
	vector float    floatColorVec0, floatColorVec1;
	vector float    modulateVec, colorVec, zero;
	vector short    colorShort;
	vector signed int colorInt;
	vector unsigned char floatColorVecPerm, modulatePerm, colorChar;
	vector unsigned char vSel = (vector unsigned char)(0x00, 0x00, 0x00, 0xff,
													   0x00, 0x00, 0x00, 0xff,
													   0x00, 0x00, 0x00, 0xff,
													   0x00, 0x00, 0x00, 0xff);
#else
	vec3_t          origin;
#endif
	float          *texCoords;
	byte           *colors;
	byte            clipBits[SHADER_MAX_VERTEXES];
	MAC_STATIC float texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte            colorArray[SHADER_MAX_VERTEXES][4];
	unsigned        hitIndexes[SHADER_MAX_INDEXES];
	int             numIndexes;
	float           scale;
	float           radius;
	vec3_t          floatColor;
	float           modulate;

	if(!backEnd.refdef.numDlights)
	{
		return;
	}

#if idppc_altivec
	// There has to be a better way to do this so that floatColor 
	// and/or modulate are already 16-byte aligned.
	floatColorVecPerm = vec_lvsl(0, (float *)floatColor);
	modulatePerm = vec_lvsl(0, (float *)&modulate);
	modulatePerm = (vector unsigned char)vec_splat((vector unsigned int)modulatePerm, 0);
	zero = (vector float)vec_splat_s8(0);
#endif

	for(l = 0; l < backEnd.refdef.numDlights; l++)
	{
		trRefDlight_t       *dl;

		if(!(tess.dlightBits & (1 << l)))
		{
			continue;			// this surface definately doesn't have any of this light
		}
		texCoords = texCoordsArray[0];
		colors = colorArray[0];

		dl = &backEnd.refdef.dlights[l];
#if idppc_altivec
		origin0 = dl->transformed[0];
		origin1 = dl->transformed[1];
		origin2 = dl->transformed[2];
#else
		VectorCopy(dl->transformed, origin);
#endif
		radius = dl->l.radius;
		scale = 1.0f / radius;

		floatColor[0] = dl->l.color[0] * 255.0f;
		floatColor[1] = dl->l.color[1] * 255.0f;
		floatColor[2] = dl->l.color[2] * 255.0f;
#if idppc_altivec
		floatColorVec0 = vec_ld(0, floatColor);
		floatColorVec1 = vec_ld(11, floatColor);
		floatColorVec0 = vec_perm(floatColorVec0, floatColorVec0, floatColorVecPerm);
#endif
		for(i = 0; i < tess.numVertexes; i++, texCoords += 2, colors += 4)
		{
#if idppc_altivec
			vec_t           dist0, dist1, dist2;
#else
			vec3_t          dist;
#endif
			int             clip;

			backEnd.pc.c_dlightVertexes++;

#if idppc_altivec
			//VectorSubtract( origin, tess.xyz[i], dist );
			dist0 = origin0 - tess.xyz[i][0];
			dist1 = origin1 - tess.xyz[i][1];
			dist2 = origin2 - tess.xyz[i][2];
			texCoords0 = 0.5f + dist0 * scale;
			texCoords1 = 0.5f + dist1 * scale;

			clip = 0;
			if(texCoords0 < 0.0f)
			{
				clip |= 1;
			}
			else if(texCoords0 > 1.0f)
			{
				clip |= 2;
			}
			if(texCoords1 < 0.0f)
			{
				clip |= 4;
			}
			else if(texCoords1 > 1.0f)
			{
				clip |= 8;
			}
			texCoords[0] = texCoords0;
			texCoords[1] = texCoords1;

			// modulate the strength based on the height and color
			if(dist2 > radius)
			{
				clip |= 16;
				modulate = 0.0f;
			}
			else if(dist2 < -radius)
			{
				clip |= 32;
				modulate = 0.0f;
			}
			else
			{
				dist2 = Q_fabs(dist2);
				if(dist2 < radius * 0.5f)
				{
					modulate = 1.0f;
				}
				else
				{
					modulate = 2.0f * (radius - dist2) * scale;
				}
			}
			clipBits[i] = clip;

			modulateVec = vec_ld(0, (float *)&modulate);
			modulateVec = vec_perm(modulateVec, modulateVec, modulatePerm);
			colorVec = vec_madd(floatColorVec0, modulateVec, zero);
			colorInt = vec_cts(colorVec, 0);	// RGBx
			colorShort = vec_pack(colorInt, colorInt);	// RGBxRGBx
			colorChar = vec_packsu(colorShort, colorShort);	// RGBxRGBxRGBxRGBx
			colorChar = vec_sel(colorChar, vSel, vSel);	// RGBARGBARGBARGBA replace alpha with 255
			vec_ste((vector unsigned int)colorChar, 0, (unsigned int *)colors);	// store color
#else
			VectorSubtract(origin, tess.xyz[i], dist);
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			clip = 0;
			if(texCoords[0] < 0.0f)
			{
				clip |= 1;
			}
			else if(texCoords[0] > 1.0f)
			{
				clip |= 2;
			}
			if(texCoords[1] < 0.0f)
			{
				clip |= 4;
			}
			else if(texCoords[1] > 1.0f)
			{
				clip |= 8;
			}
			// modulate the strength based on the height and color
			if(dist[2] > radius)
			{
				clip |= 16;
				modulate = 0.0f;
			}
			else if(dist[2] < -radius)
			{
				clip |= 32;
				modulate = 0.0f;
			}
			else
			{
				dist[2] = Q_fabs(dist[2]);
				if(dist[2] < radius * 0.5f)
				{
					modulate = 1.0f;
				}
				else
				{
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = myftol(floatColor[0] * modulate);
			colors[1] = myftol(floatColor[1] * modulate);
			colors[2] = myftol(floatColor[2] * modulate);
			colors[3] = 255;
#endif
		}

		// build a list of triangles that need light
		numIndexes = 0;
		for(i = 0; i < tess.numIndexes; i += 3)
		{
			int             i1, i2, i3;
			vec3_t          d1, d2, normal;
			float          *v1, *v2, *v3;
			float           d;

			i1 = tess.indexes[i * 3 + 0];
			i2 = tess.indexes[i * 3 + 1];
			i3 = tess.indexes[i * 3 + 2];

			v1 = tess.xyz[i1];
			v2 = tess.xyz[i2];
			v3 = tess.xyz[i3];

			VectorSubtract(v2, v1, d1);
			VectorSubtract(v3, v1, d2);
			CrossProduct(d1, d2, normal);

			d = DotProduct(normal, lightDir);
			if(d <= 0)
			{
				continue;	// not lighted
			}
			
			hitIndexes[numIndexes + 0] = i1;
			hitIndexes[numIndexes + 1] = i2;
			hitIndexes[numIndexes + 2] = i3;
			numIndexes += 3;
		}

		if(!numIndexes)
		{
			continue;
		}

		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer(2, GL_FLOAT, 0, texCoordsArray[0]);

		qglEnableClientState(GL_COLOR_ARRAY);
		qglColorPointer(4, GL_UNSIGNED_BYTE, 0, colorArray);

		GL_Bind(tr.dlightImage);
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		if(dl->additive)
		{
			GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		}
		else
		{
			GL_State(GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL);
		}
		R_DrawElements(numIndexes, hitIndexes);
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}
#endif
}
*/


/*
===============
ComputeColors
===============
*/
static void ComputeColors(shaderStage_t * pStage)
{
	int             i;

	// rgbGen
	switch (pStage->rgbGen)
	{
		case CGEN_IDENTITY:
			Com_Memset(tess.svars.colors, 0xff, tess.numVertexes * 4);
			break;
			
		default:
		case CGEN_IDENTITY_LIGHTING:
			Com_Memset(tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4);
			break;
			
		case CGEN_LIGHTING_DIFFUSE:
			RB_CalcDiffuseColor((unsigned char *)tess.svars.colors);
			break;
			
		case CGEN_EXACT_VERTEX:
			Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
			break;
			
		case CGEN_CONST:
			for(i = 0; i < tess.numVertexes; i++)
			{
				*(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
			}
			break;
			
		case CGEN_VERTEX:
			if(tr.identityLight == 1)
			{
				Com_Memcpy(tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof(tess.vertexColors[0]));
			}
			else
			{
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
					tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
					tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
			
		case CGEN_ONE_MINUS_VERTEX:
			if(tr.identityLight == 1)
			{
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
					tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
					tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
				}
			}
			else
			{
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][0] = (255 - tess.vertexColors[i][0]) * tr.identityLight;
					tess.svars.colors[i][1] = (255 - tess.vertexColors[i][1]) * tr.identityLight;
					tess.svars.colors[i][2] = (255 - tess.vertexColors[i][2]) * tr.identityLight;
				}
			}
			break;
			
		case CGEN_FOG:
		{
			fog_t          *fog;

			fog = tr.world->fogs + tess.fogNum;

			for(i = 0; i < tess.numVertexes; i++)
			{
				*(int *)&tess.svars.colors[i] = fog->colorInt;
			}
			break;
		}
			
		case CGEN_WAVEFORM:
			RB_CalcWaveColor(&pStage->rgbWave, (unsigned char *)tess.svars.colors);
			break;
			
		case CGEN_ENTITY:
			RB_CalcColorFromEntity((unsigned char *)tess.svars.colors);
			break;
			
		case CGEN_ONE_MINUS_ENTITY:
			RB_CalcColorFromOneMinusEntity((unsigned char *)tess.svars.colors);
			break;
			
		case CGEN_CUSTOM_RGB:
			RB_CalcCustomColor(&pStage->rgbExp, (unsigned char *)tess.svars.colors);
			break;
			
		case CGEN_CUSTOM_RGBs:
			RB_CalcCustomColors(&pStage->redExp, &pStage->greenExp, &pStage->redExp, (unsigned char *)tess.svars.colors);
			break;
	}

	// alphaGen
	switch (pStage->alphaGen)
	{
		case AGEN_SKIP:
			break;
			
		case AGEN_IDENTITY:
			if(pStage->rgbGen != CGEN_IDENTITY)
			{
				if((pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1) || pStage->rgbGen != CGEN_VERTEX)
				{
					for(i = 0; i < tess.numVertexes; i++)
					{
						tess.svars.colors[i][3] = 0xff;
					}
				}
			}
			break;
			
		case AGEN_CONST:
			if(pStage->rgbGen != CGEN_CONST)
			{
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][3] = pStage->constantColor[3];
				}
			}
			break;
			
		case AGEN_WAVEFORM:
			RB_CalcWaveAlpha(&pStage->alphaWave, (unsigned char *)tess.svars.colors);
			break;
			
		case AGEN_LIGHTING_SPECULAR:
			RB_CalcSpecularAlpha((unsigned char *)tess.svars.colors);
			break;
			
		case AGEN_ENTITY:
			RB_CalcAlphaFromEntity((unsigned char *)tess.svars.colors);
			break;
			
		case AGEN_ONE_MINUS_ENTITY:
			RB_CalcAlphaFromOneMinusEntity((unsigned char *)tess.svars.colors);
			break;
			
		case AGEN_VERTEX:
			if(pStage->rgbGen != CGEN_VERTEX)
			{
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.colors[i][3] = tess.vertexColors[i][3];
				}
			}
			break;
			
		case AGEN_ONE_MINUS_VERTEX:
			for(i = 0; i < tess.numVertexes; i++)
			{
				tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
			}
			break;
			
		case AGEN_PORTAL:
		{
			unsigned char   alpha;

			for(i = 0; i < tess.numVertexes; i++)
			{
				float           len;
				vec3_t          v;

				VectorSubtract(tess.xyz[i], backEnd.viewParms.or.origin, v);
				len = VectorLength(v);

				len /= tess.shader->portalRange;

				if(len < 0)
				{
					alpha = 0;
				}
				else if(len > 1)
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.svars.colors[i][3] = alpha;
			}
			break;
		}
		
		case AGEN_CUSTOM:
			RB_CalcCustomAlpha(&pStage->alphaExp, (unsigned char *)tess.svars.colors);
			break;
	}

	// fog adjustment for colors to fade out as fog increases
	if(tess.fogNum)
	{
		switch (pStage->adjustColorsForFog)
		{
			case ACFF_MODULATE_RGB:
				RB_CalcModulateColorsByFog((unsigned char *)tess.svars.colors);
				break;
				
			case ACFF_MODULATE_ALPHA:
				RB_CalcModulateAlphasByFog((unsigned char *)tess.svars.colors);
				break;
				
			case ACFF_MODULATE_RGBA:
				RB_CalcModulateRGBAsByFog((unsigned char *)tess.svars.colors);
				break;
				
			case ACFF_NONE:
				break;
		}
	}
}


/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords(shaderStage_t * pStage)
{
	int             i;
	int             b;

	for(b = 0; b < MAX_TEXTURE_BUNDLES; b++)
	{
		int             tm;

		// generate the texture coordinates
		switch(pStage->bundle[b].tcGen)
		{
			//case TCGEN_SKIP:
			//	break;
			case TCGEN_IDENTITY:
				Com_Memset(tess.svars.texCoords[b], 0, sizeof(float) * 2 * tess.numVertexes);
				break;
				
			case TCGEN_TEXTURE:
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.texCoords[b][i][0] = tess.texCoords[i][0][0];
					tess.svars.texCoords[b][i][1] = tess.texCoords[i][0][1];
				}
				break;
				
			case TCGEN_LIGHTMAP:
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.texCoords[b][i][0] = tess.texCoords[i][1][0];
					tess.svars.texCoords[b][i][1] = tess.texCoords[i][1][1];
				}
				break;
				
			case TCGEN_VECTOR:
				for(i = 0; i < tess.numVertexes; i++)
				{
					tess.svars.texCoords[b][i][0] =
						DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[0]);
					tess.svars.texCoords[b][i][1] =
						DotProduct(tess.xyz[i], pStage->bundle[b].tcGenVectors[1]);
				}
				break;
				
			case TCGEN_FOG:
				RB_CalcFogTexCoords((float *)tess.svars.texCoords[b]);
				break;
				
			case TCGEN_ENVIRONMENT_MAPPED:
				RB_CalcEnvironmentTexCoords((float *)tess.svars.texCoords[b]);
				break;
				
			case TCGEN_BAD:
				continue;
		}

		// alter texture coordinates
		for(tm = 0; tm < pStage->bundle[b].numTexMods; tm++)
		{
			// Tr3B - for multiple images per shader stage
			//if(pStage->bundle[b].tcGen == TCGEN_SKIP)
			//	continue;
			
			switch(pStage->bundle[b].texMods[tm].type)
			{
				case TMOD_NONE:
					tm = TR_MAX_TEXMODS;	// break out of for loop
					break;

				case TMOD_TURBULENT:
					RB_CalcTurbulentTexCoords(&pStage->bundle[b].texMods[tm].wave, (float *)tess.svars.texCoords[b]);
					break;

				case TMOD_ENTITY_TRANSLATE:
					RB_CalcScrollTexCoords(backEnd.currentEntity->e.shaderTexCoord, (float *)tess.svars.texCoords[b]);
					break;

				case TMOD_SCROLL:
					RB_CalcScrollTexCoords(pStage->bundle[b].texMods[tm].scroll, (float *)tess.svars.texCoords[b]);
					break;
					
				case TMOD_SCALE:
					RB_CalcScaleTexCoords(pStage->bundle[b].texMods[tm].scale, (float *)tess.svars.texCoords[b]);
					break;

				case TMOD_STRETCH:
					RB_CalcStretchTexCoords(&pStage->bundle[b].texMods[tm].wave, (float *)tess.svars.texCoords[b]);
					break;

				case TMOD_TRANSFORM:
					RB_CalcTransformTexCoords(&pStage->bundle[b].texMods[tm], (float *)tess.svars.texCoords[b]);
					break;

				case TMOD_ROTATE:
					RB_CalcRotateTexCoords(pStage->bundle[b].texMods[tm].rotateSpeed, (float *)tess.svars.texCoords[b]);
					break;
				
				case TMOD_SCROLL2:
					RB_CalcScrollTexCoords2( &pStage->bundle[b].texMods[tm].sExp,
											 &pStage->bundle[b].texMods[tm].tExp, (float *)tess.svars.texCoords[b]);
					break;
					
				case TMOD_SCALE2:
					RB_CalcScaleTexCoords2( &pStage->bundle[b].texMods[tm].sExp,
											&pStage->bundle[b].texMods[tm].tExp, (float *)tess.svars.texCoords[b]);
					break;
					
				case TMOD_CENTERSCALE:
					RB_CalcCenterScaleTexCoords(	&pStage->bundle[b].texMods[tm].sExp,
													&pStage->bundle[b].texMods[tm].tExp, (float *)tess.svars.texCoords[b]);
					break;
					
				case TMOD_SHEAR:
					// TODO
					break;
					
				case TMOD_ROTATE2:
					RB_CalcRotateTexCoords2(&pStage->bundle[b].texMods[tm].rExp, (float *)tess.svars.texCoords[b]);
					break;
				
				default:
					ri.Error(ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n",
							 pStage->bundle[b].texMods[tm].type, tess.shader->name);
					break;
			}
		}
	}
}


void RB_StageIteratorZFill()
{
	int             stage;
	
	RB_DeformTessGeometry();

	// log this call
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment(va("--- RB_StageIteratorZFill( %s ) ---\n", tess.shader->name));
	}

	// set face culling appropriately
	GL_Cull(tess.shader->cullType);

	// set polygon offset if necessary
	if(tess.shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// lock XYZ
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);	// padded for SIMD
	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}

	// call shader function
	for(stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t  *pStage = tess.xstages[stage];

		if(!pStage)
		{
			break;
		}
		
		if(!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		switch(pStage->type)
		{
			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_D_radiosity:
			case ST_COLLAPSE_lighting_DB_radiosity:
			case ST_COLLAPSE_lighting_DBS_radiosity:
			case ST_COLLAPSE_lighting_DB_direct:
			case ST_COLLAPSE_lighting_DBS_direct:
			case ST_COLLAPSE_lighting_DB_generic:
			case ST_COLLAPSE_lighting_DBS_generic:
			case ST_REFLECTIONMAP:
			case ST_REFRACTIONMAP:
			case ST_DISPERSIONMAP:
			case ST_SKYBOXMAP:
			{
				Render_zfill_FFP(stage);
				break;
			}
			
			default:
				break;
		}
	}

	// unlock arrays
	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if(tess.shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void RB_StageIteratorLighting()
{
	int				i, j;
	trRefDlight_t  *dl;
	shader_t       *attenuationShader;
	shaderStage_t  *attenuationZStage;
	
	dl = backEnd.currentLight;
	
	RB_DeformTessGeometry();

	// log this call
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment(va("--- RB_StageIteratorLighting( %s ) ---\n", tess.shader->name));
	}

	// set face culling appropriately
	GL_Cull(tess.shader->cullType);

	// set polygon offset if necessary
	if(tess.shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// lock XYZ
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);	// padded for SIMD
	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}

	// call shader function
	attenuationShader = R_GetShaderByHandle(dl->l.attenuationShader);
		
	if(attenuationShader == NULL || attenuationShader == tr.defaultShader)
		attenuationShader = tr.defaultDlightShader;
		
	attenuationZStage = attenuationShader->stages[0];
		
	for(i = 1; i < MAX_SHADER_STAGES; i++)
	{
		shaderStage_t  *attenuationXYStage = attenuationShader->stages[i];
						
		if(!attenuationXYStage)
		{
			break;
		}
			
		if(attenuationXYStage->type != ST_ATTENUATIONMAP_XY)
		{
			continue;
		}
			
		if(!RB_EvalExpression(&attenuationXYStage->ifExp, 1.0))
		{
			continue;
		}
			
		for(j = 0; j < MAX_SHADER_STAGES; j++)
		{
			shaderStage_t  *diffuseStage = tess.xstages[j];

			if(!diffuseStage)
			{
				break;
			}
		
			if(!RB_EvalExpression(&diffuseStage->ifExp, 1.0))
			{
				continue;
			}
			
			ComputeTexCoords(diffuseStage);
				
			switch(diffuseStage->type)
			{
				case ST_DIFFUSEMAP:
				case ST_COLLAPSE_lighting_D_radiosity:
					if(glConfig2.shadingLanguage100Available)
					{
						Render_lighting_D_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
					}
					else
					{
							// TODO
					}
					break;
						
				case ST_COLLAPSE_lighting_DB_radiosity:
				case ST_COLLAPSE_lighting_DB_direct:
				case ST_COLLAPSE_lighting_DB_generic:
					if(glConfig2.shadingLanguage100Available)
					{
						if(r_bumpMapping->integer)
						{	
							Render_lighting_DB_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
						}
						else
						{
							Render_lighting_D_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
						}
					}
					else
					{
							// TODO
					}
					break;
						
				case ST_COLLAPSE_lighting_DBS_radiosity:
				case ST_COLLAPSE_lighting_DBS_direct:
				case ST_COLLAPSE_lighting_DBS_generic:
					if(glConfig2.shadingLanguage100Available)
					{
						if(r_bumpMapping->integer)
						{
							if(r_specular->integer)
							{
								Render_lighting_DBS_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
							}
							else
							{
								Render_lighting_DB_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
							}
						}
						else
						{
							Render_lighting_D_omni(diffuseStage, attenuationXYStage, attenuationZStage, dl);
						}
					}
					else
					{
							// TODO
					}
					break;
						
				default:
					break;
			}
		}
	}
		
	backEnd.pc.c_dlightVertexes += tess.numVertexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes;
	backEnd.pc.c_dlightIndexes += tess.numIndexes;
	
	// reset client state
//	GL_ClientState(GLCS_DEFAULT);
	
	// clean up
	GL_Program(0);
	GL_ClientState(GLCS_DEFAULT);
	GL_SelectTexture(0);

	// unlock arrays
	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if(tess.shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void RB_StageIteratorTranslucent()
{
	int             stage;
	
	RB_DeformTessGeometry();

	// log this call
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment(va("--- RB_StageIteratorTranslucent( %s ) ---\n", tess.shader->name));
	}

	// set face culling appropriately
	GL_Cull(tess.shader->cullType);

	// set polygon offset if necessary
	if(tess.shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// lock XYZ
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);	// padded for SIMD
	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}

	// call shader function
	for(stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t  *pStage = tess.xstages[stage];

		if(!pStage)
		{
			break;
		}
		
		if(!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		switch(pStage->type)
		{
			case ST_REFLECTIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_reflection_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_REFRACTIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_refraction_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_DISPERSIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_dispersion_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_SKYBOXMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_skybox(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_HEATHAZEMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_heatHaze(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_GLOWMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_glow(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_BLOOMMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_bloom(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_BLOOM2MAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_bloom2(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			default:
			case ST_COLORMAP:
			{
				Render_generic_single_FFP(stage);
				break;
			}
		}
	}

	// unlock arrays
	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if(tess.shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void RB_StageIteratorGeneric()
{
	int             stage;
	
	RB_DeformTessGeometry();

	// log this call
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment(va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name));
	}

	// set face culling appropriately
	GL_Cull(tess.shader->cullType);

	// set polygon offset if necessary
	if(tess.shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// lock XYZ
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);	// padded for SIMD
	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}

	// call shader function
	for(stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t  *pStage = tess.xstages[stage];

		if(!pStage)
		{
			break;
		}
		
		if(!RB_EvalExpression(&pStage->ifExp, 1.0))
		{
			continue;
		}

		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		switch(pStage->type)
		{
			// Tr3B - for development only. get rid of this later to avoid overdraw
			case ST_DIFFUSEMAP:
			{
				Render_generic_single_FFP(stage);
				break;
			}
			
			// skip these if not merge into diffuse stage
			case ST_NORMALMAP:
			case ST_SPECULARMAP:
			case ST_LIGHTMAP:
				break;
			
			case ST_COLLAPSE_Generic_multi:
			{
				Render_generic_multi_FFP(stage);
				break;
			}
			
			case ST_COLLAPSE_lighting_D_radiosity:
			{
				if(r_showLightMaps->integer)
				{
					Render_lightmap_FFP(stage);
				}
				else if(tr.worldDeluxeMapping && r_showDeluxeMaps->integer)
				{
					Render_deluxemap_FFP(stage);
				}
				else if(glConfig2.shadingLanguage100Available)
				{
					Render_lighting_D_radiosity(stage);
				}
				else
				{
					Render_lighting_D_radiosity_FFP(stage);
				}
				
				break;
			}
			
			case ST_COLLAPSE_lighting_DB_radiosity:
			{
				if(r_showLightMaps->integer)
				{
					Render_lightmap_FFP(stage);
				}
				else if(tr.worldDeluxeMapping && r_showDeluxeMaps->integer)
				{
					Render_deluxemap_FFP(stage);
				}
				else if(glConfig2.shadingLanguage100Available)
				{
					if(tr.worldDeluxeMapping && r_bumpMapping->integer)
					{
						Render_lighting_DB_radiosity(stage);
					}
					else
					{
						Render_lighting_D_radiosity(stage);
					}
				}
				else
				{
					Render_lighting_D_radiosity_FFP(stage);
				}
				break;
			}
			
			case ST_COLLAPSE_lighting_DBS_radiosity:
			{
				if(r_showLightMaps->integer)
				{
					Render_lightmap_FFP(stage);
				}
				else if(tr.worldDeluxeMapping && r_showDeluxeMaps->integer)
				{
					Render_deluxemap_FFP(stage);
				}
				else if(glConfig2.shadingLanguage100Available)
				{
					if(tr.worldDeluxeMapping && r_bumpMapping->integer)
					{
						if(r_specular->integer)
						{
							Render_lighting_DBS_radiosity(stage);
						}
						else
						{
							Render_lighting_DB_radiosity(stage);
						}
					}
					else
					{
						Render_lighting_D_radiosity(stage);
					}
				}
				else
				{
					Render_lighting_D_radiosity_FFP(stage);
				}
				break;
			}
			
			case ST_COLLAPSE_lighting_DB_direct:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					if(r_bumpMapping->integer)
					{
						Render_lighting_DB_direct(stage);
					}
					else
					{
						Render_lighting_D_direct(stage);
					}
				}
				else
				{
					Render_generic_single_FFP(stage);
				}
				break;
			}
			
			case ST_COLLAPSE_lighting_DBS_direct:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					if(r_bumpMapping->integer)
					{
						if(r_specular->integer)
						{
							Render_lighting_DBS_direct(stage);
						}
						else
						{
							Render_lighting_DB_direct(stage);
						}
					}
					else
					{
						Render_lighting_D_direct(stage);
					}
				}
				else
				{
					Render_generic_single_FFP(stage);
				}
				break;
			}
			
			case ST_REFLECTIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_reflection_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_REFRACTIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_refraction_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_DISPERSIONMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_dispersion_C(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_SKYBOXMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_skybox(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_HEATHAZEMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_heatHaze(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_GLOWMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_glow(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_BLOOMMAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_bloom(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			case ST_BLOOM2MAP:
			{
				if(glConfig2.shadingLanguage100Available)
				{
					Render_bloom2(stage);
				}
				else
				{
					// TODO
				}
				break;
			}
			
			default:
			case ST_COLORMAP:
			{
				Render_generic_single_FFP(stage);
				break;
			}
		}
		
		// allow skipping out to show just lightmaps during development
		if(r_showLightMaps->integer && (pStage->bundle[0].isLightMap || pStage->bundle[1].isLightMap))
		{
			break;
		}
	}
	
	if(tess.fogNum && tess.shader->fogPass)
	{
		RB_Render_fog();
	}
	
	// clean up
	GL_Program(0);
	GL_ClientState(GLCS_DEFAULT);
//	GL_SelectTexture(0);

	// unlock arrays
	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if(tess.shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}

void RB_StageIteratorFog()
{
	RB_DeformTessGeometry();

	// log this call
	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment(va("--- RB_StageIteratorFog( %s ) ---\n", tess.shader->name));
	}

	// set face culling appropriately
	GL_Cull(tess.shader->cullType);

	// set polygon offset if necessary
	if(tess.shader->polygonOffset)
	{
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	// lock XYZ
	qglVertexPointer(3, GL_FLOAT, 16, tess.xyz);	// padded for SIMD
	if(qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment("glLockArraysEXT\n");
	}
	
	// now do fog
	if(tess.fogNum && tess.shader->fogPass)
	{
		RB_Render_fog();
	}

	// unlock arrays
	if(qglUnlockArraysEXT)
	{
		qglUnlockArraysEXT();
		GLimp_LogComment("glUnlockArraysEXT\n");
	}

	// reset polygon offset
	if(tess.shader->polygonOffset)
	{
		qglDisable(GL_POLYGON_OFFSET_FILL);
	}
}


void RB_EndSurface()
{
	shaderCommands_t *input;

	input = &tess;

	if(input->numIndexes == 0)
	{
		return;
	}

	if(input->indexes[SHADER_MAX_INDEXES - 1] != 0)
	{
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}
	if(input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0)
	{
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if(tess.shader == tr.shadowShader)
	{
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if(r_debugSort->integer && r_debugSort->integer < tess.shader->sort)
	{
		return;
	}

	// update performance counters
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	// call off to shader specific tess end function
	tess.currentStageIteratorFunc();

	// draw debugging stuff
	if(r_showtris->integer)
	{
		DrawTris(input);
	}
	if(r_shownormals->integer)
	{
		DrawNormals(input);
	}
	if(r_showTangentSpaces->integer)
	{
		DrawTangentSpaces(input);
	}
	if(r_showDeluxels->integer)
	{
		DrawDeluxels(input);
	}

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

	GLimp_LogComment("----------\n");
}
