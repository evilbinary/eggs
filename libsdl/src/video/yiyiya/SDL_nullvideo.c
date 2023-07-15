/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

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
#include "screen.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullmouse_c.h"

#define DUMMYVID_DRIVER_NAME "dummy"

/* Initialization/Query functions */
static int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void DUMMY_VideoQuit(_THIS);

/* Hardware surface functions */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

static int DUMMY_FlipHWSurface(_THIS, SDL_Surface *surface);

/* DUMMY driver bootstrap functions */

static int DUMMY_Available(void)
{
	const char *envr = SDL_getenv("SDL_VIDEODRIVER");
	if ((envr) && (SDL_strcmp(envr, DUMMYVID_DRIVER_NAME) == 0)) {
		return(1);
	}

	return(1);
}

static void DUMMY_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *DUMMY_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;
    printf("sdl create device\n");

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		SDL_memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			SDL_free(device);
		}
		return(0);
	}
	SDL_memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	device->VideoInit = DUMMY_VideoInit;
	device->ListModes = DUMMY_ListModes;
	device->SetVideoMode = DUMMY_SetVideoMode;
	device->CreateYUVOverlay = NULL;
	device->SetColors = DUMMY_SetColors;
	device->UpdateRects = DUMMY_UpdateRects;
	device->VideoQuit = DUMMY_VideoQuit;
	device->AllocHWSurface = DUMMY_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = DUMMY_LockHWSurface;
	device->UnlockHWSurface = DUMMY_UnlockHWSurface;
	device->FlipHWSurface = DUMMY_FlipHWSurface;
	device->FreeHWSurface = DUMMY_FreeHWSurface;
	device->SetCaption = NULL;
	device->SetIcon = NULL;
	device->IconifyWindow = NULL;
	device->GrabInput = NULL;
	device->GetWMInfo = NULL;
	device->InitOSKeymap = DUMMY_InitOSKeymap;
	device->PumpEvents = DUMMY_PumpEvents;

	device->free = DUMMY_DeleteDevice;

	return device;
}

VideoBootStrap DUMMY_bootstrap = {
	DUMMYVID_DRIVER_NAME, "SDL dummy video driver",
	DUMMY_Available, DUMMY_CreateDevice
};


static int DUMMY_AddMode(_THIS, int bpp, int w, int h)
{
	SDL_Rect *mode;
	int i, index;
	int next_mode;

	/* Check to see if we already have this mode */
	if ( bpp < 8 ) {  /* Not supported */
		return(0);
	}
	
	int **SDL_nummodes= this->hidden->SDL_nummodes;
	SDL_Rect ***SDL_modelist =this->hidden->SDL_modelist;



	index = ((bpp+7)/8)-1;
	for ( i=0; i<SDL_nummodes[index]; ++i ) {
		mode = SDL_modelist[index][i];
		if ( (mode->w == w) && (mode->h == h) ) {
			return(0);
		}
	}

	/* Set up the new video mode rectangle */
	mode = (SDL_Rect *)SDL_malloc(sizeof *mode);
	if ( mode == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	mode->x = 0;
	mode->y = 0;
	mode->w = w;
	mode->h = h;

	/* Allocate the new list of modes, and fill in the new mode */
	next_mode = SDL_nummodes[index];
	SDL_modelist[index] = (SDL_Rect **)
	       SDL_realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		SDL_free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	/*
	fprintf(stderr, "WARNING: You are using the SDL dummy video driver!\n");
	*/

 	screen_init();
    screen_info_t* screen = screen_info();

	this->info.current_w = screen->width;
	this->info.current_h = screen->height;

	this->info.video_mem = screen->fb.framebuffer_length /1024;
	this->info.wm_available=0;

	// this->screen = screen->buffer;
	this->hidden->buffer = screen->buffer;

	/* Determine the screen depth (use default 8-bit depth) */
	/* we change this during the SDL_SetVideoMode implementation... */
	vformat->BitsPerPixel = screen->bpp;
	// vformat->BytesPerPixel = screen->bpp/8;


	switch (vformat->BitsPerPixel){
		case 15:
		case 16:
			vformat->Bmask = 0x00007c00;
			vformat->Gmask = 0x000003e0;
			vformat->Rmask = 0x0000001f;
			vformat->BitsPerPixel = 16; /* SDL wants actual number of bits used */
			vformat->BytesPerPixel = 2;
			break;
		case 24:
		case 32:
			vformat->Bmask = 0x00ff0000;
			vformat->Gmask = 0x0000ff00;
			vformat->Rmask = 0x000000ff;
			vformat->BytesPerPixel = 4;
			break;

		default:
			vformat->Bmask = 0;
			vformat->Gmask = 0;
			vformat->Rmask = 0;
			vformat->BytesPerPixel = 1;			
			break;
	}

	DUMMY_AddMode(this,vformat->BitsPerPixel,this->info.current_w,this->info.current_h);

	/* We're done! */
	return(0);
}


SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
   	return(this->hidden->SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	// if ( this->hidden->buffer ) {
	// 	SDL_free( this->hidden->buffer );
	// }

	// this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));
	// if ( ! this->hidden->buffer ) {
	// 	SDL_SetError("Couldn't allocate buffer for requested mode");
	// 	return(NULL);
	// }

/* 	printf("Setting mode %dx%d\n", width, height); */

	SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));

	/* Allocate the new pixel format for the screen */
	if ( ! SDL_ReallocFormat(current, bpp, 0, 0, 0, 0) ) {
		SDL_free(this->hidden->buffer);
		this->hidden->buffer = NULL;
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		return(NULL);
	}

	/* Set up the new mode framebuffer */
	current->flags = flags & SDL_FULLSCREEN;
	this->hidden->w = current->w = width;
	this->hidden->h = current->h = height;
	current->pitch = current->w * (bpp / 8);
	current->pixels = this->hidden->buffer;

	/* We're done */
	return(current);
}

/* We don't actually allow hardware surfaces other than the main one */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	SDL_SetError("DUMMY_AllocHWSurface");
	return(-1);
}
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}

static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	/* do nothing. */
	screen_flush();
	
}

static int DUMMY_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	
	printf("flip\n");
	return 0;
}

int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{

	printf("set colors\n");

	/* do nothing of note. */
	return(1);
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void DUMMY_VideoQuit(_THIS)
{
	if (this->screen->pixels != NULL)
	{
		SDL_free(this->screen->pixels);
		this->screen->pixels = NULL;
	}
}
