#include "builtin/builtin.h"

#include "kc/scanner/scanner.h"
#include "kc/parser/parser.h"
#include "cmd/compile/emit.h"

static char *types[] = {
    "FILE",
    "bool",
    "char",
    "float",
    "int",
    "size_t",
    "va_list",
    "void",
    NULL,
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        panic("usage: main SOURCE_FILE");
    }
    scope_t *scope = scope_new(NULL);
    for (char **names = types; *names; names++) {
        object_t *obj = malloc(sizeof(*obj));
        obj->kind = obj_kind_TYPE;
        obj->name = *names;
        scope_insert(scope, obj);
    }
    for (int i = 1; i < argc; i++) {
        file_t *file = parser_parse_file(argv[i], scope);
        emitter_t emitter = {.fp = stdout};
        emitter_emit_file(&emitter, file);
        free(file->decls);
        free(file);
    }
    object_t *obj;
    return 0;
}
