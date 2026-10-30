# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# freetype 2.14.3
#
# 目录结构与上游一致：include/ 是公开头（含 config/ftoption.h 等配置头，
# 已手工改过，见下面的注释），src/ 只放了真正要编的模块子目录。
#   - 编入：autofit base cache cff cid gxvalid otvalid pfr psaux pshinter
#           psnames raster sfnt smooth truetype type1 type42 winfonts
#   - 未编入（ftmodule.h 里已注释掉对应 FT_USE_MODULE，否则引用到不存在的
#     xxx_driver_class 会变成未定义符号）：pcf / bdf / sdf / svg
#   - ftoption.h：开了 ZLIB（sfnt 的 PNG 位图字体 + gzip 压缩字体需要）、
#     LZW，关掉 SVG（svg 渲染器没编）。
target("freetype")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("png", "zlib")

add_files(
    'src/**/*.c'
)

# 内部头（src/base/ftcalc.h 之类）和公开头的根
add_cflags(
    '-DFT2_BUILD_LIBRARY',
    '-DFT_CONFIG_OPTION_USE_PNG',
)

add_includedirs(
    '.',
    './include',
    './src',
    public = true
)
