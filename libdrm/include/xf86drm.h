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
  int name_len;
  char* name;
  int date_len;
  char* date;
  int desc_len;
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

/* 设备枚举（布局与真实 libdrm 的 drmDevice 保持一致，Mesa loader 会用
 * businfo/deviceinfo 字段） */
#define DRM_NODE_PRIMARY 0
#define DRM_NODE_CONTROL 1
#define DRM_NODE_RENDER 2
#define DRM_NODE_MAX 3

#define DRM_BUS_PCI 0
#define DRM_BUS_USB 1
#define DRM_BUS_PLATFORM 2
#define DRM_BUS_HOST1X 3

typedef struct _drmPciBusInfo {
  uint16_t domain;
  uint8_t bus;
  uint8_t dev;
  uint8_t func;
} drmPciBusInfo, *drmPciBusInfoPtr;

#define DRM_PLATFORM_DEVICE_NAME_LEN 512
typedef struct _drmPlatformBusInfo {
  char fullname[DRM_PLATFORM_DEVICE_NAME_LEN];
} drmPlatformBusInfo, *drmPlatformBusInfoPtr;

#define DRM_HOST1X_DEVICE_NAME_LEN 512
typedef struct _drmHost1xBusInfo {
  char fullname[DRM_HOST1X_DEVICE_NAME_LEN];
} drmHost1xBusInfo, *drmHost1xBusInfoPtr;

typedef struct _drmPciDeviceInfo {
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t subvendor_id;
  uint16_t subdevice_id;
  uint8_t revision_id;
} drmPciDeviceInfo, *drmPciDeviceInfoPtr;

typedef struct _drmPlatformDeviceInfo {
  char** compatible;
} drmPlatformDeviceInfo, *drmPlatformDeviceInfoPtr;

typedef struct _drmDevice {
  char* nodes[DRM_NODE_MAX];
  int available_nodes;
  int bustype;
  union {
    drmPciBusInfoPtr pci;
    drmPlatformBusInfoPtr platform;
    drmHost1xBusInfoPtr host1x;
  } businfo;
  union {
    drmPciDeviceInfoPtr pci;
    drmPlatformDeviceInfoPtr platform;
    void* host1x;
  } deviceinfo;
} drmDevice, *drmDevicePtr;

int drmGetDevices2(unsigned int flags, drmDevicePtr devices[],
                   int max_devices);
void drmFreeDevices(drmDevicePtr devices[], int count);

/* 单个设备（Mesa loader / EGL device 枚举用） */
int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr* device);
void drmFreeDevice(drmDevicePtr* device);
int drmDevicesEqual(drmDevicePtr a, drmDevicePtr b);
int drmGetDeviceFromDevId(dev_t dev_id, uint32_t flags,
                          drmDevicePtr* device);
#define DRM_DEVICE_GET_PCI_REVISION (1 << 0)
char* drmGetDeviceNameFromFd2(int fd);
char* drmGetPrimaryDeviceNameFromFd(int fd);
char* drmGetRenderDeviceNameFromFd(int fd);
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void* data,
                        unsigned long size);

/* 版本查询：Mesa 的驱动匹配靠 name（"v3d" / "panfrost"） */
drmVersionPtr drmGetVersion(int fd);
void drmFreeVersion(drmVersionPtr v);

/* 核心入口：driver-specific ioctl 的统一转发 */
int drmIoctl(int fd, unsigned long request, void* arg);

/* DRM capability（u_screen/u_sync_provider 会查） */
#define DRM_CAP_DUMB_BUFFER 0x1
#define DRM_CAP_PRIME 0x5
#define DRM_CAP_ADDFB2_MODIFIERS 0x10
#define DRM_CAP_SYNCOBJ 0x13
#define DRM_CAP_SYNCOBJ_TIMELINE 0x14
int drmGetCap(int fd, uint64_t capability, uint64_t* value);

/* prime（DMA-BUF 导入导出，暂 stub） */
#define DRM_PRIME_CAP_IMPORT 0x1
#define DRM_PRIME_CAP_EXPORT 0x2

int drmPrimeFDToHandle(int fd, int prime_fd, unsigned int* handle);
int drmPrimeHandleToFD(int fd, unsigned int handle, unsigned int flags,
                       int* prime_fd);

/* syncobj（foot 的 SUBMIT_CL 是同步提交，基本都是时序 stub） */
int drmSyncobjCreate(int fd, uint32_t flags, uint32_t* handle);
int drmSyncobjDestroy(int fd, uint32_t handle);
int drmSyncobjHandleToFD(int fd, uint32_t handle, int* obj_fd);
int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t* handle);
int drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd);
int drmSyncobjExportSyncFile(int fd, uint32_t handle, int* sync_file_fd);
int drmSyncobjWait(int fd, const uint32_t* handles, unsigned num_handles,
                   int64_t timeout_nsec, unsigned flags,
                   uint32_t* first_signaled);
int drmSyncobjReset(int fd, const uint32_t* handles, uint32_t handle_count);
int drmSyncobjSignal(int fd, const uint32_t* handles, uint32_t handle_count);
int drmSyncobjTimelineSignal(int fd, const uint32_t* handles, uint64_t* points,
                             uint32_t handle_count);
int drmSyncobjTimelineWait(int fd, const uint32_t* handles, uint64_t* points,
                           unsigned num_handles, int64_t timeout_nsec,
                           unsigned flags, uint32_t* first_signaled);
int drmSyncobjQuery2(int fd, const uint32_t* handles, uint64_t* points,
                     uint32_t handle_count, uint32_t flags);
int drmSyncobjTransfer(int fd, uint32_t dst_handle, uint64_t dst_point,
                       uint32_t src_handle, uint64_t src_point,
                       uint32_t flags);

/* DRM ioctl 编号约定 */
#define DRM_IOCTL_BASE 'd'
#define DRM_IO(nr) _IO(DRM_IOCTL_BASE, nr)
#define DRM_IOR(nr, type) _IOR(DRM_IOCTL_BASE, nr, type)
#define DRM_IOW(nr, type) _IOW(DRM_IOCTL_BASE, nr, type)
#define DRM_IOWR(nr, type) _IOWR(DRM_IOCTL_BASE, nr, type)

#define DRM_COMMAND_BASE 0x40

/* 设备节点命名（Mesa loader / pipe-loader 用） */
#define DRM_DIR_NAME "/dev/dri"
#define DRM_DEV_NAME "%s/card%d"
#define DRM_CONTROL_NODE_NAME "%s/controlD%d"
#define DRM_RENDER_NODE_NAME "%s/renderD%d"
#define DRM_PRIMARY_MINOR_NAME "card"
#define DRM_CONTROL_MINOR_NAME "controlD"
#define DRM_RENDER_MINOR_NAME "renderD"

#endif
