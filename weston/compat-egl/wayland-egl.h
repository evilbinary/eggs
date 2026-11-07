/* wayland-egl.h 垫片。
 *
 * weston 的 shared/platform.h 在 ENABLE_EGL 下无条件 include
 * <wayland-egl.h>，但本仓库的 libwayland 不编 egl 子库（没有 GL 客户端
 * 需求），系统里也没有这个头。gl-renderer 的 surfaceless/FBO 路径并不使用
 * wl_egl_window，所以这里只给最小声明，保证编译/链接通过；将来真要用
 * wayland-egl 时再补 libwayland/egl 的构建。
 */
#ifndef YIYIYA_WAYLAND_EGL_H
#define YIYIYA_WAYLAND_EGL_H

#include <wayland-client.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wl_egl_window;

struct wl_egl_window *
wl_egl_window_create(struct wl_surface *surface, int width, int height);

void
wl_egl_window_destroy(struct wl_egl_window *egl_window);

void
wl_egl_window_resize(struct wl_egl_window *egl_window,
                     int width, int height, int dx, int dy);

void
wl_egl_window_get_attached_size(struct wl_egl_window *egl_window,
                                int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif /* YIYIYA_WAYLAND_EGL_H */
