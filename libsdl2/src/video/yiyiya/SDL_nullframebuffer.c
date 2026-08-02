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

#include "../SDL_sysvideo.h"
#include "SDL_nullframebuffer_c.h"

#include "screen.h"


#define DUMMY_SURFACE   "_SDL_DummySurface"
#define DUMMY_OWN_FB    "_SDL_DummyOwnFB"

int SDL_DUMMY_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch)
{
    const Uint32 surface_format = SDL_PIXELFORMAT_ARGB8888;
    screen_info_t* screen = screen_info();
    if (!screen || !screen->buffer) {
        return SDL_SetError("libgui screen buffer is not initialized");
    }

    *format = surface_format;
    /* 行距必须与 xwin_blit 的步进一致（gscreen.width * 4），否则 LCD 逐行错位 */
    *pitch = (((screen->width * SDL_BYTESPERPIXEL(*format)) + 3) & ~3);
    Uint32 need = (Uint32)(*pitch) * window->h;
    if ((Uint32)screen->buffer_length < need) {
        /* 屏幕缓冲按旧/错误尺寸分配（fb 未就绪时），不够窗口用：
           自备缓冲，避免 SDL 填充越界写到堆外触发内核 fault。 */
        void* own = SDL_GetWindowData(window, DUMMY_OWN_FB);
        if (!own) {
            own = SDL_malloc(need);
            if (!own) {
                return SDL_OutOfMemory();
            }
            SDL_SetWindowData(window, DUMMY_OWN_FB, own);
        }
        *pixels = own;
    } else {
        *pixels = screen->buffer;
    }
    SDL_SetWindowData(window, DUMMY_SURFACE, (void *) 1);
    return 0;
}

int SDL_DUMMY_UpdateWindowFramebuffer(_THIS, SDL_Window * window, const SDL_Rect * rects, int numrects)
{
    (void)rects;
    (void)numrects;

    if (!SDL_GetWindowData(window, DUMMY_SURFACE)) {
        return SDL_SetError("Couldn't find yiyiya framebuffer for window");
    }
    /* 自备缓冲时：先把渲染结果拷进屏幕缓冲再 flush，保证 LCD 有输出 */
    void* own = SDL_GetWindowData(window, DUMMY_OWN_FB);
    if (own) {
        screen_info_t* screen = screen_info();
        if (screen && screen->buffer) {
            Uint32 n = (Uint32)screen->buffer_length;
            if (n > 0) {
                SDL_memcpy(screen->buffer, own, n);
            }
        }
    }
    /* Present 已在帧末调用；直接 present，勿节流以免丢帧 */
    screen_flush();
    return 0;
}

void SDL_DUMMY_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
    SDL_SetWindowData(window, DUMMY_SURFACE, NULL);
    void* own = SDL_GetWindowData(window, DUMMY_OWN_FB);
    if (own) {
        SDL_free(own);
        SDL_SetWindowData(window, DUMMY_OWN_FB, NULL);
    }
}

#endif /* SDL_VIDEO_DRIVER_DUMMY */

/* vi: set ts=4 sw=4 expandtab: */
