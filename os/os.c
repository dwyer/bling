#include "os/os.h"

#include "path/path.h"

#include <sys/stat.h>
#include <dirent.h>

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
    return memdup(&file, sizeof(file));
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

extern os_FileInfo os_stat(const char *filename) {
    struct stat st;
    stat(filename, &st);
    os_FileInfo info = {
        .name = strdup(filename),
        .size = st.st_size,
        .mode = st.st_mode,
        .mod_time = st.st_mtime,
        .is_dir = S_ISDIR(st.st_mode),
        .sys = NULL,
    };
    return info;
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

extern void error_move(error_t *src, error_t **dst) {
    if (dst) {
        *dst = src;
    }
}

extern os_FileInfo **os_readdir(const char *name, error_t **error) {
    error_t *err = NULL;
    os_File *file = os_openDir(name, &err);
    if (err) {
        error_move(err, error);
        return NULL;
    }
    char **names = os_readdirnames(file, &err);
    if (err) {
        os_close(file, NULL);
        error_move(err, error);
        return NULL;
    }
    os_close(file, &err);
    if (err) {
        error_move(err, error);
        return NULL;
    }
    slice_t arr = {.size = sizeof(uintptr_t)};
    for (int i = 0; names[i] != NULL; i++) {
        char *path = path_join2(name, names[i]);
        free(names[i]);
        os_FileInfo info = os_stat(path);
        os_FileInfo *ptr = memdup(&info, sizeof(info));
        arr = append(arr, &ptr);
    }
    free(names);
    void *nil = NULL;
    arr = append(arr, &nil);
    return arr.array;
}
