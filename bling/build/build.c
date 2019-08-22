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

extern void printStrArray(char **s) {
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
    // printStrArray(args);
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
    types$Package *pkg;
    os$Time modTime;
} Package;

typedef struct {
    token$FileSet *fset;
    types$Info *info;
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
        .pkg = types$check(&b->conf, path, b->fset, NULL, b->info),
    };
    utils$Slice libs = {.size = sizeof(os$FileInfo *)};
    for (int i = 0; i < utils$Slice_len(&pkg.pkg->imports); i++) {
        types$Package *impt = NULL;
        utils$Slice_get(&pkg.pkg->imports, i, &impt);
        Package *imp = _buildPackage(b, impt->path);
        os$FileInfo *lib = imp->lib;
        assert(lib);
        if (pkg.modTime < os$FileInfo_modTime(lib)) {
            pkg.modTime = os$FileInfo_modTime(lib);
        }
        utils$Slice_append(&libs, &lib);
    }

    char *base = paths$base(path);
    char *libPath = sys$sprintf("%s/%s/lib%s.a", GEN_PATH, path, base);
    char *objPath = sys$sprintf("%s/%s/%s.o", GEN_PATH, path, base);
    char *cPath = sys$sprintf("%s/%s/%s.c", GEN_PATH, path, base);
    free(base);

    emitter$Emitter e = {
        .forwardDecl = true,
    };
    emit_rawfile(&e, "bootstrap/bootstrap.h");
    cemitter$emitPackage(&e, pkg.pkg);
    mkdirForFile(cPath);
    ioutil$writeFile(cPath, emitter$Emitter_string(&e), 0644, NULL);

    {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, CC_PATH);
        Slice_appendStrLit(&cmd, "-fms-extensions");
        Slice_appendStrLit(&cmd, "-Wno-microsoft-anon-tag");
        Slice_appendStrLit(&cmd, "-c");
        Slice_appendStrLit(&cmd, "-o");
        Slice_appendStrLit(&cmd, objPath);
        Slice_appendStrLit(&cmd, cPath);
        execute(&cmd);
    }

    if (streq(path, "cmd/blingc")) {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, CC_PATH);
        Slice_appendStrLit(&cmd, "-o");
        Slice_appendStrLit(&cmd, "blingc.out");
        Slice_appendStrLit(&cmd, objPath);
        {
            Package *pkg = NULL;
            utils$MapIter iter = utils$NewMapIter(&b->pkgs);
            while (utils$MapIter_next(&iter, NULL, &pkg)) {
                Slice_appendStrLit(&cmd, pkg->lib->_name);
            }
        }
        execute(&cmd);
    } else {
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, AR_PATH);
        Slice_appendStrLit(&cmd, "rsc");
        Slice_appendStrLit(&cmd, libPath);
        Slice_appendStrLit(&cmd, objPath);
        execute(&cmd);
        pkg.lib = os$stat(libPath, NULL);
    }

    return esc(pkg);
}

static Package *_buildPackage(Builder *b, const char *path) {
    Package *pkg = NULL;
    utils$Map_get(&b->pkgs, path, &pkg);
    if (pkg) {
        return pkg;
    }
    // sys$printf("building %s\n", path);
    if (streq(path, "bootstrap") || streq(path, "os") || streq(path, "sys")) {
        pkg = buildCPackage(b, path);
    } else {
        pkg = buildBlingPackage(b, path);
    }
    utils$Map_set(&b->pkgs, path, &pkg);
    return pkg;
}

extern void build$buildPackage(char *argv[]) {
    assert(*argv);
    char *path = *argv;
    Builder builder = {
        .fset = token$newFileSet(),
        .info = types$newInfo(),
        .force = true,
        .pkgs = utils$Map_init(sizeof(Package *)),
    };
    _buildPackage(&builder, "bootstrap");
    _buildPackage(&builder, path);
}
