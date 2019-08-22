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
const bool VERBOSE = true;

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
    if (VERBOSE) {
        printStrArray(args);
    }
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
    char *path;
    types$Package *pkg;
    char *hPath;
    char *cPath;
    char *objPath;
    utils$Slice objPaths;
    char *libPath;
    os$Time libModTime; // modtime of libPath
    os$Time srcModTime; // modtime of newest src or dep
    utils$Slice deps;
} Package;

typedef struct {
    token$FileSet *fset;
    types$Info *info;
    bool force;
    types$Config conf;
    utils$Map pkgs;
} Builder;

static void genObj(Builder *b, const char *dst, const char *src) {
    utils$Slice cmd = {.size = sizeof(char *)};
    Slice_appendStrLit(&cmd, CC_PATH);
    Slice_appendStrLit(&cmd, "-fms-extensions");
    Slice_appendStrLit(&cmd, "-Wno-microsoft-anon-tag");
    Slice_appendStrLit(&cmd, "-I");
    Slice_appendStrLit(&cmd, INCL_PATH);
    Slice_appendStrLit(&cmd, "-c");
    Slice_appendStrLit(&cmd, "-o");
    Slice_appendStrLit(&cmd, dst);
    Slice_appendStrLit(&cmd, src);
    mkdirForFile(dst);
    execute(&cmd);
}

static os$Time getFileModTime(const char *path) {
    os$Time t = 0;
    utils$Error *err = NULL;
    os$FileInfo *info = os$stat(path, &err);
    if (info) {
        t = os$FileInfo_modTime(info);
        os$FileInfo_free(info);
    }
    return t;
}

static os$Time getSrcModTime(const char *path) {
    os$Time t = 0;
    os$FileInfo **files = ioutil$readDir(path, NULL);
    for (int i = 0; files[i]; i++) {
        os$Time modTime = os$FileInfo_modTime(files[i]);
        if (bytes$hasSuffix(files[i]->_name, ".bling")) {
            if (t < modTime) {
                t = modTime;
            }
        }
        os$FileInfo_free(files[i]);
    }
    free(files);
    return t;
}

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
        genObj(b, dst, src);
        // update objFile
        os$FileInfo_free(objFile);
        objFile = os$stat(dst, &err);
    }
    return objFile;
}

static Package *_buildPackage(Builder *b, const char *path);

static Package newPackage(Builder *b, const char *path) {
    char *base = paths$base(path);
    char *genPath = paths$join2(GEN_PATH, path);
    char *libPath = sys$sprintf("%s/%s.a", genPath, base);
    Package pkg = {
        .path = strdup(path),
        .pkg = types$check(&b->conf, path, b->fset, NULL, b->info),
        .hPath = sys$sprintf("%s/%s.h", genPath, base),
        .cPath = sys$sprintf("%s/%s.c", genPath, base),
        .objPath = sys$sprintf("%s/%s.o", genPath, base),
        .libPath = libPath,
        .libModTime = getFileModTime(libPath),
        .srcModTime = getSrcModTime(path),
        .deps = {.size = sizeof(Package *)},
    };
    free(genPath);
    free(base);
    for (int i = 0; i < utils$Slice_len(&pkg.pkg->imports); i++) {
        types$Package *impt = NULL;
        utils$Slice_get(&pkg.pkg->imports, i, &impt);
        Package *dep = _buildPackage(b, impt->path);
        utils$Slice_append(&pkg.deps, &dep);
        if (pkg.srcModTime < dep->srcModTime) {
            pkg.srcModTime = dep->srcModTime;
        }
    }
    return pkg;
}

typedef struct {
    utils$Slice *s;
    int i;
    void *it;
} SliceIter;

extern bool SliceIter_next(SliceIter *iter, void *it) {
    if (iter->i < utils$Slice_len(iter->s)) {
        utils$Slice_get(iter->s, iter->i, it);
        iter->it = it;
        iter->i++;
        return true;
    }
    return false;
}

static void emitInclude(emitter$Emitter *e, const char *path) {
    char *s = sys$sprintf("#include \"%s\"\n", path);
    emitter$emitString(e, s);
    free(s);
}

static void writeFile(const char *path, const char *out) {
    if (VERBOSE) {
        sys$printf("generating %s\n", path);
    }
    mkdirForFile(path);
    ioutil$writeFile(path, out, 0644, NULL);
}

static void genHeader(Builder *b, Package *pkg) {
    emitter$Emitter e = {};
    emitter$emitString(&e, "#pragma once");
    emitter$emitNewline(&e);
    emit_rawfile(&e, "bootstrap/bootstrap.h");
    for (int i = 0; i < utils$Slice_len(&pkg->deps); i++) {
        Package *dep = NULL;
        utils$Slice_get(&pkg->deps, i, &dep);
        emitInclude(&e, dep->hPath);
    }
    cemitter$emitHeader(&e, pkg->pkg);
    char *out = emitter$Emitter_string(&e);
    writeFile(pkg->hPath, out);
    free(out);
}

static void getCFile(Builder *b, Package *pkg) {
    emitter$Emitter e = {};
    emitInclude(&e, pkg->hPath);
    cemitter$emitBody(&e, pkg->pkg);
    char *out = emitter$Emitter_string(&e);
    writeFile(pkg->cPath, out);
    free(out);
}

static Package *buildCPackage(Builder *b, const char *path) {
    Package pkg = newPackage(b, path);
    utils$Slice objFiles = {.size = sizeof(char *)};
    {
        os$FileInfo **files = ioutil$readDir(path, NULL);
        for (int i = 0; files[i]; i++) {
            bool checkTime = false;
            os$Time modTime = os$FileInfo_modTime(files[i]);
            if (bytes$hasSuffix(files[i]->_name, ".bling")) {
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
            } else if (bytes$hasSuffix(files[i]->_name, ".c")) {
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
                os$FileInfo *obj = buildCFile(b, files[i]);
                utils$Slice_append(&objFiles, &obj->_name);
                checkTime = true;
                modTime = os$FileInfo_modTime(obj);
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
            }
            os$FileInfo_free(files[i]);
        }
    }
    if (b->force || pkg.srcModTime > pkg.libModTime) {
        if (VERBOSE) {
            sys$printf("%d > %d\n", pkg.srcModTime, pkg.libModTime);
        }
        genHeader(b, &pkg);
        utils$Slice cmd = {.size = sizeof(char *)};
        Slice_appendStrLit(&cmd, AR_PATH);
        Slice_appendStrLit(&cmd, "rsc");
        Slice_appendStrLit(&cmd, pkg.libPath);
        for (int i = 0; i < utils$Slice_len(&objFiles); i++) {
            char *obj = NULL;
            utils$Slice_get(&objFiles, i, &obj);
            utils$Slice_append(&cmd, &obj);
        }
        mkdirForFile(pkg.libPath);
        execute(&cmd);
        pkg.libModTime = getFileModTime(pkg.libPath);
    }
    return esc(pkg);
}

static Package *buildBlingPackage(Builder *b, const char *path) {
    Package pkg = newPackage(b, path);
    if (b->force || pkg.srcModTime > pkg.libModTime) {
        genHeader(b, &pkg);
        getCFile(b, &pkg);
        genObj(b, pkg.objPath, pkg.cPath);
        if (VERBOSE) {
            sys$printf("%d > %d\n", pkg.srcModTime, pkg.libModTime);
        }
        if (streq(path, "cmd/blingc")) {
            utils$Slice cmd = {.size = sizeof(char *)};
            Slice_appendStrLit(&cmd, CC_PATH);
            Slice_appendStrLit(&cmd, "-o");
            Slice_appendStrLit(&cmd, "blingc.out");
            Slice_appendStrLit(&cmd, pkg.objPath);
            Package *pkg = NULL;
            utils$MapIter iter = utils$NewMapIter(&b->pkgs);
            while (utils$MapIter_next(&iter, NULL, &pkg)) {
                Slice_appendStrLit(&cmd, pkg->libPath);
            }
            execute(&cmd);
        } else {
            utils$Slice cmd = {.size = sizeof(char *)};
            Slice_appendStrLit(&cmd, AR_PATH);
            Slice_appendStrLit(&cmd, "rsc");
            Slice_appendStrLit(&cmd, pkg.libPath);
            Slice_appendStrLit(&cmd, pkg.objPath);
            execute(&cmd);
        }
    }
    return esc(pkg);
}

static Package *_buildPackage(Builder *b, const char *path) {
    Package *pkg = NULL;
    utils$Map_get(&b->pkgs, path, &pkg);
    if (pkg) {
        return pkg;
    }
    if (VERBOSE) {
        sys$printf("building %s\n", path);
    }
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
        .force = false,
        .pkgs = utils$Map_init(sizeof(Package *)),
    };
    _buildPackage(&builder, "bootstrap");
    _buildPackage(&builder, path);
}
