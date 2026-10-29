/* headless 后端只把合成结果画进 pixman 的影子缓冲，不碰真实显存。
 * 这里把整帧搬到 /dev/fb0（内核 gpu 模块注册的 framebuffer，mmap 直接拿显存），
 * 屏幕上就能看到 wayland 画面。尺寸不一致时居中放置。 */
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../libweston/libweston-internal.h"
#include "compat-fbdev.h"

static struct {
	int fd;
	uint32_t *mem;
	int w;
	int h;
	int bpp;
	struct wl_listener frame;
	int attached;
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

/* 把影子缓冲整帧搬到显存。每帧调用，中心对齐裁剪。 */
static void
fb_blit(struct weston_output *output)
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

	if (fbsink.mem == NULL || output == NULL)
		return;
	if (weston_pixman_output_get_shadow(output, &data, &stride, &w, &h) < 0)
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
	struct weston_output *output;

	(void)l;
	output = data;
	/* frame_signal 在 pixman 每次 repaint 末尾发出：此刻影子缓冲就是
	 * 本帧的合成结果，直接搬上屏。 */
	fb_blit(output);
}

int
yy_fbdev_init(struct weston_output *output)
{
	const char *paths[2] = { "/dev/fb0", "/dev/fb" };
	size_t bytes;
	int i;

	if (fbsink.fd >= 0) {
		/* 已经初始化过（多 output 时只挂第一个） */
		return 0;
	}

	for (i = 0; i < 2 && fbsink.fd < 0; i++)
		fbsink.fd = open(paths[i], O_RDWR);
	if (fbsink.fd < 0) {
		printf("FB: 没有 framebuffer（/dev/fb0 打不开），只做离屏合成\n");
		fflush(stdout);
		return -1;
	}

	fbsink.w = ioctl(fbsink.fd, YY_FB_GET_WIDTH);
	fbsink.h = ioctl(fbsink.fd, YY_FB_GET_HEIGHT);
	fbsink.bpp = ioctl(fbsink.fd, YY_FB_GET_BPP);
	if (fbsink.w <= 0 || fbsink.h <= 0 || fbsink.bpp <= 0 ||
	    (fbsink.bpp != 32 && fbsink.bpp != 16)) {
		printf("FB: 参数不支持 %dx%d@%dbpp\n", fbsink.w, fbsink.h,
		       fbsink.bpp);
		fflush(stdout);
		close(fbsink.fd);
		fbsink.fd = -1;
		return -1;
	}

	bytes = (size_t)fbsink.w * (size_t)fbsink.h * (size_t)(fbsink.bpp / 8);
	fbsink.mem = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
			  fbsink.fd, 0);
	if (fbsink.mem == MAP_FAILED) {
		printf("FB: mmap(%zu) 失败\n", bytes);
		fflush(stdout);
		fbsink.mem = NULL;
		close(fbsink.fd);
		fbsink.fd = -1;
		return -1;
	}

	if (output != NULL && !fbsink.attached) {
		fbsink.frame.notify = on_output_frame;
		wl_signal_add(&output->frame_signal, &fbsink.frame);
		fbsink.attached = 1;
	}

	printf("FB: fbdev 输出就绪 %dx%d@%dbpp mem=%p\n", fbsink.w, fbsink.h,
	       fbsink.bpp, (void *)fbsink.mem);
	fflush(stdout);
	return 0;
}
