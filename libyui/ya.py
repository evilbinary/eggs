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

if get_plat() =="stm32":
    add_files("src/backend/backend_stm32.c")
    # 添加STM32特定的编译标志
    add_cflags("-DSTM32_PLATFORM")
    # 添加STM32特定的依赖
    # add_deps("stm32_hal")  # 如果需要的话
else:
    # 默认使用SDL后端
    add_files("src/backend/backend_sdl.c")


add_cflags(' -DSDL2 ')

add_includedirs(
'.',
'./src',
public = true
)

