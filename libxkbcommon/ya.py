# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# libxkbcommon 1.7.0 核心库（静态；不走 meson）。
#
# 文件清单 = 上游 meson.build 的 libxkbcommon_sources。
#   - src/xkbcomp/parser.c / .h 是 bison 产物，已用宿主端生成好放进源码目录
#     （命令：bison --defines=src/xkbcomp/parser.h -o src/xkbcomp/parser.c \
#            -p _xkbcommon_ src/xkbcomp/parser.y）
#   - src/xkbcomp/keywords.c 上游仓库自带（gperf 产物）
#   - 不含 libxkbregistry（需要 libxml2）与 xkbcommon-x11（需要 xcb）
#
# 注意 src/compose/parser.c 是 Compose 文件的hand-written 解析器，
# 与 xkbcomp 的 bison 产物同名但无关，两者都要编。
#
# 使用方：libweston（键盘 keymap/keysym 翻译）。
target("xkbcommon")
set_kind("static")

add_packages(get_config('default_libc'))

# asprintf/vasprintf/strndup/eaccess/secure_getenv 在 musl 里需要 _GNU_SOURCE
add_defines("_GNU_SOURCE")

add_files(
    # 组合键（Compose）支持
    'src/compose/parser.c',
    'src/compose/paths.c',
    'src/compose/state.c',
    'src/compose/table.c',
    # xkbcomp：keymap 文本解析
    'src/xkbcomp/action.c',
    'src/xkbcomp/ast-build.c',
    'src/xkbcomp/compat.c',
    'src/xkbcomp/expr.c',
    'src/xkbcomp/include.c',
    'src/xkbcomp/keycodes.c',
    'src/xkbcomp/keymap.c',
    'src/xkbcomp/keymap-dump.c',
    'src/xkbcomp/keywords.c',
    'src/xkbcomp/parser.c',
    'src/xkbcomp/rules.c',
    'src/xkbcomp/scanner.c',
    'src/xkbcomp/symbols.c',
    'src/xkbcomp/types.c',
    'src/xkbcomp/vmod.c',
    'src/xkbcomp/xkbcomp.c',
    # 核心
    'src/atom.c',
    'src/context.c',
    'src/context-priv.c',
    'src/keysym.c',
    'src/keysym-utf.c',
    'src/keymap.c',
    'src/keymap-priv.c',
    'src/state.c',
    'src/text.c',
    'src/utf8.c',
    'src/utils.c',
)

# '.'（config.h 所在）保持私有：否则依赖方的 #include "config.h" 会命中
# 这里的 config.h，把自己的构建配置顶掉。
add_includedirs('./src', './include', public=True)
add_includedirs('.')
