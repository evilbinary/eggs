/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include "client.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* /dev/foot 的 fd 进程内复用：每次 open/close 会触发内核 service_close，
 * 把本线程注册过的 client 全部摘除（再调 client_call 就找不到服务了）。
 * 这里只开一次、不主动关；进程退出由内核回收 fd。 */
static int foot_cap_fd = -1;

static int foot_cap_fd_get(void) {
  if (foot_cap_fd < 0) {
    foot_cap_fd = open("/dev/foot", O_RDWR);
  }
  return foot_cap_fd;
}

static int foot_cap_call(unsigned int cmd, void* ctl) {
  int fd = foot_cap_fd_get();
  if (fd < 0) {
    printf("foot cap: open /dev/foot failed (%d)\n", fd);
    return -1;
  }
  int ret = ioctl(fd, cmd, ctl);
  return ret;
}

foot_cap_t foot_cap_get_mmio(const char* name) {
  if (name == NULL) {
    return FOOT_CAP_NONE;
  }
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.name = (char*)name;
  if (foot_cap_call(SYS_CAP_GET_MMIO, &ctl) < 0) {
    return FOOT_CAP_NONE;
  }
  return (foot_cap_t)ctl.handle;
}

int foot_cap_map(foot_cap_t cap, void* vaddr, unsigned long offset,
                 unsigned long len) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = cap;
  ctl.vaddr = (unsigned int)(unsigned long)vaddr;
  ctl.offset = (unsigned int)offset;
  ctl.len = (unsigned int)len;
  return foot_cap_call(SYS_CAP_MAP, &ctl) < 0 ? -1 : 0;
}

int foot_cap_unmap(foot_cap_t cap) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = cap;
  return foot_cap_call(SYS_CAP_UNMAP, &ctl) < 0 ? -1 : 0;
}

int foot_cap_info(foot_cap_t cap, unsigned int* pa, unsigned int* size,
                  unsigned int* rights) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = cap;
  if (foot_cap_call(SYS_CAP_INFO, &ctl) < 0) {
    return -1;
  }
  if (pa != NULL) *pa = ctl.pa;
  if (size != NULL) *size = ctl.size;
  if (rights != NULL) *rights = ctl.rights;
  return 0;
}

foot_cap_t foot_cap_derive(foot_cap_t cap, unsigned int rights) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = cap;
  ctl.rights = rights;
  if (foot_cap_call(SYS_CAP_DERIVE, &ctl) < 0) {
    return FOOT_CAP_NONE;
  }
  return (foot_cap_t)ctl.handle;
}

int foot_cap_revoke(foot_cap_t cap) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = cap;
  return foot_cap_call(SYS_CAP_REVOKE, &ctl) < 0 ? -1 : 0;
}

foot_cap_t foot_cap_alloc_frame(unsigned long size, unsigned long pa,
                               unsigned int rights, unsigned int* out_pa,
                               unsigned int* out_size) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.size = (unsigned int)size;
  ctl.pa = (unsigned int)pa;
  ctl.rights = rights;
  if (foot_cap_call(SYS_CAP_ALLOC_FRAME, &ctl) < 0) {
    return FOOT_CAP_NONE;
  }
  if (out_pa != NULL) *out_pa = ctl.pa;
  if (out_size != NULL) *out_size = ctl.size;
  return (foot_cap_t)ctl.handle;
}

foot_cap_t foot_cap_alloc_arena(unsigned long size, unsigned long alignment,
                                unsigned int rights, unsigned int* out_pa,
                                unsigned int* out_size) {
  foot_cap_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.size = (unsigned int)size;
  ctl.offset = (unsigned int)alignment; /* cap_ctl_t 的 offset 复用做物理对齐 */
  ctl.rights = rights;
  if (foot_cap_call(SYS_CAP_ALLOC_ARENA, &ctl) < 0) {
    return FOOT_CAP_NONE;
  }
  if (out_pa != NULL) *out_pa = ctl.pa;
  if (out_size != NULL) *out_size = ctl.size;
  return (foot_cap_t)ctl.handle;
}

foot_cap_t foot_cap_ep_create(void) {
  foot_ep_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  if (foot_cap_call(SYS_CAP_EP_CREATE, &ctl) < 0) {
    return FOOT_CAP_NONE;
  }
  return (foot_cap_t)ctl.handle;
}

static int foot_cap_ep_send_f(foot_cap_t ep, const void* data,
                              unsigned long len, foot_cap_t grant,
                              unsigned int flags) {
  foot_ep_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = ep;
  ctl.grant = grant;
  ctl.len = (unsigned int)len;
  ctl.flags = flags;
  ctl.buf = (char*)data;
  return foot_cap_call(SYS_CAP_EP_SEND, &ctl) < 0 ? -1 : 0;
}

static int foot_cap_ep_recv_f(foot_cap_t ep, void* data, unsigned long cap,
                              unsigned long* out_len, foot_cap_t* out_grant,
                              unsigned int flags) {
  foot_ep_ctl_t ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.handle = ep;
  ctl.len = (unsigned int)cap;
  ctl.flags = flags;
  ctl.buf = (char*)data;
  if (foot_cap_call(SYS_CAP_EP_RECV, &ctl) < 0) {
    return -1;
  }
  if (out_len != NULL) *out_len = ctl.out_len;
  if (out_grant != NULL) *out_grant = (foot_cap_t)ctl.grant;
  return 0;
}

int foot_cap_ep_send(foot_cap_t ep, const void* data, unsigned long len,
                     foot_cap_t grant) {
  return foot_cap_ep_send_f(ep, data, len, grant, 0);
}

int foot_cap_ep_recv(foot_cap_t ep, void* data, unsigned long cap,
                     unsigned long* out_len, foot_cap_t* out_grant) {
  return foot_cap_ep_recv_f(ep, data, cap, out_len, out_grant, 0);
}

int foot_cap_ep_try_send(foot_cap_t ep, const void* data, unsigned long len,
                         foot_cap_t grant) {
  return foot_cap_ep_send_f(ep, data, len, grant, FOOT_EP_NONBLOCK);
}

int foot_cap_ep_try_recv(foot_cap_t ep, void* data, unsigned long cap,
                         unsigned long* out_len, foot_cap_t* out_grant) {
  return foot_cap_ep_recv_f(ep, data, cap, out_len, out_grant,
                            FOOT_EP_NONBLOCK);
}

/* ===================== dcache 维护 ===================== */

static int foot_cacheflush(unsigned long start, unsigned long end, int flags) {
  return (int)syscall(FOOT_SYS_CACHEFLUSH, start, end, flags);
}

void foot_cache_clean(void* start, void* end) {
  foot_cacheflush((unsigned long)start, (unsigned long)end,
                  FOOT_CACHE_CLEAN_INV);
}

void foot_cache_invalidate(void* start, void* end) {
  foot_cacheflush((unsigned long)start, (unsigned long)end,
                  FOOT_CACHE_INVALIDATE);
}
