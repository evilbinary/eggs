/* cairo 的极小垫片：只为让 shared/cairo-util.h 能被解析。
 *
 * libweston 里唯一牵到 cairo 的地方是 window decoration 的
 * renderer-borders（libweston/renderer-borders.h 引 shared/cairo-util.h）。
 * 本系统没有 cairo（也不需要窗口边框阴影），renderer-borders 用
 * compat/borders-stub.c 的空实现顶掉；这个头只提供声明里出现的类型，
 * 不实现任何 cairo 函数。
 */
#ifndef CAIRO_H_SHIM
#define CAIRO_H_SHIM

#include <stdint.h>

typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;
typedef struct _cairo_font_face cairo_font_face_t;
typedef struct _cairo_scaled_font cairo_scaled_font_t;

typedef struct cairo_rectangle_int {
	int32_t x, y;
	int32_t width, height;
} cairo_rectangle_int_t;

typedef enum cairo_status {
	CAIRO_STATUS_SUCCESS = 0,
	CAIRO_STATUS_NO_MEMORY,
	CAIRO_STATUS_INVALID_RESTORE,
} cairo_status_t;

cairo_status_t cairo_surface_status(cairo_surface_t *surface);
void *cairo_surface_get_user_data(cairo_surface_t *surface, const void *key);
void cairo_surface_destroy(cairo_surface_t *surface);

#endif /* CAIRO_H_SHIM */
