/* drm.c - libdrm 替身：把 DRM ioctl 转发到 gpud 服务
 *
 * 分层：本文件是【通用框架】——driver-specific 的 ioctl 编号
 * 按驱动分发（v3d 在 v3d.c，将来 mali 在 mali.c），这里只做
 * 公共部分：版本探测、设备枚举、GEM 公共 ioctl。
 */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "xf86drm.h"
#include "client.h"

/* gpud 服务的方法号（与 foot/driver/gpu/gpu_backend.h 一致） */
#define G_GPU_LOW_BO_CREATE 110
#define G_GPU_LOW_BO_DESTROY 112
#define G_GPU_LOW_BO_OFFSET 101
#define G_GPU_LOW_SUBMIT_CL 120
#define G_GPU_LOW_SUBMIT_CSD 121
#define G_GPU_LOW_SUBMIT_TFU 122
#define G_GPU_LOW_WAIT_BO 130

/* 请求结构（与 gpud 的 gpu_backend.h 字节一致；不在 libfoot 里，
 * 这里本地定义一份最小版） */
typedef struct {
  unsigned int handle;
} drm_bo_ref;

static client_t* g_client;

static client_t* gpu_client(void) {
  if (g_client == NULL) {
    g_client = client_get("gpu");
  }
  return g_client;
}

static long gpu_call(unsigned int fn, void* args, unsigned int size) {
  client_t* c = gpu_client();
  if (c == NULL) {
    return -1;
  }
  return client_call_ex(c, (void*)(uintptr_t)fn, args, size, 5000);
}

/* ---- 设备枚举：返回一个固定的渲染节点（gpud 起来后可用） ---- */
int drmGetDevices2(unsigned int flags, drmDevicePtr devices[], int count) {
  (void)flags;
  if (count < 1 || devices == NULL) {
    return 1; /* 有 1 个设备 */
  }
  memset(devices[0], 0, sizeof(drmDevice));
  strcpy(devices[0]->nodes[DRM_NODE_RENDER], "/dev/dri/renderD128");
  devices[0]->available_nodes = 1 << DRM_NODE_RENDER;
  return 1;
}

void drmFreeDevices(drmDevicePtr devices[], int count) {
  (void)devices;
  (void)count;
}

/* ---- 版本：Mesa 的驱动匹配靠 name ---- */
drmVersionPtr drmGetVersion(int fd) {
  (void)fd;
  drmVersionPtr v = (drmVersionPtr)calloc(1, sizeof(drmVersion));
  if (v == NULL) {
    return NULL;
  }
  v->version_major = 1;
  v->version_minor = 0;
  v->version_patchlevel = 0;
  /* 名字决定 Mesa 选哪个驱动；当前只有 v3d 一个后端，
   * 将来加 mali 后这里按 fd（对应服务名）返回 "panfrost"。 */
  v->name = strdup("v3d");
  v->date = strdup("20260101");
  v->desc = strdup("foot gpud (userspace v3d)");
  return v;
}

void drmFreeVersion(drmVersionPtr v) {
  if (v == NULL) {
    return;
  }
  free(v->name);
  free(v->date);
  free(v->desc);
  free(v);
}

/* ---- prime（暂未实现，返回错误让 Mesa 走 fallback） ---- */
int drmPrimeFDToHandle(int fd, int prime_fd, unsigned int* handle) {
  (void)fd;
  (void)prime_fd;
  if (handle) {
    *handle = 0;
  }
  return -EINVAL;
}

int drmPrimeHandleToFD(int fd, unsigned int handle, unsigned int flags,
                       int* prime_fd) {
  (void)fd;
  (void)handle;
  (void)flags;
  if (prime_fd) {
    *prime_fd = -1;
  }
  return -EINVAL;
}

/* ---- driver-specific ioctl：交给各驱动的实现 ---- */
extern int v3d_drm_ioctl(unsigned long request, void* arg);

int drmIoctl(int fd, unsigned long request, void* arg) {
  (void)fd;
  /* driver-specific 区间（0x40 起）全部交给当前后端的实现。
   * GEM_CLOSE 在 v3d.c 里处理（结构定义在 v3d_drm.h）。 */
  return v3d_drm_ioctl(request, arg);
}

/* BO 的 GPU VA 查询（Mesa bufmgr 用 GET_BO_OFFSET 拿 GPU 地址） */
long drm_gpu_bo_offset(unsigned int handle) {
  drm_bo_ref ref;
  ref.handle = handle;
  return gpu_call(G_GPU_LOW_BO_OFFSET, &ref, sizeof(ref));
}

long drm_gpu_submit_cl(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_SUBMIT_CL, req, size);
}

long drm_gpu_submit_csd(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_SUBMIT_CSD, req, size);
}

long drm_gpu_submit_tfu(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_SUBMIT_TFU, req, size);
}

long drm_gpu_bo_create(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_BO_CREATE, req, size);
}
