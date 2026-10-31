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

static int foot_cap_call(unsigned int cmd, foot_cap_ctl_t* ctl) {
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
