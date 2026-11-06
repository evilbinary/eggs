/* drm.c - libdrm 替身：把 DRM ioctl 转发到 gpud 服务
 *
 * 分层：本文件是【通用框架】——driver-specific 的 ioctl 编号
 * 按驱动分发（v3d 在 v3d.c，将来 mali 在 mali.c），这里只做
 * 公共部分：版本探测、设备枚举、GEM 公共 ioctl。
 */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "xf86drm.h"
#include "drm/v3d_drm.h"
#include "drm_driver.h"
#include "client.h"

/* gpud 服务的方法号（与 foot/driver/gpu/gpu_backend.h 一致） */
#define G_GPU_LOW_BO_CREATE 110
#define G_GPU_LOW_BO_DESTROY 112
#define G_GPU_LOW_BO_OFFSET 101
#define G_GPU_LOW_SUBMIT_CL 120
#define G_GPU_LOW_SUBMIT_CSD 121
#define G_GPU_LOW_SUBMIT_TFU 122
#define G_GPU_LOW_WAIT_BO 130
#define G_GPU_LOW_SCREEN_INFO 140
#define G_GPU_LOW_SCREEN_PARAM 141

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
  /* gpud 的 BO_CREATE 会逐页写 GPU 页表并做一次 4MB 页表 cache clean；
   * 1MB 以上的 BO 在真机上可能超过 5 秒，原来的 5s 超时会留下 busy
   * slot，连续几次就 “no free slot”。给足 60s。 */
  return client_call_ex(c, (void*)(uintptr_t)fn, args, size, 60000);
}

extern const drm_driver_ops_t v3d_drm_ops;
extern const drm_driver_ops_t mali_drm_ops;

static const drm_driver_ops_t* g_driver = &v3d_drm_ops;
static int g_gpu_fd = -1;

void drm_driver_register(const drm_driver_ops_t* ops) {
  g_driver = ops;
}

void drm_driver_select(const char* name) {
  if (name != NULL &&
      (strcmp(name, "panfrost") == 0 || strcmp(name, "mali") == 0)) {
    g_driver = &mali_drm_ops;
  } else {
    g_driver = &v3d_drm_ops;
  }
}

const drm_driver_ops_t* drm_driver_current(void) {
  return g_driver;
}

/* ---- 设备枚举：返回一个固定的渲染节点。
 * YiYiYa 没有内核 DRM 节点，ioctl 全由 libdrm 替身转发 gpud；
 * 这里用 /dev/null 让 Mesa loader 的 open() 能成功。 ---- */
#define FOOT_DRM_RENDER_NODE "/dev/null"

static drmDevicePtr drm_make_device(void) {
  drmDevicePtr d = (drmDevicePtr)calloc(1, sizeof(drmDevice));
  if (d == NULL) {
    return NULL;
  }
  d->nodes[DRM_NODE_RENDER] = strdup(FOOT_DRM_RENDER_NODE);
  d->available_nodes = 1 << DRM_NODE_RENDER;
  d->bustype = DRM_BUS_PLATFORM;
  drmPlatformBusInfoPtr p =
      (drmPlatformBusInfoPtr)calloc(1, sizeof(drmPlatformBusInfo));
  if (p != NULL) {
    strcpy(p->fullname, "/soc/gpu");
    d->businfo.platform = p;
  }
  return d;
}

static void drm_destroy_device(drmDevicePtr d) {
  if (d == NULL) {
    return;
  }
  for (int i = 0; i < DRM_NODE_MAX; i++) {
    free(d->nodes[i]);
  }
  free(d->businfo.platform);
  free(d->deviceinfo.platform);
  free(d);
}

int drmGetDevices2(unsigned int flags, drmDevicePtr devices[], int count) {
  (void)flags;
  if (count < 1 || devices == NULL) {
    return 1; /* 有 1 个设备 */
  }
  devices[0] = drm_make_device();
  if (devices[0] == NULL) {
    return -1;
  }
  return 1;
}

void drmFreeDevices(drmDevicePtr devices[], int count) {
  if (devices == NULL) {
    return;
  }
  for (int i = 0; i < count; i++) {
    drm_destroy_device(devices[i]);
    devices[i] = NULL;
  }
}

int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr* device) {
  (void)fd;
  (void)flags;
  if (device == NULL) {
    return -1;
  }
  *device = drm_make_device();
  return *device != NULL ? 0 : -1;
}

void drmFreeDevice(drmDevicePtr* device) {
  if (device == NULL) {
    return;
  }
  drm_destroy_device(*device);
  *device = NULL;
}

int drmDevicesEqual(drmDevicePtr a, drmDevicePtr b) {
  const char* na;
  const char* nb;
  if (a == NULL || b == NULL) {
    return 0;
  }
  na = a->nodes[DRM_NODE_RENDER];
  nb = b->nodes[DRM_NODE_RENDER];
  if (na == NULL || nb == NULL) {
    return 0;
  }
  return strcmp(na, nb) == 0 ? 1 : 0;
}

int drmGetDeviceFromDevId(dev_t dev_id, uint32_t flags,
                          drmDevicePtr* device) {
  (void)dev_id;
  return drmGetDevice2(-1, flags, device);
}

char* drmGetDeviceNameFromFd2(int fd) {
  (void)fd;
  return strdup(FOOT_DRM_RENDER_NODE);
}

char* drmGetPrimaryDeviceNameFromFd(int fd) {
  (void)fd;
  return strdup(FOOT_DRM_RENDER_NODE);
}

char* drmGetRenderDeviceNameFromFd(int fd) {
  (void)fd;
  return strdup(FOOT_DRM_RENDER_NODE);
}

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void* data,
                        unsigned long size) {
  (void)fd;
  (void)drmCommandIndex;
  (void)data;
  (void)size;
  return -1;
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
  /* 名字决定 Mesa 选哪个驱动：由已注册的 driver adapter 提供。 */
  v->name = strdup(g_driver && g_driver->name ? g_driver->name : "gpu");
  v->date = strdup("20260101");
  v->desc = strdup("foot gpud (userspace v3d)");
  v->name_len = v->name ? (int)strlen(v->name) + 1 : 0;
  v->date_len = v->date ? (int)strlen(v->date) + 1 : 0;
  v->desc_len = v->desc ? (int)strlen(v->desc) + 1 : 0;
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

/* ---- syncobj stubs ----
 * gpud 的 SUBMIT_CL 在返回前已经等 RFC/BFC 递增，job 是同步完成的。
 * Mesa 只在 context 创建/销毁和 fence 路径用 syncobj；这里给单调句柄，
 * Wait 直接返回“已 signal”。真正的 fence 导出等异步提交再做。 */
static uint32_t g_syncobj_next = 1;

int drmSyncobjCreate(int fd, uint32_t flags, uint32_t* handle) {
  (void)fd;
  (void)flags;
  if (handle == NULL) {
    return -EINVAL;
  }
  *handle = g_syncobj_next++;
  return 0;
}

int drmSyncobjDestroy(int fd, uint32_t handle) {
  (void)fd;
  (void)handle;
  return 0;
}

int drmSyncobjHandleToFD(int fd, uint32_t handle, int* obj_fd) {
  (void)fd;
  (void)handle;
  if (obj_fd) {
    *obj_fd = -1;
  }
  return -EINVAL;
}

int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t* handle) {
  (void)fd;
  (void)obj_fd;
  if (handle) {
    *handle = 0;
  }
  return -EINVAL;
}

int drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd) {
  (void)fd;
  (void)handle;
  (void)sync_file_fd;
  return -EINVAL;
}

int drmSyncobjExportSyncFile(int fd, uint32_t handle, int* sync_file_fd) {
  (void)fd;
  (void)handle;
  if (sync_file_fd) {
    *sync_file_fd = -1;
  }
  return -EINVAL;
}

int drmSyncobjWait(int fd, const uint32_t* handles, unsigned num_handles,
                   int64_t timeout_nsec, unsigned flags,
                   uint32_t* first_signaled) {
  (void)fd;
  (void)timeout_nsec;
  (void)flags;
  if (handles == NULL || num_handles == 0) {
    return -EINVAL;
  }
  if (first_signaled) {
    *first_signaled = handles[0];
  }
  return 0;
}

int drmSyncobjReset(int fd, const uint32_t* handles, uint32_t handle_count) {
  (void)fd;
  (void)handles;
  (void)handle_count;
  return 0;
}

int drmSyncobjSignal(int fd, const uint32_t* handles, uint32_t handle_count) {
  (void)fd;
  (void)handles;
  (void)handle_count;
  return 0;
}

int drmSyncobjTimelineSignal(int fd, const uint32_t* handles, uint64_t* points,
                             uint32_t handle_count) {
  (void)fd;
  (void)handles;
  (void)points;
  (void)handle_count;
  return 0;
}

int drmSyncobjTimelineWait(int fd, const uint32_t* handles, uint64_t* points,
                           unsigned num_handles, int64_t timeout_nsec,
                           unsigned flags, uint32_t* first_signaled) {
  (void)fd;
  (void)points;
  (void)timeout_nsec;
  (void)flags;
  if (handles == NULL || num_handles == 0) {
    return -EINVAL;
  }
  if (first_signaled) {
    *first_signaled = handles[0];
  }
  return 0;
}

int drmSyncobjQuery2(int fd, const uint32_t* handles, uint64_t* points,
                     uint32_t handle_count, uint32_t flags) {
  (void)fd;
  (void)flags;
  if (points != NULL) {
    for (uint32_t i = 0; i < handle_count; i++) {
      points[i] = 0;
    }
  }
  (void)handles;
  return 0;
}

int drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                       uint32_t src_handle, uint64_t src_point,
                       uint32_t flags) {
  (void)fd;
  (void)dst_handle;
  (void)dst_point;
  (void)src_handle;
  (void)src_point;
  (void)flags;
  return 0;
}

int drmGetCap(int fd, uint64_t capability, uint64_t* value) {
  (void)fd;
  if (value == NULL) {
    return -EINVAL;
  }
  switch (capability) {
    case DRM_CAP_PRIME:
      *value = DRM_PRIME_CAP_IMPORT | DRM_PRIME_CAP_EXPORT;
      return 0;
    case DRM_CAP_DUMB_BUFFER:
      *value = 0;
      return 0;
    case DRM_CAP_ADDFB2_MODIFIERS:
      *value = 0;
      return 0;
    case DRM_CAP_SYNCOBJ:
    case DRM_CAP_SYNCOBJ_TIMELINE:
      *value = 0;
      return 0;
    default:
      return -EINVAL;
  }
}

int drmIoctl(int fd, unsigned long request, void* arg) {
  /* 记下 Mesa 使用的 DRM fd；v3d_bo_map_unsynchronized() 会拿同一个 fd
   * 调 mmap()，下面的 mmap 拦截用它识别 GPU BO 映射。 */
  g_gpu_fd = fd;

  /* 先走通用 GEM/BO/屏幕；没命中再交给驱动 adapter（v3d.c/mali.c）。 */
  int rc = drm_gem_ioctl(request, arg);
  if (rc != DRM_GEM_NOT_HANDLED) {
    return rc;
  }
  if (g_driver != NULL && g_driver->ioctl != NULL) {
    return g_driver->ioctl(request, arg);
  }
  printf("libdrm: no driver registered (ioctl 0x%lx)\n", request);
  return -EINVAL;
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

long drm_gpu_bo_destroy(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_BO_DESTROY, req, size);
}

long drm_gpu_screen_info(void* req, unsigned int size) {
  return gpu_call(G_GPU_LOW_SCREEN_INFO, req, size);
}

long drm_gpu_screen_param(unsigned int param) {
  return gpu_call(G_GPU_LOW_SCREEN_PARAM, &param, sizeof(param));
}

/* ================= 通用 GEM/BO/屏幕 ================= */
/* gpud 的服务调用（drm.c 通用 RPC 封装） */
extern long drm_gpu_bo_destroy(void* req, unsigned int size);
extern long drm_gpu_bo_offset(unsigned int handle);
extern long drm_gpu_screen_param(unsigned int param);

/* direct scanout：屏幕 framebuffer 作为一个"已有 BO"暴露。
 * GEM_OPEN 的 name 用魔数，handle 固定 100；GET_BO_OFFSET 返回屏幕 GVA。 */
#define DRM_SCREEN_NAME 1u
#define DRM_SCREEN_HANDLE 100u

/* 替身头里没有 GEM_OPEN，按内核 uapi 布局自己定义。 */
struct foot_drm_gem_open {
  uint32_t name;
  uint32_t handle;
  uint64_t size;
};
#define FOOT_DRM_IOCTL_GEM_OPEN DRM_IOWR(0x0b, struct foot_drm_gem_open)

typedef struct {
  unsigned int gva, pa, width, height, pitch, cpp, format, size;
} drm_screen_info_t;

static drm_screen_info_t g_drm_screen_info;
static int g_drm_screen_info_valid;

static int drm_screen_ensure_info(void) {
  if (g_drm_screen_info_valid) return 0;
  long gva = drm_gpu_screen_param(0);
  long pa = drm_gpu_screen_param(1);
  long w = drm_gpu_screen_param(2);
  long h = drm_gpu_screen_param(3);
  long pitch = drm_gpu_screen_param(4);
  long cpp = drm_gpu_screen_param(5);
  long fmt = drm_gpu_screen_param(6);
  long size = drm_gpu_screen_param(7);
  if (gva < 0 || w <= 0 || h <= 0 || pitch <= 0 || size <= 0) {
    return -1;
  }
  g_drm_screen_info.gva = (unsigned int)gva;
  g_drm_screen_info.pa = (unsigned int)pa;
  g_drm_screen_info.width = (unsigned int)w;
  g_drm_screen_info.height = (unsigned int)h;
  g_drm_screen_info.pitch = (unsigned int)pitch;
  g_drm_screen_info.cpp = (unsigned int)cpp;
  g_drm_screen_info.format = (unsigned int)fmt;
  g_drm_screen_info.size = (unsigned int)size;
  g_drm_screen_info_valid = 1;
  return 0;
}

#define DRM_MAX_BO_MAP 64
static struct {
  unsigned int handle;
  unsigned int cpu_va;
  unsigned int size;
  unsigned int gva;
  unsigned int cap;
} g_drm_bo_map[DRM_MAX_BO_MAP];

void* foot_gpu_bo_cpu_va(unsigned int handle) {
  for (unsigned int i = 0; i < DRM_MAX_BO_MAP; i++) {
    if (g_drm_bo_map[i].handle == handle && handle != 0) {
      return (void*)(uintptr_t)g_drm_bo_map[i].cpu_va;
    }
  }
  return NULL;
}

void drm_bo_map_record(unsigned int handle, unsigned int cpu_va,
                       unsigned int size, unsigned int gva,
                       unsigned int cap) {
  for (unsigned int i = 0; i < DRM_MAX_BO_MAP; i++) {
    if (g_drm_bo_map[i].handle == 0 || g_drm_bo_map[i].handle == handle) {
      g_drm_bo_map[i].handle = handle;
      g_drm_bo_map[i].cpu_va = cpu_va;
      g_drm_bo_map[i].size = size;
      g_drm_bo_map[i].gva = gva;
      g_drm_bo_map[i].cap = cap;
      return;
    }
  }
}

void drm_bo_map_remove(unsigned int handle) {
  for (unsigned int i = 0; i < DRM_MAX_BO_MAP; i++) {
    if (g_drm_bo_map[i].handle == handle) {
      /* 客户端 BO 的物理页由 foot arena capability 持有；
       * mmap 拦截返回的 VA 不是普通 VMA，必须显式 unmap + revoke，
       * 否则每个进程退出都会泄漏几 MB 物理内存。 */
      if (g_drm_bo_map[i].cap != 0) {
        foot_cap_unmap((foot_cap_t)g_drm_bo_map[i].cap);
        foot_cap_revoke((foot_cap_t)g_drm_bo_map[i].cap);
      }
      g_drm_bo_map[i].handle = 0;
      g_drm_bo_map[i].cpu_va = 0;
      g_drm_bo_map[i].size = 0;
      g_drm_bo_map[i].gva = 0;
      g_drm_bo_map[i].cap = 0;
    }
  }
}

/* Mesa 依赖内核在 SUBMIT 前做 cache 维护；用户态 gpud 必须自己把
 * CPU 写的 BO（BCL/RCL/tile_alloc/tile_state/资源）clean 到 DRAM。 */
void drm_bo_flush_all(void) {
  for (unsigned int i = 0; i < DRM_MAX_BO_MAP; i++) {
    if (g_drm_bo_map[i].handle != 0) {
      void* start = (void*)(uintptr_t)g_drm_bo_map[i].cpu_va;
      foot_cache_clean(start, (char*)start + g_drm_bo_map[i].size);
    }
  }
}

/* ---- 通用 GEM 操作（驱动 adapter 共用）---- */
int drm_gem_bo_close(unsigned int handle) {
  if (handle == DRM_SCREEN_HANDLE) {
    return 0; /* 屏幕 BO 不是应用分配的，不能 destroy */
  }
  struct { unsigned int handle; } ref;
  ref.handle = handle;
  drm_bo_map_remove(handle);
  return drm_gpu_bo_destroy(&ref, sizeof(ref)) == 0 ? 0 : -EINVAL;
}

int drm_gem_bo_offset(unsigned int handle, unsigned int* offset) {
  if (handle == DRM_SCREEN_HANDLE) {
    if (drm_screen_ensure_info() != 0) return -EINVAL;
    *offset = g_drm_screen_info.gva;
    return 0;
  }
  long off = drm_gpu_bo_offset(handle);
  if (off < 0) return -EINVAL;
  *offset = (unsigned int)off;
  return 0;
}

int drm_gem_bo_mmap(unsigned int handle, unsigned int* cpu_va) {
  void* p = foot_gpu_bo_cpu_va(handle);
  if (p == NULL) return -EINVAL;
  *cpu_va = (unsigned int)(uintptr_t)p;
  return 0;
}

int drm_gem_screen_open(unsigned int name, unsigned int* handle,
                        unsigned int* size) {
  if (name != DRM_SCREEN_NAME) return -EINVAL;
  if (drm_screen_ensure_info() != 0) return -EINVAL;
  *handle = DRM_SCREEN_HANDLE;
  *size = g_drm_screen_info.size;
  return 0;
}

/* v3d 的通用 GEM ioctl（Mali adapter 可以复用这些 helper，
 * 只换自己的 ioctl 编号）。 */
int drm_gem_ioctl(unsigned long request, void* arg) {
  /* Mesa 的 drm-uapi 把 GEM_CLOSE 编成 0x40086409；替身头里的宏编码
   * 可能不同，先按裸值兜住。 */
  if (request == 0x40086409UL) {
    struct drm_gem_close* c = (struct drm_gem_close*)arg;
    return drm_gem_bo_close(c->handle);
  }

  switch (request) {
    case FOOT_DRM_IOCTL_GEM_OPEN: {
      struct foot_drm_gem_open* o = (struct foot_drm_gem_open*)arg;
      unsigned int h = 0, sz = 0;
      int rc = drm_gem_screen_open(o->name, &h, &sz);
      if (rc == 0) {
        o->handle = h;
        o->size = (uint64_t)sz;
      }
      return rc;
    }

    case DRM_IOCTL_GEM_CLOSE: {
      struct drm_gem_close* c = (struct drm_gem_close*)arg;
      return drm_gem_bo_close(c->handle);
    }

    case DRM_IOCTL_V3D_GET_BO_OFFSET: {
      struct drm_v3d_get_bo_offset* g = (struct drm_v3d_get_bo_offset*)arg;
      return drm_gem_bo_offset(g->handle, &g->offset);
    }

    case DRM_IOCTL_V3D_MMAP_BO: {
      struct drm_v3d_mmap_bo* m = (struct drm_v3d_mmap_bo*)arg;
      unsigned int cpu_va = 0;
      int rc = drm_gem_bo_mmap(m->handle, &cpu_va);
      if (rc == 0) m->offset = cpu_va;
      return rc;
    }

    case DRM_IOCTL_V3D_WAIT_BO:
      /* gpud 的 SUBMIT 是同步等待，到这里 job 已完成。 */
      return 0;

    default:
      return DRM_GEM_NOT_HANDLED;
  }
}


/* ------------------------------------------------------------------
 * 默认 BO 分配器：给任意 Mesa/EGL 应用使用，不再要求应用自己实现
 * bridge_bo_alloc()。测试/应用如果提供强符号，会覆盖这个 weak 定义。
 * ------------------------------------------------------------------ */
static unsigned int g_last_bo_cap;

/* v3d.c 在 alloc 成功后取走本次 arena capability，随 BO handle 记录，
 * GEM_CLOSE 时 unmap + revoke，避免物理页泄漏。 */
unsigned int drm_bo_take_last_cap(void) {
  unsigned int cap = g_last_bo_cap;
  g_last_bo_cap = 0;
  return cap;
}

__attribute__((weak)) int bridge_bo_alloc(unsigned int size, unsigned int* pa,
                                          unsigned int* cpu_va) {
  static unsigned long next_va = 0x52000000UL;
  unsigned int p = 0, s = 0;
  foot_cap_t cap = foot_cap_alloc_arena(size, 0, 0, &p, &s);
  if (cap == FOOT_CAP_NONE || s < size) {
    return -1;
  }
  unsigned long va = next_va;
  next_va += (s + 0xFFFu) & ~0xFFFUL;
  if (foot_cap_map(cap, (void*)va, 0, s) != 0) {
    return -1;
  }
  g_last_bo_cap = cap;
  *pa = p;
  *cpu_va = (unsigned int)va;
  return 0;
}

/* ------------------------------------------------------------------
 * mmap 拦截：Mesa 的 v3d_bo_map_unsynchronized() 在
 * DRM_IOCTL_V3D_MMAP_BO 之后会对 bo->screen->fd 做 mmap。
 * 这个 fd 是给 Mesa 的假 DRM fd（/dev/null 的 dup），不是可 mmap 的
 * GPU 设备；gpud 是用户态服务，没有内核 mmap 实现。
 *
 * DRM_IOCTL_V3D_MMAP_BO 已经把 BO 的 CPU VA 放在 map.offset 里，
 * 这里看到是同一个 GPU fd 就直接返回那个 CPU VA；其余 mmap 原样走
 * 系统调用，保证 /dev/fb、malloc 等正常。
 * ------------------------------------------------------------------ */
void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t off) {
  if (fd >= 0 && fd == g_gpu_fd && !(flags & MAP_ANONYMOUS)) {
    return (void*)(uintptr_t)off;
  }
#ifndef SYS_mmap
#define SYS_mmap 222
#endif
  long ret = syscall(SYS_mmap, addr, len, prot, flags, fd, off);
  return (void*)ret;
}
