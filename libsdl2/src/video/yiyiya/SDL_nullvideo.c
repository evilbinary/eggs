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

#if SDL_VIDEO_DRIVER_DUMMY

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullframebuffer_c.h"

#include "screen.h"

#define DUMMYVID_DRIVER_NAME "yiyiya"

/* Initialization/Query functions */
static int DUMMY_VideoInit(_THIS);
static int DUMMY_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void DUMMY_VideoQuit(_THIS);
static int DUMMY_CreateWindow(_THIS, SDL_Window *window);
static int DUMMY_SetWindowInputFocus(_THIS, SDL_Window *window);
static void DUMMY_DestroyWindow(_THIS, SDL_Window *window);

/* DUMMY driver bootstrap functions */

static int
DUMMY_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if ((envr) && (SDL_strcmp(envr, DUMMYVID_DRIVER_NAME) == 0)) {
        return (1);
    }

    return (1);
}

static void
DUMMY_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
DUMMY_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    if (!DUMMY_Available()) {
        return (0);
    }

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return (0);
    }
    device->is_dummy = SDL_FALSE;

    /* Set the function pointers */
    device->VideoInit = DUMMY_VideoInit;
    device->VideoQuit = DUMMY_VideoQuit;
    device->SetDisplayMode = DUMMY_SetDisplayMode;
    device->CreateSDLWindow = DUMMY_CreateWindow;
    device->SetWindowInputFocus = DUMMY_SetWindowInputFocus;
    device->DestroyWindow = DUMMY_DestroyWindow;
    device->PumpEvents = DUMMY_PumpEvents;
    device->CreateWindowFramebuffer = SDL_DUMMY_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_DUMMY_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_DUMMY_DestroyWindowFramebuffer;

    device->free = DUMMY_DeleteDevice;
    

    return device;
}

VideoBootStrap DUMMY_bootstrap = {
    DUMMYVID_DRIVER_NAME, "SDL libgui video driver",
    DUMMY_CreateDevice
};


int
DUMMY_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    screen_init();
    screen_info_t* screen = screen_info();
    if (!screen) {
        return SDL_SetError("libgui screen_info is not available");
    }

    /* Use a fake 32-bpp desktop mode */
    SDL_zero(mode);
    if(screen->fb.format==1){ //nv12
        mode.format = SDL_PIXELFORMAT_NV12;
    }else{
        mode.format = SDL_PIXELFORMAT_ARGB8888;
    }
    mode.w = screen->width;
    mode.h = screen->height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_AddDisplayMode(&_this->displays[0], &mode);
    /* We're done! */
    return 0;
}

static int
DUMMY_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    (void)_this;
    (void)display;
    (void)mode;
    return 0;
}

static int
DUMMY_CreateWindow(_THIS, SDL_Window *window)
{
    (void)_this;
    if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        window->x = 0;
    }
    if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        window->y = 0;
    }
    window->driverdata = window;
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);
    return 0;
}

static int
DUMMY_SetWindowInputFocus(_THIS, SDL_Window *window)
{
    (void)_this;
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);
    return 0;
}

static void
DUMMY_DestroyWindow(_THIS, SDL_Window *window)
{
    (void)_this;
    if (SDL_GetMouseFocus() == window) {
        SDL_SetMouseFocus(NULL);
    }
    if (SDL_GetKeyboardFocus() == window) {
        SDL_SetKeyboardFocus(NULL);
    }
    window->driverdata = NULL;
}

void
DUMMY_VideoQuit(_THIS)
{
}

#endif /* SDL_VIDEO_DRIVER_DUMMY */

/* vi: set ts=4 sw=4 expandtab: */
