# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# mtdev（Multitouch Protocol Translation Library）静态库。
#
# 作用：把 type A 多点触控事件流（有 ABS_MT_POSITION_X/Y、无 ABS_MT_SLOT）
# 转换成 type B（带 slot）。libinput 只在 evdev_need_mtdev() 为真时用。
# 上游无任何外部依赖（只用 libc + linux/input.h），编译单元见 src/Makefile.am。
target("mtdev")
set_kind("static")
# 供 libweston.so 静态链入：共享库要求位置无关代码
add_cflags('-fPIC')

add_packages(get_config('default_libc'))

add_files(
    'src/caps.c',
    'src/core.c',
    'src/iobuf.c',
    'src/match.c',
    'src/match_four.c',
)

add_includedirs(
    './include',  # mtdev.h / mtdev-plumbing.h / mtdev-mapping.h
    './src',      # common.h / evbuf.h / iobuf.h / match.h / state.h
    public=True,
)
