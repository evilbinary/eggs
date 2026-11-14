/*
 * YiYiYa 本地补丁：fbdev 后端（直接渲染到内核 /dev/fb0）。
 *
 * 和 backend-headless 的区别只有一处：headless 把画面渲染到 renderer 内部
 * 的缓冲里就结束了（没人用），这里把 /dev/fb0 的 mmap 内存直接交给 pixman
 * 当 renderbuffer，于是每帧 repaint 就是往显存里画，零拷贝、也不需要影子
 * 缓冲。
 *
 * 用法（前端已接线）：
 *   weston --backend=fbdev-backend.so --shell=kiosk-shell.so ...
 * 尺寸默认取 framebuffer 的实际几何，和内核 sysconf 的 [vga] mode 一致。
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <dlfcn.h>

/* 全屏 RGB565 转换的 NEON 版本先关掉：先用标量路径确认稳定性，
 * 避免把渲染器崩溃和向量化改动混在一起排查。 */
#ifndef FBDEV_CONVERT_NEON
#define FBDEV_CONVERT_NEON 0
#endif

#if FBDEV_CONVERT_NEON && defined(__aarch64__)
#include <arm_neon.h>
#endif

#include <pixman.h>
#include <wayland-server-core.h>

#include <libweston/libweston.h>
#include <libweston/config-parser.h>
#include <libweston/windowed-output-api.h>
#include <libweston/zalloc.h>

#include "shared/helpers.h"
#include "backend.h"
#include "libweston-internal.h"
#include "pixel-formats.h"
#include "pixman-renderer.h"
#include "renderer-gl/gl-renderer.h"
#include "presentation-time-server-protocol.h"

#include "backend-fbdev.h"

#define DEFAULT_REFRESH 60000 /* mHz，即 60Hz */

/* linux/fb.h 的最小复刻（用户态没有内核 uapi 头） */
#define FBIOGET_VSCREENINFO 0x4600
#define FBIOGET_FSCREENINFO 0x4602
#define FBIOPAN_DISPLAY 0x4606

struct fb_bitfield {
	uint32_t offset;
	uint32_t length;
	uint32_t msb_right;
};

struct fb_var_screeninfo {
	uint32_t xres, yres, xres_virtual, yres_virtual;
	uint32_t xoffset, yoffset, bits_per_pixel, grayscale;
	struct fb_bitfield red, green, blue, transp;
	uint32_t nonstd, activate, height, width, accel_flags;
	uint32_t pixclock;
	uint32_t left_margin, right_margin, upper_margin, lower_margin;
	uint32_t hsync_len, vsync_len, sync, vmode, rotate, colorspace;
	uint32_t reserved[4];
};

struct fb_fix_screeninfo {
	char id[16];
	unsigned long smem_start;
	uint32_t smem_len, type, type_aux, visual;
	uint16_t xpanstep, ypanstep, ywrapstep;
	uint32_t line_length;
	unsigned long mmio_start;
	uint32_t mmio_len, accel;
	uint16_t capabilities;
	uint16_t reserved[2];
};

/* ---- 输入：/dev/input/eventN → weston pointer ----
 * 上游 fbdev 后端不存在，libinput 后端又依赖 udev/sysfs（本系统没有），
 * 所以这里直接读内核 evdev 的 input_event。格式与 Linux 一致
 * （aarch64 上 24 字节：i64 sec, i64 usec, u16 type, u16 code, i32 value），
 * 数据源是 duck/modules/input/evdev.c 建的 /dev/input/event0。
 * 只处理相对位移（EV_REL，鼠标）与按键（EV_KEY 的 BTN_*）；没有按键时
 * compositor 不画光标，所以这一段不做的话桌面就是"没有鼠标"。
 */
#define EV_SYN		0x00
#define EV_KEY		0x01
#define EV_REL		0x02
#define SYN_REPORT	0x00
#define REL_X		0x00
#define REL_Y		0x01

#define REL_WHEEL	0x08
#define BTN_LEFT	0x110
#define BTN_RIGHT	0x111
#define BTN_MIDDLE	0x112

struct fbdev_input_event {
	int64_t sec;
	int64_t usec;
	uint16_t type;
	uint16_t code;
	int32_t value;
};

struct fbdev_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	int fd;
	uint8_t *fb;
	size_t fb_len;
	int fb_width;
	int fb_height;
	int fb_stride;
	int fb_bpp;
	/* framebuffer 实际像素格式（16bpp=RGB565 / 32bpp=XRGB8888）。
	 * Pi5 固件即使请求 32bpp 也会给 16bpp（见 bcm2712.c 的 GET_PITCH），
	 * GL renderer 的 read_format 和 renderbuffer 都用它。 */
	const struct pixel_format_info *format;

	/* GL 读回用的 32bpp 影子缓冲（width*height*4）。
	 * V3D 的 glReadPixels 直接读成 RGB565 会得到全 0，先读 ARGB8888
	 * 再在 CPU 里转成 RGB565 写进 /dev/fb0。 */
	uint8_t *gl_shadow;
	size_t gl_shadow_len;

	int refresh;

	/* 打开时读到的模式，用于每帧 FBIOPAN_DISPLAY"踢"一下显示刷新 */
	struct fb_var_screeninfo var;

	/* 输入：seat 内嵌在 backend 里（与上游 test 后端同一套 API），
	 * 一个 evdev fd + wl_event_loop 的 fd source。 */
	struct weston_seat seat;
	bool input_ready;
	int input_fd;
	struct wl_event_source *input_source;
	int rel_x;
	int rel_y;
	/* 鼠标位移缩放，来自 weston.ini 的 [fbdev] pointer-scale（默认 1.0）。 */
	double pointer_scale;
	/* GL 是否按实际脏区重绘（来自 [fbdev] partial-damage）。
	 * 默认关：V3D 的部分重绘路径历史上有卡死记录；打开前先确认。 */
	bool partial_damage;

	/* 键盘 /dev/input/event1（内核 usb_keyboard 注册的 evdev 设备） */
	int kbd_fd;
	struct wl_event_source *kbd_source;
	bool kbd_ready;

	/* 软件光标上次画在 fb 上的位置，用于 Pixman 路径只重绘光标区域。 */
	int cursor_x;
	int cursor_y;
	int cursor_valid;

	/* 光标下面的背景像素（16x16，按 fb 实际 bpp 存）。
	 * 光标是直接写进 /dev/fb0 的：移动时先把这块背景恢复回去，
	 * 场景没变就完全不用跑 GL 整帧 + glReadPixels + 全屏转换。 */
	uint8_t cursor_bg[16 * 16 * 4];

};

struct fbdev_head {
	struct weston_head base;
};

struct fbdev_output {
	struct weston_output base;
	struct fbdev_backend *backend;

	struct wl_event_source *finish_frame_timer;
	weston_renderbuffer_t renderbuffer;
	struct weston_mode mode;
};

static inline struct fbdev_backend *
to_fbdev_backend(struct weston_backend *base)
{
	return container_of(base, struct fbdev_backend, base);
}

static inline struct fbdev_output *
to_fbdev_output(struct weston_output *base)
{
	return container_of(base, struct fbdev_output, base);
}

/* ---- 软件光标 ------------------------------------------------------
 * 本系统没有 xcursor 主题（/usr/share/icons 不存在），desktop-shell 也就
 * 不会通过 wl_pointer.set_cursor 给出光标图像，weston 的 cursor_layer 是空
 * 的。这里在每帧把合成结果写进 /dev/fb0 之后，用 CPU 画一个 16x16 箭头。
 *
 * 位置直接取 weston 自己的 pointer->pos，而不是后端另记一份：事件命中、
 * 抓取、焦点用的都是这个坐标，必须完全一致。pointer->sprite 非空说明
 * 客户端自己设置了光标（cursor_layer 里有 view，renderer 会画），这时
 * 就不再叠加软件箭头。
 */
static const char fbdev_cursor_arrow[16][17] = {
	"#               ",
	"##              ",
	"#.#             ",
	"#..#            ",
	"#...#           ",
	"#....#          ",
	"#.....#         ",
	"#......#        ",
	"#.......#       ",
	"#........#      ",
	"#.....#####     ",
	"#..#..#         ",
	"#.# #..#        ",
	"##   #..#       ",
	"#     #..#      ",
	"       ##       ",
};

static void
fbdev_cursor_damage(struct fbdev_backend *b, pixman_region32_t *damage)
{
	struct weston_pointer *pointer = b->seat.pointer_state;
	int cx, cy;

	if (pointer == NULL || pointer->sprite != NULL)
		return;

	cx = (int)pointer->pos.c.x;
	cy = (int)pointer->pos.c.y;

	/* 旧位置要把箭头擦掉；新位置要先把场景拷过来再画箭头。 */
	if (b->cursor_valid) {
		pixman_region32_union_rect(damage, damage, b->cursor_x,
					   b->cursor_y, 16, 16);
	}
	pixman_region32_union_rect(damage, damage, cx, cy, 16, 16);
}

static inline int
fbdev_cursor_bpp(struct fbdev_backend *b)
{
	return b->fb_bpp == 16 ? 2 : 4;
}

/* 把上一帧光标盖住的背景恢复回显存。
 * 恢复后显存内容重新等于"纯场景"，cursor_valid 清零。 */
static void
fbdev_cursor_restore(struct fbdev_backend *b)
{
	int bpp, x, y;

	if (!b->cursor_valid || b->fb == NULL)
		return;

	bpp = fbdev_cursor_bpp(b);
	for (y = 0; y < 16; y++) {
		int py = b->cursor_y + y;
		uint8_t *row;

		if (py < 0 || py >= b->fb_height)
			continue;
		row = (uint8_t *)b->fb + (size_t)py * b->fb_stride;
		for (x = 0; x < 16; x++) {
			int px = b->cursor_x + x;

			if (px < 0 || px >= b->fb_width)
				continue;
			memcpy(row + (size_t)px * bpp,
			       b->cursor_bg + ((size_t)y * 16 + x) * bpp,
			       (size_t)bpp);
		}
	}
	b->cursor_valid = 0;
}

static void
fbdev_draw_software_cursor(struct fbdev_backend *b)
{
	struct weston_pointer *pointer = b->seat.pointer_state;
	int cx, cy, x, y, bpp;

	if (b->fb == NULL || pointer == NULL || pointer->sprite != NULL)
		return;

	cx = (int)pointer->pos.c.x;
	cy = (int)pointer->pos.c.y;
	bpp = fbdev_cursor_bpp(b);

	/* 先保存箭头下面的背景，下一次移动时只恢复这一小块即可，
	 * 不需要为一次鼠标移动重跑整帧 GL。 */
	for (y = 0; y < 16; y++) {
		int py = cy + y;
		uint8_t *row;

		if (py < 0 || py >= b->fb_height)
			continue;
		row = (uint8_t *)b->fb + (size_t)py * b->fb_stride;
		for (x = 0; x < 16; x++) {
			int px = cx + x;

			if (px < 0 || px >= b->fb_width)
				continue;
			memcpy(b->cursor_bg + ((size_t)y * 16 + x) * bpp,
			       row + (size_t)px * bpp, (size_t)bpp);
		}
	}

	/* 再画箭头。 */
	for (y = 0; y < 16; y++) {
		int py = cy + y;
		uint8_t *row;

		if (py < 0 || py >= b->fb_height)
			continue;
		row = (uint8_t *)b->fb + (size_t)py * b->fb_stride;

		for (x = 0; x < 16; x++) {
			char c = fbdev_cursor_arrow[y][x];
			int px;

			if (c == ' ')
				continue;
			px = cx + x;
			if (px < 0 || px >= b->fb_width)
				continue;

			/* '#' 是箭头外轮廓，'.' 是内部：
			 * 黑色填充 + 白边，深浅背景下都能看清。 */
			if (b->fb_bpp == 16) {
				((uint16_t *)row)[px] =
					(c == '#') ? 0xffff : 0x0000;
			} else {
				((uint32_t *)row)[px] =
					(c == '#') ? 0x00ffffff : 0x00000000;
			}
		}
	}

	b->cursor_x = cx;
	b->cursor_y = cy;
	b->cursor_valid = 1;
}

/* ---------------------------------------------------------------- output */

static void
fbdev_set_dpms(struct weston_output *base, enum dpms_enum level)
{
	struct fbdev_output *output = to_fbdev_output(base);

	if (level == WESTON_DPMS_ON) {
		weston_output_schedule_repaint(base);
	} else if (base->repaint_status == REPAINT_AWAITING_COMPLETION) {
		wl_event_source_timer_update(output->finish_frame_timer, 0);
		weston_output_schedule_repaint_reset(base);
	}
}

static int
fbdev_output_start_repaint_loop(struct weston_output *output)
{
	struct timespec ts;

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts,
				   WP_PRESENTATION_FEEDBACK_INVALID);
	return 0;
}

/* 把 gl_shadow 里的 ARGB8888 按 damage 矩形转成 RGB565 写进 fb。
 * 全屏帧 damage 就是整屏，等价于原来的逐行循环；partial-damage 打开时
 * 只转真正脏的矩形，拖动窗口/局部刷新就不用付全屏转换的代价。 */
static void
fbdev_convert_damage(struct fbdev_backend *b, struct weston_output *base,
		     pixman_region32_t *damage)
{
	const uint32_t *src = (const uint32_t *)b->gl_shadow;
	int w = base->current_mode->width;
	int h = base->current_mode->height;
	pixman_box32_t *rects;
	int n, i;

	rects = pixman_region32_rectangles(damage, &n);
	for (i = 0; i < n; i++) {
		int x1 = rects[i].x1;
		int y1 = rects[i].y1;
		int x2 = rects[i].x2;
		int y2 = rects[i].y2;
		int x, y;

		if (x1 < 0)
			x1 = 0;
		if (y1 < 0)
			y1 = 0;
		if (x2 > w)
			x2 = w;
		if (y2 > h)
			y2 = h;
		if (x1 >= x2 || y1 >= y2)
			continue;

		for (y = y1; y < y2; y++) {
			uint16_t *dst = (uint16_t *)(b->fb +
						     (size_t)y * b->fb_stride);
			const uint32_t *row = src + (size_t)y * w;

			for (x = x1; x < x2; x++) {
				uint32_t p = row[x];
				unsigned r = (p >> 16) & 0xff;
				unsigned g = (p >> 8) & 0xff;
				unsigned bl = p & 0xff;

				dst[x] = (uint16_t)(((r >> 3) << 11) |
						    ((g >> 2) << 5) | (bl >> 3));
			}
		}
	}
}

static int
fbdev_output_repaint(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_backend *b = output->backend;
	struct weston_compositor *ec = base->compositor;
	struct weston_pointer *pointer = b->seat.pointer_state;
	pixman_region32_t damage;
	bool gl = ec->renderer->type == WESTON_RENDERER_GL;
	bool had_cursor;
	bool cursor_only;
	bool partial;
	static int repaint_no;
	int rn = repaint_no++;

	assert(output->renderbuffer);

	if (rn < 6) {
		weston_log("fbdev: repaint #%d begin\n", rn);
	}

	/* 先把上一帧的软件光标从显存里擦掉，让 fb 回到"纯场景"状态。
	 * 只碰 16x16 像素；后面的整帧 GL 转换会覆盖全屏，无副作用。 */
	had_cursor = b->cursor_valid;
	fbdev_cursor_restore(b);

	/* 这一帧要重画的区域：pixman/GL 会把它合成进 /dev/fb0 的内存里 */
	pixman_region32_init(&damage);
	weston_output_flush_damage_for_primary_plane(base, &damage);

	/* 【GL 光标帧】场景没有任何新的 damage，只是软件光标在移动。
	 * 这时文件里已经恢复了旧光标背景，画面场景与上一帧完全一致：
	 * 仍然让 renderer 跑完整帧（保持它的 buffer/fence 记账不变），
	 * 但可以省掉 glReadPixels 全屏回读 + 200 万像素 RGB565 转换。
	 * V3D 不支持只重绘脏区，所以全屏 GL 渲染这一步省不掉；
	 * 真正贵的是回读和逐像素转换。 */
	cursor_only = gl && had_cursor &&
		      !pixman_region32_not_empty(&damage) &&
		      pointer != NULL && pointer->sprite == NULL;

	/* GL 默认整帧重绘（V3D 的部分重绘历史上会卡死）。
	 * [fbdev] partial-damage=1 时按真实脏区重绘；拖窗口/局部刷新
	 * 只重画旧/新窗口矩形，不再付 1080p 全屏的代价。 */
	partial = gl && b->partial_damage && !cursor_only &&
		  pixman_region32_not_empty(&damage);

	/* Pixman：整屏软件重绘 0.5 秒，鼠标会卡；只补光标旧/新两个 16x16
	 * 小区域即可，场景本身的 damage 由 flush 正常带进来。 */
	if (gl) {
		if (!cursor_only && !partial)
			pixman_region32_union_rect(&damage, &damage, 0, 0,
						   base->current_mode->width,
						   base->current_mode->height);
	} else {
		fbdev_cursor_damage(b, &damage);
	}

	ec->renderer->repaint_output(base, &damage, output->renderbuffer);

	if (rn < 6) {
		weston_log("fbdev: repaint #%d rendered%s\n", rn,
			   cursor_only ? " (cursor-only)" : "");
	}

	/* GL 读回到 32bpp 影子缓冲；V3D 的 RGB565 glReadPixels 会返回全 0，
	 * 这里在 CPU 里转成 RGB565 写进 fb。pixman 路径直接写 fb，不需要。
	 * partial=1 时只转 damage 矩形。 */
	if (ec->renderer->type == WESTON_RENDERER_GL && b->gl_shadow != NULL &&
	    !cursor_only) {
		fbdev_convert_damage(b, base, &damage);
	}

	if (rn < 6) {
		weston_log("fbdev: repaint #%d converted\n", rn);
	}

	/* 光标画在整帧输出之后（GL 路径上面刚把影子缓冲整帧转成 RGB565）。 */
	fbdev_draw_software_cursor(b);

	pixman_region32_fini(&damage);

	/* 【每帧踢一下显示刷新】pixman 只改了显存内容，没有任何"提交"动作：
	 * QEMU 的 bcm2835-fb 只在主机侧收到 mailbox 请求（SET_VIRT_OFFSET /
	 * 改模式）时才把显存重新送进显示 surface，静默写像素屏幕不会更新。
	 * 调一次 FBIOPAN_DISPLAY（内核里走 mailbox SET_VIRT_OFFSET）既符合
	 * fbdev 翻页语义，也在真机上是无害的空翻。 */
	if (b->fd >= 0) {
		b->var.yoffset = 0;
		ioctl(b->fd, FBIOPAN_DISPLAY, &b->var);
	}

	/* 没有 vsync 硬件中断，用定时器按刷新率排下一帧 */
	weston_output_arm_frame_timer(base, output->finish_frame_timer);

	if (rn < 6) {
		weston_log("fbdev: repaint #%d done\n", rn);
	}

	return 0;
}

static int
fbdev_finish_frame_handler(void *data)
{
	struct fbdev_output *output = data;

	weston_output_finish_frame_from_timer(&output->base);
	return 1;
}

static int
fbdev_output_set_size(struct weston_output *base, int width, int height)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_backend *b = output->backend;
	struct weston_head *head;
	int output_width;
	int output_height;

	if (!output)
		return -1;

	/* 只允许设置一次 */
	assert(!base->current_mode);
	assert(base->current_scale);

	/* 屏幕尺寸由显存决定：请求值和 framebuffer 不一致时以显存为准 */
	if (b->fb_width > 0 &&
	    (width != b->fb_width || height != b->fb_height)) {
		weston_log("fbdev: requested %dx%d, using framebuffer %dx%d\n",
			   width, height, b->fb_width, b->fb_height);
		width = b->fb_width;
		height = b->fb_height;
	}

	wl_list_for_each(head, &base->head_list, output_link) {
		weston_head_set_monitor_strings(head, "yiyiya", "fbdev", NULL);
		weston_head_set_physical_size(head, width, height);
	}

	output_width = width * base->current_scale;
	output_height = height * base->current_scale;

	output->mode.flags =
		WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
	output->mode.width = output_width;
	output->mode.height = output_height;
	output->mode.refresh = b->refresh;
	wl_list_insert(&base->mode_list, &output->mode.link);

	base->current_mode = &output->mode;

	base->start_repaint_loop = fbdev_output_start_repaint_loop;
	base->repaint = fbdev_output_repaint;
	base->assign_planes = NULL;
	base->set_backlight = NULL;
	base->set_dpms = fbdev_set_dpms;
	base->switch_mode = NULL;

	return 0;
}

static int
fbdev_output_enable(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_backend *b = output->backend;
	struct weston_compositor *ec = b->compositor;
	struct wl_event_loop *loop;

	if (base->enabled)
		return 0;

	/* 新输出的显存内容还不属于任何一帧，不能拿旧坐标去恢复光标背景。 */
	b->cursor_valid = 0;

	if (ec->renderer->type != WESTON_RENDERER_PIXMAN &&
	    ec->renderer->type != WESTON_RENDERER_GL) {
		weston_log("fbdev: unsupported renderer type %d\n",
			   ec->renderer->type);
		return -1;
	}

	loop = wl_display_get_event_loop(ec->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, fbdev_finish_frame_handler,
					output);
	if (output->finish_frame_timer == NULL)
		return -1;

	if (ec->renderer->type == WESTON_RENDERER_GL) {
		struct gl_renderer_fbo_options gl_options = { 0 };
		/* FBO 用 XRGB8888（V3D 不支持 RGB565 renderbuffer）；
		 * glReadPixels 读到 32bpp 影子缓冲，再转 RGB565 写 fb。 */
		const struct pixel_format_info *format =
			pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8);
		size_t shadow_len;

		gl_options.fb_size.width = base->current_mode->width;
		gl_options.fb_size.height = base->current_mode->height;
		gl_options.area.x = 0;
		gl_options.area.y = 0;
		gl_options.area.width = base->current_mode->width;
		gl_options.area.height = base->current_mode->height;

		if (ec->renderer->gl->output_fbo_create(base, &gl_options) < 0) {
			weston_log("fbdev: gl output_fbo_create failed\n");
			return -1;
		}

		shadow_len = (size_t)base->current_mode->width *
			     base->current_mode->height * 4;
		if (b->gl_shadow == NULL || b->gl_shadow_len < shadow_len) {
			free(b->gl_shadow);
			b->gl_shadow = malloc(shadow_len);
			b->gl_shadow_len = shadow_len;
		}
		if (b->gl_shadow == NULL) {
			weston_log("fbdev: gl shadow alloc failed\n");
			ec->renderer->gl->output_destroy(base);
			return -1;
		}

		output->renderbuffer = ec->renderer->create_renderbuffer(
			base, format, b->gl_shadow,
			base->current_mode->width * 4, NULL, NULL);
		if (output->renderbuffer == NULL) {
			weston_log("fbdev: gl create_renderbuffer failed\n");
			ec->renderer->gl->output_destroy(base);
			return -1;
		}

		return 0;
	}

	/* 【对齐 backend-headless 的写法】用影子缓冲：pixman 先合成到自己的
	 * shadow image，再由渲染器把脏区一次性拷进我们的 renderbuffer（= 显存
	 * 映射）。之前 use_shadow=false 是"边合成边写正在显示的显存"，画面可能
	 * 被看到半帧状态（拖窗口时整窗闪）。开影子后写显存只发生在合成完成之后，
	 * 且拷贝只覆盖脏区，代价是一份额外内存 + 一次 memcpy。 */
	{
		struct pixman_renderer_output_options options;

		memset(&options, 0, sizeof options);
		options.use_shadow = true;
		options.format = b->format;
		options.fb_size.width = base->current_mode->width;
		options.fb_size.height = base->current_mode->height;

		if (ec->renderer->pixman->output_create(base, &options) < 0) {
			weston_log("fbdev: pixman output_create failed\n");
			return -1;
		}

		output->renderbuffer = ec->renderer->create_renderbuffer(
			base, options.format, b->fb, b->fb_stride, NULL, NULL);
		if (output->renderbuffer == NULL) {
			weston_log("fbdev: create_renderbuffer failed\n");
			ec->renderer->pixman->output_destroy(base);
			return -1;
		}
	}

	return 0;
}

static int
fbdev_output_disable(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct fbdev_backend *b = output->backend;
	struct weston_compositor *ec = base->compositor;

	if (!base->enabled)
		return 0;

	b->cursor_valid = 0;

	if (output->renderbuffer) {
		ec->renderer->destroy_renderbuffer(output->renderbuffer);
		output->renderbuffer = NULL;
	}
	if (ec->renderer->type == WESTON_RENDERER_GL)
		ec->renderer->gl->output_destroy(base);
	else
		ec->renderer->pixman->output_destroy(base);

	if (output->finish_frame_timer) {
		wl_event_source_remove(output->finish_frame_timer);
		output->finish_frame_timer = NULL;
	}

	return 0;
}

static void
fbdev_output_destroy(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);

	assert(output);

	fbdev_output_disable(base);
	weston_output_release(base);

	free(output);
}

static struct weston_output *
fbdev_output_create(struct weston_backend *backend, const char *name)
{
	struct fbdev_backend *b = to_fbdev_backend(backend);
	struct fbdev_output *output;

	assert(name);

	output = zalloc(sizeof *output);
	if (output == NULL)
		return NULL;

	output->backend = b;
	output->mode.refresh = b->refresh;

	weston_output_init(&output->base, b->compositor, name);

	output->base.destroy = fbdev_output_destroy;
	output->base.disable = fbdev_output_disable;
	output->base.enable = fbdev_output_enable;
	output->base.attach_head = NULL;

	weston_compositor_add_pending_output(&output->base, b->compositor);

	return &output->base;
}

/* ------------------------------------------------------------ 窗口化 API */

static int
fbdev_head_create(struct weston_backend *backend, const char *name)
{
	struct fbdev_backend *b = to_fbdev_backend(backend);
	struct fbdev_head *head;

	assert(name);

	head = zalloc(sizeof *head);
	if (head == NULL)
		return -1;

	weston_head_init(&head->base, name);
	head->base.backend = &b->base;
	weston_head_set_connection_status(&head->base, true);
	weston_compositor_add_head(b->compositor, &head->base);

	return 0;
}

static void
fbdev_head_destroy(struct weston_head *base)
{
	weston_head_release(base);
	free(base);
}

/* 两个成员都必须给：前端是 api->create_head(...) 直接调用的，
 * 漏一个就是 pc=0 的取指异常（日志停在 Registered plugin API 之后）。 */
static const struct weston_windowed_output_api fbdev_output_api = {
	.create_head = fbdev_head_create,
	.output_set_size = fbdev_output_set_size,
};

/* ---------------------------------------------------------------- input */

/* 一帧内累积的相对位移一次性投递（SYN_REPORT 时调用）。
 *
 * 【必须走 notify_* 而不是 weston_pointer_send_*】
 *   weston_pointer_send_motion() 只做"改指针位置 + 发给 focus client"，
 *   它不会把事件派发给当前的指针 grab（那是 notify_motion() 干的：
 *   pointer->grab->interface->motion(...)）。拖窗口时 grab 正是
 *   desktop-shell 的 move grab —— 直接调 send_motion，grab 收不到 motion，
 *   窗口就跟不走。
 *   同理 weston_pointer_send_button() 只发 wl_pointer.button，不做
 *   pointer->button_count++、不跑 button binding、也不派发给 grab
 *   （见 libweston/input.c 的 notify_button()）；button_count 恒 0 会让
 *   "按下期间保持隐式 grab / 用 grab_button 判定拖拽"这类逻辑全部失效，
 *   表现就是点得下去但拖不动。 */
static void
fbdev_input_flush_motion(struct fbdev_backend *b, struct timespec *ts)
{
	struct weston_pointer_motion_event ev;
	struct weston_coord rel;

	if (b->rel_x == 0 && b->rel_y == 0)
		return;

	rel.x = b->rel_x * b->pointer_scale;
	rel.y = b->rel_y * b->pointer_scale;
	/* 只带 REL 位：abs / rel_unaccel 传 NULL 是安全的（实现里只在
	 * 对应 mask 位置位时才解引用）。 */
	weston_pointer_motion_event_init(&ev, ts, &b->seat,
					 WESTON_POINTER_MOTION_REL,
					 NULL, &rel, NULL);
	notify_motion(&ev);
	notify_pointer_frame(&b->seat);

	{
		static int motion_log;
		if (motion_log++ < 8) {
			struct weston_pointer *p = b->seat.pointer_state;
			weston_log("fbdev: motion rel=%d,%d pos=%d,%d\n",
				   (int)rel.x, (int)rel.y,
				   p ? (int)p->pos.c.x : -1,
				   p ? (int)p->pos.c.y : -1);
		}
	}

	/* 【软件光标】没有 cursor_layer view 时，weston_pointer_move_to() 不会
	 * 给光标区域排重绘，必须自己催一帧，否则箭头只更新位置不更新画面。 */
	weston_compositor_schedule_repaint(b->compositor);

	b->rel_x = 0;
	b->rel_y = 0;
}

static void
fbdev_input_handle_button(struct fbdev_backend *b, struct timespec *ts,
			  uint16_t button, int32_t value)
{
	struct weston_pointer_button_event ev;

	weston_pointer_button_event_init(&ev, ts, &b->seat, button,
					 value ? WL_POINTER_BUTTON_STATE_PRESSED
					       : WL_POINTER_BUTTON_STATE_RELEASED);
	notify_button(&ev);
	notify_pointer_frame(&b->seat);
}

static int
fbdev_input_handler(int fd, uint32_t mask, void *data)
{
	struct fbdev_backend *b = data;
	struct fbdev_input_event ev[64];
	ssize_t len;
	ssize_t i;
	ssize_t count;

	if (!(mask & WL_EVENT_READABLE))
		return 0;

	len = read(fd, ev, sizeof ev);
	{
		static int read_log;
		if (read_log++ < 8) {
			weston_log("fbdev: input read len=%d first=%u/%u/%d\n",
				   (int)len,
				   len >= (ssize_t)sizeof ev[0] ? ev[0].type : 0,
				   len >= (ssize_t)sizeof ev[0] ? ev[0].code : 0,
				   len >= (ssize_t)sizeof ev[0] ? ev[0].value : 0);
		}
	}
	if (len <= 0)
		return 0;

	count = len / (ssize_t)sizeof ev[0];
	for (i = 0; i < count; i++) {
		struct timespec ts;

		ts.tv_sec = (time_t)ev[i].sec;
		ts.tv_nsec = (long)ev[i].usec * 1000;

		switch (ev[i].type) {
		case EV_REL:
			if (ev[i].code == REL_X)
				b->rel_x += ev[i].value;
			else if (ev[i].code == REL_Y)
				b->rel_y += ev[i].value;
			break;
		case EV_KEY:
			if (ev[i].code == BTN_LEFT ||
			    ev[i].code == BTN_RIGHT ||
			    ev[i].code == BTN_MIDDLE)
				fbdev_input_handle_button(b, &ts, ev[i].code,
							  ev[i].value);
			break;
		case EV_SYN:
			if (ev[i].code == SYN_REPORT)
				fbdev_input_flush_motion(b, &ts);
			break;
		default:
			break;
		}
	}

	return 0;
}

static int
fbdev_keyboard_handler(int fd, uint32_t mask, void *data)
{
	struct fbdev_backend *b = data;
	struct fbdev_input_event ev[64];
	ssize_t len;
	ssize_t i;
	ssize_t count;

	if (!(mask & WL_EVENT_READABLE) || !b->kbd_ready)
		return 0;

	len = read(fd, ev, sizeof ev);
	if (len <= 0)
		return 0;

	count = len / (ssize_t)sizeof ev[0];
	for (i = 0; i < count; i++) {
		struct timespec ts;
		struct weston_key_event key;

		if (ev[i].type != EV_KEY)
			continue;
		/* event1 是纯键盘设备；BTN_* 不应出现，真出现也不当键处理 */
		if (ev[i].code >= 0x100)
			continue;
		if (ev[i].value != 0 && ev[i].value != 1)
			continue;

		ts.tv_sec = (time_t)ev[i].sec;
		ts.tv_nsec = (long)ev[i].usec * 1000;
		weston_key_event_init(&key, &ts, &b->seat, ev[i].code,
				      ev[i].value ?
				      WL_KEYBOARD_KEY_STATE_PRESSED :
				      WL_KEYBOARD_KEY_STATE_RELEASED,
				      STATE_UPDATE_AUTOMATIC);
		notify_key(&key);
	}

	return 0;
}

static void
fbdev_input_init(struct fbdev_backend *b)
{
	struct wl_event_loop *loop;

	weston_seat_init(&b->seat, b->compositor, "seat0");
	b->input_ready = true;
	weston_seat_init_pointer(&b->seat);

	/* 键盘 seat 用全局 keymap（/usr/share/X11/xkb 已存在）。失败只影响
	 * 键盘，不能连累鼠标，所以单独判返回值。xkb context/规则名要在
	 * 这里显式初始化：上游只有 libinput 后端会调它。 */
	weston_compositor_set_xkb_rule_names(b->compositor, NULL);
	if (weston_seat_init_keyboard(&b->seat, NULL) == 0) {
		b->kbd_ready = true;
	} else {
		weston_log("fbdev: keyboard seat init failed (no keymap?)\n");
	}

	b->kbd_fd = open("/dev/input/event1",
			 O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (b->kbd_fd < 0) {
		weston_log("fbdev: open /dev/input/event1 failed: %s "
			   "(desktop will have no keyboard)\n", strerror(errno));
	} else {
		loop = wl_display_get_event_loop(b->compositor->wl_display);
		b->kbd_source = wl_event_loop_add_fd(loop, b->kbd_fd,
						     WL_EVENT_READABLE,
						     fbdev_keyboard_handler, b);
		if (b->kbd_source == NULL) {
			weston_log("fbdev: cannot watch /dev/input/event1\n");
			close(b->kbd_fd);
			b->kbd_fd = -1;
		} else {
			weston_log("fbdev: keyboard /dev/input/event1 ready\n");
		}
	}

	b->input_fd = open("/dev/input/event0",
			   O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (b->input_fd < 0) {
		weston_log("fbdev: open /dev/input/event0 failed: %s "
			   "(desktop will have no pointer)\n", strerror(errno));
		return;
	}

	loop = wl_display_get_event_loop(b->compositor->wl_display);
	b->input_source = wl_event_loop_add_fd(loop, b->input_fd,
					       WL_EVENT_READABLE,
					       fbdev_input_handler, b);
	if (b->input_source == NULL) {
		weston_log("fbdev: cannot watch /dev/input/event0\n");
		close(b->input_fd);
		b->input_fd = -1;
		return;
	}

	weston_log("fbdev: input /dev/input/event0 ready (relative pointer)\n");
}

static void
fbdev_input_fini(struct fbdev_backend *b)
{
	if (b->kbd_source) {
		wl_event_source_remove(b->kbd_source);
		b->kbd_source = NULL;
	}
	if (b->kbd_fd >= 0) {
		close(b->kbd_fd);
		b->kbd_fd = -1;
	}
	b->kbd_ready = false;

	if (b->input_source) {
		wl_event_source_remove(b->input_source);
		b->input_source = NULL;
	}
	if (b->input_fd >= 0) {
		close(b->input_fd);
		b->input_fd = -1;
	}
	if (b->input_ready) {
		weston_seat_release(&b->seat);
		b->input_ready = false;
	}
}

/* --------------------------------------------------------------- backend */

static void
fbdev_destroy(struct weston_backend *backend)
{
	struct fbdev_backend *b = to_fbdev_backend(backend);
	struct weston_head *head, *next;

	/* 后端负责销毁自己建的 head：libweston 在 weston_compositor_destroy()
	 * 里 assert(wl_list_empty(&compositor->head_list))，漏掉这一步就是
	 * 每次退出都断言失败（做法与 backend-headless 的 headless_destroy 一致）。 */
	wl_list_remove(&b->base.link);
	wl_list_for_each_safe(head, next, &b->compositor->head_list,
			      compositor_link) {
		if (head->backend == &b->base)
			fbdev_head_destroy(head);
	}

	fbdev_input_fini(b);

	if (b->fb)
		munmap(b->fb, b->fb_len);
	if (b->fd >= 0)
		close(b->fd);

	free(b->gl_shadow);
	free(b);
}

static int
fbdev_open(struct fbdev_backend *b, const char *path)
{
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	size_t want;

	if (path == NULL)
		path = "/dev/fb0";

	b->fd = open(path, O_RDWR);
	if (b->fd < 0) {
		weston_log("fbdev: cannot open %s: %s\n", path, strerror(errno));
		return -1;
	}

	memset(&var, 0, sizeof var);
	memset(&fix, 0, sizeof fix);
	if (ioctl(b->fd, FBIOGET_VSCREENINFO, &var) < 0 ||
	    ioctl(b->fd, FBIOGET_FSCREENINFO, &fix) < 0) {
		weston_log("fbdev: %s is not a usable framebuffer\n", path);
		return -1;
	}

	b->fb_width = (int)var.xres;
	b->fb_height = (int)var.yres;
	b->fb_stride = (int)fix.line_length;
	b->fb_bpp = (int)var.bits_per_pixel;
	b->var = var;

	if (b->fb_width <= 0 || b->fb_height <= 0 || b->fb_stride <= 0 ||
	    (var.bits_per_pixel != 32 && var.bits_per_pixel != 16)) {
		weston_log("fbdev: unsupported mode %dx%d %ubpp stride %d\n",
			   b->fb_width, b->fb_height, var.bits_per_pixel,
			   b->fb_stride);
		return -1;
	}

	b->format = pixel_format_get_info_by_pixman(
		var.bits_per_pixel == 16 ? PIXMAN_r5g6b5 : PIXMAN_x8r8g8b8);
	if (b->format == NULL) {
		weston_log("fbdev: no pixel format for %ubpp\n",
			   var.bits_per_pixel);
		return -1;
	}

	want = (size_t)b->fb_stride * (size_t)b->fb_height;
	b->fb = mmap(NULL, want, PROT_READ | PROT_WRITE, MAP_SHARED, b->fd, 0);
	if (b->fb == MAP_FAILED) {
		b->fb = NULL;
		weston_log("fbdev: mmap %u bytes failed\n", (unsigned)want);
		return -1;
	}
	b->fb_len = want;

	weston_log("fbdev: %s %dx%d %ubpp, stride %d, %u bytes mapped\n", path,
		   b->fb_width, b->fb_height, var.bits_per_pixel, b->fb_stride,
		   (unsigned)want);
	return 0;
}

static void
fbdev_cleanup(struct fbdev_backend *b)
{
	if (b->fb)
		munmap(b->fb, b->fb_len);
	if (b->fd >= 0)
		close(b->fd);
	free(b->gl_shadow);
	free(b);
}

WL_EXPORT int
weston_backend_init(struct weston_compositor *compositor,
		    struct weston_backend_config *config_base)
{
	struct weston_fbdev_backend_config *config;
	struct fbdev_backend *b;

	if (config_base == NULL ||
	    config_base->struct_version != WESTON_FBDEV_BACKEND_CONFIG_VERSION ||
	    config_base->struct_size >
		    sizeof(struct weston_fbdev_backend_config)) {
		weston_log("fbdev: backend config version/size mismatch\n");
		return -1;
	}
	config = (struct weston_fbdev_backend_config *)config_base;

	b = zalloc(sizeof *b);
	if (b == NULL)
		return -1;

	b->fd = -1;
	b->input_fd = -1;
	b->compositor = compositor;
	b->refresh = config->refresh > 0 ? config->refresh : DEFAULT_REFRESH;
	/* 【指针位移倍率】fbdev 后端直接读 evdev，不经过 libinput，所以
	 * [libinput] 的 accel-profile/accel-speed 对它无效。倍率从 weston.ini
	 * 的 [fbdev] pointer-scale 读，默认 1.0（原始 1:1）。
	 * 前端加载配置后会把实际路径写进 WESTON_CONFIG_FILE，这里复用它，
	 * 保证 -c/自定义路径和默认路径都能读到同一份文件。 */
	b->pointer_scale = 1.0;
	b->partial_damage = false;
	{
		const char *cfg_path = getenv(WESTON_CONFIG_FILE_ENV_VAR);
		struct weston_config *wc;

		if (cfg_path == NULL || cfg_path[0] == '\0')
			cfg_path = "/etc/xdg/weston/weston.ini";
		wc = weston_config_parse(cfg_path);
		if (wc != NULL) {
			struct weston_config_section *sec =
				weston_config_get_section(wc, "fbdev", NULL, NULL);
			if (sec != NULL) {
				double v = 1.0;
				bool pd = false;

				if (weston_config_section_get_double(
						sec, "pointer-scale", &v, 1.0) == 0 &&
				    v >= 0.25 && v <= 10.0) {
					b->pointer_scale = v;
				}
				if (weston_config_section_get_bool(
						sec, "partial-damage", &pd,
						false) == 0) {
					b->partial_damage = pd;
				}
			}
			weston_config_destroy(wc);
		} else {
			weston_log("fbdev: cannot read %s, pointer-scale=1.00\n",
				   cfg_path);
		}
		weston_log("fbdev: pointer-scale=%.2f partial-damage=%d\n",
			   b->pointer_scale, b->partial_damage);
	}

	/* 先打开 fb：Pi5 固件实际给的是 16bpp（即使请求 32），GL renderer
	 * 的 formats / read_format 必须按 fb 的真实 bpp 选。 */
	if (fbdev_open(b, config->device_path) < 0) {
		free(b);
		return -1;
	}

	if (config->renderer == WESTON_RENDERER_GL) {
		const struct pixel_format_info *formats[2] = {
			b->format,
			pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8),
		};
		/* 用 test-gles-teapot 验证过的 EGL device 平台。这里用
		 * dlopen/dlsym 取 eglQueryDevicesEXT，避免把 EGL 依赖的
		 * Mesa include 路径带进本模块（会和 weston config.h 打架）。 */
		void *egl_lib = dlopen("/lib/libEGL.so", RTLD_NOW);
		void *(*get_proc)(const char *) =
			egl_lib ? (void *(*)(const char *))
					  dlsym(egl_lib, "eglGetProcAddress")
				: NULL;
		PFNEGLQUERYDEVICESEXTPROC query_devices =
			get_proc ? (PFNEGLQUERYDEVICESEXTPROC)
					   get_proc("eglQueryDevicesEXT")
				 : NULL;
		EGLDeviceEXT egl_dev = EGL_NO_DEVICE_EXT;
		EGLint ndev = 0;

		if (query_devices == NULL) {
			const char *err = dlerror();
			weston_log("fbdev: cannot get eglQueryDevicesEXT: %s\n",
				   err ? err : "unknown");
		} else {
			query_devices(1, &egl_dev, &ndev);
		}

		struct gl_renderer_display_options options = {
			.egl_platform = ndev > 0 ? EGL_PLATFORM_DEVICE_EXT
						 : EGL_PLATFORM_SURFACELESS_MESA,
			.egl_native_display = ndev > 0 ? egl_dev : NULL,
			.formats = formats,
			.formats_count = formats[1] != NULL ? 2 : 1,
			.read_format = NULL,
		};

		if (weston_compositor_init_renderer(compositor,
						    WESTON_RENDERER_GL,
						    &options.base) < 0) {
			weston_log("fbdev: GL renderer init failed\n");
			fbdev_cleanup(b);
			return -1;
		}
	} else if (config->renderer == WESTON_RENDERER_PIXMAN ||
		   config->renderer == WESTON_RENDERER_AUTO) {
		if (weston_compositor_init_renderer(compositor,
						    WESTON_RENDERER_PIXMAN,
						    NULL) < 0) {
			weston_log("fbdev: pixman renderer init failed\n");
			fbdev_cleanup(b);
			return -1;
		}
	} else {
		weston_log("fbdev: unsupported renderer %d\n", config->renderer);
		fbdev_cleanup(b);
		return -1;
	}

	b->base.destroy = fbdev_destroy;
	b->base.create_output = fbdev_output_create;
	b->base.supported_presentation_clocks = WESTON_PRESENTATION_CLOCKS_SOFTWARE;

	wl_list_insert(&compositor->backend_list, &b->base.link);

	/* 已确认：weston-desktop 的崩溃由输入初始化引入，不是构建产物问题。 */
	fbdev_input_init(b);

	if (weston_plugin_api_register(compositor,
				       WESTON_WINDOWED_OUTPUT_API_NAME_FBDEV,
				       &fbdev_output_api,
				       sizeof(fbdev_output_api)) < 0) {
		weston_log("fbdev: failed to register windowed output api\n");
		fbdev_destroy(&b->base);
		return -1;
	}

	return 0;
}
