#include "strings/strings.h"

extern char *strings_join(const char *a[], int size, const char *sep) {
    return bytes$join(a, size, sep);
}

extern int strings_Builder_len(strings_Builder *b) {
    return bytes$Buffer_len(&b->_buf);
}

extern char *strings_Builder_string(strings_Builder *b) {
    return bytes$Buffer_string(&b->_buf);
}

extern int strings_Builder_write(strings_Builder *b, const char *p, int size,
        error_t **error) {
    return bytes$Buffer_write(&b->_buf, p, size, error);
}

extern void strings_Builder_writeByte(strings_Builder *b, char p,
        error_t **error) {
    bytes$Buffer_writeByte(&b->_buf, p, error);
}
