#include "os/os.h"

#include "paths/paths.h"
#include "slice/slice.h"

#include <dirent.h> // DIR, dirent, opendir, readdir, closedir
#include <fcntl.h> // open
#include <sys/stat.h> // stat
#include <unistd.h> // read, close, write, STD*_FILENO

static os$File _stdin = {.fd = STDIN_FILENO, .name = "/dev/stdin"};
static os$File _stdout = {.fd = STDOUT_FILENO, .name = "/dev/stdout"};
static os$File _stderr = {.fd = STDERR_FILENO, .name = "/dev/stderr"};

os$File *os$stdin = &_stdin;
os$File *os$stdout = &_stdout;
os$File *os$stderr = &_stderr;

extern os$File *os$newFile(uintptr_t fd, const char *name) {
    os$File file = {
        .name = strdup(name),
        .fd = fd,
    };
    return esc(file);
}

extern os$File *os$openFile(const char *filename, int mode, int perm, error$Error **error) {
    error$clear();
    int fd = open(filename, mode, perm);
    if (fd == -1) {
        error$check(error);
        return NULL;
    }
    return os$newFile(fd, filename);
}

extern os$File *os$open(const char *filename, error$Error **error) {
    return os$openFile(filename, O_RDONLY, 0, error);
}

extern os$File *os$create(const char *filename, error$Error **error) {
    return os$openFile(filename, O_CREAT | O_TRUNC | O_RDWR, DEFFILEMODE, error);
}

extern int os$read(os$File *file, char *b, int n, error$Error **error) {
    error$clear();
    n = read(file->fd, b, n);
    error$check(error);
    return n;
}

extern int os$write(os$File *file, const char *b, error$Error **error) {
    error$clear();
    int n = write(file->fd, b, strlen(b));
    error$check(error);
    return n;
}

extern void os$close(os$File *file, error$Error **error) {
    error$clear();
    if (file->is_dir) {
        closedir((DIR *)file->fd);
    } else {
        close(file->fd);
    }
    error$check(error);
}

extern os$FileInfo os$stat(const char *name, error$Error **error) {
    struct stat st;
    os$FileInfo info = {};
    error$clear();
    if (stat(name, &st) != 0) {
        error$check(error);
        return info;
    }
    info._name = strdup(name);
    info._sys = esc(st);
    return info;
}

extern void os$FileInfo_free(os$FileInfo info) {
    free(info._name);
    free(info._sys);
}

extern os$File *os$openDir(const char *name, error$Error **error) {
    DIR *dp = opendir(name);
    if (dp == NULL) {
        error$check(error);
        return NULL;
    }
    os$File *file = os$newFile((uintptr_t)dp, name);
    file->is_dir = true;
    return file;
}

extern char **os$readdirnames(os$File *file, error$Error **error) {
    slice$Slice arr = {.size = sizeof(char *)};
    DIR *dp = (DIR *)file->fd;
    for (;;) {
        struct dirent *dirent = readdir(dp);
        if (dirent == NULL) {
            break;
        }
        if (dirent->d_name[0] == '.') {
            continue;
        }
        char *name = strdup(dirent->d_name);
        slice$append(&arr, &name);
    }
    char *nil = NULL;
    slice$append(&arr, &nil);
    return arr.array;
}

extern os$FileInfo **os$readdir(os$File *file, error$Error **error) {
    error$Error *err = NULL;
    char **names = os$readdirnames(file, &err);
    if (err != NULL) {
        os$close(file, NULL);
        error$move(err, error);
        return NULL;
    }
    slice$Slice arr = {.size = sizeof(uintptr_t)};
    for (int i = 0; names[i] != NULL; i++) {
        char *path = paths$join2(file->name, names[i]);
        free(names[i]);
        os$FileInfo info = os$stat(path, &err);
        if (err != NULL) {
            os$close(file, NULL);
            error$move(err, error);
            return NULL;
        }
        os$FileInfo *ptr = esc(info);
        slice$append(&arr, &ptr);
    }
    free(names);
    const void *nil = NULL;
    slice$append(&arr, &nil);
    return arr.array;
}

extern char *os$FileInfo_name(os$FileInfo info) {
    return info._name;
}

extern uint64_t os$FileInfo_size(os$FileInfo info) {
    struct stat st = *(struct stat *)os$FileInfo_sys(info);
    return st.st_size;
}

extern os$FileMode os$FileInfo_mode(os$FileInfo info) {
    struct stat st = *(struct stat *)os$FileInfo_sys(info);
    return st.st_mode;
}

extern os$Time os$FileInfo_mod_time(os$FileInfo info) {
    struct stat st = *(struct stat *)os$FileInfo_sys(info);
    return st.st_mtime;
}

extern bool os$FileInfo_is_dir(os$FileInfo info) {
    struct stat st = *(struct stat *)os$FileInfo_sys(info);
    return S_ISDIR(st.st_mode);
}

extern void *os$FileInfo_sys(os$FileInfo info) {
    return info._sys;
}

extern const char *os$tempDir() {
    return "/tmp";
}
