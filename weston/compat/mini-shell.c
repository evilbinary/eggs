/* 最小 shell：把客户端提交的 surface 直接铺到 (0,0)、放进普通层并 map。
 *
 * libweston 不会合成"没有 shell 接管"的 surface：不仅要有 view，view 还必须
 * 挂在某个 weston_layer 的 view_list 上，repaint 时才会被遍历到。这里给冒烟
 * 测试一个 40 行的替代实现：
 *   surface 出现 → 建 view → 贴 (0,0) → 插进 NORMAL 层 → map
 * 同时每 50ms 触发一次 repaint 并读 output 影子缓冲的一个像素，抓到非零帧
 * 就打印出来（验证 client 像素 → wl_shm → 合成 → output 缓冲 整条链）。
 */
#include "../libweston/libweston-internal.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mini-shell.h"

static struct {
	struct weston_compositor *ec;
	struct weston_output *output;
	struct weston_layer layer;
	struct wl_listener create_surface;
	struct wl_listener output_frame;
	struct wl_event_source *tick;
	int reported;
	int failed;
	int has_view;
} shell;

static int shell_frames;

/* ===================== fbdev 上屏 =====================
 * headless 后端只把合成结果画进 pixman 的影子缓冲（640x480/输出尺寸），
 * 不碰真实显存。这里把整帧搬到 /dev/fb0（内核 gpu 模块已注册的 framebuffer，
 * mmap 直接拿到显存地址），这样 QEMU/真机的屏幕上就能看到 wayland 画面。
 * 尺寸不一致时居中放置（输出尺寸由 wl-weston 按 fb 尺寸来定，正常情况下
 * 是 1:1；留居中逻辑是为了"输出固定 640x480 跑在大屏上"也不至于花屏）。 */
static struct {
	int fd;
	uint32_t *mem;
	int w;
	int h;
	int bpp;
} fbsink = { .fd = -1 };

void
yy_fb_size(int *w, int *h)
{
	const char *paths[2] = { "/dev/fb0", "/dev/fb" };
	int fd = -1;
	int ww = 0;
	int hh = 0;
	int i;

	for (i = 0; i < 2 && fd < 0; i++)
		fd = open(paths[i], O_RDWR);
	if (fd >= 0) {
		ww = ioctl(fd, YY_FB_GET_WIDTH);
		hh = ioctl(fd, YY_FB_GET_HEIGHT);
		close(fd);
	}

	if (ww <= 0 || hh <= 0) {
		ww = 640;
		hh = 480;
	}
	*w = ww;
	*h = hh;
}

static void
fb_sink_init(void)
{
	const char *paths[2] = { "/dev/fb0", "/dev/fb" };
	size_t bytes;
	int i;

	for (i = 0; i < 2 && fbsink.fd < 0; i++)
		fbsink.fd = open(paths[i], O_RDWR);
	if (fbsink.fd < 0) {
		printf("SHELL: 没有 framebuffer（/dev/fb0 打不开），只做抓帧自检\n");
		fflush(stdout);
		return;
	}

	fbsink.w = ioctl(fbsink.fd, YY_FB_GET_WIDTH);
	fbsink.h = ioctl(fbsink.fd, YY_FB_GET_HEIGHT);
	fbsink.bpp = ioctl(fbsink.fd, YY_FB_GET_BPP);
	if (fbsink.w <= 0 || fbsink.h <= 0 || fbsink.bpp <= 0 ||
	    (fbsink.bpp != 32 && fbsink.bpp != 16)) {
		printf("SHELL: framebuffer 参数不支持 %dx%d@%dbpp\n", fbsink.w,
		       fbsink.h, fbsink.bpp);
		fflush(stdout);
		close(fbsink.fd);
		fbsink.fd = -1;
		return;
	}

	bytes = (size_t)fbsink.w * (size_t)fbsink.h *
		(size_t)(fbsink.bpp / 8);
	fbsink.mem = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
			  fbsink.fd, 0);
	if (fbsink.mem == MAP_FAILED) {
		printf("SHELL: framebuffer mmap(%zu) 失败\n", bytes);
		fflush(stdout);
		fbsink.mem = NULL;
		close(fbsink.fd);
		fbsink.fd = -1;
		return;
	}

	printf("SHELL: fbdev 输出就绪 %dx%d@%dbpp mem=%p (%zu 字节)\n", fbsink.w,
	       fbsink.h, fbsink.bpp, (void *)fbsink.mem, bytes);
	fflush(stdout);
}

/* 把影子缓冲整帧搬到显存。每帧调用，中心对齐裁剪。 */
static void
fb_sink_blit(void)
{
	void *data = NULL;
	int stride = 0;
	int w = 0;
	int h = 0;
	int cw;
	int ch;
	int ox;
	int oy;
	int x;
	int y;

	if (fbsink.mem == NULL || shell.output == NULL)
		return;
	if (weston_pixman_output_get_shadow(shell.output, &data, &stride, &w, &h) < 0)
		return;
	if (data == NULL || w <= 0 || h <= 0 || stride <= 0)
		return;

	cw = (w < fbsink.w) ? w : fbsink.w;
	ch = (h < fbsink.h) ? h : fbsink.h;
	ox = (fbsink.w - cw) / 2;
	oy = (fbsink.h - ch) / 2;

	for (y = 0; y < ch; y++) {
		const uint32_t *src =
			(const uint32_t *)((const uint8_t *)data + (size_t)y * stride);

		if (fbsink.bpp == 32) {
			uint32_t *dst = fbsink.mem +
					(size_t)(oy + y) * fbsink.w + ox;
			/* 影子缓冲是 XRGB8888（alpha 恒 0）；有的显示通路按
			 * ARGB8888 解释 ⇒ 不补 0xff 会整屏透明成黑。 */
			for (x = 0; x < cw; x++)
				dst[x] = src[x] | 0xff000000u;
		} else {
			uint16_t *dst = (uint16_t *)fbsink.mem +
					(size_t)(oy + y) * fbsink.w + ox;
			for (x = 0; x < cw; x++) {
				uint32_t p = src[x];
				dst[x] = (uint16_t)(((p >> 8) & 0xf800) |
						    ((p >> 5) & 0x07e0) |
						    ((p >> 3) & 0x001f));
			}
		}
	}
}

static void
on_output_frame(struct wl_listener *l, void *data)
{
	(void)l;
	(void)data;
	shell_frames++;
	/* frame_signal 在 pixman 每次 repaint 末尾发出：此刻影子缓冲就是
	 * 本帧的合成结果，直接搬上屏。 */
	fb_sink_blit();
}

static void
report_frame(void)
{
	int w = 0, h = 0;
	uint32_t px = 0;

	if (weston_pixman_output_get_size(shell.output, &w, &h) < 0) {
		printf("SHELL: output 没有影子缓冲\n");
		fflush(stdout);
		return;
	}

	weston_pixman_output_read_pixel(shell.output, 32, 32, &px);
	printf("TPASS: 合成链路通 —— output %dx%d，客户端画的像素"
	       " (32,32)=0x%08x 已进影子缓冲\n", w, h, px);
	fflush(stdout);
}

static int
tick_handler(void *data)
{
	uint32_t px = 0;
	static int n;

	(void)data;

	/* wl_event_loop_add_timer 是一次性的：重新 armed 才会再来 */
	wl_event_source_timer_update(shell.tick, 50);

	/* headless 后端只在被 schedule 时才 repaint；但场景图为空时
	 * weston_output_repaint() 里有 assert，所以必须等真的有 view 再调度。 */
	if (shell.has_view)
		weston_compositor_schedule_repaint(shell.ec);

	if (!shell.reported &&
	    weston_pixman_output_read_pixel(shell.output, 32, 32, &px) == 0 &&
	    (px & 0x00ffffffu) != 0) {
		shell.reported = 1;
		report_frame();
	}

	/* 前 8 次（约 0.4s）留痕，用于区分"定时器没跑"和"跑了但帧还是黑的"；
	 * 第 20 次（约 1s）还没抓到非零像素就判失败。 */
	if (n < 8 && shell.has_view) {
		int w = 0, h = 0;

		weston_pixman_output_get_size(shell.output, &w, &h);
		printf("SHELL: tick %d size=%dx%d frames=%d px=0x%08x\n", n, w, h,
		       shell_frames, px);
		fflush(stdout);
	}

	if (n >= 20 && !shell.reported && !shell.failed) {
		shell.failed = 1;
		printf("TFAIL: 1s 内没抓到非零像素（frames=%d），合成没通\n",
		       shell_frames);
		fflush(stdout);
	}

	n++;

	return 0; /* 继续定时 */
}

/* surface 第一次 commit 之后才有 buffer/尺寸，这时 map 才有意义 */
static struct wl_listener commit_listener;

static void
on_surface_commit(struct wl_listener *listener, void *data)
{
	struct weston_surface *surface = data;

	(void)listener;

	if (!weston_surface_is_mapped(surface)) {
		weston_surface_map(surface);
		printf("SHELL: commit 后 map（%dx%d）\n", surface->width,
		       surface->height);
		fflush(stdout);
	}

	weston_compositor_schedule_repaint(shell.ec);
}

static void
on_surface_created(struct wl_listener *listener, void *data)
{
	struct weston_surface *surface = data;
	struct weston_view *view;
	struct weston_coord_global pos = { .c = { .x = 0, .y = 0 } };

	(void)listener;

	view = weston_view_create(surface);
	if (view == NULL) {
		printf("SHELL: weston_view_create 失败\n");
		fflush(stdout);
		return;
	}

	weston_view_set_position(view, pos);
	/* 关键：不插进 layer 的 view 不会被合成 */
	weston_view_move_to_layer(view, &shell.layer.view_list);
	shell.has_view = 1;

	/* 有 view 之后才把 output 标 ready：ready 会立刻触发一次 repaint，
	 * 场景图空的时候 compositor 会断言退出。 */
	weston_output_set_ready(shell.output);

	/* map 推迟到 commit（有 buffer 之后），否则 surface 尺寸为 0 */
	commit_listener.notify = on_surface_commit;
	wl_signal_add(&surface->commit_signal, &commit_listener);

	weston_compositor_schedule_repaint(shell.ec);

	printf("SHELL: surface 已建 view 并入层（等 commit 再 map）\n");
	fflush(stdout);
}

int
mini_shell_init(struct weston_compositor *ec, struct weston_output *output)
{
	struct wl_event_loop *loop;

	shell.ec = ec;
	shell.output = output;
	shell.reported = 0;

	weston_layer_init(&shell.layer, ec);
	weston_layer_set_position(&shell.layer, WESTON_LAYER_POSITION_NORMAL);

	fb_sink_init();

	shell.create_surface.notify = on_surface_created;
	wl_signal_add(&ec->create_surface_signal, &shell.create_surface);

	shell.output_frame.notify = on_output_frame;
	wl_signal_add(&output->frame_signal, &shell.output_frame);

	loop = wl_display_get_event_loop(ec->wl_display);
	shell.tick = wl_event_loop_add_timer(loop, tick_handler, NULL);
	if (shell.tick == NULL)
		return -1;

	wl_event_source_timer_update(shell.tick, 50);
	return 0;
}
