#include "builtin/builtin.h"

#include "subc/parser/parser.h"
#include "bling/emitter/emit.h"

static char *types[] = {
    // native types
    "char",
    "float",
    "int",
    "void",

    // libc types
    "DIR",
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
        printer_t printer = {.fp = stdout};
        printer_print_file(&printer, file);
        free(file->decls);
        free(file);
    }
    return 0;
}
