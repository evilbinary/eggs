# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libffi 3.8.0（aarch64，静态库）。
#
# 不走 configure：aarch64-none-elf 交叉环境里 configure 的链接测试无法通过
# （没有 crt），改为手工 fficonfig.h + 预生成的 include/ffi.h（由 ffi.h.in
# sed 替换 @TARGET@/@VERSION@ 等占位符得到）。
# 不编 closures.c / java_raw_api.c：wayland 只用 ffi_call + prep_cif。
target("ffi")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'src/prep_cif.c',
    'src/types.c',
    'src/raw_api.c',
    'src/ffi.c',
    'src/aarch64/ffi.c',
    'src/aarch64/sysv.S',
)

add_cflags('-DFFI_BUILDING_DLL=1')

add_includedirs(
    '.',            # fficonfig.h
    './include',    # ffi.h
    './src',
    './src/aarch64',
    public=True,
)
