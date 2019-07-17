#pragma once

#include "builtin/builtin.h"

typedef struct {
    size_t size;
    bool is_ptr;
    void *(*dup)(const void *);
    int (*cmp)(const void *, const void *);
    uintptr_t (*hash)(const void *);
} desc_t;

extern int desc_cmp(const desc_t *d, const void *a, const void *b);
extern uintptr_t desc_hash(const desc_t *d, const void *a);

typedef struct {
    int len;
    slice_t pairs;
    const desc_t *key_desc;
    const desc_t *val_desc;
    int key_size;
    int val_size;
} map_t;

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

extern map_t make_map(const desc_t *key_desc, const desc_t *val_desc);
