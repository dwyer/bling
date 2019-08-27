#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

typedef u64 sys$Size;

extern int sys$errno();
extern void sys$errnoReset();
extern char *sys$errnoString();

extern int sys$execve(const char *path, char *const argv[], char *envp[]);

extern char *sys$getenv(const char *key);

// stdio
extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);

// stdlib
extern void *sys$malloc(sys$Size n);
extern void sys$free(void *ptr);
extern void *sys$realloc(void *ptr, sys$Size n);

// strings
extern int sys$memcmp(const void *dst, const void *src, sys$Size n);
extern void *sys$memcpy(void *dst, const void *src, sys$Size n);
extern void *sys$memset(void *dst, int c, sys$Size n);
extern char *sys$strdup(const char *s);
extern sys$Size sys$strlcpy(char *dst, const char *src, sys$Size n);
extern sys$Size sys$strlen(const char *s);

extern bool sys$streq(const char *a, const char *b);

extern int sys$run(char *const argv[]);
