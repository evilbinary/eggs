/* libdrm 的极小垫片：只提供 drm_fourcc.h 需要的东西。
 *
 * 内核 uapi 的 drm_fourcc.h 第一行就 #include "drm.h"，从中取 fourcc_code()
 * 与 __u32/__u64。本系统没有 DRM 设备，libweston 用到 drm_fourcc.h 只是为了
 * DRM_FORMAT_* 常量（像素格式表），因此这里只需要这两个宏 + uapi 整数类型。
 */
#ifndef DRM_H_SHIM
#define DRM_H_SHIM

#include <stdint.h>

typedef uint32_t __u32;
typedef uint64_t __u64;

#define fourcc_code(a, b, c, d) ((__u32)(a) | ((__u32)(b) << 8) | \
				 ((__u32)(c) << 16) | ((__u32)(d) << 24))

#endif /* DRM_H_SHIM */
