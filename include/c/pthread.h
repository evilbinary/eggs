#ifndef _PTHREAD_H
#define _PTHREAD_H

#include "stdint.h"

typedef unsigned long pthread_t;

typedef struct { 
	uint32_t attr;
} pthread_attr_t;

#endif