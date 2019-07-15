#include "strings.h"

extern char *strings_join(const char *a[], int size, const char *sep) {
    switch (size) {
    case 0:
        return strdup("");
    case 1:
        return strdup(a[0]);
    }
    strings_Builder b = {};
    strings_Builder_write(&b, a[0], strlen(a[0]), NULL);
    for (int i = 1; i < size; i++) {
        strings_Builder_write(&b, sep, strlen(sep), NULL);
        strings_Builder_write(&b, a[i], strlen(a[i]), NULL);
    }
    return strings_Builder_string(&b);
}

static void strings_Builder_init(strings_Builder *b) {
    if (b->_buf.size == 0) {
        b->_buf.size = sizeof(char);
        b->_buf.cap = BUFSIZ;
    }
}

extern int strings_Builder_len(strings_Builder *b) {
    return len(b->_buf);
}

extern char *strings_Builder_string(strings_Builder *b) {
    strings_Builder_init(b);
    char *s = malloc(strings_Builder_len(b) + 1);
    memcpy(s, b->_buf.array, strings_Builder_len(b));
    s[strings_Builder_len(b)] = '\0';
    return s;
}

extern int strings_Builder_write(strings_Builder *b, const char *p, int size, error_t **error) {
    strings_Builder_init(b);
    for (int i = 0; i < size; i++) {
        b->_buf = append(b->_buf, &p[i]);
    }
    return size;
}

extern void strings_Builder_writeByte(strings_Builder *b, char p, error_t **error) {
    strings_Builder_init(b);
    b->_buf = append(b->_buf, &p);
}
