#pragma once
#include "builtin/builtin.h"

$import("path");

typedef uint64_t time_Time;

typedef uint32_t os_FileMode;

typedef struct {
    uintptr_t fd;
    char *name;
    bool is_dir;
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

typedef struct {
    char *_name;
    void *_sys;
} os_FileInfo;

extern char *os_FileInfo_name(os_FileInfo info);
extern uint64_t os_FileInfo_size(os_FileInfo info);
extern os_FileMode os_FileInfo_mode(os_FileInfo info);
extern time_Time os_FileInfo_mod_time(os_FileInfo info);
extern bool os_FileInfo_is_dir(os_FileInfo info);
extern void *os_FileInfo_sys(os_FileInfo info);

extern void os_FileInfo_free(os_FileInfo info);

extern os_FileInfo os_stat(const char *filename, error_t **error);

extern os_File *os_openDir(const char *filename, error_t **error);
extern os_FileInfo **os_readdir(os_File *file, error_t **error);
