# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
# YiYiYa: ffmpeg 4.4 构建（供 app/sdl2/player 等使用）。
#
# 照 eggs/libmusl/ya.py 的 out-of-tree + configure 包装模式：
#   * ffmpeg 支持 out-of-source，但它的 configure 只要看到当前目录有
#     configure 就把 . 当源码目录（会去 sed ./libavcodec/... 而失败 ✗）；
#     已在 eggs/libffmpeg/configure 里加 .srcdir 支持（读本文件写入的路径）；
#   * ymake 的 automake 会在 sourcedir 里跑 ./configure + make install，
#     并自动加 --prefix=build/<...>/ffmpeg（产物安装到那里）；
#   * ymake 对 cli 默认带 -nostdinc/-nostdlib，ffmpeg 的编译器测试链接
#     必须显式给出 musl 的 include/库，否则报
#     "arm-none-eabi-gcc is unable to create an executable file" ✗。
target("ffmpeg")
set_kind('static')

import os as _pyos

_ff_src = _pyos.path.abspath(os.scriptdir())
_ff_oot = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                          str(get_config('mode')), 'ffmpeg-oot')
_pyos.makedirs(_ff_oot, exist_ok=True)

# configure 包装脚本：ymake 以 cwd=_ff_oot 调用 ./configure，转发给真源码。
_configure_wrapper = '#!/bin/sh\nexec "%s/configure" "$@"\n' % _ff_src
_cfg_path = _pyos.path.join(_ff_oot, 'configure')
try:
    _old_wrapper = open(_cfg_path, 'r').read()
except Exception:
    _old_wrapper = None
if _old_wrapper != _configure_wrapper:
    _w = open(_cfg_path, 'w')
    _w.write(_configure_wrapper)
    _w.close()
    _pyos.chmod(_cfg_path, 0o755)

# 真源码路径标记：ffmpeg configure 读到它就修正 source_path（见 configure 顶部注释）。
_srcdir_marker = _pyos.path.join(_ff_oot, '.srcdir')
try:
    _old_srcdir = open(_srcdir_marker, 'r').read().strip()
except Exception:
    _old_srcdir = None
if _old_srcdir != _ff_src:
    _w = open(_srcdir_marker, 'w')
    _w.write(_ff_src)
    _w.close()

set_sourcedir(_ff_oot)

# ffmpeg 的头在源码树里（libavcodec/avcodec.h 等），依赖方按
# <libavcodec/xxx.h> 引用，所以把源码根目录 public 出去。
add_includedirs('.', public = True)

add_packages(get_config('default_libc'))
add_deps("gcc")

# ---- musl 的 include / 库路径（-nostdinc/-nostdlib 下必须显式给）----
# 注意：get_build_obj_dir() 返回 build/<plat>/<arch>/<mode>/objs（下面有
# app/duck/eggs），而 musl-oot 在它的上一级 build/<plat>/<arch>/<mode>/ 下
# （ffbuild/config.log 实测：-I.../debug/objs/musl-oot/... 是错的 ✗）。
_build = _pyos.path.abspath(_pyos.path.dirname(get_build_obj_dir()))
_eggs = _pyos.path.abspath(_pyos.path.join(os.scriptdir(), '..'))
_musl_arch = {
    'arm64': 'aarch64',
    'arm': 'arm',
    'riscv': 'riscv64',
    'x86': 'i386',
}.get(get_arch_type(), 'arm')
_musl_inc = ' '.join(['-I' + p for p in [
    _pyos.path.join(_eggs, 'libmusl', 'include'),
    _pyos.path.join(_build, 'musl-oot', 'obj', 'include'),
    _pyos.path.join(_eggs, 'libmusl', 'arch', 'generic'),
    _pyos.path.join(_eggs, 'libmusl', 'arch', _musl_arch),
    _pyos.path.join(_eggs, 'include'),
]])
_musl_lib = _pyos.path.join(_build, 'musl-oot', 'lib')

# ffmpeg configure 的 --arch 名
ff_arch = {
    'arm64': 'aarch64',
    'arm': 'arm',
    'riscv': 'riscv',
    'x86': 'i386',
}.get(get_arch_type(), 'arm')

toolchains = get_toolchain()
compile = toolchains + '-'
cflags = get_cflags()
cflags_str = ' '.join(cflags) + ' ' + _musl_inc

# ffmpeg 的 CPU 探测用 $cpuflags（-mcpu=$cpu）；不给 --cpu 会得到空的
# "-mcpu="（config.log 实测：arm-none-eabi-gcc: missing argument to '-mcpu='
# ⇒ C compiler test failed ✗）。从 ymake 的架构 flags 里取真实值。
_cpu = ''
for _c in cflags:
    if _c.startswith('-mcpu=') and len(_c) > len('-mcpu='):
        _cpu = _c.split('=', 1)[1]
        break
if not _cpu:
    for _c in cflags:
        if _c.startswith('-march=') and len(_c) > len('-march='):
            _cpu = _c.split('=', 1)[1]
            break

# crt 在前（--extra-ldflags），库与 crtn 在后（--extra-libs），符合静态链接顺序。
# 注意：configure 的 test_ld 是 `$ld $LDFLAGS ...`，【不带 $CFLAGS】—— 所以
# LDFLAGS 里必须带上架构 flags（-mcpu/-mtune/-mfpu/-mfloat-abi）和 -nostdlib：
# crt1.o/libc.a 按 softfp 编译，缺 -mfloat-abi 会因 ABI 不匹配链接失败；
# 缺 -nostdlib 则 gcc 又会自动塞 newlib 的 crt0 与我们的 crt1.o 冲突 ✗。
# cflags_str 里两者都有，直接前置复用。
extra_ldflags = cflags_str + ' ' + ' '.join([
    '-static',
    '-L' + _musl_lib,
    _pyos.path.join(_musl_lib, 'crt1.o'),
    _pyos.path.join(_musl_lib, 'crti.o'),
])
extra_libs = ' '.join([
    '-L' + _musl_lib,
    '-lc', '-lgcc',
    _pyos.path.join(_musl_lib, 'crtn.o'),
])

# 注意：automake() 的参数会被 ymake 当作【产物名】（prefix+name+.a），
# 它也是"是否已构建过"的判定依据。要和 musl 同构：让产物名指向一个
# 【真实存在】的库 —— musl 写 "c"（→ musl/lib/libc.a，真实存在，所以不会
# 重复 configure）；这里写 "avcodec"（→ ffmpeg/lib/libavcodec.a，make install
# 真实产出）⇒ 第二次起 ymake 直接跳过，不再重复 configure/make install ✓。
automake("avcodec")
configure(
    '--cc=' + compile + 'gcc',
    '--ar=' + compile + 'ar',
    '--ranlib=' + compile + 'ranlib',
    '--cross-prefix=' + compile,
    '--enable-cross-compile',
    '--arch=' + ff_arch,
    *(['--cpu=' + _cpu] if _cpu else []),
    '--target-os=linux',
    '--extra-cflags=' + cflags_str,
    '--extra-ldflags=' + extra_ldflags,
    '--extra-libs=' + extra_libs,
    '--enable-static',
    '--disable-shared',
    '--disable-programs',
    '--disable-doc',
    '--disable-htmlpages',
    '--disable-manpages',
    '--disable-podpages',
    '--disable-txtpages',
    '--disable-stripping',
    '--disable-pthreads',
    '--disable-asm',
    '--disable-inline-asm',
    '--disable-vfp',
    # ---- 裁剪：player.c 只用到 avcodec/avformat/avutil/swscale(+swresample) ----
    '--disable-avdevice',
    '--disable-avfilter',
    '--disable-postproc',
    '--disable-avresample',
    # 保留 network（player 是 RTSP demo）。若链接期缺 socket 符号，
    # 改为 '--disable-network'（RTSP 用不了但能编过）。
)
