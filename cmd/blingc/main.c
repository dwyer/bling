#include "builtin/builtin.h"

#include "bling/parser/parser.h"
#include "subc/parser/parser.h"
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

bool is_ext(const char *filename, const char *ext) {
    while (*filename && *filename != '.') {
        filename++;
    }
    return !strcmp(filename, ext);
}

void usage(const char *progname) {
    panic("usage: %s -o DST SRCS", progname);
}

int main(int argc, char *argv[]) {
    char *progname = *argv;
    bool emit_as_bling = false;
    argv++;
    if (!*argv) {
        usage(progname);
    }
    const char *dst = NULL;
    if (!strcmp(*argv, "-o")) {
        argv++;
        dst = *argv;
        argv++;
    }
    scope_t *scope = scope_new(NULL);
    for (char **names = types; *names; names++) {
        object_t *obj = malloc(sizeof(object_t *));
        obj->kind = obj_kind_TYPE;
        obj->name = *names;
        scope_insert(scope, obj);
    }
    emitter_t emitter = {.fp = stdout};
    if (dst) {
        emitter.fp = fopen(dst, "w");
        emit_as_bling = is_ext(dst, ".bling");
    }
    if (!emit_as_bling) {
        char *src = ioutil_read_file("runtime/clib.h");
        emit_string(&emitter, src);
        free(src);
    }
    while (*argv) {
        char *filename = *argv;
        file_t *file = NULL;
        if (is_ext(filename, ".bling")) {
            file = parser_parse_file(filename, scope);
        } else if (is_ext(filename, ".c") || is_ext(filename, ".h")) {
            file = parser_parse_cfile(filename, scope);
        } else {
            panic("unknown file type: %s", filename);
        }
        if (emit_as_bling) {
            printer_print_file(&emitter, file);
        } else {
            emitter_emit_file(&emitter, file);
        }
        free(file->decls);
        free(file);
        argv++;
    }
    if (dst) {
        fclose(emitter.fp);
    }
    return 0;
}
