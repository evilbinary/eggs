/* weston 源码里仍有 #include "config.h"（upstream meson 生成的配置头）。
 *
 * 本仓库把 weston 配置改名为 weston-config.h 并用 -include 强制吃进来，
 * 但 gl-renderer 这类同时依赖 Mesa EGL 的模块，会把 Mesa 的
 * src/mesa/main 加进 include path；helpers.h 的 "config.h" 就会命中
 * Mesa 的 config.h，拉进 util/u_endian.h 并报 UTIL_ARCH_* unset。
 *
 * 这个垫片放在 eggs/weston（模块 include path 最前），保证始终命中
 * weston 自己的配置。
 */
#ifndef YIYIYA_WESTON_CONFIG_WRAPPER_H
#define YIYIYA_WESTON_CONFIG_WRAPPER_H

#include "weston-config.h"

#endif /* YIYIYA_WESTON_CONFIG_WRAPPER_H */
