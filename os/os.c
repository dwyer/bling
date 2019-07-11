#include "os/os.h"

#include <sys/stat.h>
#include <dirent.h>

static os_File _stdin = {.fd = STDIN_FILENO, .name = "/dev/stdin"};
static os_File _stdout = {.fd = STDOUT_FILENO, .name = "/dev/stdout"};
static os_File _stderr = {.fd = STDERR_FILENO, .name = "/dev/stderr"};

os_File *os_stdin = &_stdin;
os_File *os_stdout = &_stdout;
os_File *os_stderr = &_stderr;

extern os_File *os_openFile(const char *filename, int mode, int perm, error_t *error) {
    int fd = open(filename, mode, perm);
    if (fd == 0) {
        panic("couldn't open file: %s", filename);
    }
    os_File file = {
        .name = strdup(filename),
        .fd = fd,
    };
    return memdup(&file, sizeof(file));
}

extern os_File *os_open(const char *filename, error_t *error) {
    return os_openFile(filename, O_RDONLY, 0, error);
}

extern os_File *os_create(const char *filename, error_t *error) {
    return os_openFile(filename, O_CREAT|O_TRUNC|O_RDWR, 0666, error);
}

extern int os_read(os_File *file, char *b, int n, error_t *error) {
    return read(file->fd, b, n);
}

extern int os_write(os_File *file, const char *b, error_t *error) {
    return write(file->fd, b, strlen(b));
}

extern void os_close(os_File *file) {
    close(file->fd);
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

extern char **os_listdir(const char *dirname) {
    slice_t arr = {.size = sizeof(char *)};
    DIR *dp = opendir(dirname);
    if (dp == NULL) {
        panic("bad dirname: %s", dirname);
    }
    if (dp != NULL) {
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
    }
    closedir(dp);
    char *nil = NULL;
    arr = append(arr, &nil);
    return arr.array;
}
