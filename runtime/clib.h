#ifndef __CLIB_H__
#define __CLIB_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <sys/stat.h> // os: stat
#include <dirent.h> // os: DIR, dirent, opendir, readdir, closedir
#include <fcntl.h> // os: open
// #include <unistd.h> // os: read, close, write, STD*_FILENO

#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) memcpy(malloc(sizeof(x)), &(x), sizeof(x))

#endif
