# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("lz4")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'lib/lz4.c',
    'lib/lz4frame.c',
    'lib/lz4hc.c',
    'lib/xxhash.c'
)

add_includedirs(
    '.',
    '../include',
    'lib/',
    public = true
)