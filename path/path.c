#include "path/path.h"

#include "bytes/bytes.h"

extern char *basename(char *); // clib
extern char *dirname(char *); // clib

extern char *path$base(const char *path) {
    char *p = strdup(path);
    return basename(p);
}

extern char *path$clean(const char *path) {
    return NULL;
}

extern char *path$dir(const char *path) {
    char *p = strdup(path);
    return dirname(p);
}

extern const char *path$ext(const char *path) {
    int n = strlen(path) - 1;
    while (n > 0 && path[n] != '.') {
        n--;
    }
    return &path[n];
}

extern bool path$isAbs(const char *path) {
    return path != NULL && path[0] == '/';
}

extern char *path$join(const char **elems, int n) {
    return bytes$join(elems, n, "/");
}

extern char *path$join2(const char *a, const char *b) {
    const char *elems[] = {a, b};
    return path$join(elems, 2);
}

extern char **path$split(const char *path) {
    char **names = malloc(sizeof(char *) * 2);
    names[0] = path$dir(path);
    names[1] = path$base(path);
    return names;
}

extern bool path$match(const char *pattern, const char *path) {
    if (pattern[0] == '\0' && path[0] == '\0') {
        return true;
    }
    if (pattern[0] == '\0' || path[0] == '\0') {
        return false;
    }
    if (pattern[0] == '*') {
        bool res = path$match(pattern+1, path);
        if (res) {
            return true;
        }
        return path$match(pattern, path+1);
    }
    if (pattern[0] == path[0]) {
        return path$match(pattern+1, path+1);
    }
    return false;
}
