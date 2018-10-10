#include "subc/ast/ast.h"

static const desc_t _object_desc = {
    .size = sizeof(object_t),
};

extern scope_t *scope_new(scope_t *outer) {
    scope_t *s = malloc(sizeof(*s));
    s->outer = outer;
    s->objects = make_map(&desc_str, &_object_desc);
    return s;
}

extern object_t *scope_insert(scope_t *s, object_t *obj) {
    object_t *alt = NULL;
    map_get(&s->objects, obj->name, &obj);
    if (alt == NULL) {
        map_set(&s->objects, obj->name, &obj);
    }
    return alt;
}

extern object_t *scope_lookup(scope_t *s, char *name) {
    object_t *obj = NULL;
    map_get(&s->objects, name, &obj);
    return obj;
}
