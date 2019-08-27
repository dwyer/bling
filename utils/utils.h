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
extern void utils$Slice_unmake(utils$Slice *s);

extern int utils$Slice_len(const utils$Slice *s);
extern int utils$Slice_cap(const utils$Slice *s); // TODO replace with cap()
extern void *utils$Slice_get(const utils$Slice *s, int i, void *dst);

extern void utils$Slice_setLen(utils$Slice *s, int len);
extern void utils$Slice_set(utils$Slice *s, int i, const void *x);
extern void utils$Slice_append(utils$Slice *s, const void *x);
extern void *utils$nilArray(utils$Slice *s);

typedef struct {
    int _valSize;
    int _len;
    utils$Slice _pairs;
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
