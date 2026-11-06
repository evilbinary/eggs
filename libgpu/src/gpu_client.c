/* gpu_client.c - 客户端对接 gpud 的薄接口（present 默认 direct） */
#include "gpu_client.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client.h"

/* 与 foot/driver/gpu/gpu_backend.h 保持一致 */
#define GPU_LOW_BO_CREATE 110
#define GPU_LOW_BO_DESTROY 112
#define GPU_LOW_SCREEN_PARAM 141
#define GPU_LOW_PRESENT 142
#define GPU_FMT_RGB565 1

typedef struct {
  unsigned int pa, size, flags;
} gpu_bo_create_req;

typedef struct {
  unsigned int src_handle;
  unsigned int src_offset;
  unsigned int w, h;
  unsigned int src_fmt;
  unsigned int src_stride;
  unsigned int dst_x, dst_y;
  unsigned int mode;
  unsigned int flags;
} gpu_present_frame_req;

static client_t* g_gc;

static client_t* gpu_client(void) {
  if (g_gc == NULL) {
    g_gc = client_get("gpu");
  }
  return g_gc;
}

int gpu_screen_info(gpu_screen_info_t* info) {
  if (info == NULL) return -1;
  client_t* gc = gpu_client();
  if (gc == NULL) return -1;
  long v[8];
  for (int i = 0; i < 8; i++) {
    unsigned int param = (unsigned int)i;
    v[i] = (long)client_call_ex(gc, (void*)GPU_LOW_SCREEN_PARAM, &param,
                                sizeof(param), 5000);
  }
  info->gva = (unsigned int)v[0];
  info->pa = (unsigned int)v[1];
  info->width = (unsigned int)v[2];
  info->height = (unsigned int)v[3];
  info->pitch = (unsigned int)v[4];
  info->cpp = (unsigned int)v[5];
  info->format = (unsigned int)v[6];
  info->size = (unsigned int)v[7];
  return (info->width && info->height && info->pitch) ? 0 : -1;
}

int gpu_bo_create(unsigned int pa, unsigned int size, unsigned int* handle) {
  if (handle == NULL || pa == 0 || size == 0) return -1;
  client_t* gc = gpu_client();
  if (gc == NULL) return -1;
  gpu_bo_create_req req;
  memset(&req, 0, sizeof(req));
  req.pa = pa;
  req.size = size;
  long h = (long)client_call_ex(gc, (void*)GPU_LOW_BO_CREATE, &req, sizeof(req),
                                60000);
  if (h <= 0) return -1;
  *handle = (unsigned int)h;
  return 0;
}

int gpu_bo_destroy(unsigned int handle) {
  client_t* gc = gpu_client();
  if (gc == NULL || handle == 0) return -1;
  long rc = (long)client_call_ex(gc, (void*)GPU_LOW_BO_DESTROY, &handle,
                                 sizeof(handle), 5000);
  return rc == 0 ? 0 : -1;
}

int gpu_present_bo(unsigned int handle, unsigned int w, unsigned int h,
                   unsigned int src_fmt, unsigned int src_stride,
                   unsigned int dst_x, unsigned int dst_y, int mode) {
  if (handle == 0 || w == 0 || h == 0) return -1;
  client_t* gc = gpu_client();
  if (gc == NULL) return -1;
  gpu_present_frame_req req;
  memset(&req, 0, sizeof(req));
  req.src_handle = handle;
  req.w = w;
  req.h = h;
  req.src_fmt = src_fmt;
  req.src_stride = src_stride;
  req.dst_x = dst_x;
  req.dst_y = dst_y;
  req.mode = (mode == GPU_PRESENT_TFU) ? GPU_PRESENT_TFU : GPU_PRESENT_DIRECT;
  long rc = (long)client_call_ex(gc, (void*)GPU_LOW_PRESENT, &req, sizeof(req),
                                 10000);
  return rc == 0 ? 0 : -1;
}

/* ---- 应用侧 RGBA 上屏（自带离屏 BO）---- */
static struct {
  int inited;
  int mode;
  gpu_screen_info_t si;
  int ox, oy;
  foot_cap_t cap;
  unsigned int bo_va, bo_size, bo;
} g;

static void gpu_release_bo(void) {
  if (g.bo != 0) {
    gpu_bo_destroy(g.bo);
    g.bo = 0;
  }
  if (g.cap != FOOT_CAP_NONE) {
    foot_cap_unmap(g.cap);
    foot_cap_revoke(g.cap);
    g.cap = FOOT_CAP_NONE;
  }
  g.bo_va = 0;
  g.bo_size = 0;
}

static int gpu_present_init(int mode) {
  if (g.inited && g.mode == mode) return 0;
  if (g.inited) {
    g.mode = mode;
    return 0;
  }
  memset(&g, 0, sizeof(g));
  g.cap = FOOT_CAP_NONE;
  g.mode = mode;
  if (gpu_screen_info(&g.si) != 0) return -1;
  g.inited = 1;
  return 0;
}

static int gpu_ensure_bo(unsigned int need) {
  if (g.bo != 0 && g.bo_size >= need) return 0;
  gpu_release_bo();
  unsigned int pa = 0, sz = 0;
  g.cap = foot_cap_alloc_arena(need, 0, 0, &pa, &sz);
  if (g.cap == FOOT_CAP_NONE) return -1;
  void* va = (void*)0x54000000UL;
  if (foot_cap_map(g.cap, va, 0, sz) != 0) return -1;
  g.bo_va = (unsigned int)(uintptr_t)va;
  g.bo_size = sz;
  if (gpu_bo_create(pa, sz, &g.bo) != 0) {
    printf("gpu_client: BO create failed\n");
    return -1;
  }
  return 0;
}

int gpu_present_frame_ex(const void* rgba, unsigned int w, unsigned int h,
                         gpu_present_mode_t mode) {
  if (rgba == NULL || w == 0 || h == 0) return -1;
  int m = (mode == GPU_PRESENT_TFU) ? GPU_PRESENT_TFU : GPU_PRESENT_DIRECT;
  if (gpu_present_init(m) != 0) return -1;
  if (gpu_ensure_bo(w * h * 2) != 0) return -1;

  /* RGBA8888（row 0 = GL 底部）-> 紧凑 RGB565（row 0 = 图像顶部） */
  uint16_t* dst = (uint16_t*)(uintptr_t)g.bo_va;
  for (unsigned int y = 0; y < h; y++) {
    const uint8_t* src = (const uint8_t*)rgba + (size_t)(h - 1 - y) * w * 4;
    for (unsigned int x = 0; x < w; x++) {
      uint8_t r = src[x * 4 + 0], gg = src[x * 4 + 1], b = src[x * 4 + 2];
      dst[(size_t)y * w + x] =
          (uint16_t)(((r >> 3) << 11) | ((gg >> 2) << 5) | (b >> 3));
    }
  }
  foot_cache_clean(dst, (char*)dst + (size_t)w * h * 2);

  g.ox = ((g.si.width - (int)w) / 2) & ~1;
  g.oy = (g.si.height - (int)h) / 2;
  if (g.ox < 0) g.ox = 0;
  if (g.oy < 0) g.oy = 0;

  return gpu_present_bo(g.bo, w, h, GPU_FMT_RGB565, 0, (unsigned int)g.ox,
                        (unsigned int)g.oy, m);
}

int gpu_present_frame(const void* rgba, unsigned int w, unsigned int h) {
  return gpu_present_frame_ex(rgba, w, h, GPU_PRESENT_DEFAULT);
}

void gpu_present_shutdown(void) {
  gpu_release_bo();
  g.inited = 0;
}
