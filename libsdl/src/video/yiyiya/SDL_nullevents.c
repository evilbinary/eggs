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

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include "SDL.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"

#include "screen.h"

void DUMMY_PumpEvents(_THIS)
{
	  event_t e;
  int ret = event_poll(&e);
  if (ret <= 0) return;
  // printf("poll event\n");

  //   e.mouse.y = screen->height - e.mouse.y;
  switch (e.type) {
    case MOUSE_MOTION:
      printf(".");

      break;
    case MOUSE_BUTTON_DOWN:
      printf("down\n");

      // SDL_PrivateMouseButton(SDL_PRESSED,

      break;
    case MOUSE_BUTTON_UP:
      printf("up\n");
      break;

    case KEY_DOWN:
      // SDL_SendKeyboardKey(SDL_PRESSED, e.key);

      break;
    case KEY_UP:
      // SDL_SendKeyboardKey(SDL_RELEASED, e.key);
      break;
    default:
      break;
  }
  return;
}

void DUMMY_InitOSKeymap(_THIS)
{
	/* do nothing. */
}

/* end of SDL_nullevents.c ... */

