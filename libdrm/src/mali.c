/* mali.c - Mali（panfrost/panthor）DRM ioctl adapter 骨架
 *
 * 和 v3d.c 平级：只负责把 Mali 的 DRM UAPI 翻译成 gpud 的 GPU job。
 * 现在还没有 Mali 后端，所有 ioctl 返回 -EINVAL；结构先占位，
 * 以后接 panfrost 时只改这个文件。
 */
#include <stdio.h>
#include <errno.h>

#include "drm_driver.h"

static int mali_drm_ioctl(unsigned long nr, void* arg) {
  (void)arg;
  printf("libdrm mali: unhandled ioctl 0x%lx\n", nr);
  return -EINVAL;
}

const drm_driver_ops_t mali_drm_ops = {
    .name = "panfrost",
    .ioctl = mali_drm_ioctl,
};
