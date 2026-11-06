#ifndef DRM_DRIVER_H
#define DRM_DRIVER_H

#include <stdint.h>

/* libdrm 内部：通用 GEM/BO/屏幕 + 驱动 adapter 注册。
 * v3d.c / mali.c 只提供 drm_driver_ops_t；通用逻辑都在 drm.c。 */
typedef struct {
  const char* name; /* "v3d" / "panfrost" */
  int (*ioctl)(unsigned long nr, void* arg);
} drm_driver_ops_t;

#define DRM_GEM_NOT_HANDLED 0x7fffffff

int drm_gem_ioctl(unsigned long nr, void* arg);

void* foot_gpu_bo_cpu_va(unsigned int handle);

void drm_bo_map_record(unsigned int handle, unsigned int cpu_va,
                       unsigned int size, unsigned int gva,
                       unsigned int cap);
void drm_bo_map_remove(unsigned int handle);
void drm_bo_flush_all(void);

int drm_gem_bo_close(unsigned int handle);
int drm_gem_bo_offset(unsigned int handle, unsigned int* offset);
int drm_gem_bo_mmap(unsigned int handle, unsigned int* cpu_va);
int drm_gem_screen_open(unsigned int name, unsigned int* handle,
                        unsigned int* size);

#endif
