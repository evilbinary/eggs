/* 最小 linux/types.h（aarch64 = asm-generic int-ll64 的等价物）。
 * 上游是 include/uapi/linux/types.h + asm/types.h + linux/posix_types.h，
 * 这里只给 input.h/uinput.h 用得到的定义，避免拉进整套 uapi。 */
#ifndef YIYIYA_LINUX_TYPES_H
#define YIYIYA_LINUX_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;
typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;

typedef unsigned long __kernel_ulong_t;

#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif

#define __user
#define __force
#define __bitwise

#endif
