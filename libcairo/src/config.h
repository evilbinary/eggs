/* YiYiYa: cairo 1.18.6 的 config.h（上游由 meson 的 configuration_data()
 * 生成）。这里按 musl/aarch64 的实际情况手工给：只列 cairo 源码真正用到的
 * HAVE_*，没列出来的一律走"没有该函数/头"的分支。
 *
 * 与宿主的差异：没有 funopen/FLOCKFILE 之外的东西，locale/fenv 正常。
 * 若某天编译报 "implicit declaration" 或行为异常，优先回来确认这里缺哪个宏。
 */
#ifndef CAIRO_CONFIG_H
#define CAIRO_CONFIG_H

#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_BYTESWAP_H 1
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_PTHREAD_H 1

#define HAVE_CTIME_R 1
#define HAVE_LOCALTIME_R 1
#define HAVE_STRNDUP 1
#define HAVE_STRNDUP_UNISTD 1
#define HAVE_FLOCKFILE 1
#define HAVE_FUNOPEN 0

/* 有 pthread：cairo-mutex 走真互斥（wl_shm 多线程渲染要用） */
#define HAVE_PTHREAD 1

/* 【必须有】cairo-wideint 靠这几个宏决定用原生 64/128 位还是软件模拟结构体。
 * 缺了它们 cairo_int64_t 会变成 struct _cairo_uint64，contour/polygon 这些
 * 文件里所有整数运算都编不过（报 invalid operands to binary +）。 */
#define HAVE_UINT64_T 1
#define HAVE_INT64_T 1
/* 128 位：aarch64 上 __int128 是原生类型，走它比软件模拟快。注意 musl 的
 * <stdint.h> 不提供 uint128_t/int128_t 这两个名字（glibc 也没有），而 cairo
 * 在 HAVE_UINT128_T 时就直接用这两个名字 ⇒ 必须在这里补 typedef。 */
#define HAVE_UINT128_T 1
typedef __uint128_t uint128_t;
typedef __int128_t int128_t;

/* aarch64 允许非对齐访问，cairo 据此选快路径 */
#define HAVE_UNALIGNED_ACCESS 1

/* 没有 getline/vasnprintf 的平台差异项；glibc 专有的一律不开 */
#define HAVE_GETLINE 0
#define HAVE_ASPRINTF 0

#endif
