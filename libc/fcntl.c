#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <support.h>
#include <unistd.h>
#include "syscall.h"

#define UNIMPL() printf("unimpl %s:%d %s\n",__FILE__,__LINE__,__FUNCTION__)


int open(const char *filename, int flags, ...) {
  int fd;
  mode_t mode = 0;

  if (flags & O_CREAT) {
    va_list vargs;
    va_start(vargs, flags);
    mode = va_arg(vargs, mode_t);
    va_end(vargs);
    (void)mode;
  }

  fd = ya_open(filename, flags);

  if (fd < 0) return -1;
  return fd;
}

int creat(const char *filename, u32 mode) {
  return open(filename, O_CREAT | O_WRONLY | O_TRUNC);
}

int fcntl(int fd, int cmd, ...){
  UNIMPL();

  return 1;
}