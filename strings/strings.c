#include "strings.h"

static void copyCheck(strings_Builder *b) {
    if (b->buf.size == 0) {
        b->buf.size = sizeof(char);
        b->buf.cap = BUFSIZ;
    }
}

extern char *strings_Builder_string(strings_Builder *b) {
    copyCheck(b);
    // TODO: make a copy
    int ch = 0;
    b->buf = append(b->buf, &ch);
    return b->buf.array;
}

extern int strings_Builder_write(strings_Builder *b, const char *p, int size, error_t **error) {
    copyCheck(b);
    for (int i = 0; i < size; i++) {
        b->buf = append(b->buf, &p[i]);
    }
    return size;
}

extern void strings_Builder_writeByte(strings_Builder *b, char p, error_t **error) {
    copyCheck(b);
    b->buf = append(b->buf, &p);
}
