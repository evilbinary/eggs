# coding:utf-8
# *******************************************************************
# * Copyright 2021-2080 evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
import os
import platform
import copy

Import('env')

current = Dir('.').srcnode().path



if env.get('DEFAULT_LIBC') == 'libmusl':
    env['CFLAGS'] += ' -D__LIB_MUSL__  -static '
    env['LIBPATH'] += ['../libmusl/lib/']
    env['CPPPATH'] += [
        '#/eggs/libmusl',
        '#/eggs/libmusl/include',
        '#/eggs/libmusl/obj/include/',
        '#/eggs/libmusl/arch/generic/',
        '#/eggs/ibmusl/arch/generic/bits'
    ]
    env['LIBC'] = ['libm.a', 'libmusl.a']

    if env['ARCHTYPE'] == 'x86':
        env['CPPPATH'] += [
            '#/eggs/libmusl/arch/i386/',
            '#/eggs/libmusl/arch/i386/bits'

        ]
    elif arch_type == 'arm':
        env['CPPPATH'] += ['../libmusl/arch/arm/']
    else:
        print('no support libmusl type %s' % (arch))
else:
    env['LIBPATH'] += ['../libc/']
    env['CPPPATH'] += [
        '#/eggs/include/c',
        '../include/c',
        '../include',
    ]
    env['CFLAGS'] += '  -DLIBYC '

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
    SConscript(dirs=['libffmpeg'], exports='env')

    if env.get('DEFAULT_LIBC') == 'libmusl':
        SConscript(dirs=['libmusl'], exports='env')
    else:
        SConscript(dirs=['libc'], exports='env')

else:
    pass
