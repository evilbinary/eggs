/* 手工 fficonfig.h（libffi 3.8.0 / aarch64 裸机 Linux ABI）。
 * configure 在 aarch64-none-elf 交叉环境下无法完成（链接测试程序没有 crt），
 * 这里按 aarch64 Linux 的探测结果手工给出，只保留编译用得到的宏。
 * 参考 msvc_build/aarch64/aarch64_include/fficonfig.h 的尺寸定义。 */
#ifndef YIYIYA_FFICONFIG_H
#define YIYIYA_FFICONFIG_H

/* 目标与 ABI */
#define FFI_EXEC_TRAMPOLINE_TABLE 0
#define HAVE_LONG_DOUBLE 1
/* #undef HAVE_LONG_DOUBLE_VARIANT */

/* 基本类型尺寸（LP64） */
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 8
#define SIZEOF_SIZE_T 8

/* libc 能力（musl 全有） */
#define HAVE_ALLOCA 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_MMAP 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_UNISTD_H 1
#define HAVE_DLFCN_H 1
#define HAVE_HIDDEN_VISIBILITY_ATTRIBUTE 1

/* 本内核不提供可执行匿名映射场景下的 closure 支持，
 * 但 wayland 只用 ffi_call，closures.c 不参与编译。 */
/* #undef FFI_MMAP_EXEC_WRIT */
/* #undef FFI_MMAP_EXEC_SELINUX */
/* #undef FFI_MMAP_EXEC_APPLE */

/* 【注意】configure 会把下面这段写进 fficonfig.h（configure.ac 的 AH_BOTTOM）。
 * 不定义会导致 ffi_common.h 里 `void *ffi_data_to_code_pointer (void*) FFI_HIDDEN;`
 * 直接语法错误。静态库里不隐藏符号，展开为空。 */
#define FFI_HIDDEN
#ifdef LIBFFI_ASM
#undef FFI_HIDDEN
#define FFI_HIDDEN(name)
#endif

#endif
