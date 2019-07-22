#include "io/ioutil/ioutil.h"

extern os_FileInfo **ioutil_read_dir(const char *name, error_t **error) {
    os_FileInfo **info = NULL;
    error_t *err = NULL;
    os_File *file = os_openDir(name, &err);
    if (err != NULL) {
        goto end;
    }
    info = os_readdir(file, &err);
    if (err != NULL) {
        goto end;
    }
end:
    if (file != NULL) {
        os_close(file, NULL);
    }
    if (err != NULL) {
        error_move(err, error);
    }
    return info;
}

extern char *ioutil_read_file(const char *name, error_t **error) {
    char *ret = NULL;
    error_t *err = NULL;
    os_File *file = os_open(name, &err);
    if (err != NULL) {
        goto end;
    }
    buffer_t b = {};
    for (;;) {
        char buf[BUFSIZ];
        int n = os_read(file, buf, BUFSIZ, &err);
        if (err != NULL) {
            goto end;
        }
        buffer_write(&b, buf, n, NULL); // ignore error
        if (n < BUFSIZ) {
            break;
        }
    }
    ret = buffer_string(&b);
end:
    if (file != NULL) {
        os_close(file, &err);
    }
    if (err != NULL) {
        error_move(err, error);
    }
    return ret;
}
