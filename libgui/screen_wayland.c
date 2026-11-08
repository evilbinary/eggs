/* libgui 的 Wayland 上屏后端（SCREEN_MODE_WAYLAND）。
 *
 * 应用只负责往 screen_info()->buffer（ARGB8888）画；这里负责：
 *   screen_init_with_mode(WAYLAND) → 建 xdg-shell 窗口 + 双 wl_shm 缓冲
 *   screen_flush()                 → 把画布拷进 shm 并 wl_surface_commit
 *
 * 不依赖 wayland-egl / EGL；纯 wl_shm 上屏。
 */
#include "screen_wayland.h"

#ifdef ENABLE_WAYLAND

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cairo.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "shared/cairo-util.h"

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
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *toplevel;
  struct wl_buffer *buf[WL_BUF_COUNT];
  uint32_t *map[WL_BUF_COUNT];
  int idx;
  int configured;
  int inited;
  int width, height;
  int px, py;
  uint32_t stride;
  int busy[WL_BUF_COUNT];
  /* eventdemo 同款 CSD */
  struct theme *theme;
  struct frame *frame;
  cairo_surface_t *frame_surf;
  int win_w, win_h;
  int content_x, content_y;
  int maximized;
} wl;

static char wl_title[128] = "libgui";
static void frame_repaint_surface(void);

static void buf_release(void *data, struct wl_buffer *b) {
  (void)b;
  *(int *)data = 0;
}
static const struct wl_buffer_listener buf_listener = {buf_release};

void screen_wayland_set_title(const char *title) {
  if (!title || !title[0]) return;
  snprintf(wl_title, sizeof(wl_title), "%s", title);
  if (wl.toplevel) xdg_toplevel_set_title(wl.toplevel, wl_title);
  if (wl.frame) {
    frame_set_title(wl.frame, wl_title);
    frame_repaint_surface();
  }
}

static void frame_repaint_surface(void) {
  cairo_t *cr;
  if (!wl.frame || !wl.frame_surf) return;
  cr = cairo_create(wl.frame_surf);
  frame_repaint(wl.frame, cr);
  cairo_destroy(cr);
  cairo_surface_flush(wl.frame_surf);
}

static void frame_after_input(uint32_t serial, enum theme_location loc) {
  uint32_t st = frame_status(wl.frame);
  if (st & FRAME_STATUS_CLOSE) {
    exit(0);
  }
  if (st & FRAME_STATUS_MAXIMIZE) {
    wl.maximized = !wl.maximized;
    if (wl.maximized) {
      xdg_toplevel_set_maximized(wl.toplevel);
      frame_set_flag(wl.frame, FRAME_FLAG_MAXIMIZED);
    } else {
      xdg_toplevel_unset_maximized(wl.toplevel);
      frame_unset_flag(wl.frame, FRAME_FLAG_MAXIMIZED);
    }
  }
  if (st & FRAME_STATUS_MINIMIZE) {
    xdg_toplevel_set_minimized(wl.toplevel);
  }
  if ((st & FRAME_STATUS_MOVE) && wl.seat) {
    xdg_toplevel_move(wl.toplevel, wl.seat, serial);
  }
  if ((st & FRAME_STATUS_RESIZE) && wl.seat) {
    xdg_toplevel_resize(wl.toplevel, wl.seat, serial, loc);
  }
  frame_status_clear(wl.frame, st);
  if (st & FRAME_STATUS_REPAINT) {
    frame_repaint_surface();
  }
}

static void ptr_enter(void *data, struct wl_pointer *p, uint32_t serial,
                      struct wl_surface *s, wl_fixed_t x, wl_fixed_t y) {
  (void)data; (void)p; (void)serial; (void)s;
  wl.px = wl_fixed_to_int(x);
  wl.py = wl_fixed_to_int(y);
  frame_pointer_enter(wl.frame, NULL, wl.px, wl.py);
  frame_after_input(0, THEME_LOCATION_INTERIOR);
}
static void ptr_leave(void *data, struct wl_pointer *p, uint32_t serial,
                      struct wl_surface *s) {
  (void)data; (void)p; (void)serial; (void)s;
  frame_pointer_leave(wl.frame, NULL);
  frame_after_input(0, THEME_LOCATION_INTERIOR);
}
static void ptr_motion(void *data, struct wl_pointer *p, uint32_t time,
                       wl_fixed_t x, wl_fixed_t y) {
  (void)data; (void)p; (void)time;
  wl.px = wl_fixed_to_int(x);
  wl.py = wl_fixed_to_int(y);
  frame_pointer_motion(wl.frame, NULL, wl.px, wl.py);
  frame_after_input(0, THEME_LOCATION_INTERIOR);
}
static void ptr_button(void *data, struct wl_pointer *p, uint32_t serial,
                       uint32_t time, uint32_t button, uint32_t state) {
  enum wl_pointer_button_state bs =
      state ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED;
  enum theme_location loc;
  (void)data; (void)p; (void)time;
  loc = frame_pointer_button(wl.frame, NULL, button, bs);
  frame_after_input(serial, loc);
}
static void ptr_axis(void *data, struct wl_pointer *p, uint32_t time,
                     uint32_t axis, wl_fixed_t value) {
  (void)data; (void)p; (void)time; (void)axis; (void)value;
}
static const struct wl_pointer_listener pointer_listener = {
    ptr_enter, ptr_leave, ptr_motion, ptr_button, ptr_axis};

static void seat_caps(void *data, struct wl_seat *seat, uint32_t caps) {
  (void)data;
  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !wl.pointer) {
    wl.pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(wl.pointer, &pointer_listener, NULL);
  }
}
static const struct wl_seat_listener seat_listener = {seat_caps};

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
  else if (!strcmp(iface, "wl_seat")) {
    wl.seat = wl_registry_bind(r, id, &wl_seat_interface, 1);
    wl_seat_add_listener(wl.seat, &seat_listener, NULL);
  }
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

static int make_buffer(int i) {
  size_t size = (size_t)wl.stride * (size_t)wl.win_h;
  struct wl_shm_pool *pool;
  int fd = (int)syscall(SYS_memfd_create, "libgui-wl", 1);
  if (fd < 0) {
    return -1;
  }
  if (ftruncate(fd, (off_t)size) < 0) {
    close(fd);
    return -1;
  }
  wl.map[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (wl.map[i] == MAP_FAILED) {
    close(fd);
    return -1;
  }
  pool = wl_shm_create_pool(wl.shm, fd, (int)size);
  wl.buf[i] = wl_shm_pool_create_buffer(pool, 0, wl.win_w, wl.win_h,
                                        (int)wl.stride,
                                        WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);
  if (wl.buf[i]) {
    wl.busy[i] = 0;
    wl_buffer_add_listener(wl.buf[i], &buf_listener, &wl.busy[i]);
  }
  return wl.buf[i] ? 0 : -1;
}

int screen_wayland_init(int w, int h) {
  int i;
  if (wl.inited) {
    return 0;
  }
  wl.width = w;
  wl.height = h;
  wl.win_w = w;
  wl.win_h = h;
  wl.stride = (uint32_t)w * 4;
  wl.display = wl_display_connect(NULL);
  if (!wl.display) {
    /* 本系统 weston 的 socket 固定在 /lib/wayland-0；如果 shell/老内核给
     * 的还是 XDG_RUNTIME_DIR=/tmp，这里切到 /lib 再试一次。 */
    setenv("XDG_RUNTIME_DIR", "/lib", 1);
    wl.display = wl_display_connect(NULL);
  }
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

  /* eventdemo 同款 CSD（shared/frame.c + cairo） */
  wl.theme = theme_create();
  wl.frame = frame_create(wl.theme, w, h, FRAME_BUTTON_ALL, wl_title, NULL);
  if (wl.frame) {
    int iw = w, ih = h;
    /* frame_create 的宽高是整个窗口；把 interior 调成内容尺寸，
     * 窗口总尺寸自动加上标题栏/边框/阴影。 */
    frame_resize_inside(wl.frame, w, h);
    wl.win_w = frame_width(wl.frame);
    wl.win_h = frame_height(wl.frame);
    frame_interior(wl.frame, &wl.content_x, &wl.content_y, &iw, &ih);
    wl.width = iw;
    wl.height = ih;
    wl.stride = (uint32_t)wl.win_w * 4;
    wl.frame_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wl.win_w,
                                               wl.win_h);
    frame_repaint_surface();
    printf("screen wayland: frame %dx%d content %dx%d at %d,%d\n", wl.win_w,
           wl.win_h, wl.width, wl.height, wl.content_x, wl.content_y);
  }

  xdg_wm_base_add_listener(wl.wm_base, &wm_listener, NULL);
  wl.surface = wl_compositor_create_surface(wl.compositor);
  wl.xdg_surface = xdg_wm_base_get_xdg_surface(wl.wm_base, wl.surface);
  xdg_surface_add_listener(wl.xdg_surface, &xs_listener, NULL);
  wl.toplevel = xdg_surface_get_toplevel(wl.xdg_surface);
  xdg_toplevel_add_listener(wl.toplevel, &tl_listener, NULL);
  xdg_toplevel_set_title(wl.toplevel, wl_title);
  xdg_toplevel_set_app_id(wl.toplevel, "yiyiya.libgui");
  wl_surface_commit(wl.surface);
  wl_display_roundtrip(wl.display);

  for (i = 0; i < WL_BUF_COUNT; i++) {
    if (make_buffer(i) < 0) {
      printf("screen wayland: shm buffer %d failed\n", i);
      return -1;
    }
  }
  wl.inited = 1;
  printf("screen wayland: window %dx%d ready\n", wl.win_w, wl.win_h);
  return 0;
}

void screen_wayland_present(uint32_t *canvas, int w, int h) {
  uint32_t *dst;
  int x, y;
  if (!wl.inited) {
    return;
  }
  if (!wl.configured) {
    wl_display_roundtrip(wl.display);
  }
  /* 【必须读事件】否则 wl_buffer.release 收不到，busy 永远不清，画面
   * 会停在第一帧。非阻塞 poll，不拖慢应用主循环。 */
  wl_display_dispatch_pending(wl.display);
  if (wl_display_prepare_read(wl.display) == 0) {
    struct pollfd pfd;
    pfd.fd = wl_display_get_fd(wl.display);
    pfd.events = POLLIN;
    pfd.revents = 0;
    if (poll(&pfd, 1, 0) > 0) {
      wl_display_read_events(wl.display);
    } else {
      wl_display_cancel_read(wl.display);
    }
    wl_display_dispatch_pending(wl.display);
  }
  if (!wl.configured || w != wl.width || h != wl.height) {
    return;
  }
  /* 【双缓冲】合成器还在读的 buffer 不能覆盖，否则会闪/撕裂。 */
  {
    int pick = -1;
    if (!wl.busy[wl.idx]) {
      pick = wl.idx;
    } else if (!wl.busy[wl.idx ^ 1]) {
      pick = wl.idx ^ 1;
    }
    if (pick < 0) {
      return;
    }
    wl.busy[pick] = 1;
    dst = wl.map[pick];

    /* 1) frame（阴影/标题栏/按钮）整块拷进去 */
    if (wl.frame_surf) {
      const uint32_t *fs =
          (const uint32_t *)cairo_image_surface_get_data(wl.frame_surf);
      int fstride = cairo_image_surface_get_stride(wl.frame_surf) / 4;
      for (y = 0; y < wl.win_h; y++) {
        memcpy(dst + (size_t)y * wl.win_w, fs + (size_t)y * fstride,
               (size_t)wl.win_w * 4);
      }
    }

    /* 2) 应用画布覆盖到 interior（保留 alpha，支持半透明窗口） */
    for (y = 0; y < h; y++) {
      uint32_t *row = dst + (size_t)(wl.content_y + y) * wl.win_w +
                      wl.content_x;
      for (x = 0; x < w; x++) {
        row[x] = canvas[(size_t)y * w + x];
      }
    }

    wl_surface_attach(wl.surface, wl.buf[pick], 0, 0);
    wl_surface_damage(wl.surface, 0, 0, wl.win_w, wl.win_h);
    wl_surface_commit(wl.surface);
    wl_display_flush(wl.display);
    wl.idx = pick ^ 1;
  }
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
/* 没开 --wayland 时给空函数，保证 libgui 在其它平台也能正常链接。 */
void screen_wayland_set_title(const char *title) { (void)title; }
void screen_wayland_shutdown(void) {}

#endif /* ENABLE_WAYLAND */
