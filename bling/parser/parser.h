#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"

$import("bling/ast");
$import("bling/scanner");
$import("bling/token");
$import("io/ioutil");
$import("path");

extern file_t *parser_parse_file(char *filename);
extern file_t **parser_parseDir(const char *path, error_t **first);

typedef struct {
    scanner_t scanner;
    token_t tok;
    char *lit;
    char *filename;
    scope_t *pkg_scope;
    bool c_mode;
} parser_t;

extern void parser_declare(parser_t *p, scope_t *s, decl_t *decl, obj_kind_t kind, expr_t *name);
extern void parser_next(parser_t *p);
extern void parser_init(parser_t *p, char *filename, char *src);
extern void parser_error(parser_t *p, char *fmt, ...);
extern bool accept(parser_t *p, token_t tok);
extern void expect(parser_t *p, token_t tok);
extern expr_t *basic_lit(parser_t *p, token_t kind);
extern expr_t *identifier(parser_t *p);
extern expr_t *primary_expression(parser_t *p);
