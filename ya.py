# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
arch_type=get_arch_type()

default_libc=get_config('default_libc')

if default_libc:
    includes("./lib"+default_libc+"/ya.py")

if has_config("cpp-apps"):
    cpp_libs=['libcxx', 'libcxxabi']
    for v in cpp_libs:
        includes("./"+v+"/ya.py")



if has_config("app"):
    c_libs=[
        'libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
        'liblvgl', 'liblvgl-8.0.0', 'libuuid',
        'liblvqrcode', 'liblz4', 'libfreetype',
        'libwebp', 'libsdl2', 'libsdl2-ttf',
        'libsdl2-image', 'liblua',
        'libcmocka', 'libffmpeg',
        'libsdl', 'libsdl-ttf', 'libsdl-image',
        'libgaga',
        # 'libiconv'
        'libncurses',
        'libsdl-gfx',
        'libquickjs'

    ]

    if default_libc=='c':
        c_libs=[
            'libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
            'liblvgl', 'liblvgl-8.0.0', 'libuuid',
            'liblvqrcode', 'liblz4', 'libfreetype',
            'libwebp', 'libsdl2', 'libsdl2-ttf',
            'libsdl2-image', 'liblua',
            'libcmocka', 'libffmpeg',
            'libsdl', 'libsdl-ttf', 'libsdl-image',
            'libgaga',
            # 'libiconv'
            # 'libncurses',
            'libsdl-gfx',
            'libquickjs'
        ]
    

    if arch_type in['armv7-a','x86']:
        c_libs+=['libchez',]

    for v in c_libs:
        includes("./"+v+"/ya.py")


if has_config("wayland"):
    wayland_libs=['libwayland','libxml2']
    for v in wayland_libs:
        includes("./"+v+"/ya.py")
