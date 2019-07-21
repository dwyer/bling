#include "builtin/builtin.h"

#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "path/path.h"
#include "subc/emitter/emit.h"
#include "subc/parser/parser.h"

$import("bling/parser");
$import("bling/types");
$import("os");
$import("path");
$import("subc/parser");
$import("subc/emitter");

bool is_ext(const char *path, const char *ext) {
    return path_matchExt(ext, path);
}

void usage(const char *progname) {
    panic("usage: %s -o DST SRCS", progname);
}

void emit_rawfile(emitter_t *e, const char *filename) {
    error_t *err = NULL;
    char *src = ioutil_read_file(filename, &err);
    if (err) {
        panic(err->error);
    }
    emit_string(e, src);
    free(src);
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
    declare_builtins(scope);
    emitter_t emitter = {};
    error_t *err = NULL;
    if (dst) {
        emit_as_bling = is_ext(dst, ".bling");
    }
    if (!emit_as_bling) {
        emit_rawfile(&emitter, "builtin/libc.h");
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
                types_checkFile(file);
            }
            emitter_emit_file(&emitter, file);
        }
        free(file->decls);
        free(file);
        argv++;
    }
    if (!emit_as_bling) {
        emit_rawfile(&emitter, "bootstrap/bootstrap.c");
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
