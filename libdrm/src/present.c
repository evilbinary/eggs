/* present.c - 默认 direct 的 GPU 上屏（客户端薄壳，策略在 gpud）
 *
 * 客户端只负责把 RGBA 转成 RGB565 放进一个共享 BO，然后调用 gpud 的
 * GPU_LOW_PRESENT；屏幕参数查询、实际扫描/贴屏、direct/tfu 选择都在 gpud
 * 的 v3d_present_frame() 里。direct 默认 = 同格式 TFU 直接 DMA 到屏幕，
 * 不经过离屏转换 BO。
 */
#include "foot_present.h"

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
  unsigned int dst_x, dst_y;
  unsigned int mode;
  unsigned int flags;
} gpu_present_frame_req;

static struct {
  int inited;
  int mode;
  int sw, sh, pitch, cpp, size;
  int ox, oy;
  foot_cap_t cap;
  unsigned int bo_va, bo_size, bo;
  client_t* gc;
} g;

static int fp_init(int mode) {
  if (g.inited && g.mode == mode) return 0;
  if (g.inited) {
    /* 模式变化不需要重建屏幕/BO，只更新策略 */
    g.mode = mode;
    return 0;
  }
  memset(&g, 0, sizeof(g));
  g.cap = FOOT_CAP_NONE;
  g.mode = mode;

  g.gc = client_get("gpu");
  if (g.gc == NULL) return -1;
  long v[8];
  for (int i = 0; i < 8; i++) {
    unsigned int param = (unsigned int)i;
    v[i] = (long)client_call_ex(g.gc, (void*)GPU_LOW_SCREEN_PARAM, &param,
                                sizeof(param), 5000);
  }
  g.sw = (int)v[2];
  g.sh = (int)v[3];
  g.pitch = (int)v[4];
  g.cpp = (int)v[5];
  g.size = (int)v[7];
  if (g.sw <= 0 || g.sh <= 0 || g.pitch <= 0 || g.cpp <= 0 || g.size <= 0)
    return -1;
  g.inited = 1;
  return 0;
}

static int fp_ensure_bo(unsigned int need) {
  if (g.bo != 0 && g.bo_size >= need) return 0;
  if (g.bo != 0) {
    unsigned int handle = g.bo;
    client_call_ex(g.gc, (void*)GPU_LOW_BO_DESTROY, &handle, sizeof(handle),
                   5000);
    g.bo = 0;
  }
  if (g.cap != FOOT_CAP_NONE) {
    foot_cap_unmap(g.cap);
    foot_cap_revoke(g.cap);
    g.cap = FOOT_CAP_NONE;
  }
  unsigned int pa = 0, sz = 0;
  g.cap = foot_cap_alloc_arena(need, 0, 0, &pa, &sz);
  if (g.cap == FOOT_CAP_NONE) return -1;
  void* va = (void*)0x54000000UL;
  if (foot_cap_map(g.cap, va, 0, sz) != 0) return -1;
  g.bo_va = (unsigned int)(uintptr_t)va;
  g.bo_size = sz;
  gpu_bo_create_req req;
  memset(&req, 0, sizeof(req));
  req.pa = pa;
  req.size = sz;
  long handle = (long)client_call_ex(g.gc, (void*)GPU_LOW_BO_CREATE, &req,
                                     sizeof(req), 60000);
  if (handle <= 0) {
    printf("foot_present: BO create failed (%ld)\n", handle);
    return -1;
  }
  g.bo = (unsigned int)handle;
  return 0;
}

int foot_present_frame_ex(const void* rgba, unsigned int w, unsigned int h,
                          foot_present_mode_t mode) {
  if (rgba == NULL || w == 0 || h == 0) return -1;
  int m = (mode == FOOT_PRESENT_TFU) ? FOOT_PRESENT_TFU : FOOT_PRESENT_DIRECT;
  if (fp_init(m) != 0) return -1;
  if (fp_ensure_bo(w * h * 2) != 0) return -1;

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

  g.ox = ((g.sw - (int)w) / 2) & ~1;
  g.oy = (g.sh - (int)h) / 2;
  if (g.ox < 0) g.ox = 0;
  if (g.oy < 0) g.oy = 0;

  gpu_present_frame_req req;
  memset(&req, 0, sizeof(req));
  req.src_handle = g.bo;
  req.w = w;
  req.h = h;
  req.src_fmt = GPU_FMT_RGB565;
  req.dst_x = (unsigned int)g.ox;
  req.dst_y = (unsigned int)g.oy;
  req.mode = (unsigned int)m;
  long rc = (long)client_call_ex(g.gc, (void*)GPU_LOW_PRESENT, &req,
                                 sizeof(req), 10000);
  return rc == 0 ? 0 : -1;
}

int foot_present_frame(const void* rgba, unsigned int w, unsigned int h) {
  return foot_present_frame_ex(rgba, w, h, FOOT_PRESENT_DEFAULT);
}

void foot_present_shutdown(void) {
  if (g.bo != 0 && g.gc != NULL) {
    unsigned int handle = g.bo;
    client_call_ex(g.gc, (void*)GPU_LOW_BO_DESTROY, &handle, sizeof(handle),
                   5000);
    g.bo = 0;
  }
  if (g.cap != FOOT_CAP_NONE) {
    foot_cap_unmap(g.cap);
    foot_cap_revoke(g.cap);
    g.cap = FOOT_CAP_NONE;
  }
  g.inited = 0;
}
