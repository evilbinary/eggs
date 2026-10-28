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

/* libc / 平台能力探测（musl 全部具备） */
#define HAVE_MEMFD_CREATE 1
#define HAVE_STRCHRNUL 1
#define HAVE_MKOSTEMP 1
#define HAVE_POSIX_FALLOCATE 1
#define HAVE___BUILTIN_EXPECT 1
#define HAVE_COMPOSE_AND_KANA 0
#define HAVE_PANGO 0
#define HAVE_JPEG 0
#define HAVE_WEBP 0
#define HAVE_PAM 0

/* 不启用 EGL/vulkan：EGL_NO_X11 之类宏仅 gl-renderer 需要，这里不定义 */
#define EGL_NO_X11 0
#define MESA_EGL_NO_X11_HEADERS 0

/* musl 的 dev_t 宏在 sysmacros.h 里 */
#define MAJOR_IN_SYSMACROS 1

#define ALLOW_DEPRECATED_MODESET 0

/* shared/xalloc.h 的 OOM 提示里用了 glibc 的 program_invocation_short_name，
 * musl 没有这个全局量。这里宏替换成进程名常量（只影响错误输出文本）。 */
#define program_invocation_short_name "weston"

#endif /* WESTON_CONFIG_H */
