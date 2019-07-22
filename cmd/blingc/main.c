#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "path/path.h"
#include "subc/emitter/emitter.h"
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
    emit_string(e, "#include \"");
    emit_string(e, filename);
    emit_string(e, "\"");
    emit_newline(e);
}

void compile_c(char *argv[]) {
    const char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else {
            panic("unknown option: %s", *argv);
        }
        argv++;
    }
    scope_t *scope = scope_new(NULL);
    declare_builtins(scope);
    emitter_t emitter = {};
    while (*argv) {
        char *filename = *argv;
        file_t *file = parser_parse_cfile(filename, scope);
        file->scope = scope;
        printer_print_file(&emitter, file);
        argv++;
    }
    char *out = emitter_string(&emitter);
    os_File *file = os_stdout;
    error_t *err = NULL;
    if (dst) {
        file = os_create(dst, &err);
        if (err) {
            panic(err->error);
        }
    }
    os_write(file, out, &err);
    if (err) {
        panic(err->error);
    }
    if (dst) {
        os_close(file, &err);
        if (err) {
            panic(err->error);
        }
    }
}

void compile_bling(char *argv[]) {
    bool emit_as_bling = false;
    config_t conf = {};
    const char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else if (streq(*argv, "-w")) {
            conf.strict = true;
        } else {
            panic("unknown option: %s", *argv);
        }
        argv++;
    }
    scope_t *scope = scope_new(NULL);
    declare_builtins(scope);
    if (conf.strict) {
        print("walking BUILTINS");
        file_t *file = parser_parse_file("builtin/builtin.bling");
        file->scope = scope;
        types_checkFile(&conf, file);
        free(file->decls);
        free(file);
    }
    if (dst) {
        emit_as_bling = is_ext(dst, ".bling");
    }
    emitter_t emitter = {};
    error_t *err = NULL;
    if (!emit_as_bling) {
        emit_rawfile(&emitter, "bootstrap/bootstrap.c");
    }
    while (*argv) {
        char *filename = *argv;
        file_t *file = NULL;
        if (is_ext(filename, ".bling")) {
            file = parser_parse_file(filename);
        } else if (is_ext(filename, ".c") || is_ext(filename, ".h")) {
            file = parser_parse_cfile(filename, scope);
        } else {
            panic("unknown file type: %s", filename);
        }
        file->scope = scope;
        package_t pkg = types_checkFile(&conf, file);
        for (int i = 0; pkg.files[i]; i++) {
            file_t *file = pkg.files[i];
            if (emit_as_bling) {
                printer_print_file(&emitter, file);
            } else {
                emitter_emit_file(&emitter, file);
            }
            // free(file->decls);
            // free(file);
        }
        argv++;
    }
    char *out = emitter_string(&emitter);
    os_File *file = os_stdout;
    if (dst) {
        file = os_create(dst, &err);
    }
    os_write(file, out, &err);
    if (dst) {
        emit_as_bling = is_ext(dst, ".bling");
        os_close(file, &err);
    }
}

int main(int argc, char *argv[]) {
    char *progname = *argv;
    argv++;
    if (!*argv) {
        usage(progname);
    }
    if (streq(*argv, "-c")) {
        argv++;
        compile_c(argv);
    } else {
        compile_bling(argv);
    }
    return 0;
}
