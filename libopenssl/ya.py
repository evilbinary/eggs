# coding:utf-8
# *******************************************************************
# Copyright 2021-present evilbinary
# 作者: evilbinary on 01/01/20
# 邮箱: rootdebug@163.com
# ********************************************************************
# libcrypto（OpenSSL 3.5）—— 只出 libcrypto.a，裸机 aarch64 + musl。
#
# 为什么不用 add_files 直接列 crypto/*.c：
#   OpenSSL 的源文件清单是由 Configure 按 target/裁剪项算出来的
#   （no-dso 要去掉 dso_dlfcn.c、no-threads 去掉 threads_pthread.c ……），
#   手抄必然编不过。这里照 eggs/libmusl/ya.py 的做法：
#     set_sourcedir(<out-of-tree 目录>) + automake('c') + configure(...)
#   由 ymake 驱动 OpenSSL 自己的 Configure / make install。
#
# 为什么需要 configure 包装脚本：
#   OpenSSL 的配置脚本叫 Configure（大写、首参是 target），而 ymake 的
#   automake 固定调用 sourcedir/configure。wrapper 负责补上 target 和裁剪项。
import os as _pyos

target("crypto")
set_kind("static")

# musl 的 public includedirs（include / obj/include / arch/*）
add_packages(get_config('default_libc'))

_ossl_src = _pyos.path.abspath(os.scriptdir())
_ossl_oot = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                            str(get_config('mode')), 'openssl-oot')
_pyos.makedirs(_ossl_oot, exist_ok=True)

# out-of-tree 构建：OpenSSL 3.x 支持在空目录里执行 <srcdir>/Configure。
# target 固定 linux-aarch64（aarch64 约定），再按裸机关掉：
#   no-dso    没有 dlopen
#   no-async  没有 ucontext
#   no-threads 裸机 gcc 不认 -pthread（第一处踩到的坑）
#   no-afalgeng  AF_ALG 是 Linux 内核 crypto API，需要 linux/version.h
#   no-comp / no-tests 不需要
_configure_wrapper = (
    '#!/bin/sh\n'
    'exec "%s/Configure" linux-aarch64 no-shared no-tests no-docs no-demos '
    'no-dso no-async no-threads no-comp no-afalgeng no-apps "$@"\n' % _ossl_src)
_cfg_path = _pyos.path.join(_ossl_oot, 'configure')
try:
    _old = open(_cfg_path, 'r').read()
except Exception:
    _old = None
if _old != _configure_wrapper:
    _w = open(_cfg_path, 'w')
    _w.write(_configure_wrapper)
    _w.close()
    _pyos.chmod(_cfg_path, 0o755)

# 【配置指纹】ymake 只用「产物是否存在」判断要不要重跑 automake，
# 不会因为裁剪项变化而重配。这里把 wrapper 内容存一份：变了才清掉
# configdata.pm/Makefile 强制重配；没变就不动，产物在就直接跳过
# （与 ffmpeg / musl 的行为一致，避免每次 ya 都重跑 Configure）。
_stamp = _pyos.path.join(_ossl_oot, '.configure.fingerprint')
try:
    _old_stamp = open(_stamp, 'r').read()
except Exception:
    _old_stamp = None
if _old_stamp != _configure_wrapper:
    for _f in ('configdata.pm', 'Makefile'):
        _p = _pyos.path.join(_ossl_oot, _f)
        if _pyos.path.exists(_p):
            _pyos.remove(_p)
    _s = open(_stamp, 'w')
    _s.write(_configure_wrapper)
    _s.close()

set_sourcedir(_ossl_oot)

# 头文件两处：源码树的 include/openssl/*.h + out-of-tree 生成的 configuration.h
add_includedirs(
    _pyos.path.join(_ossl_src, 'include'),
    _pyos.path.join(_ossl_oot, 'include'),
    public=True)

add_deps("gcc")

# 【顺序要紧】必须在 automake() 之前取工具链，否则 get_toolchain() 返回的是
# automake 这个伪工具链（实测会拼出 `automake-gcc: command not found`）。
# eggs/libmusl/ya.py 也是先取 toolchains 再 automake('c')。
_tool = get_toolchain()
_cc = _tool + '-gcc'
_ar = _tool + '-ar'
_ranlib = _tool + '-ranlib'

# 【产物名就是这里】ymake 用 prefix+name+.a = libcrypto.a 判断「是否已构建」，
# 和 --prefix=<build>/crypto/lib 下 make install 的真实产物对齐。写 "c" 会让它
# 去找 libc.a，永远找不到 → 每次都重跑 Configure（引以为戒）。
automake("crypto")

# musl 的头走【安装目录】：build/<plat>/<arch>/<mode>/musl/include。
# 注意不是 musl-oot（那是 musl 自己的 out-of-tree 构建目录，
# include/ 是空的，头由 make install 落到 musl/ 下）。
_musl_root = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                             str(get_config('mode')), 'musl')
_musl_inc = ' '.join([
    '-I' + _pyos.path.abspath(_pyos.path.join(_musl_root, 'include')),
])

configure(
    'CC=' + _cc,
    'AR=' + _ar,
    'RANLIB=' + _ranlib,
    'CFLAGS=' + _musl_inc,
)
