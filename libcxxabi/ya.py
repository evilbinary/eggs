# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("cxxabi")
set_kind("static")

add_packages("musl")

add_files(
    'src/*.cpp'
)

add_includedirs(
'.',
'./include',
'../libcxx/include',
public = true
)
add_cxxflags(
    '-ffreestanding -fpic',
    ' -std=c++14 -fno-use-cxa-atexit -fno-threadsafe-statics  -D_LIBCPP_HAS_MUSL_LIBC -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION  -D_USE_MATH_DEFINES  -D_LIBCPP_BUILDING_LIBRARY -D_POSIX_C_SOURCE  -D_LIBCPP_SAFE_STATIC   -D_GNU_SOURCE -DHAVE___CXA_THREAD_ATEXIT_IMPL -fpermissive -DLIBCXXRT  -D_LIBCPP_HAS_NO_THREADS -D_LIBCXXABI_HAS_NO_THREADS ',
    public = true
)


arch=get_arch()
def_arch=arch.replace("-", "_").upper()

arch_type=get_arch_type()
def_arch_type=arch_type.replace( "-", "_").upper()

add_defines(def_arch)
add_defines(def_arch_type)

