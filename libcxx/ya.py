# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("cxx")
set_kind("static")

add_packages(get_config('default_libc'))

add_deps("cxxabi")

add_files(
    'src/*.cpp',
    'src/filesystem/*.cpp'
)

# 这里曾有第三项 './include/experimental'，它排在 './include' 之后时无害，
# 但 target 的 -I 汇总顺序由依赖拓扑决定（多 target 合并时会错位），
# 一旦 experimental 反超 include，所有 #include <vector> 都会拿到
# experimental/vector，进而牵出 experimental/memory_resource ->
# __memory -> __functional_base -> typeinfo -> __availability，
# 后者 #if 里的 __has_feature 是 clang 语法，GCC 直接报
# "missing binary operator before token"。
# 实验头一律写 <experimental/xxx> 前缀，从 './include' 同样能解析到，
# 这项本来就多余，去掉以消除歧义。
add_includedirs(
'.',
'./include',
# '../../duck/libs/include',
)
add_cxxflags(
    '-ffreestanding -fpic',
            '-fpermissive',
                '-fno-use-cxa-atexit -fno-threadsafe-statics -D_LIBCPP_HAS_MUSL_LIBC -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION    -D_LIBCPP_BUILDING_LIBRARY -D_POSIX_C_SOURCE  -D_GNU_SOURCE -D_LIBCPP_HAS_NO_MONOTONIC_CLOCK  -D_LIBCPP_HAS_NO_THREADS -D_LIBCXXABI_HAS_NO_THREADS '
, public = true)


arch=get_arch()
def_arch=arch.replace("-", "_").upper()

arch_type=get_arch_type()
def_arch_type=arch_type.replace( "-", "_").upper()

add_defines(def_arch)
add_defines(def_arch_type)

