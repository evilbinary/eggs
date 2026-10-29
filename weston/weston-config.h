/* weston 16.0.1 手工配置头（不走 meson）。
 *
 * 与 libwayland / libevdev / libinput / libpixman / libxkbcommon 的做法一致：
 * 只声明构建期探测会生成的宏，ya.py 把本目录加进 include path。
 *
 * 裁剪策略（阶段 1：headless 跑通）：
 *   - 只开 BUILD_HEADLESS_COMPOSITOR；drm/wayland/x11/pipewire/rdp/vnc 全关，
 *     因此不需要 libdrm / GBM / EGL / vulkan / libdisplay-info / seat。
 *   - ENABLE_EGL / ENABLE_VULKAN 关：渲染器只用 pixman-renderer.c。
 *   - 强制 _GNU_SOURCE / _FILE_OFFSET_BITS=64：musl 下 memfd_create、
 *     posix_fallocate、strchrnul 等需要扩展可见性。
 */
#ifndef WESTON_CONFIG_H
#define WESTON_CONFIG_H

#define _GNU_SOURCE 1
#define _FILE_OFFSET_BITS 64

#define PACKAGE "weston"
#define PACKAGE_NAME "weston"
#define PACKAGE_VERSION "16.0.1"
#define PACKAGE_STRING "weston 16.0.1"
#define PACKAGE_BUGREPORT "https://gitlab.freedesktop.org/wayland/weston/issues/"
#define PACKAGE_URL "https://wayland.freedesktop.org"
#define VERSION "16.0.1"

/* 安装路径：真机镜像里的布局（不实际 install，仅用于默认值/日志） */
#define BINDIR "/usr/bin"
#define DATADIR "/usr/share"
#define LIBEXECDIR "/usr/libexec"
#define MODULEDIR "/usr/lib/weston"
#define LIBWESTON_MODULEDIR "/usr/lib/libweston-16"

/* 后端/渲染器裁剪：只留 headless + pixman */
#define BUILD_HEADLESS_COMPOSITOR 1

/* libc / 平台能力探测（musl 全部具备）。
 *
 * 注意：weston 源码对能力宏用的是 #ifdef（不是 #if），所以"没有这个能力"
 * 必须【不定义】，写成 #define X 0 反而会打开对应分支（踩过：HAVE_PANGO 0
 * 仍然去 include <pango/pangocairo.h>）。 */
#define HAVE_MEMFD_CREATE 1
#define HAVE_STRCHRNUL 1
#define HAVE_MKOSTEMP 1
#define HAVE_POSIX_FALLOCATE 1
#define HAVE___BUILTIN_EXPECT 1

/* 未启用：HAVE_PANGO / HAVE_JPEG / HAVE_WEBP / HAVE_PAM / HAVE_LCMS /
 * ENABLE_EGL / ENABLE_VULKAN / HAVE_GBM 一律不定义。 */

/* musl 的 dev_t 宏在 sysmacros.h 里 */
#define MAJOR_IN_SYSMACROS 1

/* 默认后端（meson 在开 BUILD_DRM_COMPOSITOR 时给 "drm-backend.so"）。
 * 本移植静态链接 headless 后端，compositor.c 里的 load_backend 已被
 * compat 补丁改成直接调 weston_backend_init，不走 dlopen。 */
#define WESTON_NATIVE_BACKEND "headless-backend.so"
/* 默认 shell 同理：静态直连 kiosk-shell（见 compat/load-shell.c） */
#define WESTON_SHELL "kiosk-shell.so"

#endif /* WESTON_CONFIG_H */
