#include "bling/build/build.h"

#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "paths/paths.h"
#include "subc/cemitter/cemitter.h"
#include "subc/cparser/cparser.h"
#include "sys/sys.h"

const char *AR_PATH = "/usr/bin/ar";
const char *CC_PATH = "/usr/bin/cc";
const char *GEN_PATH = "gen";

typedef struct {
    token$FileSet *fset;
} Builder;

static void printStrArray(char **s) {
    for (int i = 0; s[i]; i++) {
        if (i) {
            sys$printf(" ");
        }
        sys$printf("%s", s[i]);
    }
    sys$printf("\n");
}

static void execute(char *args[]) {
    printStrArray(args);
    int code = sys$run(args);
    if (code != 0) {
        panic(sys$sprintf("- failed with code %d", code));
    }
}

static void Slice_appendStrLit(utils$Slice *a, const char *s) {
    utils$Slice_append(a, &s);
}

static void mkdirForFile(const char *path) {
    char *dir = paths$dir(path);
    os$mkdirAll(dir, 0755, NULL);
    free(dir);
}

static os$FileInfo *buildCFile(os$FileInfo *cFile) {
    // build an obj and return its path
    char *src = os$FileInfo_name(cFile);
    char *dst = NULL;
    {
        int i = bytes$lastIndexByte(src, '.');
        char *base = strdup(src);
        base[i] = '\0';
        dst = sys$sprintf("%s/%s.o", GEN_PATH, base);
        free(base);
    }
    utils$Error *err = NULL;
    os$FileInfo *objFile = os$stat(dst, &err);
    bool doBuild = objFile == NULL ||
        os$FileInfo_modTime(cFile) > os$FileInfo_modTime(objFile);
    if (doBuild) {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, CC_PATH);
        Slice_appendStrLit(&cmd, "-I.");
        Slice_appendStrLit(&cmd, "-c");
        Slice_appendStrLit(&cmd, "-o");
        Slice_appendStrLit(&cmd, dst);
        Slice_appendStrLit(&cmd, src);
        mkdirForFile(dst);
        char **args = NULL;
        args = utils$Slice_to_nil_array(cmd);
        execute(args);
        free(args);
        // update objFile
        os$FileInfo_free(objFile);
        objFile = os$stat(dst, &err);
    }
    return objFile;
}

static os$FileInfo *buildCPackage(Builder *b, const char *path) {
    char *base = paths$base(path);
    char *dst = sys$sprintf("%s/%s/lib%s.a", GEN_PATH, path, base);
    utils$Slice objFiles = {.size = sizeof(os$FileInfo *)};
    os$Time latestUpdate = 0;
    {
        os$FileInfo **files = ioutil$readDir(path, NULL);
        for (int i = 0; files[i]; i++) {
            if (bytes$hasSuffix(files[i]->_name, ".bling")) {
                ast$File *file = parser$parseFile(b->fset, files[i]->_name,
                        types$universe());
                (void)file; // XXX
                // TODO type check the file
                for (int i = 0; file->imports[i]; i++) {
                    // TODO build imports
                }
            } else if (bytes$hasSuffix(files[i]->_name, ".c")) {
                os$FileInfo *obj = buildCFile(files[i]);
                utils$Slice_append(&objFiles, &obj);
                if (latestUpdate < os$FileInfo_modTime(obj)) {
                    latestUpdate = os$FileInfo_modTime(obj);
                }
            }
            os$FileInfo_free(files[i]);
        }
    }
    utils$Error *err = NULL;
    os$FileInfo *libFile = os$stat(dst, &err);
    if (libFile == NULL || latestUpdate > os$FileInfo_modTime(libFile)) {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, "/usr/bin/ar");
        Slice_appendStrLit(&cmd, "rsc");
        Slice_appendStrLit(&cmd, dst);
        for (int i = 0; i < utils$Slice_len(&objFiles); i++) {
            os$FileInfo *obj = NULL;
            utils$Slice_get(&objFiles, i, &obj);
            utils$Slice_append(&cmd, &obj->_name);
        }
        char **args = utils$Slice_to_nil_array(cmd);
        mkdirForFile(dst);
        execute(args);
        free(args);
        os$FileInfo_free(libFile);
        libFile = os$stat(dst, &err);
    }
    return libFile;
}

static void buildBlingPackage(Builder *b, const char *path) {
    ast$File **fs = parser$parseDir(b->fset, path, types$universe(), NULL);
    assert(fs[0] && !fs[1]);
    ast$File *f = fs[0];
    emitter$Emitter e = {};
    emitter$emitFile(&e, f);
    print(emitter$Emitter_string(&e));
}

extern void build$buildPackage(char *argv[]) {
    assert(*argv);
    char *path = *argv;
    sys$printf("building %s\n", path);
    Builder builder = {
        .fset = token$newFileSet(),
    };
    if (streq(path, "os") || streq(path, "sys")) {
        buildCPackage(&builder, path);
    } else {
        buildBlingPackage(&builder, path);
    }
}
