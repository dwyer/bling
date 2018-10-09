#include "builtin/builtin.h"

#include "kc/scanner/scanner.h"
#include "kc/parser/parser.h"
#include "cmd/compile/emit.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        panic("usage: main SOURCE_FILE");
    }
    scope_t scope = {
        .types = {.desc = &desc_str},
    };
    for (int i = 1; i < argc; i++) {
        file_t *file = parser_parse_file(argv[i], &scope);
        emitter_t emitter = {.fp = stdout};
        emitter_emit_file(&emitter, file);
        free(file->decls);
        free(file);
    }
    return 0;
}
