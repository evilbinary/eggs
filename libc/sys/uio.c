#include "sys/uio.h"

ssize_t readv(int fd, const struct iovec *iov, int count) {
  printf("not impl readv \n");

  return -1;
}
ssize_t writev(int fd, const struct iovec *iov, int count) {
  printf("not impl writev \n");

  return -1;
}