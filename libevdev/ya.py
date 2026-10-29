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
# 供 libweston.so 静态链入：共享库要求位置无关代码
add_cflags('-fPIC')

add_packages(get_config('default_libc'))

add_files(
    'libevdev/libevdev.c',
    'libevdev/libevdev-names.c',
    'libevdev/libevdev-uinput.c',
)

# '.'（config.h 所在）保持私有：否则依赖方的 #include "config.h" 会命中
# 这里的 config.h，把自己的构建配置顶掉。
add_includedirs('./include', public=True)   # linux/input.h 等 uapi 头
add_includedirs('.')                        # config.h
