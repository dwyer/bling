#include "io/ioutil/ioutil.h"

extern char *ioutil_readAll(os$File *file, error$Error **error) {
    const int bufsiz = 1024;
    char *ret = NULL;
    error$Error *err = NULL;
    if (err != NULL) {
        goto end;
    }
    bytes$Buffer b = {};
    for (;;) {
        char buf[bufsiz];
        int n = os$read(file, buf, bufsiz, &err);
        if (err != NULL) {
            goto end;
        }
        bytes$Buffer_write(&b, buf, n, NULL); // ignore error
        if (n < bufsiz) {
            break;
        }
    }
    ret = bytes$Buffer_string(&b);
end:
    if (err != NULL) {
        error$move(err, error);
    }
    return ret;
}

extern os$FileInfo **ioutil_readDir(const char *name, error$Error **error) {
    os$FileInfo **info = NULL;
    error$Error *err = NULL;
    os$File *file = os$openDir(name, &err);
    if (err != NULL) {
        goto end;
    }
    info = os$readdir(file, &err);
    if (err != NULL) {
        goto end;
    }
end:
    if (file != NULL) {
        os$close(file, NULL);
    }
    if (err != NULL) {
        error$move(err, error);
    }
    return info;
}

extern char *ioutil_readFile(const char *name, error$Error **error) {
    error$Error *err = NULL;
    os$File *file = os$open(name, &err);
    char *ret = ioutil_readAll(file, &err);
    if (err != NULL) {
        goto end;
    }
end:
    if (file != NULL) {
        os$close(file, &err);
    }
    if (err != NULL) {
        error$move(err, error);
    }
    return ret;
}

extern void ioutil_writeFile(const char *filename, const char *data, int perm,
        error$Error **error) {
    (void)perm; // TODO use this when we have meaningful consts in os.
    error$Error *err = NULL;
    os$File *file = os$create(filename, &err);
    if (err) {
        error$move(err, error);
        goto end;
    }
    os$write(file, data, &err);
    if (err) {
        error$move(err, error);
        goto end;
    }
end:
    if (file) {
        os$close(file, NULL);
    }
}
