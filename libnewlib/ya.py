# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("newlib")
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
corss_compile=toolchains+'-'
cflags=get_cflags()

automake("newlib-c")

configure(
        'CC='+corss_compile+'gcc',
        'CROSS_COMPILE='+corss_compile,
        # '--target='+toolchains,
        '--host=arm',
        'CFLAGS="-DSYSCALL_NO_TLS '+ ' '.join(cflags)+'"' ,
        '--disable-newlib-supplied-syscalls',
        '--disable-nls',
        '--disable-optimize',
        '--enable-debug=yes',
        '--disable-thumb',
        '--disable-multilib',
        #'--enable-shared=yes',
        '--disable-shared',
        '--enable-static=yes'
)