/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/
#include <malloc.h>

#include "syscall.h"

// implements
void *malloc(unsigned long size) { return ya_alloc(size); }

void free(void *addr) { ya_free(addr); }

void *realloc(void *p, size_t size) {
  void *ptr;
  int real_size = ya_real_size(p);
  if (size == 0) {
    free(p);
    return NULL;
  }
  if (p == NULL) return malloc(size);
  if (size > real_size) {
    ptr = malloc(size);
    if(ptr!=NULL){
      memcpy(ptr, p, real_size);
    }else{
      printf("malloc error\n");
    }
    free(p);
  }
  return ptr;
}

void *calloc(size_t nobj, size_t size) {
  int malloc_size = nobj * size;
  void *p = malloc(malloc_size);
  int real_size = malloc_usable_size(p);
  memset(p, 0, real_size);
  return p;
}

size_t malloc_usable_size(void *p) { return ya_real_size(p); }