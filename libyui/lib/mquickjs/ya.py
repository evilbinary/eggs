# coding:utf-8
# *******************************************************************
# * Copyright 2025-present evilbinary
# * 作者: evilbinary on 01/20/2025
# * 邮箱: rootntsd@gmail.com
# ********************************************************************

target("mquickjs")
add_packages(get_config('default_libc'))
set_kind("static")
add_cflags(' -Wincompatible-pointer-types  -DNO_MAIN -g -MMD -D_GNU_SOURCE -fno-math-errno -fno-trapping-math ')
add_files(
    'readline_tty.c',
    'readline.c',
    'mquickjs.c',
    'dtoa.c',
    'libm.c',
    'cutils.c',
    'mqjs_std.c'
) 
add_includedirs(
    '.',
    '../include',
    public = true
)
