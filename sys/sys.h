#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

extern int sys$errno();
extern void sys$errnoReset();
extern char *sys$errnoString();

extern int sys$execve(const char *path, char *const argv[], char *envp[]);

extern char *sys$getenv(const char *key);

extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);
