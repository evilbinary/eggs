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
#   no-afalgeng AF_ALG 是 Linux 内核 crypto API，要 linux/version.h
#   no-apps     不编 openssl 命令行（裸机链接必失败）
#   no-docs / no-demos  跳过 man 手册（几千个文件，install 里最慢的一步）
#
# 【不能关 no-threads】曾经因为裸机 gcc 不认 `-pthread` 就加了 no-threads，
# 结果 OpenSSL 的 CRYPTO_THREAD_run_once 被编成空函数 ⇒ OPENSSL_init_crypto
# 的自动初始化永远不跑 ⇒ RAND_status()/RAND_bytes() 访问未初始化的全局
# libctx 直接段错误。OpenSSH 的 main() 第一步就是 seed_rng()，于是
# `sshd -h/-V/-E` 全部零输出、进程立即退出（查了很久的「sshd 没反应」）。
# 正确做法：保留线程支持，把 `-pthread` 换成等价的 `-D_REENTRANT`
# （我们的 musl 是单库，pthread 就在 libc 里，不需要额外链接）。
_configure_wrapper = (
    '#!/bin/sh\n'
    '"%s/Configure" linux-aarch64 no-shared no-tests no-docs no-demos '
    'no-dso no-async no-comp no-afalgeng no-apps "$@" || exit $?\n'
    '# 裸机 gcc 不认 glibc 的 driver 选项 -pthread，替换成等价宏\n'
    'sed -i "" "s/-pthread/-D_REENTRANT/g" Makefile\n'
    '# 【去掉 -fPIC】threads 模式下 OpenSSL 给所有 .o 加 -fPIC，静态链接进\n'
    '# ET_EXEC 会产生 R_AARCH64_RELATIVE 重定位；我们的 ELF 加载器不做这部分\n'
    '# 重定位，GOT 里是野指针 —— 表现为「刚进函数就段错误」（OPENSSL_init_crypto\n'
    '# 无输出退出）。改成 -fno-PIC 让代码按非 PIC 生成。\n'
    'sed -i "" "s/-fPIC/-fno-PIC/g" Makefile\n'
    'exit 0\n' % _ossl_src)
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

    # 源头指纹（文件名 + 大小）：没变就直接返回。
    # ymake 在「产物已存在」分支里也会调用 after_build（automake.py:54-60），
    # 所以钩子每次 ya 都会跑；没有指纹的话每次都白拷 169 个文件。
    _srcs = []
    for _d in (_pyos.path.join(_ossl_src, 'include/openssl'),
               _pyos.path.join(_ossl_oot, 'include/openssl')):
        if not _pyos.path.isdir(_d):
            continue
        for _f in sorted(_pyos.listdir(_d)):
            _s = _pyos.path.join(_d, _f)
            if _pyos.path.isfile(_s) and _f.endswith('.h'):
                _srcs.append((_f, _s))
    _sig = ','.join(['%s:%d' % (_f, _pyos.path.getsize(_s))
                     for _f, _s in _srcs])
    _stamp_h = _pyos.path.join(_dst, '.headers.stamp')
    try:
        _old_h = open(_stamp_h, 'r').read()
    except Exception:
        _old_h = None
    if _old_h == _sig and _pyos.path.exists(_pyos.path.join(_dst, 'opensslv.h')):
        return

    _n = 0
    for _f, _s in _srcs:
        _shutil.copyfile(_s, _pyos.path.join(_dst, _f))
        _n += 1
    _hs = open(_stamp_h, 'w')
    _hs.write(_sig)
    _hs.close()
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
