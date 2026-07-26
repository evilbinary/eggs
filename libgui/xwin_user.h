/*******************************************************************
 * Copyright 2021-present evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 * X Window System - User Space API
 ********************************************************************/
#ifndef XWIN_USER_H
#define XWIN_USER_H

#include "types.h"

// ========== 用户空间窗口句柄 ==========
typedef u32 xwin_handle_t;
#define XWIN_INVALID_HANDLE  0

#define XWIN_FLAG_VISIBLE    0x01
#define XWIN_FLAG_FOCUSABLE  0x02
#define XWIN_FLAG_BORDERED   0x04
#define XWIN_FLAG_DRAGGABLE  0x08
#define XWIN_FLAG_RESIZABLE  0x10
#define XWIN_FLAG_DIRECT     0x20  /* 直接写 LCD，无离屏拷贝 */

// ========== 用户空间 API (通过 syscall 调用) ==========

// 窗口管理
xwin_handle_t xwin_create(i32 x, i32 y, u32 width, u32 height, const char* title);
xwin_handle_t xwin_create_flags(i32 x, i32 y, u32 width, u32 height,
                                const char* title, u32 flags);
void xwin_destroy(xwin_handle_t win);
void xwin_move(xwin_handle_t win, i32 x, i32 y);
void xwin_resize(xwin_handle_t win, u32 width, u32 height);
void xwin_show(xwin_handle_t win, int show);
void xwin_set_title(xwin_handle_t win, const char* title);
void xwin_set_bg_color(xwin_handle_t win, u32 color);

// 绘图
void xwin_clear(xwin_handle_t win);
void xwin_fill_rect(xwin_handle_t win, i32 x, i32 y, u32 w, u32 h, u32 color);
void xwin_draw_rect(xwin_handle_t win, i32 x, i32 y, u32 w, u32 h, u32 color);
void xwin_draw_line(xwin_handle_t win, i32 x1, i32 y1, i32 x2, i32 y2, u32 color);
void xwin_draw_text(xwin_handle_t win, i32 x, i32 y, const char* text, u32 color);
void xwin_blit(xwin_handle_t win, i32 x, i32 y, const u32* data, u32 w, u32 h);

// 事件
int xwin_get_event(void* event);
void xwin_process_events(void);

// 渲染
void xwin_render(void);
void xwin_update(xwin_handle_t win);
u32* xwin_get_fb(xwin_handle_t win);
u32 xwin_get_ticks(void);

// ========== Syscall 编号 ==========
#define SYS_XWIN_BASE          0x5000

#define SYS_XWIN_CREATE        (SYS_XWIN_BASE + 0)
#define SYS_XWIN_DESTROY       (SYS_XWIN_BASE + 1)
#define SYS_XWIN_MOVE          (SYS_XWIN_BASE + 2)
#define SYS_XWIN_RESIZE        (SYS_XWIN_BASE + 3)
#define SYS_XWIN_SHOW          (SYS_XWIN_BASE + 4)
#define SYS_XWIN_SET_TITLE     (SYS_XWIN_BASE + 5)
#define SYS_XWIN_SET_BG_COLOR  (SYS_XWIN_BASE + 6)

#define SYS_XWIN_CLEAR         (SYS_XWIN_BASE + 10)
#define SYS_XWIN_FILL_RECT     (SYS_XWIN_BASE + 11)
#define SYS_XWIN_DRAW_RECT     (SYS_XWIN_BASE + 12)
#define SYS_XWIN_DRAW_LINE     (SYS_XWIN_BASE + 13)
#define SYS_XWIN_DRAW_TEXT     (SYS_XWIN_BASE + 14)
#define SYS_XWIN_BLIT          (SYS_XWIN_BASE + 15)

#define SYS_XWIN_GET_EVENT     (SYS_XWIN_BASE + 20)
#define SYS_XWIN_PROCESS_EVENTS (SYS_XWIN_BASE + 21)
#define SYS_XWIN_RENDER        (SYS_XWIN_BASE + 22)
#define SYS_XWIN_UPDATE        (SYS_XWIN_BASE + 23)
#define SYS_XWIN_GET_FB        (SYS_XWIN_BASE + 24) /* 返回窗口/LCD VA */
#define SYS_XWIN_GET_TICKS     (SYS_XWIN_BASE + 25) /* schedule ticks */



#endif // XWIN_USER_H
