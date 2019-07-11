#pragma once
#include "builtin/builtin.h"

$import("path");

typedef uint64_t time_Time;

typedef uint32_t os_FileMode;

typedef struct {
    int fd;
    char *name;
} os_File;

extern os_File *os_stdin;
extern os_File *os_stdout;
extern os_File *os_stderr;

extern os_File *os_newFile(uintptr_t fd, const char *name);

extern os_File *os_openFile(const char *filename, int mode, int perm, error_t **error);
extern void os_close(os_File *file, error_t **error);

extern os_File *os_open(const char *filename, error_t **error);
extern os_File *os_create(const char *filename, error_t **error);

extern int os_read(os_File *file, char *b, int n, error_t **error);
extern int os_write(os_File *file, const char *b, error_t **error);

extern char **os_listdir(const char *dirname);

typedef struct {
    char *name;
    uint64_t size;
    os_FileMode mode;
    time_Time mod_time;
    bool is_dir;
    void *sys;
} os_FileInfo;

extern os_FileInfo os_stat(const char *filename);
extern os_FileInfo **os_readdir(const char *dirname, error_t **error);
