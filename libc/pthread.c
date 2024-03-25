#include "pthread.h"

int pthread_create(pthread_t *restrict res,
                   const pthread_attr_t *restrict attrp, void *(*entry)(void *),
                   void *restrict arg) {
  printf("not impl pthread_create\n");

  return -1;
}

int pthread_join(pthread_t t, void **res) {
  printf("not impl pthread_join\n");

  return -1;
}