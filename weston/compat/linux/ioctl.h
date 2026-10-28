/* linux/ioctl.h 的 musl 垫片。
 *
 * Linux uapi 头（如 <linux/input.h>、libweston/linux-sync-file-uapi.h）会
 * #include <linux/ioctl.h> 取 _IOC/_IOR/_IOW/_IO 宏；musl 只提供
 * <sys/ioctl.h>，所以这里转一手。不能放进 eggs/libevdev —— 那是第三方
 * 快照，改动会被上游覆盖。
 */
#ifndef _LINUX_IOCTL_H_SHIM
#define _LINUX_IOCTL_H_SHIM

#include <sys/ioctl.h>

#endif /* _LINUX_IOCTL_H_SHIM */
