/* libdrm 的极小垫片：只声明 libweston/pixel-formats.c 用到的两个名字查询。
 *
 * 这两个函数只影响 dma-buf modifier 的可读名称（日志用），本系统没有
 * DRM/KMS 设备，实现（compat/drm-stubs.c）直接返回 NULL。
 */
#ifndef XF86DRM_H_SHIM
#define XF86DRM_H_SHIM

#include <stdint.h>

const char *drmGetFormatModifierVendor(uint64_t modifier);
const char *drmGetFormatModifierName(uint64_t modifier);

#endif /* XF86DRM_H_SHIM */
