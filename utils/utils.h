#pragma once
#include "bootstrap/bootstrap.h"

package(utils);

import("sys");

typedef struct {
    char *error;
} utils$Error;

extern utils$Error *utils$NewError(const char *error);
extern void utils$Error_move(utils$Error *src, utils$Error **dst);
extern void utils$Error_free(utils$Error *e);

// clearing and checking errno
extern void utils$clearError();
extern void utils$Error_check(utils$Error **e);

typedef struct {
    int size;
    int _len;
    int cap;
    void *_array;
} utils$Slice;

extern utils$Slice utils$Slice_make(int size); // TODO replace with make
extern void utils$Slice_unmake(void *s);

extern int utils$Slice_len(const void *s);
extern int utils$Slice_cap(const void *s); // TODO replace with cap()
extern void *utils$Slice_get(const void *s, int i, void *dst);

extern void utils$Slice_setLen(void *s, int len);
extern void utils$Slice_set(void *s, int i, const void *x);
extern void utils$Slice_append(void *s, const void *x);
extern void *utils$nilArray(void *s);

typedef struct {
    void *key;
    void *val;
} utils$MapPair;

typedef struct {
    int _valSize;
    int _len;
    array(utils$MapPair) _pairs;
} utils$Map;

extern utils$Map utils$Map_make(int valSize); // TODO replace with make
extern void utils$Map_unmake(void *m);
extern int utils$Map_len(const void *m);
extern int utils$Map_cap(const void *m); // TODO replace with cap()
extern bool utils$Map_get(const void *m, const char *key, void *val);
extern void utils$Map_set(void *m, const char *key, const void *val);

typedef struct {
    int hits;
    int misses;
    int lookups;
    int iters;
} utils$MapStats;

typedef struct {
    const utils$Map *_map;
    int _idx;
} utils$MapIter;

extern utils$MapIter utils$NewMapIter(const void *m);
extern int utils$MapIter_next(utils$MapIter *m, char **key, void *val);
