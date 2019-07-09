#include "builtin/builtin.h"

#include "bling/parser/parser.h"
#include "subc/emitter/emit.h"

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
        object_t *obj = malloc(sizeof(object_t *));
        obj->kind = obj_kind_TYPE;
        obj->name = *names;
        scope_insert(scope, obj);
    }
    emitter_t printer = {.fp = stdout};
    char *src = ioutil_read_file("runtime/clib.h");
    fputs(src, printer.fp);
    free(src);
    for (int i = 1; i < argc; i++) {
        file_t *file = parser_parse_file(argv[i], scope);
        emitter_emit_file(&printer, file);
        free(file->decls);
        free(file);
    }
    return 0;
}
