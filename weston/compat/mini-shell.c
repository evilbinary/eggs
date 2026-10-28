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

#include <stdio.h>
#include <stdlib.h>

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

static void
on_output_frame(struct wl_listener *l, void *data)
{
	(void)l;
	(void)data;
	shell_frames++;
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
