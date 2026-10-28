/* 手工 config.h（libevdev 1.14.0，autotools configure 的替代）。
 * aarch64-none-elf 交叉环境 configure 的链接测试无法通过，这里手工给。 */
#ifndef YIYIYA_LIBEVDEV_CONFIG_H
#define YIYIYA_LIBEVDEV_CONFIG_H

#define PACKAGE "libevdev"
#define PACKAGE_NAME "libevdev"
#define PACKAGE_VERSION "1.14.0"
#define PACKAGE_STRING "libevdev 1.14.0"
#define PACKAGE_TARNAME "libevdev"
#define PACKAGE_BUGREPORT "https://gitlab.freedesktop.org/libevdev/libevdev/-/issues"
#define PACKAGE_URL "http://libevdev.freedesktop.org/"

/* 本系统有（musl） */
#define HAVE_SYS_MMAN_H 1
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_ERRNO_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1

#endif
