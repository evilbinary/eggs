# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libxml2 2.15.4（XML 解析核心）
#
# 布局：config.h + include/libxml/*.h 是【宿主 configure 产物】，其余是上游源码。
# 生成方式（在本目录外做一次即可，产物已提交）：
#   tar -xf libxml2-2.15.4.tar.xz
#   cd libxml2-2.15.4 && ./configure --disable-shared --enable-static \
#       --without-python --without-iconv --without-http --without-zlib
#   cp config.h ../../../eggs/libxml2/
#   cp include/libxml/xmlversion.h ../../../eggs/libxml2/include/libxml/
#
# 编的是上游 Makefile.am 的 libxml2_la_SOURCES 核心集 + 常用的
# catalog/xmlsave/xpath/xmlregexp/pattern/xmlreader/debugXML/HTML/xmlwriter/
# xinclude。刻意没编的：nanohttp（网络）、xmlmodule（dlopen）、relaxng /
# xmlschemas / schematron（体积大且没人用）。
target("xml2")
set_kind("static")

add_packages(get_config('default_libc'))

add_files('src/*.c')

add_cflags('-DHAVE_CONFIG_H')

# 【public 只给 './include'】'.' 里有 config.h、'./src' 里有内部的 libxml.h：
# 它们如果也 public，下游库（cairo 等）源码里的 #include "config.h" 会先命中
# 这份 libxml2 的 config.h，把对方的 HAVE_* 全顶成 0（实测 cairo 因此
# 认不出 stdint.h 与 pthread 互斥锁）。
add_includedirs('./include', public=True)
add_includedirs('.', './src')
