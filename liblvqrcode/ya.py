# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("lvqrcode")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("lvgl")

add_files(
    'lv_qrcode.c',
    'qrcodegen.c'
)

add_cflags('-DLV_LVGL_H_INCLUDE_SIMPLE=1 ')

add_includedirs(
    '.'
)