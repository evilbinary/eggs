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
#include "SDL_timer.h"
#include "SDL_joystick.h"
/* YiYiYa: 手柄诊断打印开关（0=默认静默） */
#define JOY_DIAG 0
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
    /* 先判特殊键：xwin 给方向键等的 keycode（0x47-0x53 等）与 ASCII 大写
     * 字母编码重叠，若不先判会被下面的 A-Z 分支吃掉（Up→'H'）。 */
    switch (key) {
    case 0x48: return SDL_SCANCODE_UP;
    case 0x50: return SDL_SCANCODE_DOWN;
    case 0x4B: return SDL_SCANCODE_LEFT;
    case 0x4D: return SDL_SCANCODE_RIGHT;
    case 0x47: return SDL_SCANCODE_HOME;
    case 0x4F: return SDL_SCANCODE_END;
    case 0x49: return SDL_SCANCODE_PAGEUP;
    case 0x51: return SDL_SCANCODE_PAGEDOWN;
    case 0x52: return SDL_SCANCODE_INSERT;
    case 0x53: return SDL_SCANCODE_DELETE;
    case 0x10: return SDL_SCANCODE_LSHIFT;
    case 0x11: return SDL_SCANCODE_LCTRL;
    case 0x12: return SDL_SCANCODE_LALT;
    case 0x14: return SDL_SCANCODE_CAPSLOCK;
    default: break;
    }
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
  /* YiYiYa: 像 SDL1 一样在这里刷新手柄状态。
   * SDL2 默认【不会】在 PumpEvents 里刷手柄（SDL1 会 ✓）；应用若不显式调用
   * SDL_GameControllerUpdate() 或开事件态，就会很久才拿到一次方向变化 ✗
   * （实测：要长按很久才有反应、松手后 2~3 秒页面才更新）。
   * SDL_JoystickUpdate() 自带重入保护 ✓，放在这里最保险——不依赖应用怎么写 ✓。 */
  SDL_JoystickUpdate();

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
        /* YiYiYa: 诊断——xwin 键盘事件到达 SDL 的时刻（毫秒，与 UI 侧同一钟） */
        if (JOY_DIAG) printf("YIYIYA pump key=%x down t=%u\n", (unsigned)e.key,
               (unsigned)SDL_GetTicks());
        SDL_SendKeyboardKey(SDL_PRESSED, DUMMY_KeyToScancode(e.key));
        break;
      case KEY_PRESS_UP:
        if (JOY_DIAG) printf("YIYIYA pump key=%x up   t=%u\n", (unsigned)e.key,
               (unsigned)SDL_GetTicks());
        SDL_SendKeyboardKey(SDL_RELEASED, DUMMY_KeyToScancode(e.key));
        break;
      default:
        break;
    }
  }
  /* 勿在 PumpEvents 里 screen_flush：PollEvent 每帧都会泵事件，
   * 会导致无 Present 也整屏重绘闪烁；刷新只走 UpdateWindowFramebuffer。 */
}

#endif /* SDL_VIDEO_DRIVER_DUMMY */

/* vi: set ts=4 sw=4 expandtab: */
