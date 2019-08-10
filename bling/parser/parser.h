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
import("io/ioutil");
import("os");
import("paths");
import("utils");
import("sys");

extern ast$File *parser$parse_file(const char *filename);
extern ast$File **parser$parseDir(const char *path, utils$Error **first);

typedef struct {
    token$File *file;
    scanner$Scanner scanner;

    token$Pos pos;
    token$Token tok;
    char *lit;
    ast$Scope *pkg_scope;
    bool c_mode;

    char *pkgName;
} parser$Parser;

extern void parser$declare(parser$Parser *p, ast$Scope *s, ast$Decl *decl,
        ast$ObjKind kind, ast$Expr *name);
extern void parser$next(parser$Parser *p);
extern void parser$init(parser$Parser *p, const char *filename, char *src);
extern void parser$error(parser$Parser *p, token$Pos pos, char *msg);
extern void parser$errorExpected(parser$Parser *p, token$Pos pos, char *msg);
extern bool parser$accept(parser$Parser *p, token$Token tok);
extern token$Pos parser$expect(parser$Parser *p, token$Token tok);
extern ast$Expr *parser$parseBasicLit(parser$Parser *p, token$Token kind);
extern ast$Expr *parser$parseIdent(parser$Parser *p);
extern ast$Expr *parser$parsePrimaryExpr(parser$Parser *p);
extern ast$Decl *parser$parsePragma(parser$Parser *p);
