#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "paths/paths.h"
#include "subc/cemitter/cemitter.h"
#include "subc/cparser/cparser.h"
#include "sys/sys.h"

package(main);

import("bling/ast");
import("bling/emitter");
import("bling/parser");
import("bling/types");
import("bytes");
import("io/ioutil");
import("os");
import("paths");
import("subc/cemitter");
import("subc/cparser");
import("sys");
import("utils");

void usage(const char *progname) {
    panic("usage: %s -o DST SRCS", progname);
}

void emitter$emit_rawfile(emitter$Emitter *e, const char *filename) {
    utils$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic(err->error);
    }
    emitter$emitString(e, src);
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
    emitter$Emitter e = {};
    while (*argv) {
        char *filename = *argv;
        ast$File *file = cparser$parseFile(filename, types$universe());
        emitter$emitFile(&e, file);
        argv++;
    }
    char *out = emitter$Emitter_string(&e);
    os$File *file = os$stdout;
    utils$Error *err = NULL;
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
    types$Config conf = {.strict = true};
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
    emitter$Emitter e = {};
    utils$Error *err = NULL;
    if (!emit_as_bling) {
        emitter$emit_rawfile(&e, "bootstrap/bootstrap.h");
    }
    while (*argv) {
        char *filename = *argv;
        ast$File *file = NULL;
        if (bytes$hasSuffix(filename, ".bling")) {
            file = parser$parse_file(filename);
        } else if (bytes$hasSuffix(filename, ".c")) {
            file = cparser$parseFile(filename, types$universe());
        } else {
            panic("unknown file type: %s", filename);
        }
        ast$Package pkg = types$checkFile(&conf, file);
        for (int i = 0; pkg.files[i]; i++) {
            ast$File *file = pkg.files[i];
            if (emit_as_bling) {
                emitter$emitFile(&e, file);
            } else {
                cemitter$emitFile(&e, file);
            }
            free(file->decls);
            free(file);
        }
        argv++;
    }
    char *out = emitter$Emitter_string(&e);
    if (dst) {
        if (bytes$hasSuffix(dst, ".out")) {
            char *tmp = paths$join2(os$tempDir(), "tmp.c");
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
            int code = sys$execve(args[0], args, NULL);
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
