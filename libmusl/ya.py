# coding:utf-8
# *******************************************************************
# * Copyright 2021-present evilbinary
# * 作者: evilbinary on 01/01/20
# * 邮箱: rootdebug@163.com
# ********************************************************************
target("musl")
set_kind('static')

# ---- 源码树外构建（obj/lib/config.mak 都落在 build/ 下）----
# musl 支持 out-of-source 构建：在当前目录执行 <srcdir>/configure 会生成 config.mak
# 和指向 <srcdir>/Makefile 的软链，obj/ 也建在当前目录（configure:812）。
# 而 ymake 的 automake 固定在 sourcedir 里执行 ./configure + make，所以这里把
# sourcedir 指向一个 per-arch 的 build 目录，并放一个 configure 包装脚本转发到真正的
# musl configure。
# 好处：
#   * obj 在 build/ 下，源码树（submodule）不被污染；
#   * 每个 plat/arch 各自一份 obj，切 arch 不需要清理/重编；
#   * 不需要任何批量删除。
import os as _pyos

_musl_src = _pyos.path.abspath(os.scriptdir())
_musl_oot = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                            str(get_config('mode')), 'musl-oot')
_pyos.makedirs(_musl_oot, exist_ok=True)

# configure 包装脚本：ymake 会以 cwd=_musl_oot 调用它，转发给真正的 musl configure，
# musl configure 会自动把 srcdir 识别为脚本所在目录（eggs/libmusl）。
_configure_wrapper = '#!/bin/sh\nexec "%s/configure" "$@"\n' % _musl_src
_cfg_path = _pyos.path.join(_musl_oot, 'configure')
try:
    _old_wrapper = open(_cfg_path, 'r').read()
except Exception:
    _old_wrapper = None
if _old_wrapper != _configure_wrapper:
    _w = open(_cfg_path, 'w')
    _w.write(_configure_wrapper)
    _w.close()
    _pyos.chmod(_cfg_path, 0o755)

set_sourcedir(_musl_oot)

musl_arch = {
    'arm64': 'aarch64',
    'arm': 'arm',
    'riscv': 'riscv64',
    'x86': 'i386',
}.get(get_arch_type(), 'arm')

add_includedirs(
    "include",
    "obj/include",
    "arch/generic",
    "arch/" + musl_arch,
    "eggs/include",
    public = true
)
add_cflags('-Wno-error=pointer-sign','-DSYSCALL_NO_TLS',public = true)

add_deps("gcc")

toolchains=get_toolchain()
compile=toolchains+'-'
cflags=get_cflags()

automake("c")

# argv list is not shell-parsed: do not wrap CFLAGS value in quotes
cflags_str = ' '.join(
    ['-DSYSCALL_NO_TLS'] + [c for c in cflags if c != '-DSYSCALL_NO_TLS']
)

configure(
        '--target='+compile,
        'CROSS_COMPILE='+compile,
        'CFLAGS=' + cflags_str,
        '--disable-optimize',
        '--enable-debug=yes',
        #'--enable-shared=yes',
        '--disable-shared',
        '--enable-static=yes'
)
