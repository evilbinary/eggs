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
#include "screen.h"

#if SDL_VIDEO_DRIVER_DUMMY

/* Being a null driver, there's no event stream. We just define stubs for
   most of the API. */

#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullvideo.h"

static Uint8
DUMMY_TranslateMouseButton(Uint32 button)
{
    switch (button & BUTTON_MASK) {
    case BUTTON_LEFT:
        return SDL_BUTTON_LEFT;
    case BUTTON_RIGHT:
        return SDL_BUTTON_RIGHT;
    case BUTTON_MIDDLE:
        return SDL_BUTTON_MIDDLE;
    default:
        return SDL_BUTTON_LEFT;
    }
}

static SDL_Scancode
DUMMY_KeyToScancode(Uint32 key)
{
    if (key >= 'a' && key <= 'z') {
        return (SDL_Scancode)(SDL_SCANCODE_A + (key - 'a'));
    }
    if (key >= 'A' && key <= 'Z') {
        return (SDL_Scancode)(SDL_SCANCODE_A + (key - 'A'));
    }
    if (key >= '1' && key <= '9') {
        return (SDL_Scancode)(SDL_SCANCODE_1 + (key - '1'));
    }
    if (key == '0') {
        return SDL_SCANCODE_0;
    }

    switch (key) {
    case '\n':
        return SDL_SCANCODE_RETURN;
    case '\b':
        return SDL_SCANCODE_BACKSPACE;
    case '\t':
        return SDL_SCANCODE_TAB;
    case ' ':
        return SDL_SCANCODE_SPACE;
    case 0x1B:
        return SDL_SCANCODE_ESCAPE;
    case '-':
        return SDL_SCANCODE_MINUS;
    case '=':
        return SDL_SCANCODE_EQUALS;
    case '[':
        return SDL_SCANCODE_LEFTBRACKET;
    case ']':
        return SDL_SCANCODE_RIGHTBRACKET;
    case '\\':
        return SDL_SCANCODE_BACKSLASH;
    case ';':
        return SDL_SCANCODE_SEMICOLON;
    case '\'':
        return SDL_SCANCODE_APOSTROPHE;
    case ',':
        return SDL_SCANCODE_COMMA;
    case '.':
        return SDL_SCANCODE_PERIOD;
    case '/':
        return SDL_SCANCODE_SLASH;
    default:
        return SDL_SCANCODE_UNKNOWN;
    }
}

void DUMMY_PumpEvents(_THIS) {
  screen_info_t *screen = screen_info();
  event_t e;
  while (event_poll(&e) > 0) {
    SDL_Window *window = SDL_GetMouseFocus();
    int mouse_y = screen ? (screen->height - e.mouse.y) : e.mouse.y;
    switch (e.type) {
      case MOUSE_MOTION:
        SDL_SendMouseMotion(window, 0, 0, e.mouse.x, mouse_y);
        break;
      case MOUSE_BUTTON_DOWN:
        SDL_SendMouseMotion(window, 0, 0, e.mouse.x, mouse_y);
        SDL_SendMouseButton(window, 0, SDL_PRESSED,
                            DUMMY_TranslateMouseButton(e.mouse.sate));
        break;
      case MOUSE_BUTTON_UP:
        SDL_SendMouseMotion(window, 0, 0, e.mouse.x, mouse_y);
        SDL_SendMouseButton(window, 0, SDL_RELEASED,
                            DUMMY_TranslateMouseButton(e.mouse.sate));
        break;
      case KEY_PRESS_DOWN:
        SDL_SendKeyboardKey(SDL_PRESSED, DUMMY_KeyToScancode(e.key));
        break;
      case KEY_PRESS_UP:
        SDL_SendKeyboardKey(SDL_RELEASED, DUMMY_KeyToScancode(e.key));
        break;
      default:
        break;
    }
  }
  if (screen && screen_get_mode() == SCREEN_MODE_XWIN) {
    screen_flush();
  }
}

#endif /* SDL_VIDEO_DRIVER_DUMMY */

/* vi: set ts=4 sw=4 expandtab: */
