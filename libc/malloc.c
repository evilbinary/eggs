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
  return ya_realloc(p,size);
}

void *calloc(size_t nobj, size_t size) {
  return ya_calloc(nobj,size);
}

size_t malloc_usable_size(void *p) { return ya_real_size(p); }