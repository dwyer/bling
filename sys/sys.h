#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

extern int sys$errno();
extern void sys$errnoReset();
extern char *sys$errnoString();

extern int sys$execve(const char *path, char *const argv[], char *envp[]);

extern char *sys$getenv(const char *key);

// stdio
extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);

// stdlib
extern void *sys$malloc(size_t n);
extern void sys$free(void *ptr);
extern void *sys$realloc(void *ptr, size_t n);

// strings
extern int sys$memcmp(const void *dst, const void *src, size_t n);
extern void *sys$memcpy(void *dst, const void *src, size_t n);
extern void *sys$memset(void *dst, int c, size_t n);
extern char *sys$strdup(const char *s);
extern size_t sys$strlcpy(char *dst, const char *src, size_t n);
extern size_t sys$strlen(const char *s);

extern bool sys$streq(const char *a, const char *b);

extern int sys$run(char *const argv[]);
