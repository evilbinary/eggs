# coding:utf-8
# *******************************************************************
# * Copyright 2025-present evilbinary
# * 作者: evilbinary on 01/20/2025
# * 邮箱: rootntsd@gmail.com
# ********************************************************************

target("jsmodule-quickjs")
add_packages(get_config('default_libc'))

add_deps("quickjs","cjson","yui","socket")
add_cflags(' -DBUILD_NO_MAIN=1  -I. -I../../lib/quickjs -g ')


set_kind("static")
add_files(
    'js_module.c',
    'js_socket.c',
    '../jsmodule/js_common.c'
) 
add_includedirs('.', public=true)