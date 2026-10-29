/* headless 后端 → 真实显存：把 pixman 影子缓冲整帧搬到 /dev/fb0。
 * 任何 shell（mini/kiosk/ivi/desktop）都可以挂一次。 */
#ifndef YIYIYA_COMPAT_FBDEV_H
#define YIYIYA_COMPAT_FBDEV_H

#include <stdint.h>
#include <sys/ioctl.h>

struct weston_output;

/* /dev/fb0 的自研 ioctl：对应 duck/modules/vga/vga.h 的
 * VGA_IOC_READ_FRAMBUFFER_{WIDTH,HEIGHT,BPP}（_IOW('v', 4/5/6, int)）。
 * 用户态库不 include 内核头，这里就地展开同样的编码。 */
#define YY_VGA_IOC_MAGIC 'v'
#define YY_FB_GET_WIDTH _IOW(YY_VGA_IOC_MAGIC, 4, int)
#define YY_FB_GET_HEIGHT _IOW(YY_VGA_IOC_MAGIC, 5, int)
#define YY_FB_GET_BPP _IOW(YY_VGA_IOC_MAGIC, 6, int)

/* 当前显示尺寸（/dev/fb0 打不开时回退 640x480） */
void yy_fb_size(int *w, int *h);

/* 打开 /dev/fb0 并挂 output 的 frame_signal：每帧合成完就 blit 上屏。
 * 返回 0 成功；没有 framebuffer 时返回 -1（只做离屏合成）。 */
int yy_fbdev_init(struct weston_output *output);

#endif /* YIYIYA_COMPAT_FBDEV_H */
