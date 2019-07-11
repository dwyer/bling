#include "io/ioutil/ioutil.h"

extern os_FileInfo **ioutil_read_dir(const char *dirname, error_t **error) {
    return os_readdir(dirname, error);
}

extern char *ioutil_read_file(const char *filename, error_t **error) {
    os_File *file = os_open(filename, error);
    slice_t str = {.size = sizeof(char), .cap = BUFSIZ};
    for (;;) {
        char buf[BUFSIZ];
        int n = os_read(file, buf, BUFSIZ, error);
        for (int i = 0; i < n; i++) {
            str = append(str, &buf[i]);
        }
        if (n < BUFSIZ) {
            break;
        }
    }
    os_close(file, error);
    int ch = 0;
    str = append(str, &ch);
    return (char *)str.array;
}
