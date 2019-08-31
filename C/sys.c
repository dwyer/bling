#include "gen/C/C.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern int C$getErrno(void) {
    return errno;
}

extern void C$setErrno(int n) {
    errno = n;
}

extern char *C$strerror(int n) {
    return strerror(n);
}

extern int C$execve(const char *path, char *const argv[], char *const envp[]) {
    return execve(path, argv, envp);
}

extern char *C$getenv(const char *key) {
    return getenv(key);
}

extern int C$memcmp(const void *dst, const void *src, C$size_t n) {
    return memcmp(dst, src, n);
}

extern void *C$memcpy(void *dst, const void *src, C$size_t n) {
    return memcpy(dst, src, n);
}

extern void *C$memset(void *dst, int c, C$size_t n) {
    return memset(dst, c, n);
}

extern char *C$strdup(const char *s) {
    return strdup(s);
}

extern char *C$strndup(const char *s, C$size_t n) {
    return strndup(s, n);
}

extern C$size_t C$strlen(const char *s) {
    return strlen(s);
}

extern void *C$malloc(C$size_t n) {
    return malloc(n);
}

extern void C$free(void *ptr) {
    free(ptr);
}

extern void *C$realloc(void *ptr, C$size_t n) {
    return realloc(ptr, n);
}

extern int C$strcmp(const char *a, const char *b) {
    return strcmp(a, b);
}

extern C$Pid C$waitpid(C$Pid pid, int *status, int opts) {
    return wait4(pid, status, opts, NULL);
}

extern C$Pid C$fork() {
    return fork();
}

extern char **C$getEnviron() {
    extern char **environ;
    return environ;
}

#include <fcntl.h> // open

extern int C$open(const char *filename, int oflag, int perm) {
    return open(filename, oflag, perm);
}

extern int C$close(int fd) {
    return close(fd);
}

extern int C$read(int fd, void *b, C$size_t n) {
    return read(fd, b, n);
}

extern int C$write(int fd, const void *b, C$size_t n) {
    return write(fd, b, n);
}

#include <dirent.h> // DIR, dirent, opendir, readdir, closedir

extern C$Dir C$opendir(const char *filename) {
    return opendir(filename);
}

extern int C$closedir(C$Dir dir) {
    return closedir(dir);
}

extern C$Dirent C$readdir(C$Dir dir) {
    return readdir(dir);
}

extern char *C$Dirent_name(C$Dirent dirent) {
    return ((struct dirent *)dirent)->d_name;
}

#include <sys/stat.h> // stat

extern int C$stat(const char *path, C$Stat *s) {
    struct stat st = {};
    int res = stat(path, &st);
    s->size = st.st_size;
    s->mode = st.st_mode;
    s->atime = st.st_atime;
    s->mtime = st.st_mtime;
    s->ctime = st.st_ctime;
    return res;
}

extern int C$mkdir(const char *path, u32 mode) {
    return mkdir(path, mode);
}

#include <execinfo.h> // backtrace, etc.

extern int C$backtrace(void **buf, int n) {
    return backtrace(buf, n);
}

extern void C$backtrace_symbols_fd(void **buf, int n, int fd) {
    backtrace_symbols_fd(buf, n, fd);
}

extern void C$exit(int status) {
    exit(status);
}
