/* v3d_drm.c - v3d DRM UAPI → gpud GPU job 的薄 adapter
 *
 * 这里只做 v3d 专有的 ioctl 翻译；GEM/BO/屏幕/CPU VA 全部走 drm.c。
 * libdrm core 不认识 v3d。Mesa 仍然照常调用 drmIoctl(DRM_IOCTL_V3D_*)，
 * 由注册的 v3d_drm_ops 接住。
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>

#include "xf86drm.h"

#ifndef DRM_IOWR
#define DRM_IOR(nr, type) _IOR('d', nr, type)
#define DRM_IOW(nr, type) _IOW('d', nr, type)
#define DRM_IOWR(nr, type) _IOWR('d', nr, type)
#endif
#include "drm/v3d_drm.h"
#include "drm_driver.h"
#include "drm_driver.h"

/* gpud 的 v3d job 描述（不是 DRM UAPI）。 */
typedef struct {
  unsigned int bcl_handle, bcl_start, bcl_end;
  unsigned int rcl_handle, rcl_start, rcl_end;
  unsigned int qma, qms, qts;
  unsigned int w, h;
  unsigned int flags;
} v3d_cl_req;

#define V3D_SUBMIT_CL_ABS_VA (1u << 0)

typedef struct {
  unsigned int src_handle, src_offset;
  unsigned int dst_handle, dst_offset;
  unsigned int w, h;
  unsigned int src_fmt, dst_fmt;
  unsigned int flags;
} v3d_tfu_req;

typedef struct {
  unsigned int pa;
  unsigned int size;
  unsigned int flags;
} v3d_bo_req;

extern long drm_gpu_submit_cl(void* req, unsigned int size);
extern long drm_gpu_submit_tfu(void* req, unsigned int size);
extern long drm_gpu_bo_create(void* req, unsigned int size);

static int v3d_ioctl_get_param(struct drm_v3d_get_param* p) {
  /* Mesa v3d_get_device_info() 读 CORE0_IDENT0/1 推 ver/nslc/qups，
   * 读 HUB_IDENT3 推 rev/compat_rev。按真机 V3D 7.1 回答。 */
  switch (p->param) {
    case DRM_V3D_PARAM_V3D_CORE0_IDENT0:
      p->value = 0x07000000u;
      return 0;
    case DRM_V3D_PARAM_V3D_CORE0_IDENT1:
      p->value = (2u << 28) | (4u << 8) | (3u << 4) | 1u;
      return 0;
    case DRM_V3D_PARAM_V3D_HUB_IDENT1:
      p->value = (1u << 8) | 1u;
      return 0;
    case DRM_V3D_PARAM_V3D_HUB_IDENT3:
      p->value = 10u << 8;
      return 0;
    case DRM_V3D_PARAM_SUPPORTS_TFU:
    case DRM_V3D_PARAM_SUPPORTS_CSD:
    case DRM_V3D_PARAM_SUPPORTS_CACHE_FLUSH:
      p->value = 1;
      return 0;
    default:
      p->value = 0;
      return 0;
  }
}

static int v3d_ioctl_create_bo(struct drm_v3d_create_bo* c) {
  v3d_bo_req req;
  memset(&req, 0, sizeof(req));

  extern int bridge_bo_alloc(unsigned int size, unsigned int* pa,
                             unsigned int* cpu_va);
  unsigned int alloc_size = (c->size + 0xFFFu) & ~0xFFFu;
  unsigned int pa = 0, cpu_va = 0;
  if (alloc_size == 0u ||
      bridge_bo_alloc(alloc_size, &pa, &cpu_va) != 0) {
    return -ENOMEM;
  }
  req.pa = pa;
  req.size = alloc_size;
  long h = drm_gpu_bo_create(&req, sizeof(req));
  if (h <= 0) {
    return -ENOMEM;
  }
  unsigned int gva = 0;
  if (drm_gem_bo_offset((unsigned int)h, &gva) != 0) {
    return -ENOMEM;
  }
  c->handle = (uint32_t)h;
  c->offset = gva;
  c->size = alloc_size;
  extern unsigned int drm_bo_take_last_cap(void);
  drm_bo_map_record((unsigned int)h, cpu_va, alloc_size, gva,
                    drm_bo_take_last_cap());
  return 0;
}

static int v3d_ioctl_submit_cl(struct drm_v3d_submit_cl* s) {
  v3d_cl_req req;
  memset(&req, 0, sizeof(req));
  req.bcl_start = s->bcl_start;
  req.bcl_end = s->bcl_end;
  req.rcl_start = s->rcl_start;
  req.rcl_end = s->rcl_end;
  req.qma = s->qma;
  req.qms = s->qms;
  req.qts = s->qts;
  req.flags = V3D_SUBMIT_CL_ABS_VA;
  drm_bo_flush_all();
  long rc = drm_gpu_submit_cl(&req, sizeof(req));
  return rc == 0 ? 0 : -EINVAL;
}

static int v3d_ioctl_submit_tfu(struct drm_v3d_submit_tfu* t) {
  v3d_tfu_req req;
  memset(&req, 0, sizeof(req));
  req.src_handle = t->bo_handles[1];
  req.dst_handle = t->bo_handles[0];
  req.src_fmt = t->iia;
  req.dst_fmt = t->icfg;
  long rc = drm_gpu_submit_tfu(&req, sizeof(req));
  return rc == 0 ? 0 : -EINVAL;
}

static int v3d_drm_ioctl(unsigned long nr, void* arg) {
  int rc = drm_gem_ioctl(nr, arg);
  if (rc != DRM_GEM_NOT_HANDLED) {
    return rc;
  }

  switch (nr) {
    case DRM_IOCTL_V3D_GET_PARAM:
      return v3d_ioctl_get_param((struct drm_v3d_get_param*)arg);
    case DRM_IOCTL_V3D_CREATE_BO:
      return v3d_ioctl_create_bo((struct drm_v3d_create_bo*)arg);
    case DRM_IOCTL_V3D_SUBMIT_CL:
      return v3d_ioctl_submit_cl((struct drm_v3d_submit_cl*)arg);
    case DRM_IOCTL_V3D_SUBMIT_TFU:
      return v3d_ioctl_submit_tfu((struct drm_v3d_submit_tfu*)arg);
    default:
      printf("libgpu v3d: unhandled ioctl 0x%lx\n", nr);
      return -EINVAL;
  }
}

const drm_driver_ops_t v3d_drm_ops = {
    .name = "v3d",
    .ioctl = v3d_drm_ioctl,
};
