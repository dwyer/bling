#include "io/ioutil/ioutil.h"

char *ioutil_read_file(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        panic("couldn't open file: %s", filename);
    }
    slice_t str = {
        .size = sizeof(char),
        .cap = 8,
    };
    for (;;) {
        int ch = fgetc(fp);
        if (ch == EOF) {
            ch = '\0';
            str = append(str, &ch);
            break;
        }
        str = append(str, &ch);
    }
    fclose(fp);
    return str.array;
}
