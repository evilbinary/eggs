/* shared/frame.c（窗口边框/标题栏装饰，基于 cairo）的空实现。
 *
 * backend-headless 的 decorated 模式会用到 frame_ 与 theme_ 系列函数；
 * 我们始终以 decorate = false 运行（没有真实输出设备，也不需要装饰），
 * 所以这些函数不会被真正调用，只需要提供符合同一签名的定义让链接通过。
 *
 * 一旦将来接上真实输出并需要装饰，把 shared/frame.c + cairo 一并移植即可，
 * 届时删掉本文件。
 */
#include <stdint.h>

#include "shared/cairo-util.h"

struct theme *
theme_create(void)
{
	return NULL;
}

void
theme_destroy(struct theme *t)
{
	(void)t;
}

void
theme_set_background_source(struct theme *t, cairo_t *cr, uint32_t flags)
{
	(void)t;
	(void)cr;
	(void)flags;
}

struct frame *
frame_create(struct theme *t, int32_t width, int32_t height, uint32_t buttons,
	     const char *title, cairo_surface_t *icon)
{
	(void)t;
	(void)width;
	(void)height;
	(void)buttons;
	(void)title;
	(void)icon;
	return NULL;
}

void
frame_destroy(struct frame *frame)
{
	(void)frame;
}

int
frame_set_title(struct frame *frame, const char *title)
{
	(void)frame;
	(void)title;
	return 0;
}

void
frame_set_icon(struct frame *frame, cairo_surface_t *icon)
{
	(void)frame;
	(void)icon;
}

void
frame_set_flag(struct frame *frame, enum frame_flag flag)
{
	(void)frame;
	(void)flag;
}

void
frame_unset_flag(struct frame *frame, enum frame_flag flag)
{
	(void)frame;
	(void)flag;
}

void
frame_resize(struct frame *frame, int32_t width, int32_t height)
{
	(void)frame;
	(void)width;
	(void)height;
}

void
frame_resize_inside(struct frame *frame, int32_t width, int32_t height)
{
	(void)frame;
	(void)width;
	(void)height;
}

int32_t
frame_width(struct frame *frame)
{
	(void)frame;
	return 0;
}

int32_t
frame_height(struct frame *frame)
{
	(void)frame;
	return 0;
}

void
frame_border_sizes(struct frame *frame, int32_t *top, int32_t *bottom,
		   int32_t *left, int32_t *right)
{
	(void)frame;
	if (top)
		*top = 0;
	if (bottom)
		*bottom = 0;
	if (left)
		*left = 0;
	if (right)
		*right = 0;
}

void
frame_decoration_sizes(struct frame *frame, int32_t *top, int32_t *bottom,
		       int32_t *left, int32_t *right)
{
	frame_border_sizes(frame, top, bottom, left, right);
}

void
frame_interior(struct frame *frame, int32_t *x, int32_t *y,
	       int32_t *width, int32_t *height)
{
	(void)frame;
	if (x)
		*x = 0;
	if (y)
		*y = 0;
	if (width)
		*width = 0;
	if (height)
		*height = 0;
}

void
frame_input_rect(struct frame *frame, int32_t *x, int32_t *y,
		 int32_t *width, int32_t *height)
{
	frame_interior(frame, x, y, width, height);
}

void
frame_opaque_rect(struct frame *frame, int32_t *x, int32_t *y,
		  int32_t *width, int32_t *height)
{
	frame_interior(frame, x, y, width, height);
}

int
frame_get_shadow_margin(struct frame *frame)
{
	(void)frame;
	return 0;
}

uint32_t
frame_status(struct frame *frame)
{
	(void)frame;
	return FRAME_STATUS_NONE;
}

void
frame_status_clear(struct frame *frame, enum frame_status status)
{
	(void)frame;
	(void)status;
}
