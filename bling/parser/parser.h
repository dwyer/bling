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

extern ast$File *parser_parse_file(const char *filename);
extern ast$File **parser_parseDir(const char *path, error$Error **first);

typedef struct {
    token$File *file;
    scanner$Scanner scanner;

    token$Pos pos;
    token$Token tok;
    char *lit;
    ast$Scope *pkg_scope;
    bool c_mode;

    char *pkgName;
} parser_t;

extern void parser_declare(parser_t *p, ast$Scope *s, ast$Decl *decl, ast$ObjKind kind, ast$Expr *name);
extern void parser_next(parser_t *p);
extern void parser_init(parser_t *p, const char *filename, char *src);
extern void parser_error(parser_t *p, token$Pos pos, char *msg);
extern void parser_errorExpected(parser_t *p, token$Pos pos, char *msg);
extern bool accept(parser_t *p, token$Token tok);
extern token$Pos expect(parser_t *p, token$Token tok);
extern ast$Expr *basic_lit(parser_t *p, token$Token kind);
extern ast$Expr *identifier(parser_t *p);
extern ast$Expr *primary_expression(parser_t *p);
extern ast$Decl *parse_pragma(parser_t *p);
