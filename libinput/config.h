/* 手工 config.h（libinput 1.19.4，meson 的 config.h 替代）。
 *
 * 与上游 meson.build 里 configuration_data() 的内容一一对应，只是把
 * "交叉环境下无法运行的探测"改成按本系统（musl + 无 sysfs/udev）实际情况直给：
 *   · HAVE_VERSIONSORT     : 不定义，用 src/libinput-versionsort.h 自带实现
 *   · HAVE_LOCALE_H        : musl 有 locale.h / newlocale()，定义
 *   · HAVE_XLOCALE_H       : musl 没有 xlocale.h，不定义
 *   · HAVE_LIBWACOM        : 没有 libwacom，0（表盘/绘图板按能力推断）
 *   · HAVE_LIBSYSTEMD/GSTACK/GTK : 测试与调试 GUI 用不到，0
 *   · HAVE_LIBEVDEV_DISABLE_PROPERTY : libevdev 1.14 有该接口，1
 */
#ifndef YIYIYA_LIBINPUT_CONFIG_H
#define YIYIYA_LIBINPUT_CONFIG_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* 文档链接只出现在错误提示里，取与版本一致的地址 */
#define HTTP_DOC_LINK "https://wayland.freedesktop.org/libinput/doc/1.19.4"

/* 非 meson 构建：builddir_lookup() 直接返回 NULL（release 语义） */
#define MESON_BUILD_ROOT ""

#define HAVE_LOCALE_H 1

/* musl 的 dirent.h 直接提供 versionsort()（还有 strverscmp），不要再让
 * src/libinput-versionsort.h 生成静态同名实现（会与 libc 声明冲突）。 */
#define HAVE_VERSIONSORT 1

#define HAVE_LIBEVDEV_DISABLE_PROPERTY 1

#define HAVE_LIBWACOM 0

#define HAVE_GSTACK 0
#define HAVE_LIBSYSTEMD 0
#define HAVE_GTK3 0
#define HAVE_GTK4 0

/* quirks 数据目录：本系统没有随包安装 quirks，缺失时 libinput 只打一条
 * error 日志并继续工作（见 libinput_init_quirks）。运行时可用环境变量
 * LIBINPUT_QUIRKS_DIR 指向别处。 */
#define LIBINPUT_QUIRKS_DIR "/usr/share/libinput"
#define LIBINPUT_QUIRKS_OVERRIDE_FILE "/etc/libinput/local-overrides.quirks"
#define LIBINPUT_QUIRKS_SRCDIR "/usr/share/libinput"

/* 工具（libinput-debug-events 等）的安装目录，本移植只出库，给空串 */
#define LIBINPUT_TOOL_PATH ""

#endif
