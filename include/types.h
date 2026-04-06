#ifndef TYPES_H
#define TYPES_H

#if defined(ARM64) || defined(__aarch64__)
#define _Addr long
#define _Int64 long long
#define _Reg long
#else
#define _Addr int
#define _Int64 long long
#define _Reg int
#endif


typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;

typedef long long i64;
typedef unsigned long long u64;

typedef long long illong;
typedef unsigned long long ullong;

typedef float f32;
typedef double f64;

typedef unsigned int uint;
typedef unsigned long ulong;

#define true 1
#define false 0

// Avoid NULL conflict with musl and other C libraries
#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef LIBYC
#define bool char
#else
#define bool _Bool
#endif



// Address-sized integer type: u64 on ARM64, u32 on 32-bit platforms
#if defined(ARM64) || defined(__aarch64__)
typedef u64 vaddr_t;
#else
typedef u32 vaddr_t;
#endif

typedef unsigned int u_int;

#if !defined(__DEFINED_int8_t)
typedef signed char int8_t;
#define __DEFINED_int8_t
#endif

#if !defined(__DEFINED_int16_t)
typedef signed short int16_t;
#define __DEFINED_int16_t
#endif

#if !defined(__DEFINED_int32_t)
typedef signed int int32_t;
#define __DEFINED_int32_t
#endif

#if !defined(__DEFINED_int64_t)
typedef signed long long int64_t;
#define __DEFINED_int64_t
#endif

#if !defined(__DEFINED_uint8_t)
typedef unsigned char uint8_t;
#define __DEFINED_uint8_t
#endif

#if !defined(__DEFINED_uint16_t)
typedef unsigned short uint16_t;
#define __DEFINED_uint16_t
#endif

#if !defined(__DEFINED_uint32_t)
typedef unsigned int uint32_t;
#define __DEFINED_uint32_t
#endif

#if !defined(__DEFINED_uint64_t)
typedef unsigned long long uint64_t;
#define __DEFINED_uint64_t
#endif

#if !defined(_INTPTR_T) && !defined(intptr_t) && !defined(__DEFINED_intptr_t)
typedef _Addr intptr_t;
#define __DEFINED_intptr_t
#endif

#if !defined(_UINTPTR_T) && !defined(uintptr_t) && !defined(__DEFINED_uintptr_t)
typedef unsigned _Addr uintptr_t;
#define __DEFINED_uintptr_t
#endif

#if !defined(_SIZE_T) && !defined(size_t) && !defined(_HAVE_SIZE_T) && !defined(__DEFINED_size_t)
#define _HAVE_SIZE_T
#define _SIZE_T
typedef unsigned _Addr size_t;
#define __DEFINED_size_t
#endif

#if !defined(_SSIZE_T) && !defined(ssize_t) && !defined(__DEFINED_ssize_t)
#define _SSIZE_T
typedef _Addr ssize_t;
#define __DEFINED_ssize_t
#endif

#if !defined(_INTMAX_T) && !defined(intmax_t) && !defined(__DEFINED_intmax_t)
typedef long long intmax_t;
#define _INTMAX_T
#define __DEFINED_intmax_t
#endif

#if !defined(_UINTMAX_T) && !defined(uintmax_t) && !defined(__DEFINED_uintmax_t)
typedef unsigned long long uintmax_t;
#define _UINTMAX_T
#define __DEFINED_uintmax_t
#endif




struct iovec { void *iov_base; size_t iov_len; };


#endif
