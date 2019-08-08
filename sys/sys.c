#include "sys/sys.h"

#include <unistd.h>

extern int sys$execve(const char *path, char *const argv[], char *envp[]) {
    return execve(path, argv, envp);
}
