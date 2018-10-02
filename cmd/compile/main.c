#include "builtin/builtin.h"

#include "kc/scanner/scanner.h"
#include "kc/parser/parser.h"
#include "cmd/compile/emit.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        panic("usage: main SOURCE_FILE");
    }
    decl_t **decls = parser_parse_file(argv[1]);
    emit_decls(decls);
    free(decls);
    return 0;
}
