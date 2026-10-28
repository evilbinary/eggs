# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# pixman 0.42.2（静态库，只出库；demos/tests 不编）。
#
# 不走 meson/configure：config.h 手工给（见同目录 config.h 顶部说明），
# 与 libwayland / libevdev / libinput 的做法一致。
#
# 文件清单 = pixman/Makefile.sources 的 libpixman_sources（通用 C 路径）。
# 不能整目录 glob：SIMD 实现文件（sse2/ssse3/mmx/vmx/arm-neon/arm-simd/
# mips-dspr2）在 0.42 里没有 arch 宏包整个文件，会直接去 include
# <xmmintrin.h>/<altivec.h> 而失败；pixman-region.c 是 region16/32 的模板，
# 由后者 #include，不能单独编译。
#
# 使用方：libweston 的 pixman-renderer。
target("pixman")
set_kind("static")

add_packages(get_config('default_libc'))

# pixman-private.h 在未定义 HAVE_CONFIG_H 时会 #error（要求先包含 config.h）
add_defines("HAVE_CONFIG_H")

add_files(
    'pixman/pixman.c',
    'pixman/pixman-access.c',
    'pixman/pixman-access-accessors.c',
    'pixman/pixman-bits-image.c',
    'pixman/pixman-combine32.c',
    'pixman/pixman-combine-float.c',
    'pixman/pixman-conical-gradient.c',
    'pixman/pixman-filter.c',
    # 架构选择器：内部由 USE_* 宏包着，未启用时退化成空实现
    'pixman/pixman-x86.c',
    'pixman/pixman-mips.c',
    'pixman/pixman-arm.c',
    'pixman/pixman-ppc.c',
    'pixman/pixman-edge.c',
    'pixman/pixman-edge-accessors.c',
    'pixman/pixman-fast-path.c',
    'pixman/pixman-glyph.c',
    'pixman/pixman-general.c',
    'pixman/pixman-gradient-walker.c',
    'pixman/pixman-image.c',
    'pixman/pixman-implementation.c',
    'pixman/pixman-linear-gradient.c',
    'pixman/pixman-matrix.c',
    'pixman/pixman-noop.c',
    'pixman/pixman-radial-gradient.c',
    'pixman/pixman-region16.c',
    'pixman/pixman-region32.c',
    'pixman/pixman-solid-fill.c',
    'pixman/pixman-timer.c',
    'pixman/pixman-trap.c',
    'pixman/pixman-utils.c',
)

add_includedirs(
    '.',          # config.h
    './pixman',   # pixman.h / pixman-version.h
    public=True,
)
