#include "os/os.h"

#include "paths/paths.h"
#include "sys/sys.h"
#include "utils/utils.h"

static os$File _stdin = {.fd = sys$STDIN_FILENO, .name = "/dev/stdin"};
static os$File _stdout = {.fd = sys$STDOUT_FILENO, .name = "/dev/stdout"};
static os$File _stderr = {.fd = sys$STDERR_FILENO, .name = "/dev/stderr"};

os$File *os$stdin = &_stdin;
os$File *os$stdout = &_stdout;
os$File *os$stderr = &_stderr;

static void PathError_check(const char *path, utils$Error **error) {
    utils$Error *err = NULL;
    utils$Error_check(&err);
    if (err) {
        char *msg = err->error;
        err->error = sys$sprintf("PathError: %s: %s", path, msg);
        sys$free(msg);
        utils$Error_move(err, error);
    }
}

extern os$File *os$newFile(uintptr fd, const char *name) {
    os$File file = {
        .name = sys$strdup(name),
        .fd = fd,
    };
    return esc(file);
}

extern os$File *os$openFile(const char *filename, int mode, int perm,
        utils$Error **error) {
    utils$clearError();
    int fd = sys$open(filename, mode, perm);
    if (fd == -1) {
        PathError_check(filename, error);
        return NULL;
    }
    return os$newFile(fd, filename);
}

extern os$File *os$open(const char *filename, utils$Error **error) {
    return os$openFile(filename, sys$O_RDONLY, 0, error);
}

extern os$File *os$create(const char *filename, utils$Error **error) {
    return os$openFile(filename, sys$O_CREAT | sys$O_TRUNC | sys$O_RDWR, 0644,
            error);
}

extern int os$read(os$File *file, char *b, int n, utils$Error **error) {
    utils$clearError();
    n = sys$read(file->fd, b, n);
    PathError_check(file->name, error);
    return n;
}

extern int os$write(os$File *file, const char *b, utils$Error **error) {
    utils$clearError();
    int n = sys$write(file->fd, b, sys$strlen(b));
    PathError_check(file->name, error);
    return n;
}

extern void os$close(os$File *file, utils$Error **error) {
    utils$clearError();
    if (file->is_dir) {
        sys$closedir((void *)file->fd);
    } else {
        sys$close(file->fd);
    }
    PathError_check(file->name, error);
}

extern os$FileInfo *os$stat(const char *name, utils$Error **error) {
    sys$Stat st = {};
    utils$clearError();
    if (sys$stat(name, &st) != 0) {
        PathError_check(name, error);
        return NULL;
    }
    os$FileInfo info = {
        ._name = sys$strdup(name),
        ._sys = esc(st),
    };
    return esc(info);
}

extern void os$FileInfo_free(os$FileInfo *info) {
    if (info) {
        sys$free(info->_name);
        sys$free(info->_sys);
        sys$free(info);
    }
}

extern os$File *os$openDir(const char *name, utils$Error **error) {
    void *dp = sys$opendir(name);
    if (dp == NULL) {
        PathError_check(name, error);
        return NULL;
    }
    os$File *file = os$newFile((uintptr)dp, name);
    file->is_dir = true;
    return file;
}

extern array(char *) os$readdirnames(os$File *file, utils$Error **error) {
    array(char *) arr = makearray(char *);
    sys$Dir dp = (sys$Dir)file->fd;
    for (;;) {
        sys$Dirent dirent = sys$readdir(dp);
        if (dirent == NULL) {
            break;
        }
        const char *name = sys$Dirent_name(dirent);
        if (name[0] == '.') {
            continue;
        }
        append(arr, sys$strdup(name));
    }
    return arr;
}

extern array(os$FileInfo *) os$readdir(os$File *file, utils$Error **error) {
    utils$Error *err = NULL;
    array(char *) names = os$readdirnames(file, &err);
    if (err != NULL) {
        os$close(file, NULL);
        utils$Error_move(err, error);
        return names;
    }
    array(char *) arr = makearray(char *);
    for (int i = 0; i < len(names); i++) {
        char *path = paths$join2(file->name, get(char *, names, i));
        sys$free(get(char *, names, i));
        os$FileInfo *info = os$stat(path, &err);
        if (err != NULL) {
            os$close(file, NULL);
            utils$Error_move(err, error);
            return names;
        }
        append(arr, info);
    }
    utils$Slice_unmake(&names);
    return arr;
}

extern char *os$FileInfo_name(os$FileInfo *info) {
    return info->_name;
}

extern u64 os$FileInfo_size(os$FileInfo *info) {
    sys$Stat *st = (sys$Stat *)os$FileInfo_sys(info);
    return st->size;
}

extern os$FileMode os$FileInfo_mode(os$FileInfo *info) {
    sys$Stat *st = (sys$Stat *)os$FileInfo_sys(info);
    return st->mode;
}

extern os$Time os$FileInfo_modTime(os$FileInfo *info) {
    sys$Stat *st = (sys$Stat *)os$FileInfo_sys(info);
    return st->mtime;
}

extern bool os$FileInfo_isDir(os$FileInfo *info) {
    sys$Stat *st = (sys$Stat *)os$FileInfo_sys(info);
    return ((st->mode & 0170000) == 0040000);
}

extern void *os$FileInfo_sys(os$FileInfo *info) {
    return info->_sys;
}

extern const char *os$tempDir() {
    char *tmpdir = sys$getenv("TMPDIR");
    if (tmpdir) {
        return tmpdir;
    }
    return "/tmp";
}

extern void os$mkdir(const char *path, u32 mode, utils$Error **error) {
    utils$clearError();
    sys$mkdir(path, mode);
    PathError_check(path, error);
}

extern void os$mkdirAll(const char *path, u32 mode, utils$Error **error) {
    char *dir = paths$dir(path);
    if (!sys$streq(dir, ".")) {
        os$mkdirAll(dir, mode, error);
    }
    sys$free(dir);
    utils$Error *err = NULL;
    os$mkdir(path, mode, &err);
    switch (sys$errno()) {
    case 0:
    case sys$EEXIST:
        break;
    default:
        utils$Error_move(err, error);
        break;
    }
}
