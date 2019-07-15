#include "strings.h"

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
