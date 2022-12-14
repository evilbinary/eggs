#include <features.h>
#include "libc.h"

#define START "_start"

typedef struct exec {
  int argc;
  char **argv;
  char **envp;
  char *filename;
} exec_t;

// #include "crt_arch.h"

int main();
weak void _init();
weak void _fini();
int __libc_start_main(int (*)(), int, char **,
	void (*)(), void(*)(), void(*)());
void _start_c(exec_t *p) {

  int argc = p->argc;
	char **argv = p->argv;
  __libc_start_main(main, argc, argv, _init, _fini, 0);
}