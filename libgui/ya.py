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
    # frame.c/cairo-util.c 里的 #include "config.h" 会通过 ../weston
    # include path 命中 eggs/weston/config.h（它再包含 weston-config.h）。
    # 不要 -include：那会把 _GNU_SOURCE 强加给 screen.c，和 eggs 的
    # types.h 在 musl 下撞 struct iovec。
    add_cflags("-DENABLE_WAYLAND")
    add_deps("wayland", "cairo", "png", "pixman")
    add_includedirs('../wayland-protocols/gen', '../weston', '../weston/shared',
                    public=True)
    add_files(
        'screen_wayland.c',
        # eventdemo 同款 CSD
        '../weston/shared/frame.c',
        '../weston/shared/cairo-util.c',
        '../weston/shared/image-loader.c',
        '../weston/shared/file-util.c',
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
