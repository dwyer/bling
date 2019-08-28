#include "sys/sys.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

extern int sys$memcmp(const void *dst, const void *src, sys$Size n) {
    return memcmp(dst, src, n);
}

extern void *sys$memcpy(void *dst, const void *src, sys$Size n) {
    return memcpy(dst, src, n);
}

extern void *sys$memset(void *dst, int c, sys$Size n) {
    return memset(dst, c, n);
}

extern char *sys$strdup(const char *s) {
    return strdup(s);
}

extern char *sys$strndup(const char *s, sys$Size n) {
    return strndup(s, n);
}

extern sys$Size sys$strlen(const char *s) {
    return strlen(s);
}

extern void *sys$malloc(sys$Size n) {
    return malloc(n);
}

extern void sys$free(void *ptr) {
    free(ptr);
}

extern void *sys$realloc(void *ptr, sys$Size n) {
    return realloc(ptr, n);
}

extern bool sys$streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

extern int sys$run(char *const argv[]) {
    extern char **environ;
    int status = -1;
    pid_t pid = fork();
    switch (pid) {
    case -1:
        return -1;
    case 0:
        execve(argv[0], argv, environ);
        return errno;
    default:
        waitpid(pid, &status, 0);
        return status;
    }
}
