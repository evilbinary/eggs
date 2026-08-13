# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************


# 宿主平台白名单（对应 platform/* 下的目录 + em/lvgl 变体）；
# esp32/stm32 为嵌入式平台，宿主工具与 demo 一律跳过
HOST_PLATS = ("None","", "sdl", "pc", "android", "ios", "web", "lvgl", "em", "emscripten", "em-lvgl")

def is_host_plat():
    return get_plat() in HOST_PLATS

def add_flags():
    add_packages(get_config('default_libc'))
    add_cflags('-D__YIYIYA__')

    pass
add_buildin('add_flags',add_flags)
add_buildin('is_host_plat',is_host_plat)

includes("./lib/ya.py")
# src/ya.py owns the yui static library (game/perf/input/backends).
includes("./src/ya.py")
