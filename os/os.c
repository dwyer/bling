#include "os/os.h"

#include "path/path.h"

#include <sys/stat.h>
#include <dirent.h>

static const int STDIN_FILENO = 0;
static const int STDOUT_FILENO = 1;
static const int STDERR_FILENO = 2;

extern size_t read(int, void *, size_t); // libc
extern size_t write(int, const void *, size_t); // libc
extern int close(int); // libc

static os_File _stdin = {.fd = STDIN_FILENO, .name = "/dev/stdin"};
static os_File _stdout = {.fd = STDOUT_FILENO, .name = "/dev/stdout"};
static os_File _stderr = {.fd = STDERR_FILENO, .name = "/dev/stderr"};

os_File *os_stdin = &_stdin;
os_File *os_stdout = &_stdout;
os_File *os_stderr = &_stderr;

extern os_File *os_newFile(uintptr_t fd, const char *name) {
    os_File file = {
        .name = strdup(name),
        .fd = fd,
    };
    return esc(file);
}

extern os_File *os_openFile(const char *filename, int mode, int perm, error_t **error) {
    int fd = open(filename, mode, perm);
    if (fd == 0) {
        if (error != NULL) {
            *error = make_error("couldn't open file");
        }
        return NULL;
    }
    return os_newFile(fd, filename);
}

extern os_File *os_open(const char *filename, error_t **error) {
    return os_openFile(filename, O_RDONLY, 0, error);
}

extern os_File *os_create(const char *filename, error_t **error) {
    return os_openFile(filename, O_CREAT | O_TRUNC | O_RDWR, DEFFILEMODE, error);
}

extern int os_read(os_File *file, char *b, int n, error_t **error) {
    return read(file->fd, b, n);
}

extern int os_write(os_File *file, const char *b, error_t **error) {
    return write(file->fd, b, strlen(b));
}

extern void os_close(os_File *file, error_t **error) {
    if (file->is_dir) {
        closedir((DIR *)file->fd);
    } else {
        close(file->fd);
    }
}

extern os_FileInfo os_stat(const char *name, error_t **error) {
    struct stat st;
    os_FileInfo info = {};
    if (stat(name, &st) != 0) {
        if (error != NULL) {
            *error = make_sysError();
        }
        return info;
    }
    info._name = strdup(name);
    info._sys = esc(st);
    return info;
}

extern void os_FileInfo_free(os_FileInfo info) {
    free(info._name);
    free(info._sys);
}

extern os_File *os_openDir(const char *name, error_t **error) {
    DIR *dp = opendir(name);
    if (dp == NULL) {
        if (error != NULL) {
            *error = make_error("bad dirname");
        }
        return NULL;
    }
    os_File *file = os_newFile((uintptr_t)dp, name);
    file->is_dir = true;
    return file;
}

extern char **os_readdirnames(os_File *file, error_t **error) {
    slice_t arr = {.size = sizeof(char *)};
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
        arr = append(arr, &name);
    }
    char *nil = NULL;
    arr = append(arr, &nil);
    return arr.array;
}

extern os_FileInfo **os_readdir(os_File *file, error_t **error) {
    error_t *err = NULL;
    char **names = os_readdirnames(file, &err);
    if (err != NULL) {
        os_close(file, NULL);
        error_move(err, error);
        return NULL;
    }
    slice_t arr = {.size = sizeof(uintptr_t)};
    for (int i = 0; names[i] != NULL; i++) {
        char *path = path_join(file->name, names[i], NULL);
        free(names[i]);
        os_FileInfo info = os_stat(path, &err);
        if (err != NULL) {
            os_close(file, NULL);
            error_move(err, error);
            return NULL;
        }
        os_FileInfo *ptr = esc(info);
        arr = append(arr, &ptr);
    }
    free(names);
    const void *nil = NULL;
    arr = append(arr, &nil);
    return arr.array;
}

extern char *os_FileInfo_name(os_FileInfo info) {
    return info._name;
}

extern uint64_t os_FileInfo_size(os_FileInfo info) {
    struct stat st = *(struct stat *)os_FileInfo_sys(info);
    return st.st_size;
}

extern os_FileMode os_FileInfo_mode(os_FileInfo info) {
    struct stat st = *(struct stat *)os_FileInfo_sys(info);
    return st.st_mode;
}

extern time_Time os_FileInfo_mod_time(os_FileInfo info) {
    struct stat st = *(struct stat *)os_FileInfo_sys(info);
    return st.st_mtime;
}

extern bool os_FileInfo_is_dir(os_FileInfo info) {
    struct stat st = *(struct stat *)os_FileInfo_sys(info);
    return S_ISDIR(st.st_mode);
}

extern void *os_FileInfo_sys(os_FileInfo info) {
    return info._sys;
}
