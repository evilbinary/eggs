# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("uuid")
set_kind("static")

add_packages("musl")

add_files(
    'uuid/compare.c',
    'uuid/gen_uuid.c',
    'uuid/isnull.c',
    'uuid/parse.c',
    'uuid/unpack.c',
    'uuid/clear.c',
    'uuid/copy.c',
    'uuid/pack.c',
    'uuid/unparse.c',
    'uuid/uuid_time.c'
)

add_includedirs(
'.',
'./uuid',
public = true
)