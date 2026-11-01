/*
 * Copyright © 2015 Intel Corporation
 * Copyright © 2015 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef V3DX_PACK_H
#define V3DX_PACK_H

#ifndef V3D_VERSION
/* [foot] 同 v3d_macros.h：默认 7.1（本项目只支持 BCM2712） */
#  define V3D_VERSION 71
#endif

/* [foot] meson 会给 v3dx_*.c 单独传 V3D_VERSION，进而通过 v3d_context.h
 * 里的 #ifdef V3D_VERSION 引入 v3d_macros.h（定义 v3dX/V3DX 前缀宏）。
 * ya 构建无法 per-file 传宏，这里在 v3dx_pack.h 这个必经点补上，
 * 使 v3dx_*.c 能用 v3dX()/V3DX()。
 * 注意：v3d_formats.c 那类文件不经过这里（它们 include v3d_format_table.h
 * 并依赖 v3dX 未定义来同时声明两套前缀），不受影响。 */
#ifndef V3DX
#  include "broadcom/common/v3d_macros.h"
#endif

#if (V3D_VERSION == 21)
#  include "cle/v3d_packet_v21_pack.h"
#elif (V3D_VERSION == 42)
#  include "cle/v3d_packet_v42_pack.h"
#elif (V3D_VERSION == 71)
#  include "cle/v3d_packet_v71_pack.h"
#else
#  error "Need to add a pack header include for this v3d version"
#endif

#endif /* V3DX_PACK_H */
