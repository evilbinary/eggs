/*
 * YiYiYa 本地补丁：fbdev 后端（直接渲染到内核 /dev/fb0）。
 *
 * 上游 weston 只有 drm/headless/x11/wayland/pipewire/rdp/vnc 这几个后端，
 * fbdev 后端在 2019 年前后就被删掉了。我们的平台没有 DRM/KMS，只有
 * 自定义 fb（/dev/fb0，标准 fbdev ioctl），所以这里补一个最小后端：
 * pixman 直接把画面渲染进 /dev/fb0 的 mmap 内存（零拷贝，无影子缓冲）。
 */
#ifndef YIYIYA_BACKEND_FBDEV_H
#define YIYIYA_BACKEND_FBDEV_H

#include <libweston/libweston.h>

#define WESTON_FBDEV_BACKEND_CONFIG_VERSION 1

struct weston_fbdev_backend_config {
	struct weston_backend_config base;

	/* 刷新率（mHz），0 用默认 60Hz */
	int refresh;

	/* 设备节点，NULL 用 /dev/fb0 */
	char *device_path;

	/* 期望尺寸，0 表示用 framebuffer 实际几何 */
	int width;
	int height;
};

#endif /* YIYIYA_BACKEND_FBDEV_H */
