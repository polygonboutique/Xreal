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

/**************************************************************************************************************************************
 Private sound definations.
**************************************************************************************************************************************/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#if USE_OPENAL
#ifdef _MSC_VER
// MSVC users must install the OpenAL SDK which doesn't use the AL/*.h scheme.
#include <al.h>
#include <alc.h>
#elif defined(MACOS_X)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#endif
#define PAINTBUFFER_SIZE 4096 // this is in samples

#define SND_CHUNK_SIZE 1024 // samples
#define SND_CHUNK_SIZE_FLOAT (SND_CHUNK_SIZE / 2) // floats
#define SND_CHUNK_SIZE_BYTE (SND_CHUNK_SIZE * 2) // floats

typedef struct {
	int left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int right;
} portable_samplepair_t;

typedef struct adpcm_state {
	short sample;	// previous output value
	char index;		// index into stepsize table
} adpcm_state_t;

typedef struct sndBuffer_s {
	short sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s *next;
	int size;
	adpcm_state_t adpcm;
} sndBuffer;

typedef struct sfx_s {
	sndBuffer *soundData;
	qboolean defaultSound;		// couldn't be loaded, so use buzz
	qboolean inMemory;			// not in Memory
	int soundLength;
	char soundName[MAX_QPATH];
	int lastTimeUsed;
	struct sfx_s *next;
} sfx_t;

typedef struct {
	int channels;
	int samples;			// mono samples in buffer
	int submission_chunk;	// don't mix less than this #
	int samplebits;
	int speed;
	byte *buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE 0x7fffffff
#define MAX_DOPPLER_SCALE 50.0f // arbitrary

typedef struct loopSound_s {
	vec3_t origin;
	vec3_t velocity;
	sfx_t *sfx;
	int mergeFrame;
	qboolean active;
	qboolean kill;
	qboolean doppler;
	float dopplerScale;
	float oldDopplerScale;
	int framenum;
	int range;
} loopSound_t;

typedef struct {
	int allocTime;
	int startSample;		// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int entnum;				// to allow overriding a specific sound
	int entchannel;			// to allow overriding a specific sound
	int leftvol;			// 0-255 volume after spatialization
	int rightvol;			// 0-255 volume after spatialization
	int master_vol;			// 0-255 volume before spatialization
	int range;
	float dopplerScale;
	float oldDopplerScale;
	vec3_t origin;			// only use if fixed_origin is set
	qboolean fixed_origin;	// use origin instead of fetching entnum's origin
	sfx_t *thesfx;			// sfx structure
	qboolean doppler;
} channel_t;

#define WAV_FORMAT_PCM 1

typedef struct {
	int format;
	int rate;
	int width;
	int channels;
	int samples;
	int dataofs;	// chunk starts this many bytes from file start
} wavinfo_t;
// interface between Q3 sound "api" and the sound backend
typedef struct {
	void (*Shutdown)(void);
	void (*StartSound)(vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int range);
	void (*StartLocalSound)(sfxHandle_t sfx, int channelNum);
	void (*StartBackgroundTrack)(const char *intro, const char *loop);
	void (*StopBackgroundTrack)(void);
	void (*RawSamples)(int stream, int samples, int rate, int width, int channels, const byte *data, float volume);
	void (*StopAllSounds)(void);
	void (*ClearLoopingSounds)(qboolean killall);
	void (*AddLoopingSound)(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range);
	void (*AddRealLoopingSound)(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range);
	void (*StopLoopingSound)(int entityNum);
	void (*Respatialize)(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
	void (*UpdateEntityPosition)(int entityNum, const vec3_t origin);
	void (*Update)(void);
	void (*DisableSounds)(void);
	void (*BeginRegistration)(void);
	sfxHandle_t (*RegisterSound)(const char *sample);
	void (*ClearSoundBuffer)(void);
	void (*SoundInfo)(void);
	void (*SoundList)(void);
#ifdef USE_VOIP
	void (*StartCapture)(void);
	int (*AvailableCaptureSamples)(void);
	void (*Capture)(int samples, byte *data);
	void (*StopCapture)(void);
	void (*MasterGain)(float gain);
#endif
} soundInterface_t;

/*
=======================================================================================================================================

	SYSTEM SPECIFIC FUNCTIONS

=======================================================================================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);
// gets the current DMA position
int SNDDMA_GetDMAPos(void);
// shutdown the DMA xfer
void SNDDMA_Shutdown(void);
void SNDDMA_BeginPainting(void);
void SNDDMA_Submit(void);

#define MAX_CHANNELS 96

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern int numLoopChannels;
extern int s_paintedtime;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern dma_t dma;

#define MAX_RAW_SAMPLES 16384
#define MAX_RAW_STREAMS 128

extern portable_samplepair_t s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
extern int s_rawend[MAX_RAW_STREAMS];
extern cvar_t *s_volume;
extern cvar_t *s_musicVolume;
extern cvar_t *s_muted;
extern cvar_t *s_doppler;
extern cvar_t *s_testsound;
qboolean S_LoadSound(sfx_t *sfx);
void SND_free(sndBuffer *v);
sndBuffer *SND_malloc(void);
void SND_setup(void);
void S_PaintChannels(int endtime);
void S_memoryLoad(sfx_t *sfx);
void S_FreeOldestSound(void);
// spatializes a channel
void S_Spatialize(channel_t *ch);

qboolean S_Base_Init(soundInterface_t *si);
// OpenAL stuff
typedef enum {
	SRCPRI_AMBIENT = 0,	// ambient sound effects
	SRCPRI_ENTITY,		// entity sound effects
	SRCPRI_ONESHOT,		// one-shot sounds
	SRCPRI_LOCAL,		// local sounds
	SRCPRI_STREAM		// streams (music, cutscenes)
} alSrcPriority_t;

typedef int srcHandle_t;
qboolean S_AL_Init(soundInterface_t *si);
