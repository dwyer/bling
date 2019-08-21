#include "bling/build/build.h"

#include "bling/parser/parser.h"
#include "bling/types/types.h"
#include "os/os.h"
#include "paths/paths.h"
#include "subc/cemitter/cemitter.h"
#include "sys/sys.h"

const char *AR_PATH = "/usr/bin/ar";
const char *CC_PATH = "/usr/bin/cc";
const char *GEN_PATH = "gen";
const char *INCL_PATH = ".";

static void emit_rawfile(emitter$Emitter *e, const char *filename) {
    utils$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic(err->error);
    }
    emitter$emitString(e, src);
    free(src);
}

static void printStrArray(char **s) {
    for (int i = 0; s[i]; i++) {
        if (i) {
            sys$printf(" ");
        }
        sys$printf("%s", s[i]);
    }
    sys$printf("\n");
}

static void execute(utils$Slice *cmd) {
    char **args = utils$Slice_to_nil_array(*cmd);
    printStrArray(args);
    int code = sys$run(args);
    if (code != 0) {
        panic(sys$sprintf("- failed with code %d", code));
    }
    free(args);
}

static void Slice_appendStrLit(utils$Slice *a, const char *s) {
    utils$Slice_append(a, &s);
}

static void mkdirForFile(const char *path) {
    char *dir = paths$dir(path);
    os$mkdirAll(dir, 0755, NULL);
    free(dir);
}

typedef struct {
    const char *path;
    os$FileInfo *lib;
    ast$File **files;
    types$Package *pkg;
    os$Time modTime;
} Package;

typedef struct {
    token$FileSet *fset;
    bool force;
    types$Config conf;
    utils$Map pkgs;
} Builder;

static os$FileInfo *buildCFile(Builder *b, os$FileInfo *cFile) {
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
    if (b->force || doBuild) {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, CC_PATH);
        Slice_appendStrLit(&cmd, "-I");
        Slice_appendStrLit(&cmd, INCL_PATH);
        Slice_appendStrLit(&cmd, "-c");
        Slice_appendStrLit(&cmd, "-o");
        Slice_appendStrLit(&cmd, dst);
        Slice_appendStrLit(&cmd, src);
        mkdirForFile(dst);
        execute(&cmd);
        // update objFile
        os$FileInfo_free(objFile);
        objFile = os$stat(dst, &err);
    }
    return objFile;
}

static Package *_buildPackage(Builder *b, const char *path);

static Package *buildCPackage(Builder *b, const char *path) {
    char *base = paths$base(path);
    char *dst = sys$sprintf("%s/%s/lib%s.a", GEN_PATH, path, base);
    utils$Slice objFiles = {.size = sizeof(os$FileInfo *)};
    Package pkg = {
        .path = path,
    };
    {
        os$FileInfo **files = ioutil$readDir(path, NULL);
        for (int i = 0; files[i]; i++) {
            if (bytes$hasSuffix(files[i]->_name, ".bling")) {
                ast$File *file = parser$parseFile(b->fset, files[i]->_name,
                        types$universe());
                // TODO type check the file
                for (int i = 0; file->imports[i]; i++) {
                    char *path = types$constant_stringVal(
                            file->imports[i]->imp.path);
                    Package *pkg = _buildPackage(b, path);
                    os$FileInfo *lib = pkg->lib;
                    if (lib != NULL) {
                        utils$Slice_append(&objFiles, &lib);
                    }
                }
            } else if (bytes$hasSuffix(files[i]->_name, ".c")) {
                os$FileInfo *obj = buildCFile(b, files[i]);
                utils$Slice_append(&objFiles, &obj);
                if (pkg.modTime < os$FileInfo_modTime(obj)) {
                    pkg.modTime = os$FileInfo_modTime(obj);
                }
            }
            os$FileInfo_free(files[i]);
        }
    }
    utils$Error *err = NULL;
    pkg.lib = os$stat(dst, &err);
    if (b->force || pkg.lib == NULL || pkg.modTime > os$FileInfo_modTime(pkg.lib)) {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, "/usr/bin/ar");
        Slice_appendStrLit(&cmd, "rsc");
        Slice_appendStrLit(&cmd, dst);
        for (int i = 0; i < utils$Slice_len(&objFiles); i++) {
            os$FileInfo *obj = NULL;
            utils$Slice_get(&objFiles, i, &obj);
            utils$Slice_append(&cmd, &obj->_name);
        }
        mkdirForFile(dst);
        execute(&cmd);
        os$FileInfo_free(pkg.lib);
        pkg.lib = os$stat(dst, &err);
        pkg.modTime = os$FileInfo_modTime(pkg.lib);
    }
    return esc(pkg);
}

static Package *buildBlingPackage(Builder *b, const char *path) {
    Package pkg = {
        .files = parser$parseDir(b->fset, path, types$universe(), NULL),
    };
    pkg.pkg = types$check(&b->conf, path, b->fset, pkg.files, NULL);
    utils$Slice libs = {.size = sizeof(os$FileInfo *)};

    emitter$Emitter e = {.forwardDecl=true};
    emit_rawfile(&e, "bootstrap/bootstrap.h");

    ast$File *f = pkg.files[0];
    for (int i = 0; f->imports[i]; i++) {
        // for (int i = 0; pkg.files[i]; i++) {
        //     cemitter$emitFile(&e, pkg.files[i]);
        // }
        // e.forwardDecl = false;
        // cemitter$emitFile(&e, f);
        char *path = types$constant_stringVal(f->imports[i]->imp.path);
        Package *imp = _buildPackage(b, path);
        os$FileInfo *lib = imp->lib;
        assert(lib);
        if (pkg.modTime < os$FileInfo_modTime(lib)) {
            pkg.modTime = os$FileInfo_modTime(lib);
        }
        utils$Slice_append(&libs, &lib);
    }

    // TODO typecheck the file
    // TODO emit imports (forward decls only)
    // TODO build imports
    // TODO emit C code
    // print(emitter$Emitter_string(&e));
    // TODO compile C code to obj
    // TODO link obj with imported libs to make new lib

    char *base = paths$base(path);
    char *libPath = sys$sprintf("%s/%s/lib%s.a", GEN_PATH, path, base);
    char *objPath = sys$sprintf("%s/%s/%s.o", GEN_PATH, path, base);
    char *cPath = sys$sprintf("%s/%s/%s.c", GEN_PATH, path, base);
    free(base);

    mkdirForFile(cPath);
    ioutil$writeFile(cPath, emitter$Emitter_string(&e), 0644, NULL);

    {
        utils$Slice cmd = {.size = sizeof(char *)};
        // Slice_appendStrLit(&cmd, "/bin/echo");
        Slice_appendStrLit(&cmd, CC_PATH);
        Slice_appendStrLit(&cmd, "-c");
        Slice_appendStrLit(&cmd, "-o");
        Slice_appendStrLit(&cmd, objPath);
        Slice_appendStrLit(&cmd, cPath);
        execute(&cmd);
    }

    {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, AR_PATH);
        Slice_appendStrLit(&cmd, "rsc");
        Slice_appendStrLit(&cmd, libPath);
        Slice_appendStrLit(&cmd, objPath);
        execute(&cmd);
    }

    return NULL;
}

static Package *_buildPackage(Builder *b, const char *path) {
    sys$printf("building %s\n", path);
    if (streq(path, "os") || streq(path, "sys")) {
        return buildCPackage(b, path);
    }
    return buildBlingPackage(b, path);
}

extern void build$buildPackage(char *argv[]) {
    assert(*argv);
    char *path = *argv;
    Builder builder = {
        .fset = token$newFileSet(),
        .force = true,
        .pkgs = utils$Map_init(sizeof(Package *)),
    };
    _buildPackage(&builder, path);
}
