#include "builtin/builtin.h"

#include "bling/parser/parser.h"
#include "bling/walk/walk.h"
#include "os/os.h"
#include "path/path.h"
#include "subc/emitter/emit.h"
#include "subc/parser/parser.h"

$import("bling/parser");
$import("bling/walk");
$import("os");
$import("path");
$import("subc/parser");
$import("subc/emitter");

static char *native_types[] = {
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
    "uint32_t",
    "uint64_t",
    "uintptr_t",
    "va_list",

    NULL,
};

bool is_ext(const char *path, const char *ext) {
    return path_matchExt(ext, path);
}

void usage(const char *progname) {
    panic("usage: %s -o DST SRCS", progname);
}

int main(int argc, char *argv[]) {
    char *progname = *argv;
    bool emit_as_bling = false;
    bool do_walk = false;
    argv++;
    if (!*argv) {
        usage(progname);
    }
    const char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else if (streq(*argv, "-w")) {
            do_walk = true;
        } else {
            panic("unknown option: %s", *argv);
        }
        argv++;
    }
    scope_t *scope = scope_new(NULL);
    for (int i = 0; native_types[i] != NULL; i++) {
        expr_t name = {
            .type = ast_EXPR_IDENT,
            .ident = {
                .name = strdup(native_types[i]),
            },
        };
        decl_t decl = {
            .type = ast_DECL_NATIVE,
            .native = {
                .name = esc(name),
            },
        };
        scope_declare(scope, esc(decl));
    }
    emitter_t emitter = {};
    error_t *err = NULL;
    if (dst) {
        emit_as_bling = is_ext(dst, ".bling");
    }
    if (!emit_as_bling) {
        char *src = ioutil_read_file("runtime/clib.h", &err);
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
            if (do_walk) {
                walk_file(file);
            }
            emitter_emit_file(&emitter, file);
        }
        free(file->decls);
        free(file);
        argv++;
    }
    char *out = strings_Builder_string(&emitter.builder);
    os_File *file = os_stdout;
    if (dst) {
        file = os_create(dst, &err);
    }
    os_write(file, out, &err);
    if (dst) {
        emit_as_bling = is_ext(dst, ".bling");
        os_close(file, &err);
    }
    return 0;
}
