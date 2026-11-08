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
    'event.c',
    'xwin_user.c'
)

add_includedirs(
    '.',
    '../include',

    public = true
)

add_cflags("-O2")

# SCREEN_MODE_WAYLAND：libgui 内部直接用 wl_shm 上屏。
# 只有开 --wayland 时才带 libwayland 依赖；其它平台不引入。
if has_config("wayland"):
    add_cflags("-DENABLE_WAYLAND")
    add_deps("wayland")
    add_includedirs('../wayland-protocols/gen', public=True)
    add_files(
        'screen_wayland.c',
        '../wayland-protocols/gen/xdg-shell-protocol.c',
    )

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
