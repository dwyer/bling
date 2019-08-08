#pragma once
#include "bling/ast/ast.h"
#include "bytes/bytes.h"

package(emitter);

import("bling/ast");
import("bytes");

typedef struct {
    bytes$Buffer buf;
    int indent;
    bool skipSemi;
} emitter$Emitter;

extern char *emitter$Emitter_string(emitter$Emitter *e);

extern void emitter$emitString(emitter$Emitter *e, const char *s);
extern void emitter$emitSpace(emitter$Emitter *e);
extern void emitter$emitNewline(emitter$Emitter *e);
extern void emitter$emitTabs(emitter$Emitter *e);
extern void emitter$emitToken(emitter$Emitter *e, token$Token tok);

extern void emitter$emitDecl(emitter$Emitter *e, ast$Decl *decl);
extern void emitter$emitExpr(emitter$Emitter *e, ast$Expr *expr);
extern void emitter$emitStmt(emitter$Emitter *e, ast$Stmt *stmt);
extern void emitter$emitType(emitter$Emitter *e, ast$Expr *type);
extern void emitter$emitFile(emitter$Emitter *e, ast$File *file);
