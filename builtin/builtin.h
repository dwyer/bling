#pragma once

#include <ctype.h>
#include <execinfo.h> // backtrace
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define import(...)
#define package(_)

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
void print(char *fmt, ...);
void panic(char *fmt, ...);
