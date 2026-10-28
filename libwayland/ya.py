# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# wayland 1.22.0 核心库（client + server + shm + cursor）。
#
# 不走 meson：协议代码（gen/*）由宿主端 wayland-scanner 预生成后提交，
# config.h 手工维护（见同目录 config.h 顶部说明）。egl 子库不编（无 GL）。
#
# 阶段 1 目标：libwayland-client/server 编出静态库，配合 app/wayland 的
# echo compositor + 最小 wl_shm client 打通握手。
target("wayland")
set_kind("static")

add_packages(get_config('default_libc'))
# 最小 ffi（wayland 只用 ffi_call；glibc/musl 的 ffi 依赖太重）
add_deps("ffi")

add_files(
    'src/connection.c',
    'src/wayland-client.c',
    'src/wayland-server.c',
    'src/wayland-shm.c',
    'src/wayland-util.c',
    'src/wayland-os.c',
    'src/event-loop.c',
    'gen/wayland-protocol.c',
    'cursor/wayland-cursor.c',
    'cursor/xcursor.c',
    'cursor/os-compatibility.c',
)

# '.'（config.h 所在）保持私有：否则依赖方的 #include "config.h" 会命中
# 这里的 config.h，把自己的构建配置顶掉。
add_includedirs('./src', './gen', './cursor', public=True)
add_includedirs('.')
