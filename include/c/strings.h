#ifndef _STRINGS_H
#define _STRINGS_H


#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

int    bcmp(const void *, const void *, size_t);
void   bcopy(const void *, void *, size_t);
void   bzero(void *, size_t);
int    ffs(int);
char   *index(const char *, int);
char   *rindex(const char *, int);
int    strcasecmp(const char *, const char *);
int    strncasecmp(const char *, const char *, size_t);

size_t strnlen(const char *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif

