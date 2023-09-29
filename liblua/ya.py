# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("lua")
set_kind("static")

add_packages("musl")

add_files(
    "src/*.c"
)

add_includedirs(
    '.',
    '../include',
    './src',
    public = true
)