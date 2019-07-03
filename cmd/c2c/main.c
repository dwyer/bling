#include "builtin/builtin.h"

#include "subc/scanner/scanner.h"
#include "subc/parser/parser.h"
#include "subc/emitter/emit.h"

$import("builtin");
$import("subc/scanner");
$import("subc/parser");
$import("subc/emitter");

static char *types[] = {
    // native types
    "char",
    "float",
    "int",
    "void",

    // libc types
    "FILE",
    "bool",
    "size_t",
    "uintptr_t",
    "va_list",

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
