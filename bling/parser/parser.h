#pragma once
#include "builtin/builtin.h"
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope);

typedef struct {
    scanner_t scanner;
    token_t tok;
    char *lit;
    char *filename;
    scope_t *top_scope;
    scope_t *pkg_scope;
} parser_t;

extern void parser_declare(parser_t *p, scope_t *s, obj_kind_t kind, char *name);
extern void parser_next(parser_t *p);
extern void parser_init(parser_t *p, char *filename, char *src);
