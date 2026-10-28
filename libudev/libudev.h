/*
 * <libudev.h> 的对外入口。
 *
 * libudev-zero 上游的源码头文件名是 udev.h，安装时才改名为 libudev.h
 * （见其 meson.build 的 install_headers(configure_file(...))）。这里保持源码
 * 原样，用一层转接提供 Debian libudev-dev 同名的 <libudev.h>。
 */
#ifndef YIYIYA_LIBUDEV_H
#define YIYIYA_LIBUDEV_H

#include "udev.h"

#endif
