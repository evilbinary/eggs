# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libevdev 1.14.0（静态库）。不走 configure：手工 config.h。
# include/linux/{input,input-event-codes,uinput}.h 是从 Linux 6.5.10
# uapi 抽的（musl 不提供 linux/ 头）。
#
# 注意：libevdev-uinput.c 运行时需要 /dev/uinput，本内核暂无；不调用
# uinput 接口就不受影响（libinput 用的是 libevdev 的核心事件解析）。
target("evdev")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'libevdev/libevdev.c',
    'libevdev/libevdev-names.c',
    'libevdev/libevdev-uinput.c',
)

add_includedirs(
    '.',            # config.h
    './include',    # linux/input.h 等 uapi 头
    public=True,
)
