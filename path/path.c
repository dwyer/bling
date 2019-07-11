#include "path/path.h"

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
    return false;
}

extern char *path_join(int n, ...) {
    return NULL;
}

extern char *path_join2(int n, const char *elem1, const char *elem2) {
    return NULL;
}

extern char *path_match(const char *pattern, const char *name, error_t **error) {
    return NULL;
}

extern char **path_split(const char *path) {
    char **names = malloc(sizeof(char *) * 2);
    names[0] = path_dir(path);
    names[1] = path_base(path);
    return names;
}
