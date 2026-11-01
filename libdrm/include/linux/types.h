/* linux/types.h - 内核 uapi 类型的最小替身
 *
 * Mesa 自带的 include/drm-uapi/*.h 是内核 ABI 头，会 include
 * <linux/types.h>。真实的 libdrm 依赖系统内核头，我们不引内核头，
 * 这里给出 uapi 需要的固定宽度类型与 ioctl 辅助。
 */
#ifndef _LINUX_TYPES_FOOT_H
#define _LINUX_TYPES_FOOT_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

typedef __u8 __bitwise_u8_dummy;

/* 内核里常见的 little/big endian 标注类型（uapi 里只作为标注用） */
typedef __u8 __bitwise;
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

#ifndef __user
#define __user
#endif
#ifndef __force
#define __force
#endif

/* 内核的 size/addr 标注类型（uapi 里出现在 drm.h 的字段声明中） */
typedef size_t __kernel_size_t;
typedef unsigned long __kernel_ulong_t;
typedef long __kernel_long_t;
typedef int __kernel_pid_t;

#endif
