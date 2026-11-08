/* libgui 的 Wayland 上屏后端（SCREEN_MODE_WAYLAND）。
 *
 * 应用只负责往 screen_info()->buffer（ARGB8888）画；这里负责：
 *   screen_init_with_mode(WAYLAND) → 建 xdg-shell 窗口 + 双 wl_shm 缓冲
 *   screen_flush()                 → 把画布拷进 shm 并 wl_surface_commit
 *
 * 不依赖 wayland-egl / EGL；纯 wl_shm 上屏。
 */
#ifdef ENABLE_WAYLAND

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include "screen.h"

#ifndef SYS_memfd_create
#define SYS_memfd_create 279
#endif

#define WL_BUF_COUNT 2

static struct {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct xdg_wm_base *wm_base;
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *toplevel;
  struct wl_buffer *buf[WL_BUF_COUNT];
  uint32_t *map[WL_BUF_COUNT];
  int idx;
  int configured;
  int inited;
  int width, height;
  uint32_t stride;
} wl;

static void reg_global(void *data, struct wl_registry *r, uint32_t id,
                       const char *iface, uint32_t ver) {
  (void)data;
  (void)ver;
  if (!strcmp(iface, "wl_compositor"))
    wl.compositor = wl_registry_bind(r, id, &wl_compositor_interface, 1);
  else if (!strcmp(iface, "wl_shm"))
    wl.shm = wl_registry_bind(r, id, &wl_shm_interface, 1);
  else if (!strcmp(iface, "xdg_wm_base"))
    wl.wm_base = wl_registry_bind(r, id, &xdg_wm_base_interface, 1);
}
static void reg_remove(void *data, struct wl_registry *r, uint32_t id) {
  (void)data;
  (void)r;
  (void)id;
}
static const struct wl_registry_listener reg_listener = {reg_global,
                                                         reg_remove};

static void wm_ping(void *data, struct xdg_wm_base *wm, uint32_t serial) {
  (void)data;
  xdg_wm_base_pong(wm, serial);
}
static const struct xdg_wm_base_listener wm_listener = {wm_ping};

static void xs_configure(void *data, struct xdg_surface *s, uint32_t serial) {
  (void)data;
  xdg_surface_ack_configure(s, serial);
  wl.configured = 1;
}
static const struct xdg_surface_listener xs_listener = {xs_configure};

static void tl_configure(void *data, struct xdg_toplevel *t, int32_t w,
                         int32_t h, struct wl_array *st) {
  (void)data;
  (void)t;
  (void)w;
  (void)h;
  (void)st;
}
static void tl_close(void *data, struct xdg_toplevel *t) {
  (void)data;
  (void)t;
  /* 应用没有统一的退出钩子；保持画布继续画，窗口由合成器销毁。 */
}
static const struct xdg_toplevel_listener tl_listener = {tl_configure,
                                                         tl_close};

static int make_buffer(int i, int w, int h) {
  size_t size = (size_t)wl.stride * (size_t)h;
  struct wl_shm_pool *pool;
  int fd = (int)syscall(SYS_memfd_create, "libgui-wl", 1);
  if (fd < 0) {
    return -1;
  }
  if (ftruncate(fd, (off_t)size) < 0) {
    close(fd);
    return -1;
  }
  wl.map[i] =
      mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (wl.map[i] == MAP_FAILED) {
    close(fd);
    return -1;
  }
  pool = wl_shm_create_pool(wl.shm, fd, (int)size);
  wl.buf[i] = wl_shm_pool_create_buffer(pool, 0, w, h, (int)wl.stride,
                                        WL_SHM_FORMAT_XRGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);
  return wl.buf[i] ? 0 : -1;
}

int screen_wayland_init(int w, int h) {
  int i;
  if (wl.inited) {
    return 0;
  }
  wl.width = w;
  wl.height = h;
  wl.stride = (uint32_t)w * 4;
  wl.display = wl_display_connect(NULL);
  if (!wl.display) {
    printf("screen wayland: wl_display_connect failed (XDG_RUNTIME_DIR=%s)\n",
           getenv("XDG_RUNTIME_DIR") ? getenv("XDG_RUNTIME_DIR") : "?");
    return -1;
  }
  wl.registry = wl_display_get_registry(wl.display);
  wl_registry_add_listener(wl.registry, &reg_listener, NULL);
  wl_display_roundtrip(wl.display);
  if (!wl.compositor || !wl.shm || !wl.wm_base) {
    printf("screen wayland: missing globals (compositor=%p shm=%p wm=%p)\n",
           (void *)wl.compositor, (void *)wl.shm, (void *)wl.wm_base);
    return -1;
  }
  xdg_wm_base_add_listener(wl.wm_base, &wm_listener, NULL);
  wl.surface = wl_compositor_create_surface(wl.compositor);
  wl.xdg_surface = xdg_wm_base_get_xdg_surface(wl.wm_base, wl.surface);
  xdg_surface_add_listener(wl.xdg_surface, &xs_listener, NULL);
  wl.toplevel = xdg_surface_get_toplevel(wl.xdg_surface);
  xdg_toplevel_add_listener(wl.toplevel, &tl_listener, NULL);
  xdg_toplevel_set_title(wl.toplevel, "libgui");
  xdg_toplevel_set_app_id(wl.toplevel, "yiyiya.libgui");
  wl_surface_commit(wl.surface);
  wl_display_roundtrip(wl.display);

  for (i = 0; i < WL_BUF_COUNT; i++) {
    if (make_buffer(i, w, h) < 0) {
      printf("screen wayland: shm buffer %d failed\n", i);
      return -1;
    }
  }
  wl.inited = 1;
  printf("screen wayland: window %dx%d ready\n", w, h);
  return 0;
}

void screen_wayland_present(uint32_t *canvas, int w, int h) {
  uint32_t *dst;
  int i, n;
  if (!wl.inited) {
    return;
  }
  /* 应用的主循环不一定 dispatch Wayland；这里自己收 configure/close。 */
  if (!wl.configured) {
    wl_display_roundtrip(wl.display);
  }
  wl_display_dispatch_pending(wl.display);
  if (!wl.configured || w != wl.width || h != wl.height) {
    return;
  }
  dst = wl.map[wl.idx];
  n = w * h;
  for (i = 0; i < n; i++) {
    dst[i] = canvas[i] | 0xff000000u;
  }
  wl_surface_attach(wl.surface, wl.buf[wl.idx], 0, 0);
  wl_surface_damage(wl.surface, 0, 0, w, h);
  wl_surface_commit(wl.surface);
  wl_display_flush(wl.display);
  wl.idx ^= 1;
}

void screen_wayland_shutdown(void) {
  if (!wl.inited) {
    return;
  }
  if (wl.toplevel) xdg_toplevel_destroy(wl.toplevel);
  if (wl.xdg_surface) xdg_surface_destroy(wl.xdg_surface);
  if (wl.surface) wl_surface_destroy(wl.surface);
  if (wl.display) wl_display_disconnect(wl.display);
  memset(&wl, 0, sizeof(wl));
}

#else

int screen_wayland_init(int w, int h) {
  (void)w;
  (void)h;
  return -1;
}
void screen_wayland_present(uint32_t *canvas, int w, int h) {
  (void)canvas;
  (void)w;
  (void)h;
}
void screen_wayland_shutdown(void) {}

#endif /* ENABLE_WAYLAND */
