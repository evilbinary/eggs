# coding:utf-8
# libmesa 的 mesa 库构建（ya 版，不用 meson）
# 目标：libmesa.a —— broadcom 编译器 + NIR + v3d gallium 驱动
# 配套库：eggs/libdrm（替身，链接进最终应用）
#
# 生成物已手工跑 mako 脚本产出到 src/compiler/nir/gen/（见附录 I）。

target("mesa")
set_kind("static")

M = 'src'

add_packages(get_config('default_libc'))

add_defines(
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
    M + '/include',
    M + '/src',
    M + '/src/gallium/include',
    M + '/src/gallium/auxiliary',
    M + '/src/gallium/drivers',
    M + '/src/util',
    M + '/src/compiler/include',
    M + '/src/compiler/nir',
    M + '/src/compiler/nir/gen',       # mako 生成物
    M + '/src/broadcom',
    M + '/src/broadcom/cle',
    M + '/src/broadcom/common',
    M + '/src/broadcom/compiler',
    M + '/src/broadcom/qpu',
    '../libdrm/include',               # libdrm 替身
    public = true
)

# ---- broadcom：CLE + 编译器 + 驱动 ----
add_files(
    M + '/broadcom/cle/v3d_decoder.c',
    M + '/broadcom/common/v3d_device_info.c',
    M + '/broadcom/common/v3d_debug.c',
    M + '/broadcom/common/v3d_util.c',
    M + '/broadcom/common/v3d_uif.c',
    M + '/broadcom/common/v3d_format_table.c',
    M + '/broadcom/qpu/qpu_instr.c',
    M + '/broadcom/qpu/qpu_pack.c',
)

# ---- v3d 编译器（NIR → VIR → QPU），共 30 个文件 ----
add_files(
    M + '/broadcom/compiler/*.c',
)

# ---- NIR ----
add_files(
    M + '/compiler/nir/*.c',
    M + '/compiler/nir/gen/nir_intrinsics.c',
    M + '/compiler/nir/gen/nir_opcodes.c',
    M + '/compiler/nir/gen/nir_constant_expressions.c',
)
