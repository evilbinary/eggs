# coding:utf-8
# libmesa 的 mesa 库构建（ya 版，不用 meson）
# 目标：libmesa.a —— broadcom 编译器 + NIR + v3d gallium 驱动
# 配套库：eggs/libdrm（替身，链接进最终应用）

# ---- mako 生成物：构建时自动补（缺了就调 Mesa 自带的生成脚本）----
# 这批产物 meson 里是 custom_target，我们用脚本直出：
#   NIR 的 opcodes/intrinsics/constant_expressions、builtin_types.h、
#   util 的 u_format_gen.h、broadcom 的 v3d_xml.h（zipped xml blob）。
import os
import subprocess

BASE = os.path.dirname(os.path.abspath(__file__))


def _run(cmd, cwd, out):
    if os.path.exists(out) and os.path.getsize(out) > 0:
        return
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        subprocess.run(cmd, cwd=cwd, stdout=f, check=True)
    print('mesa: generated %s' % os.path.relpath(out, BASE))


def ensure_generated():
    NIR = os.path.join(BASE, 'src/compiler/nir')
    GEN = os.path.join(NIR, 'gen')
    _run(['python3', 'nir_opcodes_h.py'], NIR, GEN + '/nir_opcodes.h')
    _run(['python3', 'nir_builder_opcodes_h.py'], NIR,
         GEN + '/nir_builder_opcodes.h')
    _run(['python3', 'nir_opcodes_c.py'], NIR, GEN + '/nir_opcodes.c')
    _run(['python3', 'nir_intrinsics_h.py', '--out', 'gen/nir_intrinsics.h'],
         NIR, GEN + '/nir_intrinsics.h')
    _run(['python3', 'nir_intrinsics_indices_h.py', '--out',
          'gen/nir_intrinsics_indices.h'], NIR,
         GEN + '/nir_intrinsics_indices.h')
    _run(['python3', 'nir_intrinsics_c.py', '--out', 'gen/nir_intrinsics.c'],
         NIR, GEN + '/nir_intrinsics.c')
    _run(['python3', 'nir_constant_expressions.py'], NIR,
         GEN + '/nir_constant_expressions.c')

    COMP = os.path.join(BASE, 'src/compiler')
    _run(['python3', 'builtin_types_h.py', 'builtin_types.h'], COMP,
         COMP + '/builtin_types.h')

    FMT = os.path.join(BASE, 'src/util/format')
    _run(['python3', 'u_format_table.py', 'u_format.yaml', '--enums'], FMT,
         FMT + '/u_format_gen.h')

    CLE = os.path.join(BASE, 'src/broadcom/cle')
    _run(['python3', 'gen_zipped_xml_file.py', 'v3d_packet.xml'], CLE,
         CLE + '/v3d_xml.h')
    for v in ('33', '42', '71'):
        _run(['python3', 'gen_pack_header.py', 'v3d_packet.xml', v], CLE,
             CLE + '/v3d_packet_v%s_pack.h' % v)


ensure_generated()


target("mesa")
set_kind("static")

add_packages(get_config('default_libc'))

add_deps('zlib')

add_defines(
    'HAVE_PTHREAD=1',
    'HAVE_STRUCT_TIMESPEC=1',
    'HAVE_V3D=1',
    'GALLIUM_V3D=1',
    'USE_V3D_SIMULATOR=0',
    'HAVE_ENDIAN_H=1',
    'HAVE_SYS_MMAN_H=1',
    'PACKAGE_VERSION="24.0-foot"',
    'MESA_LLVM_VERSION_STRING="0"',
)

add_includedirs(
    '.',
    'src',                             # broadcom/common/... 形式
    'src/broadcom',                    # common/... 形式
    'src/broadcom/cle',
    'src/broadcom/common',
    'src/broadcom/compiler',
    'src/broadcom/qpu',
    'src/compiler',
    'src/compiler/nir',
    'src/compiler/nir/gen',            # mako 生成物
    'src/gallium/include',
    'src/gallium/auxiliary',
    'src/util',
    'src/c11',
    'src/c11/impl',
    'include',
    '../libdrm/include',               # libdrm 替身
    public = true
)

# ---- broadcom：CLE + 编译器支撑 ----
add_files(
    'src/broadcom/cle/v3d_decoder.c',
    'src/broadcom/common/v3d_device_info.c',
    'src/broadcom/common/v3d_debug.c',
    'src/broadcom/common/v3d_util.c',
    'src/broadcom/common/v3d_uif.c',
    'src/broadcom/common/v3d_format_table.c',
    'src/broadcom/qpu/qpu_instr.c',
    'src/broadcom/qpu/qpu_pack.c',
)

# ---- v3d 编译器（NIR → VIR → QPU），共 30 个文件 ----
add_files(
    'src/broadcom/compiler/*.c',
)

# ---- NIR ----
add_files(
    'src/compiler/nir/*.c',
    'src/compiler/nir/gen/nir_intrinsics.c',
    'src/compiler/nir/gen/nir_opcodes.c',
    'src/compiler/nir/gen/nir_constant_expressions.c',
)
