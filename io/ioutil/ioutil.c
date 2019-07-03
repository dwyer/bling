#include "io/ioutil/ioutil.h"

#include "fmt/fmt.h"

extern os_FileInfo **ioutil_read_dir(const char *dirname) {
    desc_t desc = {.size = sizeof(uintptr_t)};
    slice_t arr = {.desc = &desc, .cap = 4};
    char **names = os_listdir(dirname);
    for (int i = 0; names[i] != NULL; i++) {
        char *path = fmt_sprintf("%s/%s", dirname, names[i]);
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

char *ioutil_read_file(const char *filename) {
    os_File *file = os_open(filename);
    desc_t desc = {.size = sizeof(char)};
    slice_t str = {.desc = &desc, .cap = 8};
    for (;;) {
        char buf[BUFSIZ];
        int n = os_read(file, buf, BUFSIZ);
        for (int i = 0; i < n; i++) {
            str = append(str, &buf[i]);
        }
        if (n < BUFSIZ) {
            break;
        }
    }
    os_close(file);
    int ch = 0;
    str = append(str, &ch);
    return (char *)str.array;
}
