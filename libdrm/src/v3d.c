/* v3d.c - libdrm 里 v3d 驱动特定的 ioctl 实现
 *
 * 字段对齐内核 uapi（include/drm/v3d_drm.h）：
 *   bcl/rcl 的 start/end 是【绝对 GPU VA】（Mesa 自己用
 *   bo->offset + 偏移拼好），直接透传给 gpud 的 SUBMIT_CL。
 */
#include <string.h>
#include <errno.h>

#include "xf86drm.h"
#include "drm/v3d_drm.h"

/* gpud 的 SUBMIT_CL 请求（与 gpu_backend.h 字节一致，GPU VA 版） */
typedef struct {
  unsigned int bcl_start, bcl_end;
  unsigned int rcl_start, rcl_end;
  unsigned int w, h;
} v3d_cl_req;

typedef struct {
  unsigned int src_handle, src_offset;
  unsigned int dst_handle, dst_offset;
  unsigned int w, h;
  unsigned int src_fmt, dst_fmt;
  unsigned int flags;
} v3d_tfu_req;

typedef struct {
  unsigned int src_handle, src_offset;
  unsigned int dst_handle, dst_offset;
  unsigned int sx, sy;
  unsigned int dx, dy;
  unsigned int w, h;
  unsigned int flags;
} v3d_blit_req;

typedef struct {
  unsigned int src;
  unsigned int sx, sy;
  unsigned int dx, dy;
  unsigned int w, h;
  unsigned int src_fmt;
  unsigned int flags;
} v3d_present_req;

typedef struct {
  unsigned int pa;
  unsigned int size;
  unsigned int flags;
} v3d_bo_req;

/* 由 drm.c 提供的服务调用封装 */
extern long drm_gpu_submit_cl(void* req, unsigned int size);
extern long drm_gpu_submit_tfu(void* req, unsigned int size);
extern long drm_gpu_bo_create(void* req, unsigned int size);
extern long drm_gpu_bo_destroy(void* req, unsigned int size);
long drm_gpu_bo_offset(unsigned int handle);

int v3d_drm_ioctl(unsigned long request, void* arg) {
  switch (request) {
    case DRM_IOCTL_GEM_CLOSE: {
      struct drm_gem_close* c = (struct drm_gem_close*)arg;
      /* 本地的 BO 引用结构（与 gpud 的 gpu_bo_ref 字节一致） */
      struct { unsigned int handle; } ref;
      ref.handle = c->handle;
      return drm_gpu_bo_destroy(&ref, sizeof(ref)) == 0 ? 0 : -EINVAL;
    }

    case DRM_IOCTL_V3D_GET_PARAM: {
      struct drm_v3d_get_param* p = (struct drm_v3d_get_param*)arg;
      /* 靠 enum drm_v3d_param 的序号回答：v3d 7.1 = 1 核 4 slice，
       * 支持 TFU/CSD/cache-flush，不支持 perfmon/super-pages。 */
      switch (p->param) {
        case DRM_V3D_PARAM_V3D_HUB_IDENT1:
          p->value = 4; /* nslices */
          return 0;
        case DRM_V3D_PARAM_SUPPORTS_TFU:
        case DRM_V3D_PARAM_SUPPORTS_CSD:
        case DRM_V3D_PARAM_SUPPORTS_CACHE_FLUSH:
          p->value = 1;
          return 0;
        case DRM_V3D_PARAM_SUPPORTS_PERFMON:
        case DRM_V3D_PARAM_SUPPORTS_MULTISYNC_EXT:
        case DRM_V3D_PARAM_SUPPORTS_SUPER_PAGES:
          p->value = 0;
          return 0;
        default:
          p->value = 0;
          return 0;
      }
    }

    case DRM_IOCTL_V3D_CREATE_BO: {
      struct drm_v3d_create_bo* c = (struct drm_v3d_create_bo*)arg;
      v3d_bo_req req;
      memset(&req, 0, sizeof(req));
      /* 【BO 内存由桥分配】应用侧 foot_cap_alloc_arena 拿物理连续页
       * （CPU 可直接访问），pa 交给 gpud 导入映射 GPU VA。 */
      extern int bridge_bo_alloc(unsigned int size, unsigned int* pa,
                                 unsigned int* cpu_va);
      unsigned int pa = 0, cpu_va = 0;
      if (bridge_bo_alloc(c->size, &pa, &cpu_va) != 0) {
        return -ENOMEM;
      }
      req.pa = pa;
      req.size = c->size;
      long h = drm_gpu_bo_create(&req, sizeof(req));
      if (h <= 0) {
        return -ENOMEM;
      }
      c->handle = (uint32_t)h;
      c->offset = cpu_va; /* Mesa 的 map 路径下一轮细化 */
      return 0;
    }

    case DRM_IOCTL_V3D_GET_BO_OFFSET: {
      struct drm_v3d_get_bo_offset* g = (struct drm_v3d_get_bo_offset*)arg;
      long off = drm_gpu_bo_offset(g->handle);
      if (off < 0) {
        return -EINVAL;
      }
      g->offset = (uint32_t)off;
      return 0;
    }

    case DRM_IOCTL_V3D_SUBMIT_CL: {
      struct drm_v3d_submit_cl* s = (struct drm_v3d_submit_cl*)arg;
      v3d_cl_req req;
      req.bcl_start = s->bcl_start;
      req.bcl_end = s->bcl_end;
      req.rcl_start = s->rcl_start;
      req.rcl_end = s->rcl_end;
      /* w/h 不在 uapi 里（Mesa 由 CL 内的 TILE_BINNING_MODE_CFG 决定），
       * gpud 侧 tile 配置按 CL 自身为准——传 0 表示由后端忽略。 */
      req.w = 0;
      req.h = 0;
      long rc = drm_gpu_submit_cl(&req, sizeof(req));
      return rc == 0 ? 0 : -EINVAL;
    }

    case DRM_IOCTL_V3D_SUBMIT_TFU: {
      /* uapi 的 TFU 是寄存器级字段（icfg/iia/iis/…），
       * bo_handles[0]=输出、[1..]=输入。gpud 的 SUBMIT_TFU 期望
       * handle+offset 形式——icfg 里已含格式与尺寸信息，
       * 下一轮再细化映射；现在按「输入 bo[1] → 输出 bo[0]」直传。 */
      struct drm_v3d_submit_tfu* t = (struct drm_v3d_submit_tfu*)arg;
      v3d_tfu_req req;
      memset(&req, 0, sizeof(req));
      req.src_handle = t->bo_handles[1];
      req.dst_handle = t->bo_handles[0];
      req.src_fmt = t->iia;   /* 供 gpud 参考（暂存输入地址配置） */
      req.dst_fmt = t->icfg;
      long rc = drm_gpu_submit_tfu(&req, sizeof(req));
      return rc == 0 ? 0 : -EINVAL;
    }

    default:
      printf("libdrm v3d: unhandled ioctl 0x%lx\r\n", request);
      return -EINVAL;
  }
}
