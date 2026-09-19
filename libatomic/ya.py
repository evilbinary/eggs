# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# armv5 裸机的原子操作补充（SDL2 / miniaudio 等库需要，arm-none-eabi 的
# libgcc 不提供 __sync_xxx / __atomic_xxx）。其它架构本文件编译为空，
# 由 GCC 内联或 libgcc 提供，不会重复定义 ⇒ 所有平台都可以安全链接。
target("atomic")
set_kind("static")
add_packages(get_config('default_libc'))
add_files(
    'atomic_armv5.c',
)
add_includedirs(
    '.',
    public = true
)
