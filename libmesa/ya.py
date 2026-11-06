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

# Mesa 的 GLSL 前端是 C++。仓库自带 libc++（-fpic 构建），而裸机工具链的
# libstdc++.a 不是 PIC，不能链进 v3d_dri.so，因此这里显式把 cxx/cxxabi
# target 拉进来（默认只有 cpp-apps 配置才会 include 它们）。
if not has_config('cpp-apps'):
    includes('../libcxx/ya.py')
    includes('../libcxxabi/ya.py')


def _run(cmd, cwd, out):
    if os.path.exists(out) and os.path.getsize(out) > 0:
        return
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'w') as f:
        subprocess.run(cmd, cwd=cwd, stdout=f, check=True)
    print('mesa: generated %s' % os.path.relpath(out, BASE))


def _runf(cmd, cwd, out):
    """生成器自己写 -o 文件（bison/flex/xxd），stdout 不捕获。"""
    if os.path.exists(out) and os.path.getsize(out) > 0:
        return
    os.makedirs(os.path.dirname(out), exist_ok=True)
    subprocess.run(cmd, cwd=cwd, stdout=subprocess.DEVNULL, check=True)
    print('mesa: generated %s' % os.path.relpath(out, BASE))


def _glob_files(rel):
    import glob as _g
    return [os.path.relpath(f, BASE) for f in _g.glob(os.path.join(BASE, rel))]




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
    # builtin_types.c：定义 296 个 glsl_type_builtin_* 变量（glsl_types.c 只引用）
    _run(['python3', 'builtin_types_c.py', 'builtin_types.c'], COMP,
         COMP + '/builtin_types.c')

    _run(['python3', 'nir_opt_algebraic.py', '--out', 'nir_opt_algebraic.c'],
         NIR, NIR + '/nir_opt_algebraic.c')   # 优化规则表（py 生成，很大）

    BCOMP = os.path.join(BASE, 'src/broadcom/compiler')
    # v3d 的 algebraic lower 规则（生成；-p 指向 nir 目录，与 meson 一致）
    _run(['python3', 'v3d_nir_lower_algebraic.py', '-p', '../compiler/nir'],
         BCOMP, BCOMP + '/v3d_nir_lower_algebraic.c')

    FMT = os.path.join(BASE, 'src/util/format')
    _run(['python3', 'u_format_table.py', 'u_format.yaml', '--enums'], FMT,
         FMT + '/u_format_gen.h')
    _run(['python3', 'u_format_table.py', 'u_format.yaml'], FMT,
         FMT + '/u_format_table.c')           # format 描述表

    CLE = os.path.join(BASE, 'src/broadcom/cle')
    _run(['python3', 'gen_zipped_xml_file.py', 'v3d_packet.xml'], CLE,
         CLE + '/v3d_xml.h')
    for v in ('33', '42', '71'):
        _run(['python3', 'gen_pack_header.py', 'v3d_packet.xml', v], CLE,
             CLE + '/v3d_packet_v%s_pack.h' % v)


    UTIL = os.path.join(BASE, 'src/util')
    _run(['python3', 'driconf_static.py', '00-mesa-defaults.conf',
          '../gallium/drivers/v3d/00-v3d-defaults.conf',
          'driconf_static.h'], UTIL, UTIL + '/driconf_static.h')

    # ---- Mesa 前端（EGL/GLES2）生成物 ----
    # 统一放 gen/ 下，不污染上游源码；mesa_core target 把 gen/* 加进 include。
    MESA = os.path.join(BASE, 'src/mesa')
    GGEN_SRC = os.path.join(MESA, 'glapi/glapi/gen')
    GEN = os.path.join(BASE, 'gen')
    GG = os.path.join(GEN, 'mesa/glapi/glapi/gen')
    GM = os.path.join(GEN, 'mesa/main')
    GL = os.path.join(GEN, 'compiler/glsl')
    GLC = os.path.join(GL, 'glcpp')
    GP = os.path.join(GEN, 'mesa/program')
    GSV = os.path.join(GEN, 'compiler/spirv')
    for d in (GG, GM, GL, GLC, GP, GSV):
        os.makedirs(d, exist_ok=True)

    # mesa/main 生成物
    _run(['python3', 'get_hash_generator.py', '-f',
          '../glapi/glapi/gen/gl_and_es_API.xml'], os.path.join(MESA, 'main'),
         GM + '/get_hash.h')
    _run(['python3', 'format_info.py', 'formats.csv'],
         os.path.join(MESA, 'main'), GM + '/format_info.h')
    _runf(['python3', 'format_fallback.py', 'formats.csv',
           GM + '/format_fallback.c'], os.path.join(MESA, 'main'),
          GM + '/format_fallback.c')

    # glapi：dispatch / marshal / enums / trace
    _run(['python3', 'gl_enums.py', '-f', '../registry/gl.xml'],
         GGEN_SRC, GG + '/enums.c')
    _run(['python3', 'api_exec_init.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_exec_init.c')
    _run(['python3', 'api_exec_decl_h.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_exec_decl.h')
    _run(['python3', 'api_save_init_h.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_save_init.h')
    _run(['python3', 'api_save_h.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_save.h')
    _run(['python3', 'api_beginend_init_h.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_beginend_init.h')
    _run(['python3', 'api_hw_select_init_h.py', '-f', 'gl_API.xml'],
         GGEN_SRC, GG + '/api_hw_select_init.h')
    _run(['python3', 'gl_table.py', '-f', 'gl_and_es_API.xml', '-m',
          'dispatch'], GGEN_SRC, GG + '/dispatch.h')
    _run(['python3', 'marshal_generated_h.py', 'gl_and_es_API.xml', '8'],
         GGEN_SRC, GG + '/marshal_generated.h')
    _run(['python3', 'unmarshal_table_c.py', 'gl_and_es_API.xml', '8'],
         GGEN_SRC, GG + '/unmarshal_table.c')
    _run(['python3', 'api_trace_c.py', '-f', 'gl_and_es_API.xml'],
         GGEN_SRC, GG + '/api_trace.c')
    for _x in range(8):
        _run(['python3', 'marshal_generated_c.py', 'gl_and_es_API.xml',
              str(_x), '8', '8'], GGEN_SRC,
             GG + '/marshal_generated%d.c' % _x)
    _run(['python3', '../../mapi_abi.py', '--printer', 'shared-glapi',
          '--gl_symbols', '../../../../glx/libgl-symbols.txt',
          'gl_and_es_API.xml'], GGEN_SRC, GG + '/shared_glapi_mapi_tmp.h')
    _run(['python3', '../../new/gen_gldispatch_mapi.py', 'glesv2',
          '../registry/gl.xml'], GGEN_SRC, GG + '/es2_glapi_mapi_tmp.h')

    # GLSL parser/lexer + 表达式表 + 内嵌 GLSL 源码
    GLS = os.path.join(BASE, 'src/compiler/glsl')
    _runf(['bison', '-Wno-deprecated', '-o', GL + '/glsl_parser.cpp',
           '-p', '_mesa_glsl_', '--defines=' + GL + '/glsl_parser.h',
           'glsl_parser.yy'], GLS, GL + '/glsl_parser.cpp')
    _runf(['flex', '-DYY_USE_CONST=', '-o', GL + '/glsl_lexer.cpp',
           'glsl_lexer.ll'], GLS, GL + '/glsl_lexer.cpp')
    for _mode in ('enum', 'strings', 'constant'):
        _run(['python3', 'ir_expression_operation.py', _mode], GLS,
             GL + ('/ir_expression_operation.h' if _mode == 'enum'
                   else '/ir_expression_operation_%s.h' % _mode))
    for _src, _name, _out in (
            ('float64.glsl', 'float64_source', 'float64_glsl.h'),
            ('CrossPlatformSettings_piece_all.glsl',
             'cross_platform_settings_piece_all_header',
             'cross_platform_settings_piece_all.h'),
            ('bc1.glsl', 'bc1_source', 'bc1_glsl.h'),
            ('bc4.glsl', 'bc4_source', 'bc4_glsl.h'),
            ('etc2_rgba_stitch.glsl', 'etc2_rgba_stitch_source',
             'etc2_rgba_stitch_glsl.h'),
            ('astc_decoder.glsl', 'astc_source', 'astc_glsl.h')):
        _runf(['python3', '../../util/xxd.py', _src, GL + '/' + _out,
               '-n', _name], GLS, GL + '/' + _out)

    GLC_SRC = os.path.join(GLS, 'glcpp')
    _runf(['bison', '-Wno-deprecated', '-o', GLC + '/glcpp-parse.c',
           '-p', 'glcpp_parser_', '--defines=' + GLC + '/glcpp-parse.h',
           'glcpp-parse.y'], GLC_SRC, GLC + '/glcpp-parse.c')
    _runf(['flex', '-DYY_USE_CONST=', '-o', GLC + '/glcpp-lex.c',
           'glcpp-lex.l'], GLC_SRC, GLC + '/glcpp-lex.c')

    PRG = os.path.join(MESA, 'program')
    _runf(['flex', '-DYY_USE_CONST=', '-o', GP + '/lex.yy.c',
           'program_lexer.l'], PRG, GP + '/lex.yy.c')
    _runf(['bison', '-Wno-deprecated', '-o', GP + '/program_parse.tab.c',
           '--defines=' + GP + '/program_parse.tab.h', 'program_parse.y'],
          PRG, GP + '/program_parse.tab.c')

    _runf(['python3', 'bin/git_sha1_gen.py', '--output',
           GEN + '/git_sha1.h'], BASE, GEN + '/git_sha1.h')

    SPV = os.path.join(BASE, 'src/compiler/spirv')
    _runf(['python3', 'spirv_info_gen.py', '--json', 'spirv.core.grammar.json',
           '--out-h', GSV + '/spirv_info.h', '--out-c', GSV + '/spirv_info.c'],
          SPV, GSV + '/spirv_info.h')
    _runf(['python3', 'vtn_gather_types_c.py', 'spirv.core.grammar.json',
           GSV + '/vtn_gather_types.c'], SPV, GSV + '/vtn_gather_types.c')
    _runf(['python3', 'vtn_generator_ids_h.py', 'spir-v.xml',
           GSV + '/vtn_generator_ids.h'], SPV, GSV + '/vtn_generator_ids.h')

    GAUX = os.path.join(GEN, 'gallium/auxiliary')
    _runf(['python3', 'u_indices_gen.py', GAUX + '/u_indices_gen.c'],
          os.path.join(BASE, 'src/gallium/auxiliary/indices'),
          GAUX + '/u_indices_gen.c')
    _runf(['python3', 'u_unfilled_gen.py', GAUX + '/u_unfilled_gen.c'],
          os.path.join(BASE, 'src/gallium/auxiliary/indices'),
          GAUX + '/u_unfilled_gen.c')
    _run(['python3', 'draw/draw_nir_lower_opcodes.py', '-p',
          '../../compiler/nir'],
         os.path.join(BASE, 'src/gallium/auxiliary'),
         GAUX + '/draw_nir_lower_opcodes.c')


ensure_generated()


target("mesa")
set_kind("static")

add_packages(get_config('default_libc'))

add_deps('zlib')

add_cflags(
    ' -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L '
    ' -std=gnu11 '
    ' -fPIC '
    ' -D__linux__ '        # 裸机 ELF 工具链不定义 __linux__，
                           # 而 Mesa 的 detect_os.h 完全靠它判定平台
    # 注意：不传 -DV3D_VERSION。v3d_macros.h / v3dx_pack.h 已默认 71；
    # 而 v3d_formats.c 等文件靠「V3D_VERSION 未定义」来同时声明两套
    # 前缀符号（v3d_format_table.h 的 #ifdef v3dX 分支），命令行传了会破坏它。
    ' -DDETECT_ARCH_AARCH64=1 '
    # meson 会探测这些 libc 能力并写进 config；musl 全部具备，直接给
    ' -DHAVE_SYSCONF -DHAVE_GETRANDOM -DHAVE_GETENTROPY '
    ' -DHAVE_MALLOC_USABLE_SIZE -DHAVE_CLOCK_GETTIME -DHAVE_TIMESPEC_GET '
    ' -DHAVE_STRTOD_L -DHAVE_MEMFD_CREATE '
    # 下面这批原本用 add_defines 传，但 ymake 的 defines 对 Mesa 的编译
    # 单元不生效（实测：c11/threads.h 因缺 HAVE_PTHREAD 走了 fallback 分支，
    # 进而 CLOCK_MONOTONIC/NULL 未声明）。统一并进 cflags。
    ' -DHAVE_PTHREAD=1 -DHAVE_STRUCT_TIMESPEC=1 '
    ' -DHAVE_FUNC_ATTRIBUTE_PACKED=1 '
    ' -DHAVE_V3D=1 -DGALLIUM_V3D=1 -DUSE_V3D_SIMULATOR=0 '
    ' -DHAVE_ENDIAN_H=1 -DHAVE_SYS_MMAN_H=1 -DHAVE_LIBDRM -DMAJOR_IN_SYSMACROS=1 '
    ' -DHAVE_LINUX_FUTEX_H=1 -DTHREAD_SANITIZER=0 '
    ' -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 '
    # 关掉 format 的 NEON 特化（Mesa 官方开关），否则要额外编
    # u_format_unpack_neon.c 及其整表
    ' -DNO_FORMAT_ASM '
    # musl 自带 GNU 版 qsort_r，u_qsort.h 里是 static inline，
    # 因此不需要编 C++ 的 u_qsort.cpp
    ' -DHAVE_GNU_QSORT_R '
    ' -DPACKAGE_VERSION="24.0_foot" -DMESA_LLVM_VERSION_STRING=0 '
)

add_defines(
    'HAVE_PTHREAD=1',
    'HAVE_STRUCT_TIMESPEC=1',
    'HAVE_V3D=1',
    'GALLIUM_V3D=1',
    'USE_V3D_SIMULATOR=0',
    'UTIL_ARCH_LITTLE_ENDIAN=1',
    'UTIL_ARCH_BIG_ENDIAN=0',
)

add_includedirs(
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
    'include',
    '../libdrm/include',               # libdrm 替身
    public = true
)

# ---- broadcom：CLE + 编译器支撑（非版本文件，只编一次）----
add_files(
    'src/broadcom/cle/v3d_decoder.c',
    'src/broadcom/clif/clif_dump.c',
    'src/broadcom/clif/v3dx_dump.c',
    'src/broadcom/common/v3d_device_info.c',
    'src/broadcom/common/v3d_debug.c',
    'src/broadcom/common/v3d_util.c',
    'src/broadcom/common/v3d_tiling.c',
    'src/broadcom/common/v3d_submit_util.c',
    'src/broadcom/common/v3d_uif.c',
    'src/broadcom/common/v3d_format_table.c',
    'src/broadcom/perfcntrs/v3d_perfcntrs.c',
    'src/broadcom/perfcntrs/v3dx_counter.c',
)

# 编译器 + QPU 打包表（V3D_VERSION=71 由全局 cflags 给出）
add_files(
    'src/broadcom/qpu/qpu_instr.c',
    'src/broadcom/qpu/qpu_pack.c',
    'src/broadcom/qpu/qpu_disasm.c',   # v3d_qpu_decode / v3d_qpu_disasm
    'src/broadcom/compiler/*.c',
)

# ---- NIR ----
# nir_stub.c 是「无调试输出」的桩实现，与 nir_print.c 的 nir_print_shader
# 冲突（meson 里二选一）。用 python glob 过滤掉，其余全收。
# 注意：ymake 执行 ya.py 时的 cwd 不是本目录，glob 必须用 BASE 绝对路径，
# 否则匹配为空 —— NIR 的 100 多个文件会静默不进构建列表（踩过）。
import glob as _glob

_nir_files = [
    os.path.relpath(f, BASE)
    for f in _glob.glob(os.path.join(BASE, 'src/compiler/nir/*.c'))
    if not f.endswith('nir_stub.c')
]
print('mesa: nir files = %d' % len(_nir_files))
add_files(*_nir_files)
add_files(
    'src/compiler/nir/gen/nir_intrinsics.c',
    'src/compiler/nir/gen/nir_opcodes.c',
    'src/compiler/nir/gen/nir_constant_expressions.c',
)

# ---- Mesa util（编译器依赖：ralloc/hash/call_once/os_* 等）----
# 清单来自 src/util/meson.build 的 files_mesa_util（68 个），
# 刻意不用 glob —— 目录里含 x86/ppc/win32 等平台专用文件。
add_files(
    'src/util/anon_file.c',
    'src/util/bitscan.c',
    'src/util/blob.c',
    'src/util/build_id.c',
    'src/util/cnd_monotonic.c',
    'src/util/compress.c',
    'src/util/thread_sched.c',
    'src/util/crc32.c',
    'src/util/dag.c',
    'src/util/disk_cache.c',
    'src/util/disk_cache_os.c',
    'src/util/double.c',
    'src/util/fast_idiv_by_const.c',
    'src/util/float8.c',
    'src/util/fossilize_db.c',
    'src/util/futex.c',
    'src/util/half_float.c',
    'src/util/hash_table.c',
    'src/util/helpers.c',
    'src/util/u_idalloc.c',
    'src/util/log.c',
    'src/util/lut.c',
    'src/util/memstream.c',
    'src/util/os_file.c',
    'src/util/os_file_notify.c',
    'src/util/os_memory_fd.c',
    'src/util/os_socket.c',
    'src/util/pb_slab.c',
    'src/util/u_process.c',
    'src/util/rwlock.c',
    'src/util/ralloc.c',
    'src/util/rand_xor.c',
    'src/util/range_minimum_query.c',
    'src/util/rb_tree.c',
    'src/util/register_allocate.c',
    'src/util/rgtc.c',
    'src/util/set.c',
    'src/util/simple_mtx.c',
    'src/util/slab.c',
    'src/util/softfloat.c',
    'src/util/sparse_array.c',
    'src/util/string_buffer.c',
    'src/util/strndup.c',
    'src/util/strtod.c',
    'src/util/u_atomic.c',
    'src/util/u_call_once.c',
    'src/util/u_dl.c',
    'src/util/u_dynarray.c',
    'src/util/u_hash_table.c',
    'src/util/u_queue.c',
    'src/util/u_range_remap.c',
    'src/util/u_shader_variant_cache.c',
    'src/util/u_string.c',
    'src/util/u_thread.c',
    'src/util/u_vector.c',
    'src/util/os_time.c',
    'src/util/os_misc.c',
    'src/util/streaming-load-memcpy.c',
    'src/util/u_math.c',
    'src/util/u_mm.c',
    'src/util/u_debug.c',
    'src/util/u_debug_memory.c',
    'src/util/u_cpu_detect.c',
    'src/util/u_printf.c',
    'src/util/u_worklist.c',
    'src/util/vl_zscan_data.c',
    'src/util/vma.c',
    'src/util/mesa_cache_db.c',
    'src/util/mesa_cache_db_multipart.c',
    'src/gallium/auxiliary/util/u_debug_describe.c',
    'src/gallium/auxiliary/util/u_upload_mgr.c',
    'src/gallium/auxiliary/util/u_blitter.c',
    'src/gallium/auxiliary/util/u_draw.c',
    'src/gallium/auxiliary/util/u_framebuffer.c',
    'src/gallium/auxiliary/util/u_helpers.c',
    'src/gallium/auxiliary/util/u_prim.c',
    'src/gallium/auxiliary/util/u_prim_restart.c',
    'src/gallium/auxiliary/util/u_surface.c',
    'src/gallium/auxiliary/util/u_screen.c',
    'src/gallium/auxiliary/util/u_transfer.c',
    'src/gallium/auxiliary/util/u_transfer_helper.c',
    'src/gallium/auxiliary/util/u_vbuf.c',
    'src/gallium/auxiliary/util/u_tile.c',
    'src/gallium/auxiliary/util/u_gen_mipmap.c',
    'src/gallium/auxiliary/util/u_driconf.c',
    'src/gallium/auxiliary/util/u_sampler.c',
    'src/gallium/auxiliary/util/u_draw_quad.c',
    'src/gallium/auxiliary/tgsi/tgsi_transform.c',
    'src/gallium/auxiliary/tgsi/tgsi_from_mesa.c',
    'src/gallium/auxiliary/tgsi/tgsi_exec.c',
    'src/gallium/auxiliary/nir/nir_draw_helpers.c',
    'src/gallium/auxiliary/nir/nir_to_tgsi.c',
    *[f for f in _glob_files('src/gallium/auxiliary/draw/*.c')
      if 'llvm' not in os.path.basename(f)],
    'gen/gallium/auxiliary/u_indices_gen.c',
    'gen/gallium/auxiliary/draw_nir_lower_opcodes.c',
    'gen/gallium/auxiliary/u_unfilled_gen.c',
    'src/gallium/auxiliary/indices/u_primconvert.c',
    'src/gallium/auxiliary/translate/translate.c',
    'src/gallium/auxiliary/translate/translate_generic.c',
    'src/gallium/auxiliary/translate/translate_cache.c',
    'src/gallium/auxiliary/util/u_simple_shaders.c',
    'src/gallium/auxiliary/util/u_resource.c',
    'src/gallium/auxiliary/util/u_bitmask.c',
    'src/gallium/auxiliary/util/u_texture.c',
    'src/gallium/auxiliary/tgsi/tgsi_dump.c',
    'src/gallium/auxiliary/tgsi/tgsi_build.c',
    'src/gallium/auxiliary/tgsi/tgsi_info.c',
    'src/gallium/auxiliary/tgsi/tgsi_iterate.c',
    'src/gallium/auxiliary/tgsi/tgsi_parse.c',
    'src/gallium/auxiliary/tgsi/tgsi_sanity.c',
    'src/gallium/auxiliary/cso_cache/cso_hash.c',
    'src/gallium/auxiliary/cso_cache/cso_cache.c',
    'src/gallium/auxiliary/cso_cache/cso_context.c',
    'src/gallium/auxiliary/tgsi/tgsi_scan.c',
    'src/gallium/auxiliary/tgsi/tgsi_strings.c',
    'src/gallium/auxiliary/tgsi/tgsi_text.c',
    'src/gallium/auxiliary/tgsi/tgsi_ureg.c',
    'src/gallium/auxiliary/tgsi/tgsi_util.c',
    'src/gallium/auxiliary/nir/tgsi_to_nir.c',
    'src/util/xmlconfig.c',
    'src/c11/impl/time.c',
)

# util 的 format 子目录 + blake3
add_files(
    'src/util/format/u_format.c',
    'src/util/format/u_format_bptc.c',
    'src/util/format/u_format_etc.c',
    'src/util/format/u_format_fxt1.c',
    'src/util/format/u_format_latc.c',
    'src/util/format/u_format_other.c',
    'src/util/format/u_format_rgtc.c',
    'src/util/format/u_format_s3tc.c',
    'src/util/format/u_format_yuv.c',
    'src/util/format/u_format_zs.c',
    'src/util/format/u_format_table.c',   # 生成物：util_format_description 等
    'src/util/format_srgb.c',             # 生成物：srgb 查表
    'src/util/foot_qsort_r.c',            # musl 无 qsort_r，自实现
)

# ---- blake3（disk_cache 用）----
# dispatch 在 aarch64 上会选 NEON 实现（blake3_neon.c），x86 特化不需要。
add_files(
    'src/util/blake3/blake3.c',
    'src/util/blake3/blake3_dispatch.c',
    'src/util/blake3/blake3_portable.c',
    'src/util/blake3/blake3_neon.c',
    'src/util/mesa-blake3.c',
)

# ---- shader_enums（gl_*_name 系列）----
add_files(
    'src/compiler/shader_enums.c',
)

# ---- glsl_types（编译器类型系统；Mesa 24 是 C 实现）----
add_files(
    'src/compiler/glsl_types.c',
    'src/compiler/builtin_types.c',   # 生成物：glsl_type_builtin_* 的定义
)

# ---- v3d gallium 驱动 + winsys（对接 libdrm 替身 → gpud）----
add_files(
    'src/gallium/drivers/v3d/*.c',
    'src/gallium/winsys/v3d/drm/v3d_drm_winsys.c',
)

add_includedirs(
    'src/gallium/drivers',             # v3d/v3d_screen.h 形式
    'src/gallium/drivers/v3d',
    'src/gallium/winsys/v3d/drm',
    'src/broadcom/perfcntrs',
)


# ==========================================================================
# 说明：meson 里编译器与 v3dx_* 会按 V3D_VERSION=42/71 各编一份
# （libv3d_compiler@ver），因为 v3d_util.h 的 v3d_X() 宏 switch 同时引用
# v3d42_/v3d71_ 两套前缀符号。ya 的 add_files 不支持 per-file defines，
# 同一文件用不同宏编两次只能分 target；而分 target 又带来静态库链接
# 顺序问题（ymake 只在单 target 产出多个 .a 时才加 --start-group）。
# 本项目只支持 V3D 7.1（BCM2712），因此把 v3d_util.h 的 v3d_X 宏
# 收敛到 71 分支（见该文件的 [foot] 注释），单份编译即可。
# ==========================================================================


# ==========================================================================
# Mesa 前端 core（GLES/EGL 用）：Mesa state tracker + GLSL + DRI + loader
# 说明：这里只加"前端新文件"；util/NIR/broadcom 已在 target("mesa") 里。
# 生成的 glapi/GLSL parser 文件在 ensure_generated() 里生成。
# ==========================================================================
target("mesa_core")
set_kind("static")

add_packages(get_config('default_libc'))
add_deps('zlib')
add_deps('mesa')
# Mesa GLSL 前端是 C++；用仓库自带的 libc++（-fpic 构建），
# 裸机工具链的 libstdc++.a 不是 PIC，不能链进 .so。
add_deps('cxx', 'cxxabi')

add_cflags(
    ' -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L '
    ' -std=gnu11 -fPIC '
    ' -D__linux__ -DDETECT_ARCH_AARCH64=1 '
    ' -DHAVE_SYSCONF -DHAVE_GETRANDOM -DHAVE_GETENTROPY '
    ' -DHAVE_MALLOC_USABLE_SIZE -DHAVE_CLOCK_GETTIME -DHAVE_TIMESPEC_GET '
    ' -DHAVE_STRTOD_L -DHAVE_MEMFD_CREATE '
    ' -DHAVE_PTHREAD=1 -DHAVE_STRUCT_TIMESPEC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 '
    ' -DHAVE_V3D=1 -DGALLIUM_V3D=1 -DUSE_V3D_SIMULATOR=0 '
    ' -DHAVE_ENDIAN_H=1 -DHAVE_SYS_MMAN_H=1 -DHAVE_LIBDRM -DMAJOR_IN_SYSMACROS=1 '
    ' -DHAVE_LINUX_FUTEX_H=1 -DTHREAD_SANITIZER=0 '
    ' -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 '
    ' -DNO_FORMAT_ASM -DHAVE_GNU_QSORT_R '
    ' -DPACKAGE_VERSION="26.2.6" -DPACKAGE_BUGREPORT="https://gitlab.freedesktop.org/mesa/mesa/-/issues" '
    ' -DMESA_LLVM_VERSION_STRING=0 '
    ' -DHAVE_OPENGL=0 -DHAVE_OPENGL_ES_1=0 -DHAVE_OPENGL_ES_2=1 '
)
add_cxxflags(
    ' -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L '
    ' -std=gnu++17 -fPIC -fno-use-cxa-atexit '
    ' -D_LIBCPP_HAS_MUSL_LIBC -D_LIBCPP_HAS_NO_LIBRARY_ALIGNED_ALLOCATION '
    ' -D_LIBCPP_HAS_NO_THREADS -D_LIBCXXABI_HAS_NO_THREADS '
    ' -D__linux__ -DDETECT_ARCH_AARCH64=1 '
    ' -DHAVE_SYSCONF -DHAVE_GETRANDOM -DHAVE_GETENTROPY '
    ' -DHAVE_MALLOC_USABLE_SIZE -DHAVE_CLOCK_GETTIME -DHAVE_TIMESPEC_GET '
    ' -DHAVE_STRTOD_L -DHAVE_MEMFD_CREATE '
    ' -DHAVE_PTHREAD=1 -DHAVE_STRUCT_TIMESPEC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 '
    ' -DHAVE_V3D=1 -DGALLIUM_V3D=1 -DUSE_V3D_SIMULATOR=0 '
    ' -DHAVE_ENDIAN_H=1 -DHAVE_SYS_MMAN_H=1 -DHAVE_LIBDRM -DMAJOR_IN_SYSMACROS=1 '
    ' -DHAVE_LINUX_FUTEX_H=1 -DTHREAD_SANITIZER=0 '
    ' -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 '
    ' -DNO_FORMAT_ASM -DHAVE_GNU_QSORT_R '
    ' -DPACKAGE_VERSION="26.2.6" -DPACKAGE_BUGREPORT="https://gitlab.freedesktop.org/mesa/mesa/-/issues" '
    ' -DMESA_LLVM_VERSION_STRING=0 '
    ' -DHAVE_OPENGL=0 -DHAVE_OPENGL_ES_1=0 -DHAVE_OPENGL_ES_2=1 '
)

add_includedirs(
    'src', 'src/mesa', 'src/mesa/main', 'src/mesa/state_tracker',
    'src/mesa/program', 'src/mesa/vbo', 'src/mesa/glapi',
    'src/mesa/glapi/glapi', 'src/mesa/glapi/glapi/gen',
    'src/mesa/glapi/shared-glapi', 'src/mesa/glapi/es2api',
    'src/compiler', 'src/compiler/nir', 'src/compiler/nir/gen',
    'src/compiler/spirv', 'gen/compiler/spirv',
    'src/compiler/glsl', 'src/compiler/glsl/glcpp',
    'src/gallium/frontends/dri', 'src/loader',
    'src/gallium/include', 'src/gallium/auxiliary',
    'src/util',
    'include', '../libdrm/include',
    'gen', 'gen/mesa', 'gen/mesa/main', 'gen/mesa/glapi/glapi/gen',
    'gen/mesa/program', 'gen/compiler/glsl', 'gen/compiler/glsl/glcpp',
)

add_files(
    *[f for f in _glob_files('src/mesa/main/*.c')
      if os.path.basename(f) != 'sse_minmax.c'],
    *_glob_files('src/mesa/main/*.cpp'),
    *_glob_files('src/mesa/state_tracker/*.c'),
    *_glob_files('src/mesa/state_tracker/*.cpp'),
    *_glob_files('src/mesa/program/*.c'),
    *_glob_files('src/mesa/vbo/*.c'),
    *_glob_files('src/mesa/math/*.c'),
    'src/mesa/glapi/shared-glapi/core.c',
    *[f for f in _glob_files('src/gallium/frontends/dri/*.c')
      if os.path.basename(f) not in
      ('kopper.c', 'loader_dri3_helper.c', 'drisw.c')],
    *[f for f in _glob_files('src/loader/*.c')
      if os.path.basename(f) != 'loader_wayland_helper.c'],
    # 生成物
    'gen/mesa/main/format_fallback.c',
    'gen/compiler/spirv/spirv_info.c',
    'gen/compiler/spirv/vtn_gather_types.c',
    'src/compiler/spirv/gl_spirv.c',
    'src/compiler/spirv/spirv_to_nir.c',
    *[f for f in _glob_files('src/compiler/spirv/vtn_*.c')
      if os.path.basename(f) != 'vtn_bindgen2.c'],
    'gen/mesa/glapi/glapi/gen/enums.c',
    'gen/mesa/glapi/glapi/gen/api_exec_init.c',
    'gen/mesa/glapi/glapi/gen/api_trace.c',
    'gen/mesa/glapi/glapi/gen/unmarshal_table.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated0.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated1.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated2.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated3.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated4.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated5.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated6.c',
    'gen/mesa/glapi/glapi/gen/marshal_generated7.c',
    'gen/compiler/glsl/glsl_parser.cpp',
    'gen/compiler/glsl/glsl_lexer.cpp',
    'gen/compiler/glsl/glcpp/glcpp-lex.c',
    'gen/compiler/glsl/glcpp/glcpp-parse.c',
    'gen/mesa/program/lex.yy.c',
    'gen/mesa/program/program_parse.tab.c',
)

# GLSL 前端：排除 standalone 工具入口（main.cpp/standalone*.cpp）
_glsl_cpp = [f for f in _glob_files('src/compiler/glsl/*.cpp')
             if os.path.basename(f) not in ('main.cpp', 'standalone.cpp',
                                            'standalone_scaffolding.cpp')]
add_files(
    *_glsl_cpp,
    *_glob_files('src/compiler/glsl/*.c'),
    *_glob_files('src/compiler/glsl/glcpp/*.c'),
)


# ==========================================================================
# Mesa 前端：v3d_dri.so（DRI 驱动）+ libEGL + libGLESv2
#
# 与上游 non-glvnd 结构一致：Mesa core / DRI frontend / glapi 都在
# v3d_dri.so 里；libEGL、libGLESv2 直接 link 它（DT_NEEDED），从而共用
# 同一份 _glapi_* dispatch 状态。运行时把 libv3d_dri.so 放 /lib 即可。
# ==========================================================================
_MESA_FRONT_DEFS = (
    ' -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L '
    ' -D__linux__ -D__unix__ -DDETECT_ARCH_AARCH64=1 '
    ' -DHAVE_SYSCONF -DHAVE_GETRANDOM -DHAVE_GETENTROPY '
    ' -DHAVE_MALLOC_USABLE_SIZE -DHAVE_CLOCK_GETTIME -DHAVE_TIMESPEC_GET '
    ' -DHAVE_STRTOD_L -DHAVE_MEMFD_CREATE '
    ' -DHAVE_PTHREAD=1 -DHAVE_STRUCT_TIMESPEC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 '
    ' -DHAVE_V3D=1 -DGALLIUM_V3D=1 -DUSE_V3D_SIMULATOR=0 '
    ' -DHAVE_ENDIAN_H=1 -DHAVE_SYS_MMAN_H=1 -DHAVE_LIBDRM -DMAJOR_IN_SYSMACROS=1 '
    ' -DHAVE_LINUX_FUTEX_H=1 -DTHREAD_SANITIZER=0 '
    ' -DUTIL_ARCH_LITTLE_ENDIAN=1 -DUTIL_ARCH_BIG_ENDIAN=0 '
    ' -DNO_FORMAT_ASM -DHAVE_GNU_QSORT_R '
    ' -DPACKAGE_VERSION="26.2.6" '
    ' -DPACKAGE_BUGREPORT="https://gitlab.freedesktop.org/mesa/mesa/-/issues" '
    ' -DMESA_LLVM_VERSION_STRING=0 '
    ' -DHAVE_OPENGL=0 -DHAVE_OPENGL_ES_1=0 -DHAVE_OPENGL_ES_2=1 '
)

_MESA_FRONT_INCS = (
    'src', 'src/mesa', 'src/mesa/main', 'src/mesa/state_tracker',
    'src/mesa/program', 'src/mesa/vbo', 'src/mesa/glapi',
    'src/mesa/glapi/glapi', 'src/mesa/glapi/glapi/gen',
    'src/mesa/glapi/shared-glapi', 'src/mesa/glapi/es2api',
    'src/compiler', 'src/compiler/nir', 'src/compiler/nir/gen',
    'src/compiler/spirv', 'gen/compiler/spirv',
    'src/compiler/glsl', 'src/compiler/glsl/glcpp',
    'src/gallium/frontends/dri', 'src/loader',
    'src/gallium/include', 'src/gallium/auxiliary',
    'src/gallium/drivers', 'src/gallium/drivers/v3d',
    'src/gallium/winsys', 'src/gallium/winsys/v3d/drm',
    'src/util',
    'include', '../libdrm/include',
    'gen', 'gen/mesa', 'gen/mesa/main', 'gen/mesa/glapi/glapi/gen',
    'gen/mesa/program', 'gen/compiler/glsl', 'gen/compiler/glsl/glcpp',
)

# ---- v3d_dri.so：DRI 驱动 ----
target("v3d_dri")
set_kind("shared")
add_packages(get_config('default_libc'))
add_deps('gcc')
add_deps('mesa_core', 'mesa', 'drm', 'zlib', 'cxx', 'cxxabi')
add_cflags(_MESA_FRONT_DEFS + ' -fPIC -std=gnu11 ')
add_cxxflags(_MESA_FRONT_DEFS + ' -fPIC -std=gnu++17 ')
add_includedirs(*_MESA_FRONT_INCS)
add_files('src/gallium/targets/dri/dri_target.c')
# 关键：dri_util.o 是 DRI 前端的“根”。libmesa_core.a 里没人引用它
# （应用是通过 libEGL 调 driCreateNewScreen3），静态库链接不会把它拉进来。
# 直接把该源文件加入本 target，让对象出现在 ld 命令行上，从而把整个
# DRI 前端 + state tracker + Mesa core 拉进 v3d_dri.so。
add_files('src/gallium/frontends/dri/dri_util.c')
add_files(
    'src/gallium/auxiliary/pipe-loader/pipe_loader.c',
    'src/gallium/auxiliary/pipe-loader/pipe_loader_drm.c',
)
add_includedirs('src/gallium/auxiliary/pipe-loader')
add_ldflags('-lm')

# ---- libEGL.so：EGL 前端（device + surfaceless 平台）----
target("EGL")
set_kind("shared")
add_packages(get_config('default_libc'))
add_deps('gcc')
add_deps('v3d_dri')
add_cflags(_MESA_FRONT_DEFS +
           ' -fPIC -std=gnu11 -DHAVE_LIBDRM -DUSE_LIBGLVND=0 '
           ' -D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_SURFACELESS ')
add_includedirs(*_MESA_FRONT_INCS)
add_includedirs('src/egl', 'src/egl/main')
_egl_srcs = [f for f in _glob_files('src/egl/main/*.c')
             if os.path.basename(f) not in ('eglglvnd.c', 'egldispatchstubs.c')]
add_files(
    *_egl_srcs,
    'src/egl/drivers/dri2/egl_dri2.c',
    'src/egl/drivers/dri2/platform_device.c',
    'src/egl/drivers/dri2/platform_surfaceless.c',
)

# ---- libGLESv2.so：GLES2 公共入口（glapi dispatch stubs）----
target("GLESv2")
set_kind("shared")
add_packages(get_config('default_libc'))
add_deps('gcc')
add_deps('v3d_dri')
add_cflags(_MESA_FRONT_DEFS + ' -fPIC -std=gnu11 ')
add_includedirs(*_MESA_FRONT_INCS)
add_files('src/mesa/glapi/es2api/libgles2_public.c')
