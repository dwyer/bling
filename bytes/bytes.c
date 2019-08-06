#include "bytes/bytes.h"

extern bool bytes_hasSuffix(const char *s, const char *suffix) {
    for (int i = 0; s[i]; i++) {
        if (streq(&s[i], suffix)) {
            return true;
        }
    }
    return false;
}

extern char *bytes_join(const char *a[], int size, const char *sep) {
    switch (size) {
    case 0:
        return strdup("");
    case 1:
        return strdup(a[0]);
    }
    buffer_t b = {};
    buffer_write(&b, a[0], strlen(a[0]), NULL);
    for (int i = 1; i < size; i++) {
        buffer_write(&b, sep, strlen(sep), NULL);
        buffer_write(&b, a[i], strlen(a[i]), NULL);
    }
    return buffer_string(&b);
}

static void buffer_init(buffer_t *b) {
    if (b->size == 0) {
        b->size = sizeof(char);
        b->cap = 1024;
    }
}

extern int buffer_len(buffer_t *b) {
    return len(*b);
}

extern char *buffer_bytes(buffer_t *b) {
    buffer_init(b);
    return b->array;
}

extern char *buffer_string(buffer_t *b) {
    buffer_init(b);
    char *s = malloc(buffer_len(b) + 1);
    memcpy(s, b->array, buffer_len(b));
    s[buffer_len(b)] = '\0';
    return s;
}

extern int buffer_write(buffer_t *b, const char *p, int size, error_t **error) {
    buffer_init(b);
    if (size < 0) {
        size = strlen(p);
    }
    for (int i = 0; i < size; i++) {
        *b = append(*b, &p[i]);
    }
    return size;
}

extern void buffer_writeByte(buffer_t *b, char p, error_t **error) {
    buffer_init(b);
    *b = append(*b, &p);
}
