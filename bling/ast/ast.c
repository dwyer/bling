#include "bling/ast/ast.h"

static const desc_t _object_desc = {
    .size = sizeof(object_t),
};

extern scope_t *scope_new(scope_t *outer) {
    scope_t *s = malloc(sizeof(scope_t));
    s->outer = outer;
    s->objects = make_map(&desc_str, &_object_desc);
    s->filenames = make_slice(sizeof(char *), 0, 0);
    return s;
}

extern void scope_deinit(scope_t *s) {
    map_deinit(&s->objects);
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

extern void walk_file(file_t *file) {
    for (int i = 0; file->decls != NULL && file->decls[i] != NULL; i++) {
        fprintf(stderr, "decl: %p\n", file->decls[i]);
    }
}
