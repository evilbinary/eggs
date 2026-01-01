# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
includes("./lib/ya.py")

target("yui")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("cjson","sdl2","sdl2-ttf","sdl2-image")

set_kind('static')
add_files("src/*.c")
add_files("src/components/*.c")

add_cflags(' -DSDL2 ')

add_includedirs(
'.',
'./src',
public = true
)

