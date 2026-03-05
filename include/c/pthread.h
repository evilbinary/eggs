#ifndef _PTHREAD_H
#define _PTHREAD_H

#include "stdint.h"

typedef int32_t pthread_t;

#ifndef _HAVE_PTHREAD_ATTR_T
#define _HAVE_PTHREAD_ATTR_T
typedef struct {
	uint32_t attr;
} pthread_attr_t;
#endif

/* Scheduling policies */
#define SCHED_OTHER    0
#define SCHED_FIFO     1
#define SCHED_RR       2

/* Pthread attributes constants */
#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_CREATE_DETACHED     1

#define PTHREAD_INHERIT_SCHED       0
#define PTHREAD_EXPLICIT_SCHED      1

#define PTHREAD_SCOPE_PROCESS       0
#define PTHREAD_SCOPE_SYSTEM        1

/* Mutex types */
#define PTHREAD_MUTEX_NORMAL        0
#define PTHREAD_MUTEX_RECURSIVE     1
#define PTHREAD_MUTEX_ERRORCHECK    2
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL

/* Scheduling parameters */
struct sched_param {
    int sched_priority;
};

/* Function declarations */
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval);
pthread_t pthread_self(void);

int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param);
int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);
int pthread_setschedprio(pthread_t thread, int prio);

#endif