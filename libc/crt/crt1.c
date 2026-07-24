#include "stdio.h"

__attribute__((weak)) int main();

int __libc_start_main(int (*)(), int, char **,char **);

typedef struct exec{
    int argc;
    char** argv;
    char **envp;
    char* filename;
}exec_t;

void start(exec_t *p) {
  int ret = __libc_start_main(main, p->argc, p->argv,p->envp);
  exit(ret);
}


int __libc_start_main(int (*main)(int, char **, char **), int argc,
                      char **argv,char **envp) {
  //init libc here
  return main(argc,argv,envp);
}