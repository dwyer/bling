#include "path/path.h"

static const char SEP = '/';
static const char END = '\0';

extern char *path_base(const char *path) {
    char *p = strdup(path);
    return basename(p);
}

extern char *path_clean(const char *path) {
    return NULL;
}

extern char *path_dir(const char *path) {
    char *p = strdup(path);
    return dirname(p);
}

extern const char *path_ext(const char *path) {
    int n = strlen(path) - 1;
    while (n > 0 && path[n] != '.') {
        n--;
    }
    return &path[n];
}

extern bool path_isAbs(const char *path) {
    return path != NULL && path[0] == '/';
}

extern char *path_join(const char *elem, ...) {
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

extern char **path_split(const char *path) {
    char **names = malloc(sizeof(char *) * 2);
    names[0] = path_dir(path);
    names[1] = path_base(path);
    return names;
}

extern bool path_match(const char *pattern, const char *path) {
    if (pattern[0] == '\0' && path[0] == '\0') {
        return true;
    }
    if (pattern[0] == '\0' || path[0] == '\0') {
        return false;
    }
    if (pattern[0] == '*') {
        bool res = path_match(pattern+1, path);
        if (res) {
            return true;
        }
        return path_match(pattern, path+1);
    }
    if (pattern[0] == path[0]) {
        return path_match(pattern+1, path+1);
    }
    return false;
}

extern bool path_matchExt(const char *ext, const char *path) {
    return streq(ext, path_ext(path));
}
