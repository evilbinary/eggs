/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#ifndef SYSCALL_H
#define SYSCALL_H
#include "stdint.h"

#define ISR_SYSCALL 0x30

#if defined(ARM) || defined(X86)
enum {
  SYS_EXIT = 1,
  SYS_FORK = 2,
  SYS_READ = 3,
  SYS_WRITE = 4,
  SYS_OPEN = 5,
  SYS_CLOSE = 6,
  SYS_UNLINK = 10,
  SYS_EXEC = 11,
  SYS_CHDIR = 12,
  SYS_SEEK = 19,
  SYS_GETPID = 20,
  SYS_ALARM = 27,
  SYS_RENAME = 38,
  SYS_PIPE = 42,
  SYS_DUP = 41,
  SYS_BRK = 45,
  SYS_SBRK = 46,
  SYS_IOCTL = 54,
  SYS_UMASK = 60,
  SYS_DUP2 = 63,
  SYS_GETPPID = 64,
  SYS_READDIR = 89,
  SYS_STAT = 106,
  SYS_FSTAT = 108,
  SYS_SYSINFO = 116,
  SYS_CLONE = 120,
  SYS_FCHDIR = 133,
  SYS_LLSEEK = 140,
  SYS_WRITEV = 146,
  SYS_READV = 145,
  SYS_YIELD = 158,
  SYS_GETCWD = 183,
  SYS_MMAP2 = 192,
  SYS_MPROTECT = 125,
  SYS_NANOSLEEP = 162,
  SYS_MREMAP = 163,
  SYS_RT_SIGACTION = 174,
  SYS_RT_SIGPROCMASK = 175,
  SYS_GETDENTS = 141,
  SYS_MUNMAP = 91,
  SYS_FCNT64 = 221,
  SYS_GETDENTS64 = 220,
  SYS_CLOCK_NANOSLEEP = 230,
  SYS_SET_THREAD_AREA = 243,
  SYS_STATX = 383,
  SYS_CLOCK_GETTIME64 = 403,

  SYS_PRINT = 500,
  SYS_PRINT_AT = 501,
  SYS_DEV_READ = 502,
  SYS_DEV_WRITE = 503,
  SYS_DEV_IOCTL = 504,
  SYS_TEST = 505,
  SYS_MAP = 506,
  SYS_UMAP = 507,
  SYS_VALLOC = 508,
  SYS_VFREE = 509,
  SYS_VHEAP = 510,
  SYS_DUMPS = 511,
  SYS_SELF = 512,
  SYS_MEMINFO = 513,
  SYS_THREAD_SELF = 514,
};

#else
enum {
  SYS_EXIT = 1,
  SYS_FORK = 2,
  SYS_READ = 3,
  SYS_WRITE = 4,
  SYS_OPEN = 5,
  SYS_CLOSE = 6,
  SYS_UNLINK = 10,
  SYS_EXEC = 11,
  SYS_CHDIR = 12,
  SYS_SEEK = 19,
  SYS_GETPID = 20,
  SYS_ALARM = 27,
  SYS_RENAME = 38,
  SYS_PIPE = 42,
  SYS_DUP = 41,
  SYS_BRK = 45,
  SYS_SBRK = 46,
  SYS_IOCTL = 54,
  SYS_UMASK = 60,
  SYS_DUP2 = 63,
  SYS_GETPPID = 64,
  SYS_READDIR = 89,
  SYS_STAT = 106,
  SYS_FSTAT = 108,
  SYS_SYSINFO = 116,
  SYS_CLONE = 120,
  SYS_FCHDIR = 133,
  SYS_LLSEEK = 140,
  SYS_WRITEV = 146,
  SYS_READV = 145,
  SYS_YIELD = 158,
  SYS_GETCWD = 183,
  SYS_MMAP2 = 192,
  SYS_MPROTECT = 125,
  SYS_NANOSLEEP = 162,
  SYS_MREMAP = 163,
  SYS_RT_SIGACTION = 174,
  SYS_RT_SIGPROCMASK = 175,
  SYS_GETDENTS = 141,
  SYS_MUNMAP = 91,
  SYS_FCNT64 = 221,
  SYS_GETDENTS64 = 220,
  SYS_CLOCK_NANOSLEEP = 230,
  SYS_SET_THREAD_AREA = 243,
  SYS_STATX = 383,
  SYS_CLOCK_GETTIME64 = 403,

  SYS_PRINT = 500,
  SYS_PRINT_AT = 501,
  SYS_DEV_READ = 502,
  SYS_DEV_WRITE = 503,
  SYS_DEV_IOCTL = 504,
  SYS_TEST = 505,
  SYS_MAP = 506,
  SYS_UMAP = 507,
  SYS_VALLOC = 508,
  SYS_VFREE = 509,
  SYS_VHEAP = 510,
  SYS_DUMPS = 511,
  SYS_SELF = 512,
  SYS_MEMINFO = 513,
  SYS_THREAD_SELF = 514,
};
#endif

void* syscall0(u32 num);
void* syscall1(u32 num, void* arg0);
void* syscall2(u32 num, void* arg0, void* arg1);
void* syscall3(u32 num, void* arg0, void* arg1, void* arg2);
void* syscall4(u32 num, void* arg0, void* arg1, void* arg2, void* arg3);
void* syscall5(u32 num, void* arg0, void* arg1, void* arg2, void* arg3,
               void* arg4);

#endif