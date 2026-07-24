# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("webp")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    "src/dec/*.c",
    'src/demux/*.c',
    'src/dsp/*.c',
    'src/enc/*.c',
    'src/mux/*.c',
    'src/utils/*.c'
)

add_includedirs(
'.',
'./src',
public = true
)