# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libinput 1.19.4（静态库，只出库；tools/test/doc 不编）。
#
# 不走 meson：config.h / libinput-version.h / libinput-git-version.h 手工给
# （做法与 libevdev、libwayland 一致，见同目录 config.h 顶部说明）。
#
# 依赖（与 Debian libinput-dev 的 Depends 一致）：
#   libevdev  → eggs/libevdev（1.14.0）
#   mtdev     → eggs/libmtdev（协议 A→B 多点触控转换）
#   libudev   → eggs/libudev（libudev-zero 实现）
#   libwacom  → 未移植（只服务绘图板，且拉 glib；config.h 里 HAVE_LIBWACOM=0）
# epoll 是 Linux 内置能力（非 BSD 的 libepoll-shim），不需要额外库。
target("input")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("evdev", "mtdev", "udev")

add_files(
    'src/*.c',
)

add_includedirs(
    '.',          # config.h / libinput-version.h
    './src',      # libinput.h 等对外头
    './include',  # linux/input.h uapi 头
    public=True,
)
