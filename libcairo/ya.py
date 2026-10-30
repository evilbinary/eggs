# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# cairo 1.18.6
#
# 目录布局：src/ 里既放源码也放公开头（cairo 上游就是这么平铺的），
#   src/cairo-features.h 与顶层 config.h 是【手工生成】的（上游由 meson 的
#   configure_file() 产出，没有 .h.in 模板），内容见这两个文件里的注释。
#
# 编入的文件 = 上游 src/meson.build 的 cairo_sources（核心）+ 这些后端：
#   cairo-png / cairo-ft / cairo-svg / cairo-pdf / cairo-ps / cairo-script /
#   cairo-tee
# 刻意不编：xlib / xcb / quartz / win32 / gl / egl / vg / drm（本系统没有
#   对应窗口系统；weston 的 pixman 渲染器只用 image surface）。
# 依赖：pixman（image 后端）、freetype（字体）、png+zlib（png 读写、
#   pdf/ps/script 的压缩流）、xml2（SVG surface 的 XML 解析）。
target("cairo")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("pixman", "freetype", "png", "zlib", "xml2")

add_files('src/*.c')

add_cflags('-DHAVE_CONFIG_H')

add_includedirs(
    '.',        # config.h
    './src',    # <cairo.h> / cairoint.h / cairo-features.h
    public=True
)
