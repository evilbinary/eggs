# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("gui")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("jpeg","png","algorithm")

add_files(
    'screen.c',
    'event.c'
)

add_includedirs(
    '.',
    '../include',

    public = true
)

add_cflags("-O2")

target("image")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("jpeg","png","algorithm")

add_files(
    'image.c',
    'jpeg_decoder.c',
    'png_decoder.c'
)

add_includedirs(
    '.',
    public = true
)
