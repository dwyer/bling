#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"

#include "kc/scanner/scanner.h"
#include "kc/parser/parser.h"
#include "emit/emit.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        panic("usage: main SOURCE_FILE");
    }
    src = read_file(argv[1]);
    decl_t **decls = parse_file();
    emit_decls(decls);
    free(decls);
    free(src);
    return 0;
}
