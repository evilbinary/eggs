#include "stdio.h"

int main();

int __libc_start_main(int (*)(), int, char **);

void start(long *p) {
  int argc = p[0];
  char **argv = (void *)(p + 1);
  char **envp = (void *)(p + 2);
  int ret = __libc_start_main(main, argc, argv);
  exit(ret);
}


int __libc_start_main(int (*main)(int, char **, char **), int argc,
                      char **argv) {
  char **envp = argv + argc + 1;
  //init libc here
  return main(argc,argv,envp);
}