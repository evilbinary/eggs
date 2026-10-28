/* YiYiYa 上的 wayland 手工配置（替代 meson 生成）。
 * 只保留 wayland-os.c / cursor 需要的探测结果，见 zdata/wayland.md。
 * 【新增宏前先确认内核真的支持】——例如 memfd_create 尚未实现，
 * HAVE_MEMFD_CREATE 必须为 0（wayland-shm.c 只是跳过 seal 检查）。 */
#ifndef YIYIYA_WAYLAND_CONFIG_H
#define YIYIYA_WAYLAND_CONFIG_H

/* accept4 已实现（sysfn_net.c sys_accept4） */
#define HAVE_ACCEPT4 1

/* Linux 语义正常：SCM_RIGHTS 收 fd 不需要额外 fcntl（Darwin 才 broken） */
#define HAVE_BROKEN_MSG_CMSG_CLOEXEC 0

/* mkstemp 家族：musl 提供 mkostemp，O_CLOEXEC 由 fcntl 兜底 */
#define HAVE_MKOSTEMP 1

/* posix_fallocate 未实现：wayland-os.c 有 write 填充回退 */
#define HAVE_POSIX_FALLOCATE 0

/* memfd_create(319) 已实现（sysfn_memfd.c，文件后端 /tmp/.memfd-*），
 * wayland 建池走 memfd + ftruncate + 文件 MAP_SHARED */
#define HAVE_MEMFD_CREATE 1

/* FAT 的 readdir 不提供可靠 d_type，走 stat 回退 */
#define HAVE_D_TYPE 0

/* Darwin 专有探测：wayland-os.c 用 #ifdef 判断（定义成 0 也算"有"），
 * 所以这两个宏必须【完全不定义】。 */
/* #undef HAVE_SYS_UCRED_H */
/* #undef HAVE_XUCRED_CR_PID */

#endif
