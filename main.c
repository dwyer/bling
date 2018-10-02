#include "util.h"

#include "scanner.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        panic("usage: main SOURCE_FILE");
    }
    src = read_file(argv[1]);
    decl_t **decls = parse_file();
    free(decls);
    free(src);
    return 0;
}
