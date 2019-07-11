#include "path/path.h"

static const char SEP = '/';
static const char END = '\0';

extern char *path_base(const char *path) {
    char *p = strdup(path);
    return basename(p);
}

extern char *path_clean(const char *path) {
    return path;
}

extern char *path_dir(const char *path) {
    char *p = strdup(path);
    return dirname(p);
}

extern char *path_ext(const char *path) {
    int n = strlen(path) - 1;
    while (n > 0 && path[n] != '.') {
        n--;
    }
    return &path[n];
}

extern bool path_isAbs(const char *path) {
    return path != NULL && path[0] == '/';
}

extern char *path_join(int n, ...) {
    va_list ap;
    va_start(ap, n);
    slice_t buf = {.size = sizeof(char)};
    while (n > 0) {
        char *s = (char *)va_arg(ap, uintptr_t);
        n--;
        for (int i = 0; s[i]; i++) {
            buf = append(buf, &s[i]);
        }
        if (n > 0) {
            buf = append(buf, &SEP);
        }
    }
    buf = append(buf, &END);
    va_end(ap);
    return buf.array;
}

extern char *path_join0(const char *elem, ...) {
    va_list ap;
    va_start(ap, elem);
    slice_t buf = {.size = sizeof(char)};
    for (;;) {
        for (int i = 0; elem[i]; i++) {
            buf = append(buf, &elem[i]);
        }
        elem = (char *)va_arg(ap, uintptr_t);
        if (elem == NULL) {
            break;
        }
        buf = append(buf, &SEP);
    }
    va_end(ap);
    buf = append(buf, &END);
    return buf.array;
}

extern char *path_join2(const char *elem1, const char *elem2) {
    return path_join0(elem1, elem2, NULL);
}

extern char **path_split(const char *path) {
    char **names = malloc(sizeof(char *) * 2);
    names[0] = path_dir(path);
    names[1] = path_base(path);
    return names;
}
