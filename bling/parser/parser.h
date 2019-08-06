#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"

import("bling/ast");
import("bling/scanner");
import("bling/token");
import("bytes");
import("fmt");
import("io/ioutil");
import("path");

extern ast_File *parser_parse_file(const char *filename);
extern ast_File **parser_parseDir(const char *path, error_t **first);

typedef struct {
    token_File *file;
    scanner_t scanner;

    pos_t pos;
    token_t tok;
    char *lit;
    ast_Scope *pkg_scope;
    bool c_mode;

    char *pkgName;
} parser_t;

extern void parser_declare(parser_t *p, ast_Scope *s, decl_t *decl, obj_kind_t kind, expr_t *name);
extern void parser_next(parser_t *p);
extern void parser_init(parser_t *p, const char *filename, char *src);
extern void parser_error(parser_t *p, pos_t pos, char *msg, ...);
extern bool accept(parser_t *p, token_t tok);
extern pos_t expect(parser_t *p, token_t tok);
extern expr_t *basic_lit(parser_t *p, token_t kind);
extern expr_t *identifier(parser_t *p);
extern expr_t *primary_expression(parser_t *p);
extern decl_t *parse_pragma(parser_t *p);
