#pragma once

#include "runtime/clib.h"

extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);

extern int strcmp(const char *, const char *);
extern char *strdup(const char *);
extern char *strerror(int);
extern size_t strlcpy(char *, const char *, size_t);
extern size_t strlen(const char *);
