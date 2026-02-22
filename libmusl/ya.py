# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("musl")
set_kind('static')
set_sourcedir(os.scriptdir())

add_includedirs(
    "include",
    "eggs/include",
    public = true
)
add_cflags('-Wno-error=pointer-sign','-DSYSCALL_NO_TLS',public = true)

add_deps("gcc")

toolchains=get_toolchain()
compile=toolchains+'-'
cflags=get_cflags()

automake("c")

configure(
        '--target='+compile,
        'CROSS_COMPILE='+compile,
        'CFLAGS="-DSYSCALL_NO_TLS '+ ' '.join(cflags)+'"' ,
        '--disable-optimize',
        '--enable-debug=yes',
        #'--enable-shared=yes',
        '--disable-shared',
        '--enable-static=yes'
)