/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#ifndef SCREEN_WAYLAND_H
#define SCREEN_WAYLAND_H

#include <stdint.h>


/* Wayland 上屏后端（screen_wayland.c）。非 Wayland 构建时是空实现。 */
int screen_wayland_init(int w, int h);
void screen_wayland_present(uint32_t *canvas, int w, int h);
void screen_wayland_set_title(const char *title);
void screen_wayland_shutdown(void);


#endif
