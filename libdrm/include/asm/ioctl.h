/* asm/ioctl.h - 内核 uapi 的 ioctl 宏替身
 *
 * musl 的 <sys/ioctl.h> 已提供 _IO/_IOR/_IOW/_IOWR，
 * 内核 uapi（drm.h）通过 asm/ioctl.h 拿这些宏。
 */
#ifndef _ASM_IOCTL_FOOT_H
#define _ASM_IOCTL_FOOT_H

#include <sys/ioctl.h>

#endif
