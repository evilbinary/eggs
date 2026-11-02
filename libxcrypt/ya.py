# coding:utf-8
# *******************************************************************
# Copyright 2021-present evilbinary
# 作者: evilbinary on 01/01/20
# 邮箱: rootdebug@163.com
# ********************************************************************
# libcrypt（libxcrypt 4.5.2）—— 给 OpenSSH 的 crypt(3) 用。
#
# 为什么需要：OpenSSH 的密码认证在 auth-passwd.c 里调 `crypt(password, salt)`
# 校验 /etc/shadow。musl 刻意不实现 crypt，所以必须单独提供 libcrypt.a。
# （注意 libcrypt 和 OpenSSL 的 libcrypto 是两个完全不同的库。）
#
# 构建方式和 eggs/libopenssl 完全一致：out-of-tree + automake + configure。
import os as _pyos

target("crypt")
set_kind("static")

add_packages(get_config('default_libc'))

_xc_src = _pyos.path.abspath(os.scriptdir())
_xc_oot = _pyos.path.abspath(_pyos.path.join(
    'build', str(get_plat()), str(get_arch()), str(get_config('mode')),
    'libxcrypt-oot'))
_pyos.makedirs(_xc_oot, exist_ok=True)

_xc_prefix = _pyos.path.abspath(_pyos.path.join(
    'build', str(get_plat()), str(get_arch()), str(get_config('mode')),
    'crypt', 'lib'))

_musl_root = _pyos.path.abspath(_pyos.path.join(
    'build', str(get_plat()), str(get_arch()), str(get_config('mode')), 'musl'))
_musl_inc = _pyos.path.join(_musl_root, 'include')
_musl_lib = _pyos.path.join(_musl_root, 'lib')

# 裸机裁剪：
#   --disable-shared        只要静态库
#   --enable-hashes=strong  只留 SHA-256/512、yescrypt 这类强 hash，
#                           去掉 DES/MD5（既小又快，且 glibc 默认也是 $6$）
#   --disable-werror        交叉编译时 configure 会试各种特性，别让警告变错误
_configure_wrapper = (
    '#!/bin/sh\n'
    '# 【已配置就跳过】ymake 只认产物 libcrypt.a，配置成功后如果没有这个文件，\n'
    '# 每次 ya 都会再调一次本脚本。用 config.status/Makefile 挡住。\n'
    'if [ -f config.status ] && [ -f Makefile ]; then\n'
    '    echo "libxcrypt: already configured, skip"\n'
    '    exit 0\n'
    'fi\n'
    + (('"%s/configure" ' % _xc_src) +
       '--host=%s --disable-shared --enable-static '
       '--enable-hashes=strong --disable-werror "$@" || exit $?\n'
       % get_toolchain())
    +
    # 【给 libtool 去掉 crt】crt1.o/crti.o/crtn.o 只用于让 configure 的
    # 「C compiler works」测试能链接通过；libtool 建 .la 时要求所有输入都是
    # 自己的 .lo 对象，见到 .o 就报
    #   cannot build libtool library from non-libtool objects
    'sed -i "" -e "s|[^ ]*/crt1\\\\.o||g" -e "s|[^ ]*/crti\\\\.o||g" '
    '-e "s|[^ ]*/crtn\\\\.o||g" Makefile\n'
    'exit 0\n')

_cfg_path = _pyos.path.join(_xc_oot, 'configure')
try:
    _old = open(_cfg_path, 'r').read()
except Exception:
    _old = None
if _old != _configure_wrapper:
    _w = open(_cfg_path, 'w')
    _w.write(_configure_wrapper)
    _w.close()
    _pyos.chmod(_cfg_path, 0o755)

# 配置指纹（只绑参数，改注释不重配）
_cfg_args = ('--host=%s --enable-hashes=strong' % get_toolchain())
_stamp = _pyos.path.join(_xc_oot, '.configure.fingerprint')
try:
    _old_stamp = open(_stamp, 'r').read()
except Exception:
    _old_stamp = None

set_sourcedir(_xc_oot)

add_includedirs(_pyos.path.join(_xc_src, 'crypt'), public=True)

add_deps("gcc")
_tool = get_toolchain()

_cflags = '-I' + _musl_inc + ' -O2 -fno-strict-aliasing'
_ldflags = ' '.join([
    '-nostdlib', '-static', '-L' + _musl_lib,
    _pyos.path.join(_musl_lib, 'crt1.o'),
    _pyos.path.join(_musl_lib, 'crti.o'),
])
_libs = ' '.join([
    '-L' + _musl_lib, '-lc', '-lgcc',
    _pyos.path.join(_musl_lib, 'crtn.o'),
])

_fp = '|'.join([_cfg_args, _cflags, _ldflags, _libs, 'v1'])
if _old_stamp != _fp:
    for _f in ('config.status', 'Makefile'):
        _p = _pyos.path.join(_xc_oot, _f)
        if _pyos.path.exists(_p):
            _pyos.remove(_p)
    _s = open(_stamp, 'w')
    _s.write(_fp)
    _s.close()


def _install_headers(_target):
    """libxcrypt 的 make install 会装 crypt.h；若中途失败就手动补一份。"""
    import shutil as _shutil
    _dst_dir = _pyos.path.join(_xc_prefix, 'include')
    _pyos.makedirs(_dst_dir, exist_ok=True)
    for _name in ('crypt.h',):
        _s = _pyos.path.join(_xc_src, 'crypt', _name)
        if _pyos.path.isfile(_s):
            _shutil.copyfile(_s, _pyos.path.join(_dst_dir, _name))


after_build(_install_headers)

automake("crypt")

configure(
    'CC=' + _tool + '-gcc',
    'AR=' + _tool + '-ar',
    'RANLIB=' + _tool + '-ranlib',
    'CFLAGS=' + _cflags,
    'CPPFLAGS=-I' + _musl_inc,
    'LDFLAGS=' + _ldflags,
    'LIBS=' + _libs,
)
