# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("freetype")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'src/**/*.c'
)

add_cflags('-DSDL_IMAGE_USE_COMMON_BACKEND',
    '-DLOAD_BMP -DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM',
    '-DLOAD_SVG -DLOAD_TGA -DLOAD_XCF -DLOAD_XPM -DLOAD_XV',
        '-DLOAD_PNG -DLOAD_WEBP',
    '-DLOAD_JPG')

add_includedirs(
    '.',
    './include',
    public = true
)