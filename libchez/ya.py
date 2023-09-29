# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("chez")
set_kind("static")

add_packages("musl")
add_deps("ncurses","zlib","uuid","lz4")

build_type = 'a6le'
cflags = ''

arch_type= get_arch_type()

if arch_type == 'arm':
    build_type = 'arm32le'
    cflags = '-DDISABLE_X11 -DFEATURE_EXPEDITOR -DFEATURE_ICONV -DARMV6 -DINLINES -DGC_MACROS -DARMV6 -DMB_LEN_MAX=8 -D_LARGEFILE64_SOURCE -DHAVE_HIDDEN'
elif arch_type == 'x86':
    build_type = 'i3le'
    cflags = '-DI386 -DINLINES -DGC_MACROS  -DMB_LEN_MAX=8 -D_LARGEFILE64_SOURCE -DHAVE_HIDDEN'
else:
    print('libchez not support', arch_type)


add_files(
    'c/statics.c',
    'c/segment.c',
    'c/alloc.c',
    'c/symbol.c',
    'c/intern.c',
    'c/gcwrapper.c',
    'c/number.c',
    'c/schsig.c',
    'c/io.c',
    'c/new-io.c',
    'c/print.c',
    'c/fasl.c',
    'c/stats.c',
    'c/foreign.c',
    'c/prim.c',
    'c/prim5.c',
    'c/flushcache.c',
    'c/schlib.c',
    'c/thread.c',
    'c/expeditor.c',
    'c/scheme.c',
    'c/gc.c',
    'c/gc-ocd.c',
    'c/gc-oce.c',
    'c/gc-011.c',
    'c/arm32le.c',
    'c/compress-io.c'
)

add_cflags(cflags)
add_cflags(' -D__LIB_MUSL__ ')

add_includedirs(
    '.',
    '../include',
    './'+build_type,
    public = true
)