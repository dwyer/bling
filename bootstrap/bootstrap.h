#ifndef BLING_BOOTSTRAP_H
#define BLING_BOOTSTRAP_H

#define $$ESC(x) ({ \
        typeof(x) $0 = (x); \
        runtime$memcpy(runtime$malloc(sizeof $0), &$0, sizeof $0); \
        })

#define esc(x) $$ESC(x)

#define array(T) runtime$Slice
#define map(T) runtime$Map

#define delete(x) _Generic((x), \
        runtime$Slice: runtime$Array_unmake, \
        runtime$Map: runtime$Map_unmake, \
        default: runtime$free)(x)

#define append(a, x) do { \
    typeof(x) $0 = (x); \
    runtime$Array_append((runtime$Slice*)&(a), sizeof(x), &$0); \
} while (0)

#define assert(x) do { if (!(x)) print("!!! ASSERT FAILED: " # x); } while (0)
#define copy(x) _Generic((x) string: runtime$strdup)(x)

#define len(x) _Generic((x), \
        runtime$Slice: runtime$Slice_len, \
        runtime$Map: runtime$Map_len, \
        char const*: runtime$strlen, \
        char *: runtime$strlen)((x))

#define makearray(T) (runtime$Slice){}
#define makemap(T) (runtime$Map){._valSize = sizeof(T)}

#define $$arrayget(T, a, i) (*(T*)runtime$Array_get((runtime$Slice*)&(a), sizeof(T), (i), nil))

#define $$mapget(T, m, k) ({ \
        T $$0; \
        runtime$Map_get((runtime$Map*)&(m), (k), &$$0); \
        $$0; \
        })

#define $$mapset(m, k, v) runtime$Map_set((runtime$Map*)&(m), (k), ({ typeof(v) $$0 = (v); &$$0; }))

#define nil ((void*)0)

#define false 0
#define true 1

#define panic(s) runtime$_panic(s)
#define print(s) runtime$_print(s)

typedef char bool;

typedef char const *string;
typedef void *voidptr;
typedef char *charptr;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef u32 uint;
typedef u64 uintptr;

#endif
