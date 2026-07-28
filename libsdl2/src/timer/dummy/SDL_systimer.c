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

#if defined(SDL_TIMER_DUMMY) || defined(SDL_TIMERS_DISABLED)

#include "SDL_timer.h"

#ifdef __YIYIYA__
#include <time.h>
#include <sys/time.h>

static struct timespec yiyiya_start_ts;
static int yiyiya_ticks_inited = 0;
#endif

static SDL_bool ticks_started = SDL_FALSE;

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;

#ifdef __YIYIYA__
    clock_gettime(CLOCK_MONOTONIC, &yiyiya_start_ts);
    yiyiya_ticks_inited = 1;
#endif
}

void
SDL_TicksQuit(void)
{
    ticks_started = SDL_FALSE;
}

Uint64
SDL_GetTicks64(void)
{
    if (!ticks_started) {
        SDL_TicksInit();
    }

#ifdef __YIYIYA__
    if (yiyiya_ticks_inited) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        Uint64 elapsed_ms = (Uint64)(now.tv_sec - yiyiya_start_ts.tv_sec) * 1000ULL
                          + (Uint64)(now.tv_nsec - yiyiya_start_ts.tv_nsec) / 1000000ULL;
        return elapsed_ms;
    }
#endif

    SDL_Unsupported();
    return 0;
}

Uint64
SDL_GetPerformanceCounter(void)
{
    return SDL_GetTicks();
}

Uint64
SDL_GetPerformanceFrequency(void)
{
    return 1000;
}

void
SDL_Delay(Uint32 ms)
{
#ifdef __YIYIYA__
    if (ms > 0) {
        struct timespec req;
        req.tv_sec = ms / 1000U;
        req.tv_nsec = (long)(ms % 1000U) * 1000000L;
        nanosleep(&req, NULL);
    }
#else
    SDL_Unsupported();
#endif
}

#endif /* SDL_TIMER_DUMMY || SDL_TIMERS_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
