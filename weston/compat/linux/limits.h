/* musl/newlib 的工具链不提供 linux/limits.h（内核 uapi 头）。
 * glibc 的版本只是把 limits.h 里的整型上下限再摘一遍，这里直接转发。 */
#ifndef YIYIYA_LINUX_LIMITS_H
#define YIYIYA_LINUX_LIMITS_H

#include <limits.h>

#endif /* YIYIYA_LINUX_LIMITS_H */
