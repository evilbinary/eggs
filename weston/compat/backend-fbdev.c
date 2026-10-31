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
	struct fbdev_backend *b = output->backend;
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
	if (b->fd >= 0) {
		b->var.yoffset = 0;
		ioctl(b->fd, FBIOPAN_DISPLAY, &b->var);
	}

	/* 没有 vsync 硬件中断，用定时器按刷新率排下一帧 */
	weston_output_arm_frame_timer(base, output->finish_frame_timer);

	/* 头几帧打点：采样中心/左上/面板区 + 隔点统计颜色种类，排查上屏问题很有用 */
	fbdev_frames++;
	if (fbdev_frames == 1 || fbdev_frames % 120 == 0) {
		int w = base->current_mode->width;
		int h = base->current_mode->height;
		size_t stride = b->fb_stride;
		const uint8_t *fb = b->fb;
		uint32_t c_center, c_tl, c_panel;
		int x, y, other = 0, total = 0, redish = 0;

		c_center = ((const uint32_t *)(fb + (size_t)(h / 2) * stride))[w / 2];
		c_tl = ((const uint32_t *)(fb + (size_t)100 * stride))[100];
		c_panel = ((const uint32_t *)(fb + (size_t)16 * stride))[w / 2];

		/* 隔 8 像素扫一遍全帧，统计非背景色（黑/0）与红色像素比例 */
		for (y = 0; y < h; y += 8) {
			const uint32_t *row = (const uint32_t *)(fb + (size_t)y * stride);

			for (x = 0; x < w; x += 8) {
				uint32_t p = row[x];

				total++;
				if (p == 0)
					other++;
				if ((p & 0x00f00000) && !(p & 0x0000f000))
					redish++;
			}
		}

		weston_log("fbdev: frame %d center=0x%08x tl(100,100)=0x%08x panel=0x%08x blank=%d%% red=%d%%\n",
			   fbdev_frames, c_center, c_tl, c_panel,
			   total ? other * 100 / total : 0,
			   total ? redish * 100 / total : 0);
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

	/* 【对齐 backend-headless 的写法】用影子缓冲：pixman 先合成到自己的
	 * shadow image，再由渲染器把脏区一次性拷进我们的 renderbuffer（= 显存
	 * 映射）。之前 use_shadow=false 是"边合成边写正在显示的显存"，画面可能
	 * 被看到半帧状态（拖窗口时整窗闪）。开影子后写显存只发生在合成完成之后，
	 * 且拷贝只覆盖脏区，代价是一份额外内存 + 一次 memcpy。 */
	memset(&options, 0, sizeof options);
	options.use_shadow = true;
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

	rel.x = b->rel_x;
	rel.y = b->rel_y;
	/* 只带 REL 位：abs / rel_unaccel 传 NULL 是安全的（实现里只在
	 * 对应 mask 位置位时才解引用）。 */
	weston_pointer_motion_event_init(&ev, ts, &b->seat,
					 WESTON_POINTER_MOTION_REL,
					 NULL, &rel, NULL);
	notify_motion(&ev);
	notify_pointer_frame(&b->seat);

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

static void
fbdev_input_init(struct fbdev_backend *b)
{
	struct wl_event_loop *loop;

	weston_seat_init(&b->seat, b->compositor, "seat0");
	b->input_ready = true;
	weston_seat_init_pointer(&b->seat);
	/* 【对照实验】暂时不建键盘，用来区分崩溃是 pointer 还是 keyboard 引入 */
	/* if (weston_seat_init_keyboard(&b->seat, NULL) < 0)
		weston_log("fbdev: keyboard init failed (continue without)\n"); */

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
	b->input_fd = -1;
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
