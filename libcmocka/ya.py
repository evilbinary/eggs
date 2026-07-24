# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("cmocka")
set_kind("static")

add_packages(get_config('default_libc'))

add_files(
    'src/*.c'
)

add_cflags('-DHAVE_SIGNAL_H')

add_includedirs(
    '.',
    './include',
    public= true
)