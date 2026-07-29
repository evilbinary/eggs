/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include "screen.h"

#include "bmp.h"
#include "event.h"
#include "fcntl.h"
#include "image.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "unistd.h"

// 启用 XWIN 用户态实现
#define XWIN_USER_IMPL
#include "xwin_user.h"

#ifdef MIYOO
#define DB_BUFFER 1
#define NV12 1
// #define ROTATE_180 1
#endif

screen_info_t gscreen;

/* 用户态耗时：用 xwin_get_ticks（与内核 schedule tick 一致，1tick=1ms@1kHz） */
static u32 screen_now_ms(void) { return xwin_get_ticks(); }

static u32 g_acc_fill_ms;
static u32 g_acc_blit_ms;
static u32 g_acc_update_ms;
static u32 g_acc_render_ms;
static u32 g_acc_frames;
static u32 g_fps_t0;
static int g_stats_inited;

static void screen_stats_frame_done(void) {
  g_acc_frames++;
  if (g_acc_frames < 60) return;
  {
    u32 now = screen_now_ms();
    u32 dt = now - g_fps_t0;
    u32 fps = dt > 0 ? (g_acc_frames * 1000u) / dt : 0;
    printf("gui: app_fps=%d fill=%dms blit=%dms update=%dms render=%dms "
           "direct=%d (n=%d dt=%d)\n",
           fps, g_acc_fill_ms, g_acc_blit_ms, g_acc_update_ms, g_acc_render_ms,
           (int)(gscreen.screen_mode == SCREEN_MODE_XWIN), g_acc_frames, dt);
    g_acc_fill_ms = g_acc_blit_ms = g_acc_update_ms = g_acc_render_ms = 0;
    g_acc_frames = 0;
    g_fps_t0 = now;
  }
}

u8 SCREEN_ASCII[] = {

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*" ",32*/
    0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00,
    0x18, 0x18, 0x00, 0x00, /*"!",33*/
    0x00, 0x48, 0x6C, 0x24, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*""",34*/
    0x00, 0x00, 0x00, 0x24, 0x24, 0x24, 0x7F, 0x12, 0x12, 0x12, 0x7F, 0x12,
    0x12, 0x12, 0x00, 0x00, /*"#",35*/
    0x00, 0x00, 0x08, 0x1C, 0x2A, 0x2A, 0x0A, 0x0C, 0x18, 0x28, 0x28, 0x2A,
    0x2A, 0x1C, 0x08, 0x08, /*"$",36*/
    0x00, 0x00, 0x00, 0x22, 0x25, 0x15, 0x15, 0x15, 0x2A, 0x58, 0x54, 0x54,
    0x54, 0x22, 0x00, 0x00, /*"%",37*/
    0x00, 0x00, 0x00, 0x0C, 0x12, 0x12, 0x12, 0x0A, 0x76, 0x25, 0x29, 0x11,
    0x91, 0x6E, 0x00, 0x00, /*"&",38*/
    0x00, 0x06, 0x06, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"'",39*/
    0x00, 0x40, 0x20, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x10,
    0x10, 0x20, 0x40, 0x00, /*"(",40*/
    0x00, 0x02, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08,
    0x08, 0x04, 0x02, 0x00, /*")",41*/
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x6B, 0x1C, 0x1C, 0x6B, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, /*"*",42*/
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x7F, 0x08, 0x08, 0x08,
    0x08, 0x00, 0x00, 0x00, /*"+",43*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x04, 0x03, /*",",44*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"-",45*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x00, 0x00, /*".",46*/
    0x00, 0x00, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04,
    0x04, 0x02, 0x02, 0x00, /*"/",47*/
    0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x24, 0x18, 0x00, 0x00, /*"0",48*/
    0x00, 0x00, 0x00, 0x08, 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x3E, 0x00, 0x00, /*"1",49*/
    0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x20, 0x20, 0x10, 0x08, 0x04,
    0x42, 0x7E, 0x00, 0x00, /*"2",50*/
    0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x20, 0x18, 0x20, 0x40, 0x40, 0x42,
    0x22, 0x1C, 0x00, 0x00, /*"3",51*/
    0x00, 0x00, 0x00, 0x20, 0x30, 0x28, 0x24, 0x24, 0x22, 0x22, 0x7E, 0x20,
    0x20, 0x78, 0x00, 0x00, /*"4",52*/
    0x00, 0x00, 0x00, 0x7E, 0x02, 0x02, 0x02, 0x1A, 0x26, 0x40, 0x40, 0x42,
    0x22, 0x1C, 0x00, 0x00, /*"5",53*/
    0x00, 0x00, 0x00, 0x38, 0x24, 0x02, 0x02, 0x1A, 0x26, 0x42, 0x42, 0x42,
    0x24, 0x18, 0x00, 0x00, /*"6",54*/
    0x00, 0x00, 0x00, 0x7E, 0x22, 0x22, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x00, 0x00, /*"7",55*/
    0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x24, 0x18, 0x24, 0x42, 0x42,
    0x42, 0x3C, 0x00, 0x00, /*"8",56*/
    0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x64, 0x58, 0x40, 0x40,
    0x24, 0x1C, 0x00, 0x00, /*"9",57*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x18, 0x00, 0x00, /*":",58*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x08, 0x04, /*";",59*/
    0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10,
    0x20, 0x40, 0x00, 0x00, /*"<",60*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"=",61*/
    0x00, 0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08,
    0x04, 0x02, 0x00, 0x00, /*">",62*/
    0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x46, 0x40, 0x20, 0x10, 0x10, 0x00,
    0x18, 0x18, 0x00, 0x00, /*"?",63*/
    0x00, 0x00, 0x00, 0x1C, 0x22, 0x5A, 0x55, 0x55, 0x55, 0x55, 0x2D, 0x42,
    0x22, 0x1C, 0x00, 0x00, /*"@",64*/
    0x00, 0x00, 0x00, 0x08, 0x08, 0x18, 0x14, 0x14, 0x24, 0x3C, 0x22, 0x42,
    0x42, 0xE7, 0x00, 0x00, /*"A",65*/
    0x00, 0x00, 0x00, 0x1F, 0x22, 0x22, 0x22, 0x1E, 0x22, 0x42, 0x42, 0x42,
    0x22, 0x1F, 0x00, 0x00, /*"B",66*/
    0x00, 0x00, 0x00, 0x7C, 0x42, 0x42, 0x01, 0x01, 0x01, 0x01, 0x01, 0x42,
    0x22, 0x1C, 0x00, 0x00, /*"C",67*/
    0x00, 0x00, 0x00, 0x1F, 0x22, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x22, 0x1F, 0x00, 0x00, /*"D",68*/
    0x00, 0x00, 0x00, 0x3F, 0x42, 0x12, 0x12, 0x1E, 0x12, 0x12, 0x02, 0x42,
    0x42, 0x3F, 0x00, 0x00, /*"E",69*/
    0x00, 0x00, 0x00, 0x3F, 0x42, 0x12, 0x12, 0x1E, 0x12, 0x12, 0x02, 0x02,
    0x02, 0x07, 0x00, 0x00, /*"F",70*/
    0x00, 0x00, 0x00, 0x3C, 0x22, 0x22, 0x01, 0x01, 0x01, 0x71, 0x21, 0x22,
    0x22, 0x1C, 0x00, 0x00, /*"G",71*/
    0x00, 0x00, 0x00, 0xE7, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42,
    0x42, 0xE7, 0x00, 0x00, /*"H",72*/
    0x00, 0x00, 0x00, 0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x3E, 0x00, 0x00, /*"I",73*/
    0x00, 0x00, 0x00, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x11, 0x0F, /*"J",74*/
    0x00, 0x00, 0x00, 0x77, 0x22, 0x12, 0x0A, 0x0E, 0x0A, 0x12, 0x12, 0x22,
    0x22, 0x77, 0x00, 0x00, /*"K",75*/
    0x00, 0x00, 0x00, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x42, 0x7F, 0x00, 0x00, /*"L",76*/
    0x00, 0x00, 0x00, 0x77, 0x36, 0x36, 0x36, 0x36, 0x2A, 0x2A, 0x2A, 0x2A,
    0x2A, 0x6B, 0x00, 0x00, /*"M",77*/
    0x00, 0x00, 0x00, 0xE3, 0x46, 0x46, 0x4A, 0x4A, 0x52, 0x52, 0x52, 0x62,
    0x62, 0x47, 0x00, 0x00, /*"N",78*/
    0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x22, 0x1C, 0x00, 0x00, /*"O",79*/
    0x00, 0x00, 0x00, 0x3F, 0x42, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x02, 0x02,
    0x02, 0x07, 0x00, 0x00, /*"P",80*/
    0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x41, 0x41, 0x41, 0x41, 0x4D, 0x53,
    0x32, 0x1C, 0x60, 0x00, /*"Q",81*/
    0x00, 0x00, 0x00, 0x3F, 0x42, 0x42, 0x42, 0x3E, 0x12, 0x12, 0x22, 0x22,
    0x42, 0xC7, 0x00, 0x00, /*"R",82*/
    0x00, 0x00, 0x00, 0x7C, 0x42, 0x42, 0x02, 0x04, 0x18, 0x20, 0x40, 0x42,
    0x42, 0x3E, 0x00, 0x00, /*"S",83*/
    0x00, 0x00, 0x00, 0x7F, 0x49, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x1C, 0x00, 0x00, /*"T",84*/
    0x00, 0x00, 0x00, 0xE7, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x3C, 0x00, 0x00, /*"U",85*/
    0x00, 0x00, 0x00, 0xE7, 0x42, 0x42, 0x22, 0x24, 0x24, 0x14, 0x14, 0x18,
    0x08, 0x08, 0x00, 0x00, /*"V",86*/
    0x00, 0x00, 0x00, 0x6B, 0x49, 0x49, 0x49, 0x49, 0x55, 0x55, 0x36, 0x22,
    0x22, 0x22, 0x00, 0x00, /*"W",87*/
    0x00, 0x00, 0x00, 0xE7, 0x42, 0x24, 0x24, 0x18, 0x18, 0x18, 0x24, 0x24,
    0x42, 0xE7, 0x00, 0x00, /*"X",88*/
    0x00, 0x00, 0x00, 0x77, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x1C, 0x00, 0x00, /*"Y",89*/
    0x00, 0x00, 0x00, 0x7E, 0x21, 0x20, 0x10, 0x10, 0x08, 0x04, 0x04, 0x42,
    0x42, 0x3F, 0x00, 0x00, /*"Z",90*/
    0x00, 0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x78, 0x00, /*"[",91*/
    0x00, 0x00, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x10, 0x10, 0x20,
    0x20, 0x20, 0x40, 0x40, /*"\",92*/
    0x00, 0x1E, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x1E, 0x00, /*"]",93*/
    0x00, 0x38, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"^",94*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFF, /*"_",95*/
    0x00, 0x06, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /*"`",96*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x42, 0x78, 0x44, 0x42,
    0x42, 0xFC, 0x00, 0x00, /*"a",97*/
    0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x1A, 0x26, 0x42, 0x42, 0x42,
    0x26, 0x1A, 0x00, 0x00, /*"b",98*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x02, 0x02, 0x02,
    0x44, 0x38, 0x00, 0x00, /*"c",99*/
    0x00, 0x00, 0x00, 0x60, 0x40, 0x40, 0x40, 0x78, 0x44, 0x42, 0x42, 0x42,
    0x64, 0xD8, 0x00, 0x00, /*"d",100*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x42, 0x7E, 0x02, 0x02,
    0x42, 0x3C, 0x00, 0x00, /*"e",101*/
    0x00, 0x00, 0x00, 0xF0, 0x88, 0x08, 0x08, 0x7E, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x3E, 0x00, 0x00, /*"f",102*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x22, 0x22, 0x1C, 0x02,
    0x3C, 0x42, 0x42, 0x3C, /*"g",103*/
    0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x3A, 0x46, 0x42, 0x42, 0x42,
    0x42, 0xE7, 0x00, 0x00, /*"h",104*/
    0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0E, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x3E, 0x00, 0x00, /*"i",105*/
    0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x38, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x22, 0x1E, /*"j",106*/
    0x00, 0x00, 0x00, 0x03, 0x02, 0x02, 0x02, 0x72, 0x12, 0x0A, 0x16, 0x12,
    0x22, 0x77, 0x00, 0x00, /*"k",107*/
    0x00, 0x00, 0x00, 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x3E, 0x00, 0x00, /*"l",108*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x92, 0x92, 0x92, 0x92,
    0x92, 0xB7, 0x00, 0x00, /*"m",109*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x46, 0x42, 0x42, 0x42,
    0x42, 0xE7, 0x00, 0x00, /*"n",110*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x42,
    0x42, 0x3C, 0x00, 0x00, /*"o",111*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x26, 0x42, 0x42, 0x42,
    0x22, 0x1E, 0x02, 0x07, /*"p",112*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x42, 0x42,
    0x44, 0x78, 0x40, 0xE0, /*"q",113*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x4C, 0x04, 0x04, 0x04,
    0x04, 0x1F, 0x00, 0x00, /*"r",114*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x42, 0x02, 0x3C, 0x40,
    0x42, 0x3E, 0x00, 0x00, /*"s",115*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x30, 0x00, 0x00, /*"t",116*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x42, 0x42, 0x42, 0x42,
    0x62, 0xDC, 0x00, 0x00, /*"u",117*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE7, 0x42, 0x24, 0x24, 0x14,
    0x08, 0x08, 0x00, 0x00, /*"v",118*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x49, 0x49, 0x55, 0x55,
    0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76,
    0x24, 0x18, 0x18, 0x18, 0x24, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xE7, 0x42, 0x24, 0x24, 0x14, 0x18, 0x08, 0x08, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x22, 0x10, 0x08, 0x08,
    0x44, 0x7E, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x10,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xC0, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x06, 0x08, 0x08, 0x08, 0x08, 0x08, 0x10, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x06, 0x00, 0x0C, 0x32, 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static inline uint32_t rgb2y(uint8_t *rgb) {
  // return(((257 * rgb[0])/1000) + ((504 * rgb[1])/1000) + ((98 * rgb[2])/1000)
  // + 16);
  register uint32_t b = rgb[0];
  register uint32_t g = rgb[1];
  register uint32_t r = rgb[2];
  return ((306 * r + 601 * g + 117 * b) >> 10);
}

static inline void rgb2uv(uint8_t *rgb, uint8_t *uv) {
  register uint32_t b = rgb[0];
  register uint32_t g = rgb[1];
  register uint32_t r = rgb[2];
  uv[0] = ((446 * b - 150 * r - 296 * g) / 1024) + 0x7F;
  uv[1] = ((630 * r - 527 * g - 102 * b) / 1024) + 0x7F;
}

// int rgb2nv12(uint8_t *out, uint32_t *in, int w, int h) {
//   uint8_t *y = out;
//   uint8_t *uv = y + w * h;
//   uint32_t *rgb = (in + w * h);

//   for (int i = 0; i < w; i++) {
//     for (int j = 0; j < h; j++) {
//       // 内嵌 rgb2uv 函数
//       uint32_t pixel = *rgb;
//       uint32_t b = pixel & 0xFF;
//       uint32_t g = (pixel >> 8) & 0xFF;
//       uint32_t r = (pixel >> 16) & 0xFF;
//       *y = (306 * r + 601 * g + 117 * b) >> 10;
//       y++;
//       rgb--;
//     }
//   }

//   return 0;
// }

int rgb2nv12(uint8_t *out, uint32_t *in, int w, int h) {
  uint8_t *y = out;
  uint8_t *uv = y + w * h;
  uint32_t *rgb = (in + w * h);

  for (int i = 0; i < h; i += 2) {
    for (int j = 0; j < w; j += 2) {
      rgb--;
      *y = rgb2y((uint8_t *)rgb);
      y++;

      rgb2uv(rgb, uv);
      uv += 2;

      rgb--;
      *y = rgb2y((uint8_t *)rgb);
      y++;
    }
    for (int j = 0; j < w; j += 2) {
      rgb--;
      *y = rgb2y((uint8_t *)rgb);
      y++;

      rgb--;
      *y = rgb2y((uint8_t *)rgb);
      y++;
    }
  }

  return 0;
}

inline void screen_put_pixel(u32 x, u32 y, u32 c) {
  i32 x_max = gscreen.width - 1;   // 每行像素数
  i32 y_max = gscreen.height - 1;  // 每列像素数

#ifdef ROTATE_180
  x = gscreen.width - 1 - x;
  y = gscreen.height - 1 - y;
#endif

  // 防止越界
  if (x >= x_max) {
    return;
  }
  if (y >= y_max) {
    return;
  }

  gscreen.buffer[gscreen.width * y + x] = c | 0xFF000000u;
}

// 画点函数
void screen_draw_poi32(i32 x, i32 y, i32 color) {
  i32 x_max = gscreen.width - 1;   // 每行像素数
  i32 y_max = gscreen.height - 1;  // 每列像素数

#ifdef ROTATE_180
  x = gscreen.width - 1 - x;
  y = gscreen.height - 1 - y;
#endif
  // 防止越界
  if (x >= x_max) {
    return;
  }
  if (y >= y_max) {
    return;
  }

  // 取得显存线性地址
  u32 *video = (u32 *)(*((u32 *)gscreen.buffer));

  // 计算点的偏移量
  i32 offset = y * (x_max + 1) + x;
  *(video + offset) = color;
}

// 取得指定点色彩
i32 screen_get_poi32_color(i32 x, i32 y) {
  i32 x_max = gscreen.width - 1;   // 每行像素数
  i32 y_max = gscreen.height - 1;  // 每列像素数
#ifdef ROTATE_180
  x = gscreen.width - 1 - x;
  y = gscreen.height - 1 - y;
#endif
  // 防止越界
  if (x > x_max) {
    x = x_max;
  }
  if (y > y_max) {
    y = y_max;
  }

  // 取得显存线性地址
  u32 *video = (u32 *)(*((u32 *)gscreen.buffer));

  // 计算点的偏移量
  i32 offset = y * (x_max + 1) + x;
  return *(video + offset);
}

void screen_draw_vline(i32 x1, i32 x2, i32 y, i32 color) {
  i32 x_max = gscreen.width;
  i32 y_max = gscreen.height;
  if (gscreen.buffer == NULL || y < 0 || y >= y_max) return;
  if (x1 < 0) x1 = 0;
  if (x2 > x_max) x2 = x_max;
  if (x1 >= x2) return;

  u32 c = (u32)color | 0xFF000000u;
  u32* row = gscreen.buffer + (u32)y * (u32)gscreen.width + (u32)x1;
  for (i32 i = 0; i < x2 - x1; i++) row[i] = c;
}

void screen_draw_hline(i32 y1, i32 y2, i32 x, i32 color) {
  register i32 i;
  i32 x_max = gscreen.width - 1;   // 每行像素数
  i32 y_max = gscreen.height - 1;  // 每列像素数
  // 防止越界
  if (y1 >= y_max) {
    y1 = y_max;
  }
  if (y2 >= y_max) {
    y2 = y_max;
  }
  if (x >= x_max) {
    x = x_max;
  }
  for (i = y1; i < y2; i++) screen_put_pixel(x, i, color);
}
void screen_draw_line(u32 x1, u32 y1, u32 x2, u32 y2, u32 color) {
  int t;
  int xerr = 0, yerr = 0, delta_x, delta_y, distance;
  int incx, incy, row, col;
  delta_x = x2 - x1;  // 计算坐标增量
  delta_y = y2 - y1;
  row = x1;
  col = y1;
  if (delta_x > 0) {
    incx = 1;  // 设置单步方向
  } else if (delta_x == 0) {
    incx = 0;  // 垂直线
  } else {
    incx = -1;
    delta_x = -delta_x;
  }
  if (delta_y > 0) {
    incy = 1;
  } else if (delta_y == 0) {
    incy = 0;  // 水平线
  } else {
    incy = -1;
    delta_y = -delta_y;
  }
  if (delta_x > delta_y) {
    distance = delta_x;  // 选取基本增量坐标轴
  } else {
    distance = delta_y;
  }
  for (t = 0; t <= distance + 1; t++) {  // 画线输出
    screen_put_pixel(row, col, color);   // 画点
    xerr += delta_x;
    yerr += delta_y;
    if (xerr > distance) {
      xerr -= distance;
      row += incx;
    }
    if (yerr > distance) {
      yerr -= distance;
      col += incy;
    }
  }
}

void screen_clear_screen(void) {
  i32 screen_size, i;
  screen_size = gscreen.width * gscreen.height;
  for (i = 0; i < screen_size; i++) gscreen.buffer[i] = 0;
}

void screen_fill_rect(i32 x, i32 y, i32 w, i32 h, u32 color) {
  if (gscreen.buffer == NULL || w <= 0 || h <= 0) return;
  i32 x0 = x < 0 ? 0 : x;
  i32 y0 = y < 0 ? 0 : y;
  i32 x1 = x + w;
  i32 y1 = y + h;
  if (x1 > (i32)gscreen.width) x1 = (i32)gscreen.width;
  if (y1 > (i32)gscreen.height) y1 = (i32)gscreen.height;
  if (x0 >= x1 || y0 >= y1) return;

  u32 t0 = screen_now_ms();
  u32 c = color | 0xFF000000u;
  u32 row_w = (u32)(x1 - x0);
  for (i32 row = y0; row < y1; row++) {
    u32* dst = gscreen.buffer + (u32)row * gscreen.width + (u32)x0;
    for (u32 i = 0; i < row_w; i++) dst[i] = c;
  }
  g_acc_fill_ms += screen_now_ms() - t0;
}

void screen_put_ascii(i32 x, i32 y, u8 ch, i32 color) {
  register i32 i, j;
#ifdef ROTATE_180
  x = gscreen.width - 1 - x;
  y = gscreen.height - 1 - y;
#endif
  for (i = 0; i < CHAR_HEIGHT; i++)
    for (j = 0; j < ASC_WIDTH; j++)
      if ((*(gscreen.ASC + ch * ASC_WIDTH + i) >> (ASC_WIDTH - j - 1)) & 1)
        screen_put_pixel(x + j, y + i, color);
}

// 画矩形函数
void screen_draw_rect(i32 x1, i32 y1, i32 x2, i32 y2, i32 color,
                      i32 dose_fill_it) {
  screen_draw_vline(y1, x1, x2, color);
  screen_draw_vline(y2, x1, x2, color);
  screen_draw_hline(x1, y1, y2, color);
  screen_draw_hline(x2, y1, y2, color);

  if (dose_fill_it) {
    screen_fill_rect(x1, y1, x2, y2, color);
  }
}

void screen_printf(i32 x, i32 y, char *fmt, ...) {
  int i;
  char buf[1024];
  memset(buf, 0, 1024);
  va_list args;
  va_start(args, fmt);
  i = vsprintf(buf, fmt, args);
  va_end(args);
  screen_draw_string(x, y, buf);
}

void screen_draw_string(i32 x, i32 y, i8 *str) {
  screen_draw_string_with_color(x, y, str, 0xffffffff, 0);
}
void screen_draw_char(i32 x, i32 y, u16 ch) {
  screen_draw_char_witdh_color(x, y, ch, 0xffffffff, 0);
}

void screen_draw_char_witdh_color(i32 x, i32 y, u16 ch, u32 frcolor,
                                  u32 bgcolor) {
  if (y < 0 || y >= (gscreen.height - 16)) {
    return;
  }
  u32 i, j;
  u16 code, z;
  u8 *lp;
  code = ch;
  if (code < 0x20) return;
  if (code < 0x7f) {
    if (x > (gscreen.width - 1)) return;
    code = code - 0x20;
    lp = SCREEN_ASCII + code * 16;
    for (i = 0; i < 16; i++) {
      z = *lp++;
      for (j = 0; j < 8; j++) {
        if (z & 0x01) {
          screen_put_pixel(x + j, y + i, frcolor);
        } else if (bgcolor != 0) {
          screen_put_pixel(x + j, y + i, bgcolor);
        }
        z = z >> 1;
      }
    }
  } else if (code > 0xa1a1) {
    if (x > (gscreen.width - 1)) return;
    code = code - 0xa1a1;
    if (code > 0x0f00) code = code - 0x600;
    lp = ((u8 *)0x40000 + ((code >> 8) * 94 + (code & 0xff)) * 32);
    for (i = 0; i < 16; i++) {
      z = *lp++;
      z = (z << 8) + *lp++;
      for (j = 0; j < 16; j++) {
        if (z & 0x8000) {
          screen_put_pixel(x + j, y + i, frcolor);
        } else if (bgcolor != 0) {
          screen_put_pixel(x + j, y + i, bgcolor);
        }
        z = z << 1;
      }
    }
  }
}
void screen_draw_string_with_color(i32 x, i32 y, i8 *str, u32 frcolor,
                                   u32 bgcolor) {
  u16 code;

#ifdef ROTATE_180
  x = gscreen.width - 1 - x;
  y = gscreen.height - 1 - y;
#endif

  while ((*str != '\0')) {
    code = *str++;
    if (code > 0x80) code = (code << 8) + *str++;
    screen_draw_char_witdh_color(x, y, code, frcolor, bgcolor);
    x += 8;
  }
}

// 色调转换函数，把 5:5:5 格式转换成标准的 5:6:5 格式
i32 screen_change_color_form_555_to_565(i32 color_form_555) {
  // 把 555 中间的色彩 * 2，为了掩色处理方便，如果不是 0，则末位恒置 1
  union {
    i32 color;
    struct {
      u8 b : 5;
      u8 g : 6;
      u8 r : 5;
    };
  } u1;

  union {
    i32 color;
    struct {
      u8 b : 5;
      u8 g : 5;
      u8 r : 5;
      u8 alpha : 1;  // alpha 通道，但这里不用
    };
  } u2;

  u2.color = color_form_555;
  u1.r = u2.r;
  if (u2.g) {
    u1.g = (i32)u2.g * 2 + 1;
  } else {
    u1.g = 0;
  }
  u1.b = u2.b;

  return u1.color;
}

void screen_show_bitmap(i32 x, i32 y, i32 width, i32 height, bitmap_t *bitmap) {
  int i, j;
  int hsrc = bitmap->h;
  int wsrc = bitmap->w;
  int hdes = height;
  int wdes = width;
  int ysrc = 0;
  int xsrc = 0;
  int xdes = x;
  int ydes = y;
  if (bitmap == NULL) {
    return;
  }
  if (bitmap->bitperpixel == 16) {
    u16 *pdes = NULL;
    u16 *psrc = NULL;
    for (j = 0; j < hsrc && j < hdes; j++) {
      psrc = (u16 *)(bitmap->bits) + (ysrc + j) * bitmap->w + xsrc;
      pdes = (u16 *)(gscreen.buffer) + (ydes + j) * gscreen.width + xdes;
      for (i = 0; i < wsrc && i < wdes; i++) {
        pdes[i] = psrc[i];
      }
    }
  } else if (bitmap->bitperpixel == 32) {
    u32 *p32des = NULL;
    u32 *p32src = NULL;
    for (j = 0; j < hsrc && j < hdes; j++) {
      p32src = (u32 *)(bitmap->bits) + (ysrc + j) * bitmap->w + xsrc;
      p32des = (u32 *)(gscreen.buffer) + (ydes + j) * gscreen.width + xdes;
      for (i = 0; i < wsrc && i < wdes; i++) {
        p32des[i] = p32src[i];
      }
    }
  }
}

// 显示 pbmp 格式的图片
void screen_show_bmp_picture(i32 x, i32 y, void *bmp_addr, i32 mask_color,
                             i32 dose_use_mask_color) {
  // 这里只支持 windows 标准 16 位 bmp 格式图片，（1:5:5:5）
  struct bmp_bmp_head_struct *bmp_head = (struct bmp_bmp_head_struct *)bmp_addr;
  i32 width = bmp_head->info_head.width;
  i32 height = bmp_head->info_head.height;
  if (width < 0 || height < 0) return;
  // 下面记算存储每个点的色彩的信息所在的位置
  i32 *color = (i32 *)((i32)bmp_addr + bmp_head->offset);

  // 由于一行的字节数比须是 4 的倍数，因此，这里先计算每行需要的填充数，除 2
  // 是因为每个像素两个字节
  i32 fill_length = width * 2 % 4 / 2;

  // bmp 的存放顺序是从下到上，从左到右
  for (i32 i = height - 1; i >= 0; --i) {
    for (i32 j = 0; j < width; ++j) {
      // 取得每个点的色彩信息
      // 由于 windows 默认的是 555 格式，而 pyos 用的是 565
      // 格式，因此先进行一下转换
      i32 temp_color = screen_change_color_form_555_to_565(*color);
      if (!dose_use_mask_color || temp_color != mask_color) {
        // 画出每个点
        screen_draw_poi32(x + j, y + i, temp_color);
      }
      ++color;
    }
    // 填充
    color += fill_length;
  }
}

void screen_init() {
  screen_init_with_mode(SCREEN_MODE_XWIN);
}

/* buffer 模式：buffer 保留 cached malloc，screen_flush 由 xwin_blit 原子拷贝。
 * 适用于增量渲染的 app（SDL/ymain），避免 DE 扫描到半帧造成闪烁。 */
void screen_init_buffer() {
  screen_init_with_mode(SCREEN_MODE_XWIN_BUFFER);
}

/* 读 /dev/fb 信息到 gscreen.fb；成功返回 fd，失败返回 -1。
 * 宽高/地址用 ioctl 返回值，不依赖 INFO 写用户缓冲。 */
static int screen_read_fb_info(void) {
  int fd = open("/dev/fb", O_RDWR);
  if (fd < 0) {
    return -1;
  }
  u32 w = (u32)ioctl(fd, IOC_READ_FRAMBUFFER_WIDTH, 0);
  u32 h = (u32)ioctl(fd, IOC_READ_FRAMBUFFER_HEIGHT, 0);
  u32 bpp = (u32)ioctl(fd, IOC_READ_FRAMBUFFER_BPP, 0);
  u32 fb = (u32)ioctl(fd, IOC_READ_FRAMBUFFER, 0);
  /* 勿用 IOC_READ_FRAMBUFFER_INFO 整结构拷贝：内核 vga_device_t 更长，
   * 会写穿 gscreen.fb 后面的 screen_mode / xwin_handle。 */
  if (w > 0) gscreen.fb.width = w;
  if (h > 0) gscreen.fb.height = h;
  if (bpp > 0) gscreen.fb.bpp = bpp;
  if (fb != 0) gscreen.fb.frambuffer = (u32*)(uintptr_t)fb;
  if (gscreen.fb.framebuffer_count == 0) {
    gscreen.fb.framebuffer_count = 1;
  }
  if (gscreen.fb.framebuffer_length == 0 && gscreen.fb.width > 0 &&
      gscreen.fb.height > 0) {
    u32 bytes = gscreen.fb.bpp >= 8 ? (gscreen.fb.bpp / 8) : 4;
    gscreen.fb.framebuffer_length =
        gscreen.fb.width * gscreen.fb.height * bytes;
  }
  return fd;
}

void screen_init_with_mode(screen_mode_t mode) {
  gscreen.screen_mode = mode;

  if (mode == SCREEN_MODE_XWIN || mode == SCREEN_MODE_XWIN_BUFFER) {
    /* 始终读 fb：要分辨率，更要 VA（DIRECT 直绘 LCD） */
    int fbfd = screen_read_fb_info();
    if (fbfd >= 0) {
      close(fbfd);
    }
    if (gscreen.fb.width > 0 && gscreen.fb.height > 0) {
      gscreen.width = (int)gscreen.fb.width;
      gscreen.height = (int)gscreen.fb.height;
      if (gscreen.fb.bpp > 0) {
        gscreen.bpp = (int)gscreen.fb.bpp;
      }
    }
    if (gscreen.width == 0) gscreen.width = 480;
    if (gscreen.height == 0) gscreen.height = 320;
    if (gscreen.bpp == 0) gscreen.bpp = 32;
    gscreen.buffer_length = gscreen.width * gscreen.height * 4;
    gscreen.buffer = malloc(gscreen.buffer_length);
    gscreen.pbuffer = gscreen.buffer;
    gscreen.cur.x = 0;
    gscreen.cur.y = 0;
    gscreen.ASC = SCREEN_ASCII;
    gscreen.fd = -1;
    /* XWIN 不用 /dev/fb flush，避免 fd=-1 时误走 FB ioctl */
    gscreen.fb.framebuffer_count = 0;

    /* 全屏 DIRECT：内核绑 LCD；经 xwin_get_fb 取 VA（ioctl 返回 0xfb****** 会被当成负 errno） */
    gscreen.xwin_handle = xwin_create_flags(0, 0, gscreen.width, gscreen.height,
                                            "", XWIN_FLAG_DIRECT);
    /* 不在这里 set_bg/clear：等 get_fb 绑好 VA 后再由 fill_rect 上色 */

    {
      u32* fb = xwin_get_fb(gscreen.xwin_handle);
      u32 fb_addr = fb != NULL ? (u32)(uintptr_t)fb : 0;
      if (fb_addr >= 0xfb000000u && fb_addr < 0xfe000000u) {
        if (mode == SCREEN_MODE_XWIN_BUFFER) {
          /* buffer 模式：窗口 DIRECT（内核快速路径），但 buffer 保留 cached
           * malloc。screen_flush 时 xwin_blit 一次性原子拷贝到 LCD FB。 */
          gscreen.fb.frambuffer = fb;
          printf("screen init xwin %dx%d fb=%p (buffer, cached render)\n",
                 gscreen.width, gscreen.height, (void*)fb);
        } else {
          /* DIRECT 模式：buffer 直接指向 LCD FB（零拷贝）。
           * 适用于原子渲染的 app（infones, gnuboy 等）。 */
          if (gscreen.buffer != NULL) {
            free(gscreen.buffer);
          }
          gscreen.buffer = fb;
          gscreen.pbuffer = fb;
          gscreen.fb.frambuffer = fb;
          printf("screen init xwin DIRECT %dx%d fb=%p\n",
                 gscreen.width, gscreen.height, (void*)gscreen.buffer);
        }
      } else {
        printf("screen init xwin mode %dx%d (fb=%p addr=%x, blit path)\n",
               gscreen.width, gscreen.height, (void*)fb, fb_addr);
      }
    }
    event_init();
    printf("event init end\n");
    return;
  }

  // FB 模式初始化
  int fd = screen_read_fb_info();
  printf("screen init fd:%d\n", fd);

  gscreen.buffer = gscreen.fb.frambuffer;
  gscreen.pbuffer = gscreen.fb.frambuffer;
  gscreen.width = gscreen.fb.width;
  gscreen.height = gscreen.fb.height;
  gscreen.bpp = gscreen.fb.bpp > 0 ? gscreen.fb.bpp : 32;
  gscreen.fd = fd;
  gscreen.xwin_handle = 0;
  gscreen.cur.x = 0;
  gscreen.cur.y = 0;
  gscreen.ASC = SCREEN_ASCII;
  if (gscreen.fb.framebuffer_count == 0) {
    gscreen.fb.framebuffer_count = 1;
  }
  if (gscreen.fb.framebuffer_length == 0 && gscreen.width > 0 &&
      gscreen.height > 0) {
    gscreen.fb.framebuffer_length =
        gscreen.width * gscreen.height * (gscreen.bpp / 8);
  }
  if (gscreen.buffer_length == 0) {
    gscreen.buffer_length = gscreen.fb.framebuffer_length;
  }

#ifdef DB_BUFFER
  gscreen.buffer_length = gscreen.width * gscreen.height * 8;
  gscreen.buffer = malloc(gscreen.buffer_length);

#endif

  printf("screen init fb mode %dx%d bpp:%d fb count:%d len:%d buffer len:%d\n",
         gscreen.width, gscreen.height, gscreen.fb.bpp,
         gscreen.fb.framebuffer_count, gscreen.fb.framebuffer_length,
         gscreen.buffer_length);
  event_init();
  printf("event init end\n");
}

void screen_set_mode(screen_mode_t mode) {
  if (gscreen.screen_mode == mode) return;

  // 销毁旧模式的资源
  if ((gscreen.screen_mode == SCREEN_MODE_XWIN ||
       gscreen.screen_mode == SCREEN_MODE_XWIN_BUFFER) &&
      gscreen.xwin_handle) {
    xwin_destroy(gscreen.xwin_handle);
    gscreen.xwin_handle = 0;
  }
  if (gscreen.buffer && (gscreen.screen_mode == SCREEN_MODE_XWIN ||
       gscreen.screen_mode == SCREEN_MODE_XWIN_BUFFER) &&
      gscreen.buffer != (u32*)gscreen.fb.frambuffer) {
    free(gscreen.buffer);
    gscreen.buffer = NULL;
  }

  // 重新初始化新模式
  screen_init_with_mode(mode);
}

screen_mode_t screen_get_mode(void) {
  return gscreen.screen_mode;
}

void screen_set_title(const char* title) {
  if (gscreen.screen_mode == SCREEN_MODE_XWIN && gscreen.xwin_handle) {
    xwin_set_title(gscreen.xwin_handle, title);
  }
}

void screen_set_size(u32 width, u32 height) {
  if (gscreen.screen_mode == SCREEN_MODE_XWIN && gscreen.xwin_handle) {
    // 已经初始化，重新创建窗口
    xwin_destroy(gscreen.xwin_handle);
    gscreen.xwin_handle = xwin_create_flags(0, 0, width, height, "",
                                            XWIN_FLAG_DIRECT);
    xwin_set_bg_color(gscreen.xwin_handle, 0xFF1E1E1E);
  }
  // 更新尺寸
  gscreen.width = width;
  gscreen.height = height;
  gscreen.buffer_length = width * height * 4;
  // 重新分配 buffer
  if (gscreen.buffer) {
    free(gscreen.buffer);
  }
  gscreen.buffer = malloc(gscreen.buffer_length);
  gscreen.pbuffer = gscreen.buffer;
}

screen_info_t *screen_info() { return &gscreen; }

void screen_flush() {
  u32 t0, t1;

  /* XWIN 模式（DIRECT / BUFFER） */
  if (gscreen.screen_mode == SCREEN_MODE_XWIN ||
      gscreen.screen_mode == SCREEN_MODE_XWIN_BUFFER) {
    if (gscreen.xwin_handle == 0) {
      printf("xwin handle is null\n");
      return;
    }
    if (!g_stats_inited) {
      g_fps_t0 = screen_now_ms();
      g_stats_inited = 1;
      printf("gui: stats fill/blit/update/render (ms sum per 60 frames)\n");
    }
    /* DIRECT: buffer=LCD FB, 跳过拷贝; buffer: cached buffer 一次性原子 blit */
    t0 = screen_now_ms();
    if (gscreen.buffer != NULL &&
        gscreen.buffer != (u32*)gscreen.fb.frambuffer) {
      xwin_blit(gscreen.xwin_handle, 0, 0, (const u32*)gscreen.buffer,
                gscreen.width, gscreen.height);
    }
    t1 = screen_now_ms();
    g_acc_blit_ms += t1 - t0;

    t0 = t1;
    xwin_update(gscreen.xwin_handle);
    t1 = screen_now_ms();
    g_acc_update_ms += t1 - t0;

    t0 = t1;
    xwin_render();
    t1 = screen_now_ms();
    g_acc_render_ms += t1 - t0;

    screen_stats_frame_done();
    return;
  }

  /* FB 模式 */
  if (gscreen.fb.framebuffer_count <= 0) {
    if (gscreen.width > 0 && gscreen.height > 0 && gscreen.fd >= 0) {
      gscreen.fb.framebuffer_count = 1;
      if (gscreen.fb.framebuffer_length == 0) {
        gscreen.fb.framebuffer_length =
            gscreen.width * gscreen.height * (gscreen.bpp / 8);
      }
    } else {
      printf("init screen has some error,maybe no init first\n");
      return;
    }
  }
  u32 current_index = gscreen.fb.framebuffer_index;
  gscreen.fb.framebuffer_index =
      (++gscreen.fb.framebuffer_index) % gscreen.fb.framebuffer_count;

#ifdef DB_BUFFER
  if (gscreen.fb.format == 1) {
    rgb2nv12(gscreen.pbuffer, gscreen.buffer, gscreen.width, gscreen.height);
  } else {
    memcpy(gscreen.pbuffer, gscreen.buffer, gscreen.buffer_length);
  }
#else
  gscreen.buffer = gscreen.fb.frambuffer +
      gscreen.width * gscreen.height * gscreen.fb.framebuffer_index;
#endif

  if (gscreen.fd < 0) {
    return;
  }
  ioctl(gscreen.fd, IOC_FLUSH_FRAMBUFFER, current_index);
}

void do_screen_thread(void) {
  u32 i = 0;
  u32 count = 0;
  char buf[2] = {0};
  char wheel[] = {'\\', '|', '/', '-'};

  mouse_data_t mouse_data;
  mouse_data.x = 0;
  mouse_data.y = 0;
  int fd = open("/dev/mouse", 0);
  for (;;) {
    buf[0] = wheel[i++];
    // continue;
    // syscall3(SYS_PRINT_AT, buf, 101, 0);
    count++;
    if (i % 4 == 0) i = 0;
    for (u32 y = 0; y < gscreen.height; y++) {
      screen_put_pixel((gscreen.width - gscreen.height) / 2 + y, y, 0x0000ff);
      screen_put_pixel((gscreen.height + gscreen.width) / 2 - y, y, 0xff0000);
    }
    screen_draw_line(0, 0, 140, 140, 0xff0000);
    screen_fill_rect(10, 20, 30, 30, 0xff0000);
    screen_printf(200, 10, "hello,YiYiYa");

    read(fd, &mouse_data, sizeof(mouse_data_t));
    // syscall3(SYS_READ, fd, &mouse_data, sizeof(mouse_data_t));
    screen_printf(10, 100, "%d %d", mouse_data.x, mouse_data.y);
    screen_fill_rect(mouse_data.x, gscreen.height - mouse_data.y, 4, 4,
                     0x00ff00);
  }
}
