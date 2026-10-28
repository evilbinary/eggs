/* libxkbcommon 1.7.0 手工配置头（不走 meson）。
 *
 * 与 libwayland / libevdev / libinput / libpixman 的做法一致：只声明构建期
 * 探测会生成的宏，ya.py 把本目录加进 include path，源码里的
 * #include "config.h" 即指向这里。
 *
 * 取舍：
 *   - 只做 libxkbcommon 核心（不含 libxkbregistry，那个要 libxml2）。
 *   - HAVE_* 全部按 musl 的真实能力给（asprintf/vasprintf/strndup/mmap/
 *     eaccess/secure_getenv 都有）。
 *   - DFLT_XKB_CONFIG_ROOT 指向镜像里的 xkeyboard-config 数据目录。
 */
#ifndef XKBCOMMON_CONFIG_H
#define XKBCOMMON_CONFIG_H

#define LIBXKBCOMMON_VERSION "1.7.0"

/* 运行时 xkeyboard-config 数据根目录（evdev 规则/keycodes/symbols 都在里面） */
#define DFLT_XKB_CONFIG_ROOT "/usr/share/X11/xkb"
#define DFLT_XKB_CONFIG_EXTRA_PATH ""
#define XLOCALEDIR "/usr/share/X11/locale"

/* RMLVO 缺省值（与上游 meson 选项默认一致） */
#define DEFAULT_XKB_RULES "evdev"
#define DEFAULT_XKB_MODEL "pc105"
#define DEFAULT_XKB_LAYOUT "us"
#define DEFAULT_XKB_VARIANT NULL
#define DEFAULT_XKB_OPTIONS NULL

#define EXIT_INVALID_USAGE 2

#define HAVE_ASPRINTF 1
#define HAVE_VASPRINTF 1
#define HAVE_STRNDUP 1
#define HAVE_MMAP 1
#define HAVE_UNISTD_H 1
#define HAVE_EACCESS 1
#define HAVE_SECURE_GETENV 1
#define HAVE___BUILTIN_EXPECT 1

#endif /* XKBCOMMON_CONFIG_H */
