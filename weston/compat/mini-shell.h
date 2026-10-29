#ifndef YIYIYA_MINI_SHELL_H
#define YIYIYA_MINI_SHELL_H

#include <stdint.h>
#include <sys/ioctl.h>

struct weston_compositor;
struct weston_output;

/* pixman-renderer.c 里的本地补丁导出（读 output 影子缓冲） */
int weston_pixman_output_get_size(struct weston_output *output, int *w, int *h);
int weston_pixman_output_read_pixel(struct weston_output *output, int x, int y,
				    uint32_t *argb);
/* 整帧搬运用：影子缓冲的原始像素、行距、尺寸（data 只读，归 pixman 所有） */
int weston_pixman_output_get_shadow(struct weston_output *output, void **data,
				    int *stride, int *w, int *h);
/* 输出影子缓冲里单个像素（自检用） */
int weston_pixman_output_read_pixel(struct weston_output *output, int x, int y,
				    uint32_t *argb);

/* /dev/fb0 的自研 ioctl：对应 duck/modules/vga/vga.h 的
 * VGA_IOC_READ_FRAMBUFFER_{WIDTH,HEIGHT,BPP}（_IOW('v', 4/5/6, int)）。
 * 用户态库不 include 内核头，这里就地展开同样的编码。 */
#define YY_VGA_IOC_MAGIC 'v'
#define YY_FB_GET_WIDTH _IOW(YY_VGA_IOC_MAGIC, 4, int)
#define YY_FB_GET_HEIGHT _IOW(YY_VGA_IOC_MAGIC, 5, int)
#define YY_FB_GET_BPP _IOW(YY_VGA_IOC_MAGIC, 6, int)

/* 当前显示尺寸（/dev/fb0 打不开时回退 640x480）。server 拿它定 output 尺寸，
 * client 拿它定画布大小，两边一致才能 1:1 上屏。 */
void yy_fb_size(int *w, int *h);

/* 最小 shell：接管 surface → view → map，定时抓帧，并把合成结果 blit 到
 * /dev/fb0（没有 framebuffer 时只做抓帧自检）。 */
int mini_shell_init(struct weston_compositor *ec, struct weston_output *output);

#endif /* YIYIYA_MINI_SHELL_H */
