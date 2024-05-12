# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("sdl2")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("gui")

add_files(
    'src/*.c',
    'src/audio/*.c',
    'src/audio/yiyiya/*.c',
    # 'src/audio/aaudio/*.c',
    # 'src/audio/openslES/*.c',
    'src/atomic/SDL_atomic.c',
    'src/atomic/SDL_spinlock.c',
    # 'src/core/android/*.c',
    'src/cpuinfo/*.c',
    # 'src/dynapi/*.c',
    'src/events/*.c',
    'src/file/*.c',
    # 'src/haptic/*.c',
    # 'src/haptic/android/*.c',
    # 'src/hidapi/*.c',
    # 'src/hidapi/android/*.cpp',
    'src/joystick/*.c',
    'src/joystick/dummy/*.c',
    # 'src/joystick/hidapi/*.c',
    # 'src/joystick/virtual/*.c',
    # 'src/loadso/dlopen/*.c',
    # 'src/locale/*.c',
    # 'src/locale/android/*.c',
    # 'src/misc/*.c',
    # 'src/misc/android/*.c',
    # 'src/power/*.c',
    # 'src/power/android/*.c',
    # 'src/filesystem/android/*.c',
    # 'src/sensor/*.c',
    # 'src/sensor/android/*.c',
    'src/render/*.c',
    'src/render/*/*.c',
    'src/stdlib/*.c',
    'src/libm/*.c',
    'src/thread/*.c',
    # 'src/thread/generic/*.c',
    'src/thread/pthread/*.c',
    'src/timer/*.c',
    'src/timer/unix/*.c',
    'src/video/*.c',
    'src/video/yiyiya/*.c',
    'src/video/yuv2rgb/*.c',
    'src/main/dummy/*.c'
)

add_cflags(' -D__YIYIYA__ -D__GNUC__')

add_includedirs(
'.',
'./include',
'../libgui/',
public = true
)