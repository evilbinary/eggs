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
#     set_sourcedir(<out-of-tree 目录>) + automake('crypto') + configure(...)
#   由 ymake 驱动 OpenSSL 自己的 Configure / make install。
import os as _pyos

target("crypto")
set_kind("static")

# musl 的 public includedirs（include / obj/include / arch/*）
add_packages(get_config('default_libc'))

_ossl_src = _pyos.path.abspath(os.scriptdir())
_ossl_oot = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                            str(get_config('mode')), 'openssl-oot')
_pyos.makedirs(_ossl_oot, exist_ok=True)

# --prefix 指向的位置（make install 的落地目录，也是 .a 的判定路径）
_ossl_prefix = _pyos.path.abspath(_pyos.path.join(
    'build', str(get_plat()), str(get_arch()), str(get_config('mode')),
    'crypto', 'lib'))

# musl 的头走【安装目录】：build/<plat>/<arch>/<mode>/musl/include。
# 不是 musl-oot/include（那是 musl 自己的 out-of-tree 构建目录，里面是空的）。
_musl_root = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                             str(get_config('mode')), 'musl')
_musl_inc = '-I' + _pyos.path.abspath(_pyos.path.join(_musl_root, 'include'))

# out-of-tree 构建：OpenSSL 3.x 支持在空目录里执行 <srcdir>/Configure。
# 裸机关掉的项：
#   no-dso      没有 dlopen
#   no-async    没有 ucontext
#   no-threads  裸机 gcc 不认 -pthread
#   no-afalgeng AF_ALG 是 Linux 内核 crypto API，要 linux/version.h
#   no-apps     不编 openssl 命令行（裸机链接必失败）
#   no-docs / no-demos  跳过 man 手册（几千个文件，install 里最慢的一步）
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

# 配置指纹：ymake 只用「产物是否存在」判断要不要重跑 automake，不会因为
# 裁剪项变化而重配。这里把 wrapper 存一份：变了才清 configdata.pm/Makefile。
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

# 头文件两处：源码树的 include/openssl/*.h + out-of-tree 生成的 *.h
add_includedirs(
    _pyos.path.join(_ossl_src, 'include'),
    _pyos.path.join(_ossl_oot, 'include'),
    public=True)


def _install_headers(_target):
    """把 openssl 头补到 prefix/include/openssl。

    为什么需要：`make install` 在装 cmake/pkgconfig 配置时就会失败
    （`cp: exporters/OpenSSLConfig.cmake: No such file or directory`），
    后面的 install_dev 没执行 —— 结果 lib/libcrypto.a 有了、头却没有，
    OpenSSH 的 configure 就报 "OpenSSL headers missing"。
    钩子在「产物已存在」时也会被 ymake 调用（automake.py:54-60），所以幂等。

    【注册位置】after_build() 挂到「当前 node」，automake() 会 node_start
    一个新节点，所以必须在 automake() 之前注册，否则 ymake 在 target 节点上
    找不到它、函数永远不执行。
    """
    import shutil as _shutil
    _dst = _pyos.path.join(_ossl_prefix, 'include', 'openssl')
    _pyos.makedirs(_dst, exist_ok=True)
    _n = 0
    for _d in (_pyos.path.join(_ossl_src, 'include/openssl'),
               _pyos.path.join(_ossl_oot, 'include/openssl')):
        if not _pyos.path.isdir(_d):
            continue
        for _f in _pyos.listdir(_d):
            _s = _pyos.path.join(_d, _f)
            if _pyos.path.isfile(_s) and _f.endswith('.h'):
                _shutil.copyfile(_s, _pyos.path.join(_dst, _f))
                _n += 1
    print('openssl: installed {} headers -> {}'.format(_n, _dst))


after_build(_install_headers)

add_deps("gcc")

# 【顺序要紧】必须在 automake() 之前取工具链，否则 get_toolchain() 返回的是
# automake 这个伪工具链（实测会拼出 `automake-gcc: command not found`）。
_tool = get_toolchain()

# 【产物名】automake 的参数就是产物名（lib<name>.a），也是「是否已构建」的
# 判定依据。写 "c" 会让 ymake 去找 libc.a，永远找不到 → 每次都重跑 Configure。
automake("crypto")

configure(
    'CC=' + _tool + '-gcc',
    'AR=' + _tool + '-ar',
    'RANLIB=' + _tool + '-ranlib',
    'CFLAGS=' + _musl_inc,
)
