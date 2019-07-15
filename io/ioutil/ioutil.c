#include "io/ioutil/ioutil.h"

extern os_FileInfo **ioutil_read_dir(const char *name, error_t **error) {
    error_t *err = NULL;
    os_File *file = os_openDir(name, &err);
    if (err != NULL) {
        error_move(err, error);
        return NULL;
    }
    os_FileInfo **info = os_readdir(file, &err);
    if (err != NULL) {
        os_close(file, NULL);
        error_move(err, error);
        return NULL;
    }
    os_close(file, &err);
    if (err != NULL) {
        error_move(err, error);
        return NULL;
    }
    return info;
}

extern char *ioutil_read_file(const char *name, error_t **error) {
    os_File *file = os_open(name, error);
    strings_Builder builder = {};
    for (;;) {
        char buf[BUFSIZ];
        int n = os_read(file, buf, BUFSIZ, error);
        strings_Builder_write(&builder, buf, n, error);
        if (n < BUFSIZ) {
            break;
        }
    }
    os_close(file, error);
    return strings_Builder_string(&builder);
}
