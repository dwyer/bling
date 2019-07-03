#include "os/os.h"

#include <sys/stat.h>
#include <dirent.h>

extern os_File *os_open(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == 0) {
        panic("couldn't open file: %s", filename);
    }
    os_File file = {
        .name = strdup(filename),
        .fd = fd,
    };
    return memdup(&file, sizeof(file));
}

extern int os_read(os_File *file, char *b, int n) {
    return read(file->fd, b, n);
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
    desc_t desc = {.size = sizeof(uintptr_t)};
    slice_t arr = {.desc = &desc, .cap = 4};
    DIR *dp = opendir(dirname);
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
