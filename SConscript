# coding:utf-8
# *******************************************************************
# * Copyright 2021-2080 evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
import os
import platform
import copy
from xenv.env import add_libc

Import('appEnv')

env=appEnv

current = Dir('.').srcnode().path

add_libc(env)

returns=[]

libc=None

if env.get('APP'):
    SConscript(dirs=['libjpeg'], exports='env')
    SConscript(dirs=['libgui'], exports='env')
    SConscript(dirs=['libetk'], exports='env')
    SConscript(dirs=['libzlib'], exports='env')
    SConscript(dirs=['libpng'], exports='env')
    SConscript(dirs=['liblvgl'], exports='env')
    SConscript(dirs=['libuuid'], exports='env')
    SConscript(dirs=['liblvqrcode'], exports='env')
    SConscript(dirs=['liblz4'], exports='env')
    SConscript(dirs=['libfreetype'], exports='env')
    SConscript(dirs=['libwebp'], exports='env')
    SConscript(dirs=['libsdl2'], exports='env')
    SConscript(dirs=['libsdl2-image'], exports='env')
    SConscript(dirs=['libsdl2-ttf'], exports='env')
    SConscript(dirs=['liblua'], exports='env')
    SConscript(dirs=['libchez'], exports='env')
    SConscript(dirs=['libcmocka'], exports='env')
    # SConscript(dirs=['libffmpeg'], exports='env')
    # SConscript(dirs=['libstdc++'], exports='env')
    SConscript(dirs=['libcxx'], exports='env')
    SConscript(dirs=['libcxxabi'], exports='env')
    SConscript(dirs=['libsdl'], exports='env')
    SConscript(dirs=['libsdl-ttf'], exports='env')
    SConscript(dirs=['libsdl-image'], exports='env')


    if env.get('DEFAULT_LIBC') == 'libmusl':
        libc=SConscript(dirs=['libmusl'], exports='env')
    elif env.get('DEFAULT_LIBC') == 'libnewlib':
        libc=SConscript(dirs=['libnewlib'], exports='env')
    else:
        libc=SConscript(dirs=['libc'], exports='env')
else:
    pass

returns+=libc

Export('appEnv')

Return('returns')
