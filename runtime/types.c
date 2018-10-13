#include "runtime.h"

#include <string.h> // strtok

const desc_t desc_int = {
    .size = sizeof(int),
};

static uintptr_t djb2(const char *s) {
    // http://www.cse.yorku.ca/~oz/hash.html
    uintptr_t hash = 5381;
    int ch = *s;
    while (ch) {
        hash = ((hash << 5) + hash) + ch;
        s++;
        ch = *s;
    }
    return hash;
}

const desc_t desc_str = {
    .size = sizeof(char *),
    .dup = (void *(*)(const void *))strdup,
    .cmp = (int (*)(const void *, const void *))strcmp,
    .hash = (uintptr_t (*)(const void *))djb2,
};
