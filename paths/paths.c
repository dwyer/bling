#include "paths/paths.h"

#include "bytes/bytes.h"

extern char *basename(char *); // clib
extern char *dirname(char *); // clib

extern char *paths$base(const char *path) {
    char *p = strdup(path);
    return basename(p);
}

extern char *paths$clean(const char *path) {
    return NULL;
}

extern char *paths$dir(const char *path) {
    char *p = strdup(path);
    return dirname(p);
}

extern const char *paths$ext(const char *path) {
    int n = strlen(path) - 1;
    while (n > 0 && path[n] != '.') {
        n--;
    }
    return &path[n];
}

extern bool paths$isAbs(const char *path) {
    return path != NULL && path[0] == '/';
}

extern char *paths$join(const char **elems, int n) {
    return bytes$join(elems, n, "/");
}

extern char *paths$join2(const char *a, const char *b) {
    const char *elems[] = {a, b};
    return paths$join(elems, 2);
}

extern char **paths$split(const char *path) {
    char **names = malloc(sizeof(char *) * 2);
    names[0] = paths$dir(path);
    names[1] = paths$base(path);
    return names;
}

extern bool paths$match(const char *pattern, const char *path) {
    if (*pattern == '\0' && *path == '\0') {
        return true;
    }
    if (*pattern == '\0' || *path == '\0') {
        return false;
    }
    if (*pattern == '*') {
        return paths$match(pattern+1, path) || paths$match(pattern, path+1);
    }
    if (*pattern == *path) {
        return paths$match(pattern+1, path+1);
    }
    return false;
}
