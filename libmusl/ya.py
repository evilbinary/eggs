# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("musl")
set_kind('static')
set_sourcedir(os.scriptdir())

musl_arch = {
    'arm64': 'aarch64',
    'arm': 'arm',
    'riscv': 'riscv64',
    'x86': 'i386',
}.get(get_arch_type(), 'arm')

add_includedirs(
    "include",
    "obj/include",
    "arch/generic",
    "arch/" + musl_arch,
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
