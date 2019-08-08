#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "path/path.h"
#include "subc/emitter/emitter.h"
#include "subc/parser/parser.h"

import("bling/parser");
import("bling/types");
import("os");
import("path");
import("subc/parser");
import("subc/emitter");

int execve(const char *path, char *const argv[], char *envp[]);

void usage(const char *progname) {
    panic("usage: %s -o DST SRCS", progname);
}

void emit_rawfile(emitter_t *e, const char *filename) {
    error$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic(err->error);
    }
    emit_string(e, src);
    free(src);
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
    emitter_t emitter = {};
    while (*argv) {
        char *filename = *argv;
        ast$File *file = parser_parse_cfile(filename, types_universe());
        printer_print_file(&emitter, file);
        argv++;
    }
    char *out = emitter_string(&emitter);
    os$File *file = os$stdout;
    error$Error *err = NULL;
    if (dst) {
        file = os$create(dst, &err);
        if (err) {
            panic(err->error);
        }
    }
    os$write(file, out, &err);
    if (err) {
        panic(err->error);
    }
    if (dst) {
        os$close(file, &err);
        if (err) {
            panic(err->error);
        }
    }
}

void compile_bling(char *argv[]) {
    bool emit_as_bling = false;
    config_t conf = {.strict = true};
    char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else {
            panic("unknown option: %s", *argv);
        }
        argv++;
    }
    if (dst) {
        emit_as_bling = bytes$hasSuffix(dst, ".bling");
    }
    emitter_t emitter = {};
    error$Error *err = NULL;
    if (!emit_as_bling) {
        emit_rawfile(&emitter, "bootstrap/bootstrap.h");
    }
    while (*argv) {
        char *filename = *argv;
        ast$File *file = NULL;
        if (bytes$hasSuffix(filename, ".bling")) {
            file = parser_parse_file(filename);
        } else if (bytes$hasSuffix(filename, ".c")) {
            file = parser_parse_cfile(filename, types_universe());
        } else {
            panic("unknown file type: %s", filename);
        }
        ast$Package pkg = types_checkFile(&conf, file);
        for (int i = 0; pkg.files[i]; i++) {
            ast$File *file = pkg.files[i];
            if (emit_as_bling) {
                printer_print_file(&emitter, file);
            } else {
                emitter_emit_file(&emitter, file);
            }
            free(file->decls);
            free(file);
        }
        argv++;
    }
    char *out = emitter_string(&emitter);
    if (dst) {
        if (bytes$hasSuffix(dst, ".out")) {
            char *tmp = path$join2(os$tempDir(), "tmp.c");
            ioutil$writeFile(tmp, out, 0644, NULL);
            char *args[] = {
                "/usr/bin/cc",
                "-o", dst,
                tmp,
                "bazel-bin/bootstrap/libbootstrap.a",
                "bazel-bin/fmt/libfmt.a",
                "bazel-bin/os/libos.a",
                NULL,
            };
            int code = execve(args[0], args, NULL);
            if (code) {
                print("%s exited with code: %d", argv[0], code);
            }
        } else {
            ioutil$writeFile(dst, out, 0644, NULL);
        }
    } else {
        os$write(os$stdout, out, &err);
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
