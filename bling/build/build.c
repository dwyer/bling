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
    sys$free(src);
}

static void printStrArray(array(char *) s) {
    for (int i = 0; i < len(s); i++) {
        if (i) {
            sys$printf(" ");
        }
        sys$printf("%s", get(char *, s, i));
    }
    sys$printf("\n");
}

static void execute(array(char *) *cmd) {
    if (VERBOSE) {
        printStrArray(*cmd);
    }
    char **args = utils$nilArray(cmd);
    int code = sys$run(args);
    if (code != 0) {
        panic(sys$sprintf("- failed with code %d", code));
    }
    sys$free(args);
}

static void mkdirForFile(const char *path) {
    char *dir = paths$dir(path);
    os$mkdirAll(dir, 0755, NULL);
    sys$free(dir);
}

typedef struct Package Package;

typedef struct Package {
    char *path;
    types$Package *pkg;
    char *hPath;
    char *cPath;
    char *objPath;
    array(char *) objPaths;
    char *libPath;
    os$Time libModTime; // modtime of libPath
    os$Time srcModTime; // modtime of newest src or dep
    array(Package *) deps;
    bool isCmd;
} Package;

typedef struct {
    token$FileSet *fset;
    types$Info *info;
    bool force;
    types$Config conf;
    map(Package *) pkgs;
} Builder;

static void genObj(Builder *b, const char *dst, const char *src) {
    array(char *) cmd = makearray(char *);
    append(cmd, CC_PATH);
    append(cmd, (char *)"-fms-extensions");
    append(cmd, (char *)"-Wno-microsoft-anon-tag");
    append(cmd, (char *)"-g");
    append(cmd, (char *)"-I");
    append(cmd, INCL_PATH);
    append(cmd, (char *)"-c");
    append(cmd, (char *)"-o");
    append(cmd, dst);
    append(cmd, src);
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
    array(os$FileInfo *) files = ioutil$readDir(path, NULL);
    for (int i = 0; i < len(files); i++) {
        os$FileInfo *file = get(os$FileInfo *, files, i);
        os$Time modTime = os$FileInfo_modTime(file);
        if (bytes$hasSuffix(os$FileInfo_name(file), ".bling")) {
            if (t < modTime) {
                t = modTime;
            }
        }
        os$FileInfo_free(file);
    }
    utils$Slice_unmake(&files);
    return t;
}

static os$FileInfo *buildCFile(Builder *b, os$FileInfo *cFile) {
    // build an obj and return its path
    char *src = os$FileInfo_name(cFile);
    char *dst = NULL;
    {
        int i = bytes$lastIndexByte(src, '.');
        char *base = sys$strdup(src);
        base[i] = '\0';
        dst = sys$sprintf("%s/%s.o", GEN_PATH, base);
        sys$free(base);
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
    types$Package *typesPkg = types$check(&b->conf, path, b->fset, NULL, b->info);
    bool isCmd = typesPkg->name ? sys$streq(typesPkg->name, "main") : false;
    char *genPath = paths$join2(GEN_PATH, path);
    char *libPath = sys$sprintf("%s/%s.a", genPath, base);
    if (isCmd) {
        libPath = sys$sprintf("%s/%s", genPath, base);
    } else {
        libPath = sys$sprintf("%s/%s.a", genPath, base);
    }
    Package pkg = {
        .path = sys$strdup(path),
        .pkg = typesPkg,
        .hPath = sys$sprintf("%s/%s.h", genPath, base),
        .cPath = sys$sprintf("%s/%s.c", genPath, base),
        .objPath = sys$sprintf("%s/%s.o", genPath, base),
        .libPath = libPath,
        .libModTime = getFileModTime(libPath),
        .srcModTime = getSrcModTime(path),
        .deps = makearray(Package *),
        .isCmd = isCmd,
    };
    sys$free(genPath);
    sys$free(base);
    for (int i = 0; i < len(pkg.pkg->imports); i++) {
        Package *dep =
            _buildPackage(b, get(types$Package *, pkg.pkg->imports, i)->path);
        append(pkg.deps, dep);
        if (pkg.srcModTime < dep->srcModTime) {
            pkg.srcModTime = dep->srcModTime;
        }
    }
    return pkg;
}

static void emitInclude(emitter$Emitter *e, const char *path) {
    char *s = sys$sprintf("#include \"%s\"\n", path);
    emitter$emitString(e, s);
    sys$free(s);
}

static void writeFile(const char *path, const char *out) {
    if (VERBOSE) {
        // sys$printf("generating %s\n", path);
    }
    mkdirForFile(path);
    ioutil$writeFile(path, out, 0644, NULL);
}

static void genHeader(Builder *b, Package *pkg) {
    emitter$Emitter e = {};
    emitter$emitString(&e, "#pragma once");
    emitter$emitNewline(&e);
    emit_rawfile(&e, "bootstrap/bootstrap.h");
    for (int i = 0; i < len(pkg->deps); i++) {
        emitInclude(&e, get(Package *, pkg->deps, i)->hPath);
    }
    cemitter$emitHeader(&e, pkg->pkg);
    char *out = emitter$Emitter_string(&e);
    writeFile(pkg->hPath, out);
    sys$free(out);
}

static void getCFile(Builder *b, Package *pkg) {
    emitter$Emitter e = {};
    emitInclude(&e, pkg->hPath);
    cemitter$emitBody(&e, pkg->pkg);
    char *out = emitter$Emitter_string(&e);
    writeFile(pkg->cPath, out);
    sys$free(out);
}

static Package *buildCPackage(Builder *b, const char *path) {
    Package pkg = newPackage(b, path);
    array(char *) objFiles = makearray(char *);
    {
        array(os$FileInfo *) files = ioutil$readDir(path, NULL);
        for (int i = 0; i < len(files); i++) {
            bool checkTime = false;
            os$FileInfo *file = get(os$FileInfo *, files, i);
            os$Time modTime = os$FileInfo_modTime(file);
            if (bytes$hasSuffix(os$FileInfo_name(file), ".bling")) {
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
            } else if (bytes$hasSuffix(os$FileInfo_name(file), ".c")) {
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
                os$FileInfo *obj = buildCFile(b, file);
                append(objFiles, obj->_name);
                checkTime = true;
                modTime = os$FileInfo_modTime(obj);
                if (pkg.srcModTime < modTime) {
                    pkg.srcModTime = modTime;
                }
            }
            os$FileInfo_free(file);
        }
    }
    if (b->force || pkg.srcModTime > pkg.libModTime) {
        if (VERBOSE) {
            // sys$printf("%d > %d\n", pkg.srcModTime, pkg.libModTime);
        }
        genHeader(b, &pkg);
        array(char *) cmd = makearray(char *);
        append(cmd, AR_PATH);
        append(cmd, (char *)"rsc");
        append(cmd, pkg.libPath);
        for (int i = 0; i < len(objFiles); i++) {
            append(cmd, get(char *, objFiles, i));
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
            // sys$printf("%d > %d\n", pkg.srcModTime, pkg.libModTime);
        }
        if (pkg.isCmd) {
            array(char *) cmd = makearray(char *);
            append(cmd, CC_PATH);
            append(cmd, (char *)"-o");
            append(cmd, pkg.libPath);
            append(cmd, pkg.objPath);
            Package *pkg = NULL;
            utils$MapIter iter = utils$NewMapIter(&b->pkgs);
            while (utils$MapIter_next(&iter, NULL, &pkg)) {
                append(cmd, pkg->libPath);
            }
            execute(&cmd);
        } else {
            array(char *) cmd = makearray(char *);
            append(cmd, AR_PATH);
            append(cmd, (char *)"rsc");
            append(cmd, pkg.libPath);
            append(cmd, pkg.objPath);
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
        // sys$printf("building %s\n", path);
    }
    if (sys$streq(path, "bootstrap") || sys$streq(path, "os")
            || sys$streq(path, "sys")) {
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
        .pkgs = utils$Map_make(sizeof(Package *)),
    };
    _buildPackage(&builder, "bootstrap");
    _buildPackage(&builder, path);
}
