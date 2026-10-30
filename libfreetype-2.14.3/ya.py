# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# freetype 2.14.3
#
# 【源码树是上游原样，一个文件都不删】
# 编译列表 = 上游 single-object 构建的那套文件：
#   - 每个模块的"聚合文件"（autofit/autofit.c、base/ftbase.c、sfnt/sfnt.c…）
#     它们用 #include "xxx.c" 把同目录实现串起来；成员 .c 不能再单独编，
#     否则同一符号出现两次（multiple definition）。
#   - base/ 下不属于 ftbase.c 聚合的"扩展"文件（ftinit/ftsystem/ftbitmap…）
#   - gzip 模块只编 ftgzip.c：zlib 用 eggs/libzlib（见 ftoption.h 里的
#     FT_CONFIG_OPTION_SYSTEM_ZLIB），所以 src/gzip 里那份 zlib 副本
#     （inflate.c/adler32.c/crc32.c/zutil.c…）不用编。
# 刻意没编：gxvalid/gxvfgen.c（上游的开发期生成器，有 main）。pcf/bdf/sdf/svg
# 这几个模块的目录也没纳入，ftmodule.h 里对应 FT_USE_MODULE 已注释。
target("freetype")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps("png", "zlib")

add_files(
    'src/autofit/autofit.c',
    'src/base/ftbase.c',
    'src/base/ftbbox.c',
    'src/base/ftbdf.c',
    'src/base/ftbitmap.c',
    'src/base/ftcid.c',
    'src/base/ftdebug.c',
    'src/base/ftfstype.c',
    'src/base/ftgasp.c',
    'src/base/ftglyph.c',
    'src/base/ftgxval.c',
    'src/base/ftinit.c',
    'src/base/ftmm.c',
    'src/base/ftotval.c',
    'src/base/ftpatent.c',
    'src/base/ftpfr.c',
    'src/base/ftstroke.c',
    'src/base/ftsynth.c',
    'src/base/ftsystem.c',
    'src/base/fttype1.c',
    'src/base/ftwinfnt.c',
    'src/cache/ftcache.c',
    'src/cff/cff.c',
    'src/cid/type1cid.c',
    'src/gxvalid/gxvalid.c',
    'src/gzip/ftgzip.c',
    'src/otvalid/otvalid.c',
    'src/pfr/pfr.c',
    'src/psaux/psaux.c',
    'src/pshinter/pshinter.c',
    'src/psnames/psnames.c',
    'src/raster/raster.c',
    'src/sfnt/sfnt.c',
    'src/smooth/smooth.c',
    'src/truetype/truetype.c',
    'src/type1/type1.c',
    'src/type42/type42.c',
    'src/winfonts/winfnt.c',
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
    public = True
)
