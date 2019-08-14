#include "sys/sys.h"

#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

extern int sys$errno() {
    return errno;
}

extern void sys$errnoReset() {
    errno = 0;
}

extern char *sys$errnoString() {
    return strerror(errno);
}

extern int sys$execve(const char *path, char *const argv[], char *envp[]) {
    return execve(path, argv, envp);
}

extern char *sys$getenv(const char *key) {
    return getenv(key);
}
