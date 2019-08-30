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

extern sys$Pid sys$waitpid(sys$Pid pid, int *status, int opts) {
    return waitpid(pid, status, opts);
}

extern sys$Pid sys$fork() {
    return fork();
}

extern char **sys$environ() {
    extern char **environ;
    return environ;
}

#include <fcntl.h> // open

extern int sys$open(const char *filename, int oflag, int perm) {
    return open(filename, oflag, perm);
}

extern int sys$close(int fd) {
    return close(fd);
}

extern int sys$read(int fd, void *b, sys$Size n) {
    return read(fd, b, n);
}

extern int sys$write(int fd, const void *b, sys$Size n) {
    return write(fd, b, n);
}

#include <dirent.h> // DIR, dirent, opendir, readdir, closedir

extern sys$Dir sys$opendir(const char *filename) {
    return opendir(filename);
}

extern int sys$closedir(sys$Dir dir) {
    return closedir(dir);
}

extern sys$Dirent sys$readdir(sys$Dir dir) {
    return readdir(dir);
}

extern char *sys$Dirent_name(sys$Dirent dirent) {
    return ((struct dirent *)dirent)->d_name;
}

#include <sys/stat.h> // stat

extern int sys$stat(const char *path, sys$Stat *s) {
    struct stat st = {};
    int res = stat(path, &st);
    s->size = st.st_size;
    s->mode = st.st_mode;
    s->atime = st.st_atime;
    s->mtime = st.st_mtime;
    s->ctime = st.st_ctime;
    return res;
}

extern int sys$mkdir(const char *path, u32 mode) {
    return mkdir(path, mode);
}
