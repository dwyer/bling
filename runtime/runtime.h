#pragma once

#include "runtime/clib.h"

extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);

extern int strcmp(const char *, const char *);
extern char *strdup(const char *);
extern char *strerror(int);
extern size_t strlcpy(char *, const char *, size_t);
extern size_t strlen(const char *);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} slice_t;

extern slice_t slice_init(int size, int len, int cap);
extern void slice_deinit(slice_t *s);

extern int slice_len(const slice_t *s);
extern int slice_cap(const slice_t *s);
extern void *slice_ref(const slice_t *s, int i);
extern void slice_get(const slice_t *s, int i, void *dst);

extern void slice_set_len(slice_t *s, int len);
extern void slice_set(slice_t *s, int i, const void *x);
extern void slice_append(slice_t *s, const void *x);
extern void *slice_to_nil_array(slice_t s);

extern void *memdup(const void *src, size_t size);
