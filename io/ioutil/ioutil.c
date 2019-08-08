#include "io/ioutil/ioutil.h"

extern char *ioutil$readAll(os$File *file, errors$Error **error) {
    const int bufsiz = 1024;
    char *ret = NULL;
    errors$Error *err = NULL;
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
        errors$Error_move(err, error);
    }
    return ret;
}

extern os$FileInfo **ioutil$readDir(const char *name, errors$Error **error) {
    os$FileInfo **info = NULL;
    errors$Error *err = NULL;
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
        errors$Error_move(err, error);
    }
    return info;
}

extern char *ioutil$readFile(const char *name, errors$Error **error) {
    errors$Error *err = NULL;
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
        errors$Error_move(err, error);
    }
    return ret;
}

extern void ioutil$writeFile(const char *filename, const char *data, int perm,
        errors$Error **error) {
    (void)perm; // TODO use this when we have meaningful consts in os.
    errors$Error *err = NULL;
    os$File *file = os$create(filename, &err);
    if (err) {
        errors$Error_move(err, error);
        goto end;
    }
    os$write(file, data, &err);
    if (err) {
        errors$Error_move(err, error);
        goto end;
    }
end:
    if (file) {
        os$close(file, NULL);
    }
}
