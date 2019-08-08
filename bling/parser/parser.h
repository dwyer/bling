#pragma once
#include "io/ioutil/ioutil.h"
#include "bling/ast/ast.h"
#include "bling/scanner/scanner.h"
#include "bling/token/token.h"

package(parser);

import("bling/ast");
import("bling/scanner");
import("bling/token");
import("bytes");
import("fmt");
import("io/ioutil");
import("path");

extern ast$File *parser$parse_file(const char *filename);
extern ast$File **parser$parseDir(const char *path, error$Error **first);

typedef struct {
    token$File *file;
    scanner$Scanner scanner;

    token$Pos pos;
    token$Token tok;
    char *lit;
    ast$Scope *pkg_scope;
    bool c_mode;

    char *pkgName;
} parser$t;

extern void parser$declare(parser$t *p, ast$Scope *s, ast$Decl *decl,
        ast$ObjKind kind, ast$Expr *name);
extern void parser$next(parser$t *p);
extern void parser$init(parser$t *p, const char *filename, char *src);
extern void parser$error(parser$t *p, token$Pos pos, char *msg);
extern void parser$errorExpected(parser$t *p, token$Pos pos, char *msg);
extern bool parser$accept(parser$t *p, token$Token tok);
extern token$Pos parser$expect(parser$t *p, token$Token tok);
extern ast$Expr *parser$parseBasicLit(parser$t *p, token$Token kind);
extern ast$Expr *parser$parseIdent(parser$t *p);
extern ast$Expr *parser$parsePrimaryExpr(parser$t *p);
extern ast$Decl *parser$parsePragma(parser$t *p);
