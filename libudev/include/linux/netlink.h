/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * linux/netlink.h 的最小子集（uapi）。
 *
 * 来源：Linux uapi 的 netlink.h，只保留 libudev-zero 需要的那部分
 * （struct sockaddr_nl / sockaddr_nl 常量 / nlmsghdr 与 NLMSG 宏）。
 * musl 不提供 linux/ 下的 uapi 头，Debian 环境里这些由 linux-libc-dev 给。
 */
#ifndef _UAPI_LINUX_NETLINK_H
#define _UAPI_LINUX_NETLINK_H

#include <linux/types.h>

/* AF_NETLINK 由 libc 的 <sys/socket.h> 定义（musl: 16） */

struct sockaddr_nl {
	unsigned short nl_family;       /* AF_NETLINK（= sa_family_t，2 字节） */
	unsigned short nl_pad;          /* zero */
	__u32 nl_pid;                   /* port ID */
	__u32 nl_groups;                /* multicast groups mask */
};

struct nlmsghdr {
	__u32 nlmsg_len;   /* Length of message including header */
	__u16 nlmsg_type;  /* Message content */
	__u16 nlmsg_flags; /* Additional flags */
	__u32 nlmsg_seq;   /* Sequence number */
	__u32 nlmsg_pid;   /* Sending process port ID */
};

struct nlmsgerr {
	int error;
	struct nlmsghdr msg;
};

/* netlink 协议族 */
#define NETLINK_ROUTE 0
#define NETLINK_KOBJECT_UEVENT 15

/* setsockopt 等级（bind 多播组用） */
#define SOL_NETLINK 270
#define NETLINK_ADD_MEMBERSHIP 1
#define NETLINK_DROP_MEMBERSHIP 2

/* 消息类型 */
#define NLMSG_NOOP 0x1
#define NLMSG_ERROR 0x2
#define NLMSG_DONE 0x3
#define NLMSG_OVERRUN 0x4
#define NLMSG_MIN_TYPE 0x10

#define NLMSG_ALIGNTO 4U
#define NLMSG_ALIGN(len) (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1))
#define NLMSG_HDRLEN ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len) + NLMSG_HDRLEN)
#define NLMSG_SPACE(len) NLMSG_ALIGN(NLMSG_LENGTH(len))
#define NLMSG_DATA(nlh) ((void *)(((char *)(nlh)) + NLMSG_HDRLEN))
#define NLMSG_NEXT(nlh, len)                                                   \
	((len) -= NLMSG_ALIGN((nlh)->nlmsg_len),                               \
	 (struct nlmsghdr *)(((char *)(nlh)) + NLMSG_ALIGN((nlh)->nlmsg_len)))
#define NLMSG_OK(nlh, len)                                                     \
	((len) >= (int)sizeof(struct nlmsghdr) &&                              \
	 (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && (nlh)->nlmsg_len <= (len))
#define NLMSG_PAYLOAD(nlh, len) ((nlh)->nlmsg_len - NLMSG_SPACE((len)))

#endif
