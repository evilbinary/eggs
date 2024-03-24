# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("sdl-gfx")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("sdl","webp")

add_files(
    '*.c'
)

add_cflags('-DSDL_IMAGE_USE_COMMON_BACKEND',
    '-DLOAD_GIF -DLOAD_LBM -DLOAD_PCX -DLOAD_PNM',
    '-DLOAD_SVG -DLOAD_TGA  -DLOAD_XPM -DLOAD_XV',
    '-DLOAD_PNG',
    '-DFAST_IS_JPEG -DLOAD_WEBP -DLOAD_BMP -DLOAD_XCF -DLOAD_JPG'
    )

add_includedirs(
'.',
'../libgui/',
    public = true
)