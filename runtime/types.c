#include "runtime.h"

#include <string.h> // strtok

const desc_t desc_int = {
    .size = sizeof(int),
};

static uintptr_t djb2(const char *s) {
    // http://www.cse.yorku.ca/~oz/hash.html
    uintptr_t hash = 5381;
    int ch;
    while ((ch = *s++))
        hash = ((hash << 5) + hash) + ch;
    return hash;
}

const desc_t desc_str = {
    .size = sizeof(char *),
    .dup = (void *)strdup,
    .cmp = (void *)strcmp,
    .hash = (void *)djb2,
};
