#pragma once

#include <ctype.h>
#include <execinfo.h> // backtrace
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define import(...)
#define package(_)

#define log(fmt, ...) fprintf(stderr, fmt "\n", ## __VA_ARGS__);

#define panic(fmt, ...) do { \
    fprintf(stderr, fmt "\n", ## __VA_ARGS__); \
    void *buf[1000]; \
    int n = backtrace(buf, 1000); \
    backtrace_symbols_fd(buf, n, 2); \
    exit(1); \
} while (0)

#define error(fmt, ...) do { \
    fprintf(stderr, "%d:%d " fmt "\n", line(), col(), ## __VA_ARGS__); \
    void *buf[1000]; \
    int n = backtrace(buf, 1000); \
    backtrace_symbols_fd(buf, n, 2); \
    exit(1); \
} while (0)

#define memdup(src, size) memcpy(malloc((size)), (src), (size))
#define copy(src) memdup((src), sizeof(*(src)))

typedef struct {
    void *array;
    int len;
    int cap;
    size_t size;
} slice_t;

int len(slice_t s);
int cap(slice_t s);
void *get_ptr(slice_t s, int index);
slice_t append(slice_t s, void *obj);
