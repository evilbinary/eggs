#ifndef YIYIYA_MINI_SHELL_H
#define YIYIYA_MINI_SHELL_H

#include <stdint.h>

struct weston_compositor;
struct weston_output;

/* pixman-renderer.c 里的本地补丁导出（读 output 影子缓冲） */
int weston_pixman_output_get_size(struct weston_output *output, int *w, int *h);
int weston_pixman_output_read_pixel(struct weston_output *output, int x, int y,
				    uint32_t *argb);

/* 最小 shell：接管 surface → view → map，并定时抓帧 */
int mini_shell_init(struct weston_compositor *ec, struct weston_output *output);

#endif /* YIYIYA_MINI_SHELL_H */
