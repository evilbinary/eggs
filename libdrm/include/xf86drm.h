/* xf86drm.h - libdrm 替身（Mesa 所需符号的最小子集）
 *
 * 通用 GPU 用户态框架：v3d / mali(panfrost) 等 DRM 驱动的 ioctl
 * 都从这里转发到 gpud 服务。符号名与真实 libdrm 保持一致，
 * Mesa 链接本库即可零改动运行。
 */
#ifndef XF86DRM_FOOT_H
#define XF86DRM_FOOT_H

#include <stdint.h>
#include <sys/types.h>

typedef struct _drmVersion {
  int version_major;
  int version_minor;
  int version_patchlevel;
  char* name;
  char* date;
  char* desc;
} drmVersion, *drmVersionPtr;

typedef struct _drmVersionNum {
  int version_major;
  int version_minor;
  int version_patchlevel;
  const char* name;
  const char* date;
  const char* desc;
} drmVersionNum;

/* 设备枚举 */
typedef struct {
  char nodes[4][64];
  int available_nodes;
  int bustype;
} drmDevice, *drmDevicePtr;

#define DRM_NODE_PRIMARY 0
#define DRM_NODE_RENDER 2

int drmGetDevices2(unsigned int flags, drmDevicePtr devices[],
                   int max_devices);
void drmFreeDevices(drmDevicePtr devices[], int count);

/* 版本查询：Mesa 的驱动匹配靠 name（"v3d" / "panfrost"） */
drmVersionPtr drmGetVersion(int fd);
void drmFreeVersion(drmVersionPtr v);

/* 核心入口：driver-specific ioctl 的统一转发 */
int drmIoctl(int fd, unsigned long request, void* arg);

/* prime（DMA-BUF 导入导出，暂 stub） */
int drmPrimeFDToHandle(int fd, int prime_fd, unsigned int* handle);
int drmPrimeHandleToFD(int fd, unsigned int handle, unsigned int flags,
                       int* prime_fd);

/* DRM ioctl 编号约定 */
#define DRM_IOCTL_BASE 'd'
#define DRM_IO(nr) _IO(DRM_IOCTL_BASE, nr)
#define DRM_IOR(nr, type) _IOR(DRM_IOCTL_BASE, nr, type)
#define DRM_IOW(nr, type) _IOW(DRM_IOCTL_BASE, nr, type)
#define DRM_IOWR(nr, type) _IOWR(DRM_IOCTL_BASE, nr, type)

#define DRM_COMMAND_BASE 0x40

#endif
