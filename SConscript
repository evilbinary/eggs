# coding:utf-8
# *******************************************************************
# * Copyright 2021-2080 evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
import os
import platform
import copy

Import('appEnv')

env = appEnv

current = Dir('.').srcnode().path


returns = []

libc = None

if env.get('APP'):
    dirs = ['libjpeg', 'libgui', 'libetk', 'libzlib', 'libpng',
            'liblvgl', 'liblvgl-8.0.0', 'libuuid',
            'liblvqrcode', 'liblz4', 'libfreetype',
            'libwebp', 'libsdl2', 'libsdl2-ttf',
            'libsdl2-image', 'liblua',
            'libchez', 'libcmocka', 'libffmpeg',
            # 'libstdc++'
            'libcxx', 'libcxxabi',
            'libsdl', 'libsdl-ttf', 'libsdl-image',
            'libgaga',
            # 'libiconv'
            'libncurses',
            ]

    SConscript(dirs=dirs)

    if env.get('DEFAULT_LIBC') == 'libmusl':
        libc = SConscript(dirs=['libmusl'], exports='env')
    elif env.get('DEFAULT_LIBC') == 'libnewlib':
        libc = SConscript(dirs=['libnewlib'], exports='env')
    else:
        libc = SConscript(dirs=['libc'], exports='env')
else:
    pass

returns += libc

Export('appEnv')

Return('returns')
