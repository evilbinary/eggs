/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifndef SDL_dspaudio_h_
#define SDL_dspaudio_h_

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS   SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    /* The file descriptor for the audio device */
    int audio_fd;

    /* Raw mixing buffer */
    Uint8 *mixbuf;
    int mixlen;

        /* Support for audio timing using a timer, in addition to select() */
	float frame_ticks;
	float next_frame;

};

/* Old variable names */
#define frame_ticks		(this->hidden->frame_ticks)
#define next_frame		(this->hidden->next_frame)

#define FUDGE_TICKS 10      /* The scheduler overhead ticks per frame */


#define AFMT_U16_LE 16
#define AFMT_S16_LE 16
#define AFMT_S16_BE 16
#define AFMT_U8 8

#define SNDCTL_DSP_SETFMT 11
#define SNDCTL_DSP_GETFMTS 22
#define SNDCTL_DSP_CHANNELS 33
#define SNDCTL_DSP_SPEED 44
#define SNDCTL_DSP_SETFRAGMENT 55
#define SNDCTL_DSP_SETTRIGGER 66


#define PCM_ENABLE_OUTPUT 1

#endif /* SDL_dspaudio_h_ */
/* vi: set ts=4 sw=4 expandtab: */
