#include "bling/build/build.h"
#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "paths/paths.h"
#include "subc/cemitter/cemitter.h"
#include "subc/cparser/cparser.h"
#include "sys/sys.h"

package(main);

import("bling/ast");
import("bling/build");
import("bling/emitter");
import("bling/parser");
import("bling/token");
import("bling/types");
import("bytes");
import("io/ioutil");
import("os");
import("paths");
import("subc/cemitter");
import("subc/cparser");
import("sys");
import("utils");

void emit_rawfile(emitter$Emitter *e, const char *filename) {
    utils$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic(err->error);
    }
    emitter$emitString(e, src);
    free(src);
}

static bool isForwardDecl(ast$Decl *decl) {
    switch (decl->kind) {
    case ast$DECL_FUNC:
        return decl->func.body == NULL;
    case ast$DECL_PRAGMA:
        return true;
    case ast$DECL_TYPEDEF:
        {
            switch (decl->typedef_.type->kind) {
            case ast$TYPE_STRUCT:
                return decl->typedef_.type->struct_.fields == NULL
                    || decl->typedef_.type->struct_.fields[0] == NULL;
            default:
                return false;
            }
        }
    case ast$DECL_VALUE:
        return decl->value.value == NULL;
    default:
        return false;
    }
}

void compile_c(char *argv[]) {
    const char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else {
            panic(sys$sprintf("unknown option: %s", *argv));
        }
        argv++;
    }
    emitter$Emitter e = {};
    token$FileSet *fset = token$newFileSet();
    while (*argv) {
        char *filename = *argv;
        ast$File *file = cparser$parseFile(fset, filename, types$universe());
        // types$Config conf = {.strict = true, .cMode = true};
        // types$checkFile(&conf, fset, file, NULL);
        bool allowForward = file->name && (ast$isIdentNamed(file->name, "os")
                || ast$isIdentNamed(file->name, "sys"));
        if (!allowForward) {
            int i = 0;
            for (int j = 0; file->decls[j]; j++) {
                if (!isForwardDecl(file->decls[j])) {
                    file->decls[i] = file->decls[j];
                    i++;
                }
            }
            file->decls[i] = NULL;
        }
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
    utils$Error *err = NULL;
    // ignore error
    types$Config conf = {.strict = true};
    char *dst = NULL;
    while (**argv == '-') {
        if (streq(*argv, "-o")) {
            argv++;
            dst = *argv;
        } else {
            panic(sys$sprintf("unknown option: %s", *argv));
        }
        argv++;
    }
    emitter$Emitter e = {};
    emit_rawfile(&e, "bootstrap/bootstrap.h");
    token$FileSet *fset = token$newFileSet();
    while (*argv) {
        char *filename = *argv;
        ast$File *file = NULL;
        if (bytes$hasSuffix(filename, ".bling")) {
            file = parser$parseFile(fset, filename, types$universe());
        } else if (bytes$hasSuffix(filename, ".c")) {
            file = cparser$parseFile(fset, filename, types$universe());
        } else {
            panic(sys$sprintf("unknown file type: %s", filename));
        }
        types$Package *pkg = types$checkFile(&conf, paths$dir(filename), fset,
                file, NULL);
        for (int i = 0; i < utils$Slice_len(&pkg->files); i++) {
            ast$File *file = NULL;
            utils$Slice_get(&pkg->files, i, &file);
            e.forwardDecl = true;
            cemitter$emitFile(&e, file);
            e.forwardDecl = false;
            cemitter$emitFile(&e, file);
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
                "bazel-bin/os/libos.a",
                "bazel-bin/sys/libsys.a",
                NULL,
            };
            int code = sys$run(args);
            if (code != 0) {
                panic(sys$sprintf("- failed with code %d", code));
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
        panic(sys$sprintf("usage: %s -o DST SRCS", progname));
    }
    if (streq(*argv, "build")) {
        argv++;
        build$buildPackage(argv);
    } else if (streq(*argv, "-c")) {
        argv++;
        compile_c(argv);
    } else {
        compile_bling(argv);
    }
    return 0;
}
