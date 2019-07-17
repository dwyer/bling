#ifndef __CLIB_H__
#define __CLIB_H__

#include <sys/stat.h> // os: stat
#include <dirent.h> // os: DIR, dirent, opendir, readdir, closedir
#include <fcntl.h> // os: open
#include <unistd.h> // os: read, close

#include <execinfo.h> // backtrace
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <libgen.h> // basename, dirname

#include <ctype.h> // isspace
#include <time.h> // clock

#include <errno.h>

#undef assert
#define assert(x) do { if (!(x)) panic("assert failed: " # x); } while (0)

#define esc(x) memcpy(malloc(sizeof(x)), &(x), sizeof(x))

#endif
