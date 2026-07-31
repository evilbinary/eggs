# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************

def add_flags():
    add_packages(get_config('default_libc'))
    add_cflags('-D__YIYIYA__')

    pass
add_buildin('add_flags',add_flags)

includes("./lib/ya.py")
# src/ya.py owns the yui static library (game/perf/input/backends).
includes("./src/ya.py")
