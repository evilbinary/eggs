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

# 动态链接器文件名由 support.musl_ldso_name() 统一给出（arm32→ld-musl-arm.so.1、
# arm64→ld-musl-aarch64.so.1、riscv→ld-musl-riscv64.so.1、x86→ld-musl-i386.so.1），
# 与 xenv/env.py 里写进 PT_INTERP 的路径保持同一来源。
try:
    import support as _support
    _ldso_name = _support.musl_ldso_name(get_arch_type())
except Exception:
    _ldso_name = 'ld-musl-' + musl_arch + '.so.1'

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
        # 同时出动态库：libc.so（就是镜像里的 /lib/<ldso 名字>，
        # 内核 loader 的 PT_INTERP 会加载它，dlopen/dlsym 由其中的 ldso 提供真实现）。
        # 静态 libc.a 照旧生成，其它程序继续静态链接。
        '--enable-shared',
        '--enable-static=yes',
        # musl 默认把 ld-musl-<arch>.so.1 装到 $(syslibdir)=/lib（宿主机路径），
        # 装进构建目录里，再由下面的部署逻辑拷到 app/resource/lib 供镜像使用。
        '--syslibdir=' + _pyos.path.abspath(
            _pyos.path.join('build', str(get_plat()), str(get_arch()),
                            str(get_config('mode')), 'musl', 'lib'))
)


# 动态链接器落到镜像的 /lib 下：内核 loader 按 PT_INTERP 里的
# "/lib/ld-musl-<arch>.so.1" 打开它，dlopen 加载的 .so 也由它解析。
# （musl 的 libc.so 本身就是动态链接器：入口是 ldso 的 _dlstart。）
_dyn_loader_dst_dir = _pyos.path.join('app', 'resource', 'lib')
_musl_install_lib = _pyos.path.join('build', str(get_plat()), str(get_arch()),
                                    str(get_config('mode')), 'musl', 'lib')

# 优先用 musl 自己装出来的名字（SUBARCH 之类若有差异也自动跟上），
# 再回退到按架构拼出来的名字 + 直接取 libc.so。
_dyn_loader_srcs = []
for _dir in (_musl_install_lib, _pyos.path.join(_musl_oot, 'lib')):
    _d = _pyos.path.join(_dir, _ldso_name)
    if _pyos.path.exists(_d):
        _dyn_loader_srcs.append((_d, _ldso_name))
        break
_dyn_loader_srcs += [
    (_pyos.path.join(_musl_install_lib, 'libc.so'), _ldso_name),
    (_pyos.path.join(_musl_oot, 'lib', 'libc.so'), _ldso_name),
]

for _src, _name in _dyn_loader_srcs:
    if not _pyos.path.exists(_src):
        continue
    _pyos.makedirs(_dyn_loader_dst_dir, exist_ok=True)
    _dst = _pyos.path.join(_dyn_loader_dst_dir, _name)
    if (not _pyos.path.exists(_dst) or
            _pyos.path.getmtime(_src) > _pyos.path.getmtime(_dst)):
        import shutil as _shutil
        _shutil.copyfile(_src, _dst)
        print('copy dynamic loader {} -> {}'.format(_src, _dst))
    break
else:
    print('warn: {} not built yet, skip dynamic loader deploy'.format(_ldso_name))
