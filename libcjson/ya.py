# coding:utf-8
# *******************************************************************
# * Copyright 2025-present evilbinary
# * 作者: evilbinary on 01/20/2025
# * 邮箱: rootntsd@gmail.com
# ********************************************************************
target("cjson")
add_packages(get_config('default_libc'))
set_kind("static")
add_files(
    'cJSON_Utils.c',
    'cJSON.c',
    # 'cJSON.h'
)
add_includedirs(
    '.',
    '../include',
    './src',
    public = true
)

