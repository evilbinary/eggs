#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include "stdint.h"

typedef struct {
    uint32_t value;
    uint32_t waiters;
} sem_t;

#define SEM_FAILED ((sem_t *)0)

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

#endif
