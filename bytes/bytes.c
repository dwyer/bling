#include "bytes/bytes.h"

extern bool bytes$hasSuffix(const char *b, const char *suffix) {
    for (int i = 0; b[i]; i++) {
        if (streq(&b[i], suffix)) {
            return true;
        }
    }
    return false;
}

extern int bytes$indexByte(const char *b, char c) {
    for (int i = 0; b[i]; i++) {
        if (b[i] == c) {
            return i;
        }
    }
    return -1;
}

extern char *bytes$join(const char *a[], int size, const char *sep) {
    switch (size) {
    case 0:
        return strdup("");
    case 1:
        return strdup(a[0]);
    }
    bytes$Buffer b = {};
    bytes$Buffer_write(&b, a[0], strlen(a[0]), NULL);
    for (int i = 1; i < size; i++) {
        bytes$Buffer_write(&b, sep, strlen(sep), NULL);
        bytes$Buffer_write(&b, a[i], strlen(a[i]), NULL);
    }
    return bytes$Buffer_string(&b);
}

static void bytes$Buffer_init(bytes$Buffer *b) {
    if (b->size == 0) {
        b->size = sizeof(char);
        b->cap = 1024;
    }
}

extern int bytes$Buffer_len(bytes$Buffer *b) {
    return utils$Slice_len(b);
}

extern char *bytes$Buffer_bytes(bytes$Buffer *b) {
    bytes$Buffer_init(b);
    return b->array;
}

extern char *bytes$Buffer_string(bytes$Buffer *b) {
    bytes$Buffer_init(b);
    char *s = malloc(bytes$Buffer_len(b) + 1);
    memcpy(s, b->array, bytes$Buffer_len(b));
    s[bytes$Buffer_len(b)] = '\0';
    return s;
}

extern int bytes$Buffer_write(bytes$Buffer *b, const char *p, int size, utils$Error **error) {
    bytes$Buffer_init(b);
    if (size < 0) {
        size = strlen(p);
    }
    for (int i = 0; i < size; i++) {
        utils$Slice_append(b, &p[i]);
    }
    return size;
}

extern void bytes$Buffer_writeByte(bytes$Buffer *b, char p, utils$Error **error) {
    bytes$Buffer_init(b);
    utils$Slice_append(b, &p);
}
