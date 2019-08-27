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

extern int sys$memcmp(const void *dst, const void *src, u64 n) {
    return memcmp(dst, src, n);
}

extern void *sys$memcpy(void *dst, const void *src, u64 n) {
    return memcpy(dst, src, n);
}

extern void *sys$memset(void *dst, int c, u64 n) {
    return memset(dst, c, n);
}

extern char *sys$strdup(const char *s) {
    return strdup(s);
}

extern u64 sys$strlcpy(char *dst, const char *src, u64 n) {
    return strlcpy(dst, src, n);
}

extern u64 sys$strlen(const char *s) {
    return strlen(s);
}

extern void *sys$malloc(u64 n) {
    return malloc(n);
}

extern void sys$free(void *ptr) {
    free(ptr);
}

extern void *sys$realloc(void *ptr, u64 n) {
    return realloc(ptr, n);
}

extern bool sys$streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
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
