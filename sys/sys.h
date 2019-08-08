#pragma once
#include "bootstrap/bootstrap.h"

package(sys);

extern void sys$printf(const char *s, ...);
extern char *sys$sprintf(const char *s, ...);
extern int sys$execve(const char *path, char *const argv[], char *envp[]);
