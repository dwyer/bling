#pragma once

#define $import(_)
#define $package(_)

#include "runtime/clib.h"

extern void *nil;

extern void print(const char *fmt, ...);
extern void panic(const char *fmt, ...);
extern bool streq(const char *a, const char *b);

extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);

extern char *strdup(const char *);
extern size_t strlcpy(char *, const char *, size_t);
extern size_t strlen(const char *);

extern void *malloc(size_t);
extern void free(void *);
extern void *realloc(void *, size_t); // used by slice append
