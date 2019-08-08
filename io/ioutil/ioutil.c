#include "io/ioutil/ioutil.h"

extern char *ioutil$readAll(os$File *file, utils$Error **error) {
    const int bufsiz = 1024;
    char *ret = NULL;
    utils$Error *err = NULL;
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
        utils$Error_move(err, error);
    }
    return ret;
}

extern os$FileInfo **ioutil$readDir(const char *name, utils$Error **error) {
    os$FileInfo **info = NULL;
    utils$Error *err = NULL;
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
        utils$Error_move(err, error);
    }
    return info;
}

extern char *ioutil$readFile(const char *name, utils$Error **error) {
    utils$Error *err = NULL;
    os$File *file = os$open(name, &err);
    char *ret = ioutil$readAll(file, &err);
    if (err != NULL) {
        goto end;
    }
end:
    if (file != NULL) {
        os$close(file, &err);
    }
    if (err != NULL) {
        utils$Error_move(err, error);
    }
    return ret;
}

extern void ioutil$writeFile(const char *filename, const char *data, int perm,
        utils$Error **error) {
    (void)perm; // TODO use this when we have meaningful consts in os.
    utils$Error *err = NULL;
    os$File *file = os$create(filename, &err);
    if (err) {
        utils$Error_move(err, error);
        goto end;
    }
    os$write(file, data, &err);
    if (err) {
        utils$Error_move(err, error);
        goto end;
    }
end:
    if (file) {
        os$close(file, NULL);
    }
}
