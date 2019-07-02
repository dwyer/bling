#pragma once

#include "runtime/clib.h"

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

typedef struct {
    size_t size;
    bool is_ptr;
    void *(*dup)(const void *);
    int (*cmp)(const void *, const void *);
    uintptr_t (*hash)(const void *);
} desc_t;

typedef struct {
    int len;
    int cap;
    void *array;
    const desc_t *desc;
} slice_t;

typedef struct {
    int len;
    slice_t pairs;
    const desc_t *key_desc;
    const desc_t *val_desc;
} map_t;

extern int desc_cmp(const desc_t *d, const void *a, const void *b);
extern uintptr_t desc_hash(const desc_t *d, const void *a);

extern slice_t slice_init(const desc_t *desc, int len, int cap);
extern void slice_deinit(slice_t *s);

extern int slice_len(const slice_t *s);
extern int slice_cap(const slice_t *s);
extern void *slice_ref(const slice_t *s, int i);
extern void slice_get(const slice_t *s, int i, void *dst);

extern void slice_set_len(slice_t *s, int len);
extern void slice_set(slice_t *s, int i, const void *x);
extern void slice_append(slice_t *s, const void *x);

typedef enum {
    map_status_ok = 1,
} map_status_t;

extern map_t map_init(const desc_t *key_desc, const desc_t *val_desc);
extern void map_deinit(map_t *m);
extern int map_len(const map_t *m);
extern int map_cap(const map_t *m);
extern int map_get(const map_t *m, const void *key, void *val);
extern bool map_has_key(map_t *m, const void *key);
extern void map_set(map_t *m, const void *key, const void *val);

extern const desc_t desc_int;
extern const desc_t desc_str;

extern int map_hits;
extern int map_misses;
extern int map_lookups;
extern int map_iters;

typedef struct {
    const map_t *_map;
    int _idx;
} map_iter_t;

extern map_iter_t map_iter(const map_t *m);
extern int map_iter_next(map_iter_t *m, void *key, void *val);
