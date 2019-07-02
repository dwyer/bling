#include "bling/parser/parser.h"

void init(parser_t *p, const char *filename, const char *src) {

}

file_t *parse_file(parser_t *p) {
    return NULL;
}

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope) {
    char *src = ioutil_read_file(filename);
    parser_t p;
    init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    file_t *file = parse_file(&p);
    free(src);
    object_t *obj = NULL;
    for (map_iter_t it = map_iter(&pkg_scope->objects); map_iter_next(&it, NULL, &obj);) {
        fprintf(stderr, "%s: %d\n", obj->name, obj->kind);
    }
    return file;
}
