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
    const int bufsiz = 1024;
    char *ret = NULL;
    error_t *err = NULL;
    os_File *file = os_open(name, &err);
    if (err != NULL) {
        goto end;
    }
    buffer_t b = {};
    for (;;) {
        char buf[bufsiz];
        int n = os_read(file, buf, bufsiz, &err);
        if (err != NULL) {
            goto end;
        }
        buffer_write(&b, buf, n, NULL); // ignore error
        if (n < bufsiz) {
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

extern void ioutil_writeFile(const char *filename, const char *data, int perm,
        error_t **error) {
    (void)perm; // TODO use this when we have meaningful consts in os.
    error_t *err = NULL;
    os_File *file = os_create(filename, &err);
    if (err) {
        error_move(err, error);
        goto end;
    }
    os_write(file, data, &err);
    if (err) {
        error_move(err, error);
        goto end;
    }
end:
    if (file) {
        os_close(file, NULL);
    }
}
