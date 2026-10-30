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

#include <pixman.h>
#include <wayland-server-core.h>

#include <libweston/libweston.h>
#include <libweston/windowed-output-api.h>
#include <libweston/zalloc.h>

#include "shared/helpers.h"
#include "backend.h"
#include "libweston-internal.h"
#include "pixel-formats.h"
#include "pixman-renderer.h"
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

struct fbdev_backend {
	struct weston_backend base;
	struct weston_compositor *compositor;

	int fd;
	uint8_t *fb;
	size_t fb_len;
	int fb_width;
	int fb_height;
	int fb_stride;

	int refresh;

	/* 打开时读到的模式，用于每帧 FBIOPAN_DISPLAY"踢"一下显示刷新 */
	struct fb_var_screeninfo var;
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

/* 已合成帧数（只用于周期性打点） */
static int fbdev_frames;

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
	static int srdbg = 0;

	if (srdbg++ < 6)
		weston_log("fbdev: start_repaint_loop #%d\n", srdbg);

	weston_compositor_read_presentation_clock(output->compositor, &ts);
	weston_output_finish_frame(output, &ts,
				   WP_PRESENTATION_FEEDBACK_INVALID);
	return 0;
}

static int
fbdev_output_repaint(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct weston_compositor *ec = base->compositor;
	pixman_region32_t damage;

	assert(output->renderbuffer);

	/* 这一帧要重画的区域：pixman 会直接把它合成进 /dev/fb0 的内存里 */
	pixman_region32_init(&damage);
	weston_output_flush_damage_for_primary_plane(base, &damage);
	ec->renderer->repaint_output(base, &damage, output->renderbuffer);
	pixman_region32_fini(&damage);

	/* 【每帧踢一下显示刷新】pixman 只改了显存内容，没有任何"提交"动作：
	 * QEMU 的 bcm2835-fb 只在主机侧收到 mailbox 请求（SET_VIRT_OFFSET /
	 * 改模式）时才把显存重新送进显示 surface，静默写像素屏幕不会更新。
	 * 调一次 FBIOPAN_DISPLAY（内核里走 mailbox SET_VIRT_OFFSET）既符合
	 * fbdev 翻页语义，也在真机上是无害的空翻。 */
	if (output->backend->fd >= 0) {
		output->backend->var.yoffset = 0;
		ioctl(output->backend->fd, FBIOPAN_DISPLAY,
		      &output->backend->var);
	}

	/* 没有 vsync 硬件中断，用定时器按刷新率排下一帧 */
	weston_output_arm_frame_timer(base, output->finish_frame_timer);

	/* 头几帧打个点：显存中心像素（0 就是没画上东西），排查上屏问题时很有用 */
	fbdev_frames++;
	if (fbdev_frames == 1 || fbdev_frames % 120 == 0) {
		const uint8_t *row = output->backend->fb +
			(size_t)(base->current_mode->height / 2) *
				output->backend->fb_stride;
		uint32_t px = ((const uint32_t *)row)[base->current_mode->width / 2];

		weston_log("fbdev: frame %d, center pixel 0x%08x\n", fbdev_frames,
			   px);
	}

	return 0;
}

static int
fbdev_finish_frame_handler(void *data)
{
	struct fbdev_output *output = data;
	/* 【诊断·临时】看帧定时器到底有没有按时到点（只打前 6 次/每 60 次） */
	static int ffdbg = 0;

	if (ffdbg++ < 6 || ffdbg % 60 == 0)
		weston_log("fbdev: finish_frame_handler #%d status=%d\n", ffdbg,
			   (int)output->base.repaint_status);

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
	struct pixman_renderer_output_options options;

	if (base->enabled)
		return 0;

	if (ec->renderer->type != WESTON_RENDERER_PIXMAN) {
		weston_log("fbdev: only the pixman renderer is supported\n");
		return -1;
	}

	loop = wl_display_get_event_loop(ec->wl_display);
	output->finish_frame_timer =
		wl_event_loop_add_timer(loop, fbdev_finish_frame_handler,
					output);
	if (output->finish_frame_timer == NULL)
		return -1;

	/* 不建影子缓冲：pixman 直接画进 framebuffer 映射 */
	memset(&options, 0, sizeof options);
	options.use_shadow = false;
	options.format = pixel_format_get_info_by_pixman(PIXMAN_x8r8g8b8);
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

	return 0;
}

static int
fbdev_output_disable(struct weston_output *base)
{
	struct fbdev_output *output = to_fbdev_output(base);
	struct weston_compositor *ec = base->compositor;

	if (!base->enabled)
		return 0;

	if (output->renderbuffer) {
		ec->renderer->destroy_renderbuffer(output->renderbuffer);
		output->renderbuffer = NULL;
	}
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

/* 两个成员都必须给：前端是 api->create_head(...) 直接调用的，
 * 漏一个就是 pc=0 的取指异常（日志停在 Registered plugin API 之后）。 */
static const struct weston_windowed_output_api fbdev_output_api = {
	.create_head = fbdev_head_create,
	.output_set_size = fbdev_output_set_size,
};

/* --------------------------------------------------------------- backend */

static void
fbdev_destroy(struct weston_backend *backend)
{
	struct fbdev_backend *b = to_fbdev_backend(backend);

	if (b->fb)
		munmap(b->fb, b->fb_len);
	if (b->fd >= 0)
		close(b->fd);

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
	b->var = var;

	if (b->fb_width <= 0 || b->fb_height <= 0 || b->fb_stride <= 0 ||
	    var.bits_per_pixel != 32) {
		weston_log("fbdev: unsupported mode %dx%d %ubpp stride %d\n",
			   b->fb_width, b->fb_height, var.bits_per_pixel,
			   b->fb_stride);
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
	b->compositor = compositor;
	b->refresh = config->refresh > 0 ? config->refresh : DEFAULT_REFRESH;

	if (weston_compositor_init_renderer(compositor, WESTON_RENDERER_PIXMAN,
					    NULL) < 0) {
		weston_log("fbdev: pixman renderer init failed\n");
		free(b);
		return -1;
	}

	if (fbdev_open(b, config->device_path) < 0) {
		free(b);
		return -1;
	}

	b->base.destroy = fbdev_destroy;
	b->base.create_output = fbdev_output_create;
	b->base.supported_presentation_clocks = WESTON_PRESENTATION_CLOCKS_SOFTWARE;

	wl_list_insert(&compositor->backend_list, &b->base.link);

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
