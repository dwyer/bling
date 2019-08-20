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

extern int sys$memcmp(const void *dst, const void *src, size_t n) {
    return memcmp(dst, src, n);
}

extern void *sys$memcpy(void *dst, const void *src, size_t n) {
    return memcpy(dst, src, n);
}

extern void *sys$memset(void *dst, int c, size_t n) {
    return memset(dst, c, n);
}

extern char *sys$strdup(const char *s) {
    return strdup(s);
}

extern size_t sys$strlcpy(char *dst, const char *src, size_t n) {
    return strlcpy(dst, src, n);
}

extern size_t sys$strlen(const char *s) {
    return strlen(s);
}

extern void *sys$malloc(size_t n) {
    return malloc(n);
}

extern void sys$free(void *ptr) {
    free(ptr);
}

extern void *sys$realloc(void *ptr, size_t n) {
    return realloc(ptr, n);
}

extern int sys$run(char *const argv[]) {
    int status = -1;
    pid_t pid = fork();
    switch (pid) {
    case -1:
        return -1;
    case 0:
        execve(argv[0], argv, NULL);
        return errno;
    default:
        waitpid(pid, &status, 0);
        return status;
    }
}
