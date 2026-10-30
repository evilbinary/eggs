/* YiYiYa: cairo 1.18 由 meson 的 configure_file() 生成此文件（无模板，就是把
 * feature_conf 里 set 过的键写成 #define）。这里手工给等价内容。
 *
 * 启用的后端只保留纯内存/文件类（image / png / ft / svg / pdf / ps / script /
 * tee / recording / observer / mime / user-font）；xlib/xcb/quartz/win32/gl/
 * egl/vg/drm 一律不启用 —— 它们要么需要 X11/GL 头，要么本系统没有对应后端。
 * fontconfig（CAIRO_HAS_FC_FONT）留到移植 libfontconfig 时再开。
 */
#ifndef CAIRO_FEATURES_H
#define CAIRO_FEATURES_H

#define CAIRO_HAS_IMAGE_SURFACE 1
#define CAIRO_HAS_MIME_SURFACE 1
#define CAIRO_HAS_RECORDING_SURFACE 1
#define CAIRO_HAS_OBSERVER_SURFACE 1
#define CAIRO_HAS_USER_FONT 1
#define CAIRO_HAS_PNG_FUNCTIONS 1
#define CAIRO_HAS_FT_FONT 1
#define CAIRO_HAS_SVG_SURFACE 1
#define CAIRO_HAS_PDF_SURFACE 1
#define CAIRO_HAS_PS_SURFACE 1
#define CAIRO_HAS_SCRIPT_SURFACE 1
#define CAIRO_HAS_TEE_SURFACE 1

/* 互斥锁实现：cairo-mutex-impl-private.h 用 `#elif CAIRO_HAS_PTHREAD` 选
 * pthread 实现（没有它就直接 #error）。musl 有完整 pthread。 */
#define CAIRO_HAS_PTHREAD 1

#endif
