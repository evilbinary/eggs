# coding:utf-8
# *******************************************************************
# * Copyright 2025-present evilbinary
# * 作者: evilbinary on 01/20/2025
# * 邮箱: rootntsd@gmail.com
# ********************************************************************
target("yaml")
add_packages(get_config('default_libc'))

set_kind("static")
add_files(
    '*.c',
) 
add_includedirs('.')
add_cflags(' -I.')
