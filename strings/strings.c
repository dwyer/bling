#include "strings/strings.h"

extern char *strings$join(const char *a[], int size, const char *sep) {
    return bytes$join(a, size, sep);
}

extern int strings$Builder_len(strings$Builder *b) {
    return bytes$Buffer_len(&b->_buf);
}

extern char *strings$Builder_string(strings$Builder *b) {
    return bytes$Buffer_string(&b->_buf);
}

extern int strings$Builder_write(strings$Builder *b, const char *p, int size,
        error$Error **error) {
    return bytes$Buffer_write(&b->_buf, p, size, error);
}

extern void strings$Builder_writeByte(strings$Builder *b, char p,
        error$Error **error) {
    bytes$Buffer_writeByte(&b->_buf, p, error);
}
