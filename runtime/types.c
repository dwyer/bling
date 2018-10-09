#include "runtime.h"

#include <string.h> // strtok

const desc_t desc_int = {
    .size = sizeof(int),
};

static unsigned int strphash(const char *s) {
    // http://www.cse.yorku.ca/~oz/hash.html
    unsigned int hash = 5381;
    int ch;
    while ((ch = *s++))
        hash = ((hash << 5) + hash) + ch;
    return hash;
}

const desc_t desc_str = {
    .size = sizeof(char *),
    .is_ptr = true,
    .cmp = (void *)strcmp,
    .hash = (void *)strphash,
};
