#include "runtime.h"

#include <string.h> // strtok

const desc_t desc_int = {
    .size = sizeof(int),
};

static int strpcmp(const void **sp, const void **tp) {
    return strcmp(*sp, *tp);
}

static char **strpcpy(char **dstp, const char **srcp) {
    *dstp = strdup(*srcp);
    return dstp;
}

static unsigned int strphash(const char **sp) {
    // http://www.cse.yorku.ca/~oz/hash.html
    unsigned int hash = 5381;
    const char *s = *sp;
    int ch;
    while ((ch = *s++))
        hash = ((hash << 5) + hash) + ch;
    return hash;
}

const desc_t desc_str = {
    .size = sizeof(char *),
    .cpy = (void *)strpcpy,
    .cmp = (void *)strpcmp,
    .hash = (void *)strphash,
};
