#include <stdio.h>
#include <stdlib.h>
// #include <pthread.h>
// #include <sys/mman.h>
#include <unistd.h>

#include "syscall.h"
#include "time.h"
#include "yiyiya.h"
#include "sys/stat.h"

u32 time_fd = -1;

void* ya_valloc(void* addr, size_t size) {
  return syscall2(SYS_VALLOC, addr, size);
}

void ya_vfree(void* addr) { syscall1(SYS_VFREE, addr); }

u32 ya_open(const char* filename, int flags) {
  return syscall2(SYS_OPEN, filename, flags);
}

u32 ya_close(int fd) { return syscall1(SYS_CLOSE, fd); }

size_t ya_write(u32 fd, void* buf, size_t nbytes) {
  return syscall3(SYS_WRITE, fd, buf, nbytes);
}
size_t ya_read(u32 fd, void* buf, size_t nbytes) {
  return syscall3(SYS_READ, fd, buf, nbytes);
}

size_t ya_seek(u32 fd, ulong offset, int whence) {
  return syscall3(SYS_SEEK, fd, offset, whence);
}

uint32_t secs_of_years(int years) {
  uint32_t days = 0;
  years += 2000;
  while (years > 1969) {
    days += 365;
    if (years % 4 == 0) {
      if (years % 100 == 0) {
        if (years % 400 == 0) {
          days++;
        }
      } else {
        days++;
      }
    }
    years--;
  }
  return days * 86400;
}

uint32_t secs_of_month(int months, int year) {
  year += 2000;

  uint32_t days = 0;
  switch (months) {
    case 11:
      days += 30;
    case 10:
      days += 31;
    case 9:
      days += 30;
    case 8:
      days += 31;
    case 7:
      days += 31;
    case 6:
      days += 30;
    case 5:
      days += 31;
    case 4:
      days += 30;
    case 3:
      days += 31;
    case 2:
      days += 28;
      if ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) {
        days++;
      }
    case 1:
      days += 31;
    default:
      break;
  }
  return days * 86400;
}

u32 ya_time(time_t* current) {
  if (time_fd == -1) {
    time_fd = ya_open("/dev/time", 0);
  }
  if (time_fd < 0) return 0;
  rtc_time_t time;
  time.day = 1;
  time.hour = 0;
  time.minute = 0;
  time.month = 1;
  time.second = 0;
  time.year = 1900;
  int ret = ya_read(time_fd, &time, sizeof(rtc_time_t));
  if (ret < 0) {
    return 0;
  }
  uint32_t seconds = secs_of_years(time.year - 1) +
                     secs_of_month(time.month - 1, time.year) +
                     (time.day - 1) * 86400 + time.hour * 3600 +
                     time.minute * 60 + time.second + 0;
  *current = seconds;
  return ret;
}

int ya_ioctl(int fd, int cmd, void* args) {
  return syscall3(SYS_IOCTL, fd, cmd, args);
}

int ya_fork() { return syscall0(SYS_FORK); }

int ya_getpid() { return syscall0(SYS_GETPID); }

int ya_getppid() { return syscall0(SYS_GETPPID); }

int ya_pipe(int fds[2]) { return syscall1(SYS_PIPE, fds); }

int ya_dup(int fd) { return syscall1(SYS_DUP, fd); }
int ya_dup2(int oldfd, int newfd) { return syscall2(SYS_DUP2, oldfd, newfd); }

int ya_exec(const char* pathname, char* const argv[], char* const envp[]) {
  return syscall3(SYS_EXEC, pathname, argv, envp);
}

void ya_exit(u32 ret) { syscall1(SYS_EXIT, ret); }

int ya_readdir(int fd, int index, struct dirent* dirent) {
  return syscall3(SYS_READDIR, fd, dirent, index);
}

int ya_rename(const char* old, const char* new) {
  return syscall2(SYS_RENAME, old, new);
}

int ya_getcwd(char* buf, size_t size) {
  return syscall2(SYS_GETCWD, buf, size);
}

int ya_chdir(char* path) { return syscall1(SYS_CHDIR, path); }

int ya_fchdir(int fd) { return syscall1(SYS_FCHDIR, fd); }

int ya_fstat(int fd, struct stat *buf) { return syscall2(SYS_STAT, fd,buf); }

int ya_stat(const char *path, struct stat *buf) { return syscall2(SYS_STAT, path,buf); }


int ya_nanosleep(const struct timespec *req, struct timespec *rem){
	return syscall4(SYS_CLOCK_NANOSLEEP,1, 0, req, rem);
}


void * ya_mremap(void *old_addr, size_t old_len, size_t new_len, int flags, ...){

	return (void *)syscall4(SYS_MREMAP, old_addr, old_len, new_len, flags);
}

void __main() {
  // printf("main call\n");
}