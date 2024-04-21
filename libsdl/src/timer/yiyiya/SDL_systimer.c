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
#include "SDL_config.h"

#include <sys/time.h>

#if defined(SDL_TIMER_DUMMY) || defined(SDL_TIMERS_DISABLED)

#include "SDL_timer.h"
#include "../SDL_timer_c.h"

#ifdef HAVE_CLOCK_GETTIME
static struct timespec start;
#else
static struct timeval start;
#endif /* HAVE_CLOCK_GETTIME */

static SDL_bool ticks_started = SDL_FALSE;

void SDL_StartTicks(void)
{
	/* Set first ticks value */
#if HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC,&start);
#else
	gettimeofday(&start, NULL);
#endif

}

Uint32 SDL_GetTicks (void)
{

#if HAVE_CLOCK_GETTIME
	Uint32 ticks;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC,&now);
	ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_nsec-start.tv_nsec)/1000000;
	return(ticks);
#else
	Uint32 ticks;
	struct timeval now;
	gettimeofday(&now, NULL);
	ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
	return(ticks);
#endif
}

void SDL_Delay (Uint32 ms)
{
	
	usleep(1000*ms);
}

#include "SDL_thread.h"

/* Data to handle a single periodic alarm */
static int timer_alive = 0;
static SDL_Thread *timer = NULL;

static int RunTimer(void *unused)
{
	while ( timer_alive ) {
		if ( SDL_timer_running ) {
			SDL_ThreadedTimerCheck();
		}
		SDL_Delay(1);
	}
	return(0);
}

/* This is only called if the event thread is not running */
int SDL_SYS_TimerInit(void)
{
	timer_alive = 1;
	timer = SDL_CreateThread(RunTimer, NULL);
	if ( timer == NULL )
		return(-1);
	return(SDL_SetTimerThreaded(1));
}

void SDL_SYS_TimerQuit(void)
{
	timer_alive = 0;
	if ( timer ) {
		SDL_WaitThread(timer, NULL);
		timer = NULL;
	}
}

int SDL_SYS_StartTimer(void)
{
	SDL_SetError("Internal logic error: threaded timer in use");
	return(-1);
}

void SDL_SYS_StopTimer(void)
{
	return;
}



#endif /* SDL_TIMER_DUMMY || SDL_TIMERS_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
