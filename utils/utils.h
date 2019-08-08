#pragma once
#include "bootstrap/bootstrap.h"

package(utils);

typedef struct {
    int size;
    int len;
    int cap;
    void *array;
} utils$Slice;

extern utils$Slice utils$Slice_init(int size);
extern void utils$Slice_deinit(utils$Slice *s);

extern int utils$Slice_len(const utils$Slice *s);
extern int utils$Slice_cap(const utils$Slice *s);
extern void *utils$Slice_ref(const utils$Slice *s, int i);
extern void utils$Slice_get(const utils$Slice *s, int i, void *dst);

extern void utils$Slice_set_len(utils$Slice *s, int len);
extern void utils$Slice_set(utils$Slice *s, int i, const void *x);
extern void utils$Slice_append(utils$Slice *s, const void *x);
extern void *utils$Slice_to_nil_array(utils$Slice s);

typedef struct {
    int len;
    int key_size;
    int val_size;
    utils$Slice pairs;
} utils$Map;

typedef enum {
    utils$Map_status_ok = 1,
} utils$Map_status_t;

extern utils$Map utils$Map_init(int val_size);
extern void utils$Map_deinit(utils$Map *m);
extern int utils$Map_len(const utils$Map *m);
extern int utils$Map_cap(const utils$Map *m);
extern int utils$Map_get(const utils$Map *m, const char *key, void *val);
extern bool utils$Map_has_key(utils$Map *m, const char *key);
extern void utils$Map_set(utils$Map *m, const char *key, const void *val);

extern int utils$Map_hits;
extern int utils$Map_misses;
extern int utils$Map_lookups;
extern int utils$Map_iters;

typedef struct {
    const utils$Map *_map;
    int _idx;
} utils$Map_iter_t;

extern utils$Map_iter_t utils$Map_iter(const utils$Map *m);
extern int utils$Map_iter_next(utils$Map_iter_t *m, char **key, void *val);
