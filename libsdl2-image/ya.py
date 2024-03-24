# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("sdl2-image")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("sdl2")

add_files(
    '*.c'
)

add_cflags(' -D__YIYIYA__ ')

add_includedirs(
'.',
'../libgui/',
    public = true
)