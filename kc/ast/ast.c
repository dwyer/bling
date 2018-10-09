#include "kc/ast/ast.h"

extern scope_t *scope_new(scope_t *outer) {
    scope_t *s = malloc(sizeof(*s));
    s->outer = outer;
    s->types = make_slice(&desc_str, 0, 0);
    return s;
}

extern void scope_insert_type(scope_t *s, char *name) {
    s->types = append(s->types, &name);
}

extern int scope_lookup_type(scope_t *s, char *name) {
    for (int i = 0; i < len(s->types); i++) {
        if (!strcmp(*(char **)get_ptr(s->types, i), name)) {
            return 1;
        }
    }
    return 0;
}
