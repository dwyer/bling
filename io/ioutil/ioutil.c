#include "io/ioutil/ioutil.h"

char *read_file(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        panic("couldn't open file: %s", filename);
    }
    slice_t str = {
        .size = sizeof(char),
        .cap = 8,
    };
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        str = append(str, &ch);
    }
    ch = '\0';
    str = append(str, &ch);
    fclose(fp);
    return str.array;
}
