#include "bling/emitter/emitter.h"
#include "bling/token/token.h"

extern char *emitter$Emitter_string(emitter$Emitter *e) {
    return bytes$Buffer_string(&e->buf);
}

static void emitter$emitBytes(emitter$Emitter *e, const char *s, int n) {
    bytes$Buffer_write(&e->buf, s, n, NULL);
}

extern void emitter$emitString(emitter$Emitter *e, const char *s) {
    emitter$emitBytes(e, s, strlen(s));
}

extern void emitter$emitSpace(emitter$Emitter *e) {
    emitter$emitString(e, " ");
}

extern void emitter$emitNewline(emitter$Emitter *e) {
    emitter$emitString(e, "\n");
}

extern void emitter$emitTabs(emitter$Emitter *e) {
    for (int i = 0; i < e->indent; i++) {
        emitter$emitString(e, "    ");
    }
}

extern void emitter$emitToken(emitter$Emitter *e, token$Token tok) {
    if (e->skipSemi && tok == token$SEMICOLON) {
        return;
    }
    emitter$emitString(e, token$string(tok));
}

extern bool hasPrefix(const char *s, const char *prefix) {
    for (int i = 0; prefix[i]; i++) {
        if (s[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

extern void emitter$emitExpr(emitter$Emitter *e, ast$Expr *expr) {
    if (!expr) {
        panic("emitter$emitExpr: expr is NULL");
    }
    switch (expr->kind) {

    case ast$EXPR_BASIC_LIT:
        emitter$emitString(e, expr->basic.value);
        break;

    case ast$EXPR_BINARY:
        emitter$emitExpr(e, expr->binary.x);
        emitter$emitSpace(e);
        emitter$emitToken(e, expr->binary.op);
        emitter$emitSpace(e);
        emitter$emitExpr(e, expr->binary.y);
        break;

    case ast$EXPR_CALL:
        emitter$emitExpr(e, expr->call.func);
        emitter$emitToken(e, token$LPAREN);
        for (ast$Expr **args = expr->call.args; args && *args; ) {
            emitter$emitExpr(e, *args);
            args++;
            if (*args) {
                emitter$emitToken(e, token$COMMA);
                emitter$emitSpace(e);
            }
        }
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_CAST:
        emitter$emitToken(e, token$LT);
        emitter$emitType(e, expr->cast.type);
        emitter$emitToken(e, token$GT);
        emitter$emitSpace(e);
        emitter$emitExpr(e, expr->cast.expr);
        break;

    case ast$EXPR_COMPOSITE_LIT:
        // emitter$emitType(e, expr->composite.type);
        emitter$emitToken(e, token$LBRACE);
        if (expr->composite.list && *expr->composite.list) {
            emitter$emitNewline(e);
            e->indent++;
            for (ast$Expr **exprs = expr->composite.list; *exprs; exprs++) {
                emitter$emitTabs(e);
                emitter$emitExpr(e, *exprs);
                emitter$emitToken(e, token$COMMA);
                emitter$emitNewline(e);
            }
            e->indent--;
            emitter$emitTabs(e);
        }
        emitter$emitToken(e, token$RBRACE);
        break;

    case ast$EXPR_IDENT:
        if (e->pkg
                && hasPrefix(expr->ident.name, e->pkg)
                && expr->ident.name[strlen(e->pkg)] == '$') {
            emitter$emitString(e, &expr->ident.name[strlen(e->pkg)+1]);
        } else {
            int i = bytes$indexByte(expr->ident.name, '$');
            if (i >= 0) {
                emitter$emitBytes(e, expr->ident.name, i);
                emitter$emitToken(e, token$PERIOD);
                emitter$emitString(e, &expr->ident.name[i+1]);
            } else {
                emitter$emitString(e, expr->ident.name);
            }
        }
        break;

    case ast$EXPR_INDEX:
        emitter$emitExpr(e, expr->index.x);
        emitter$emitToken(e, token$LBRACK);
        emitter$emitExpr(e, expr->index.index);
        emitter$emitToken(e, token$RBRACK);
        break;

    case ast$EXPR_KEY_VALUE:
        emitter$emitExpr(e, expr->key_value.key);
        emitter$emitToken(e, token$COLON);
        emitter$emitSpace(e);
        emitter$emitExpr(e, expr->key_value.value);
        break;

    case ast$EXPR_PAREN:
        emitter$emitToken(e, token$LPAREN);
        emitter$emitExpr(e, expr->paren.x);
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_SELECTOR:
        emitter$emitExpr(e, expr->selector.x);
        emitter$emitToken(e, token$PERIOD);
        emitter$emitExpr(e, expr->selector.sel);
        break;

    case ast$EXPR_SIZEOF:
        emitter$emitToken(e, token$SIZEOF);
        emitter$emitToken(e, token$LPAREN);
        emitter$emitType(e, expr->sizeof_.x);
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_STAR:
        emitter$emitToken(e, token$MUL);
        emitter$emitExpr(e, expr->star.x);
        break;

    case ast$EXPR_TERNARY:
        emitter$emitExpr(e, expr->ternary.cond);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$QUESTION_MARK);
        emitter$emitSpace(e);
        emitter$emitExpr(e, expr->ternary.x);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$COLON);
        emitter$emitSpace(e);
        emitter$emitExpr(e, expr->ternary.y);
        break;

    case ast$EXPR_UNARY:
        emitter$emitToken(e, expr->unary.op);
        emitter$emitExpr(e, expr->unary.x);
        break;

    default:
        panic("Unknown expr");
        break;
    }
}

extern void emitter$emitStmt(emitter$Emitter *e, ast$Stmt *stmt) {
    switch (stmt->kind) {

    case ast$STMT_ASSIGN:
        emitter$emitExpr(e, stmt->assign.x);
        emitter$emitSpace(e);
        emitter$emitToken(e, stmt->assign.op);
        emitter$emitSpace(e);
        emitter$emitExpr(e, stmt->assign.y);
        break;

    case ast$STMT_BLOCK:
        emitter$emitToken(e, token$LBRACE);
        emitter$emitNewline(e);
        e->indent++;
        for (ast$Stmt **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->kind) {
            case ast$STMT_LABEL:
                break;
            default:
                emitter$emitTabs(e);
                break;
            }
            emitter$emitStmt(e, *stmts);
            emitter$emitNewline(e);
        }
        e->indent--;
        emitter$emitTabs(e);
        emitter$emitToken(e, token$RBRACE);
        break;

    case ast$STMT_CASE:
        if (stmt->case_.exprs && *stmt->case_.exprs) {
            emitter$emitToken(e, token$CASE);
            emitter$emitSpace(e);
            for (int i = 0; stmt->case_.exprs[i]; i++) {
                if (i > 0) {
                    emitter$emitToken(e, token$COMMA);
                    emitter$emitSpace(e);
                }
                emitter$emitExpr(e, stmt->case_.exprs[i]);
            }
        } else {
            emitter$emitToken(e, token$DEFAULT);
        }
        emitter$emitToken(e, token$COLON);
        if (stmt->case_.stmts) {
            ast$Stmt *first = stmt->case_.stmts[0];
            bool oneline = false;
            if (first != NULL) {
                switch (first->kind) {
                case ast$STMT_BLOCK:
                case ast$STMT_JUMP:
                case ast$STMT_RETURN:
                    oneline = stmt->case_.stmts[1] == NULL;
                    break;
                default:
                    break;
                }
            }
            if (oneline) {
                emitter$emitSpace(e);
                emitter$emitStmt(e, first);
                emitter$emitNewline(e);
            } else {
                emitter$emitNewline(e);
                e->indent++;
                for (int i = 0; stmt->case_.stmts[i]; i++) {
                    emitter$emitTabs(e);
                    emitter$emitStmt(e, stmt->case_.stmts[i]);
                    emitter$emitNewline(e);
                }
                e->indent--;
            }
        }
        break;

    case ast$STMT_DECL:
        emitter$emitDecl(e, stmt->decl.decl);
        break;

    case ast$STMT_EMPTY:
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_EXPR:
        emitter$emitExpr(e, stmt->expr.x);
        break;

    case ast$STMT_IF:
        emitter$emitToken(e, token$IF);
        emitter$emitSpace(e);
        emitter$emitExpr(e, stmt->if_.cond);
        emitter$emitSpace(e);
        emitter$emitStmt(e, stmt->if_.body);
        if (stmt->if_.else_) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$ELSE);
            emitter$emitSpace(e);
            emitter$emitStmt(e, stmt->if_.else_);
        }
        break;

    case ast$STMT_ITER:
        emitter$emitToken(e, stmt->iter.kind);
        if (stmt->iter.kind == token$FOR) {
            emitter$emitSpace(e);
            if (stmt->iter.init) {
                emitter$emitStmt(e, stmt->iter.init);
            }
            emitter$emitToken(e, token$SEMICOLON);
        }
        if (stmt->iter.cond) {
            emitter$emitSpace(e);
            emitter$emitExpr(e, stmt->iter.cond);
        }
        if (stmt->iter.kind == token$FOR) {
            emitter$emitToken(e, token$SEMICOLON);
            if (stmt->iter.post) {
                emitter$emitSpace(e);
                e->skipSemi = true;
                emitter$emitStmt(e, stmt->iter.post);
                e->skipSemi = false;
            }
        }
        emitter$emitSpace(e);
        emitter$emitStmt(e, stmt->iter.body);
        break;

    case ast$STMT_JUMP:
        emitter$emitToken(e, stmt->jump.keyword);
        if (stmt->jump.label) {
            emitter$emitSpace(e);
            emitter$emitExpr(e, stmt->jump.label);
        }
        break;

    case ast$STMT_LABEL:
        emitter$emitExpr(e, stmt->label.label);
        emitter$emitToken(e, token$COLON);
        emitter$emitNewline(e);
        emitter$emitTabs(e);
        emitter$emitStmt(e, stmt->label.stmt);
        break;

    case ast$STMT_POSTFIX:
        emitter$emitExpr(e, stmt->postfix.x);
        emitter$emitToken(e, stmt->postfix.op);
        break;

    case ast$STMT_RETURN:
        emitter$emitToken(e, token$RETURN);
        if (stmt->return_.x) {
            emitter$emitSpace(e);
            emitter$emitExpr(e, stmt->return_.x);
        }
        break;

    case ast$STMT_SWITCH:
        emitter$emitToken(e, token$SWITCH);
        emitter$emitSpace(e);
        emitter$emitExpr(e, stmt->switch_.tag);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$LBRACE);
        emitter$emitNewline(e);
        for (ast$Stmt **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emitter$emitTabs(e);
            emitter$emitStmt(e, *stmts);
        }
        emitter$emitTabs(e);
        emitter$emitToken(e, token$RBRACE);
        break;

    default:
        panic("Unknown stmt");
        break;
    }
}

static bool is_void(ast$Expr *type) {
    return type == NULL || (type->kind == ast$EXPR_IDENT && streq(type->ident.name, "void"));
}

extern void emitter$emitType(emitter$Emitter *e, ast$Expr *type) {
    if (type->is_const) {
        emitter$emitToken(e, token$CONST);
        emitter$emitSpace(e);
    }
    switch (type->kind) {
    case ast$EXPR_IDENT:
    case ast$EXPR_SELECTOR:
        emitter$emitExpr(e, type);
        break;

    case ast$TYPE_ARRAY:
        emitter$emitToken(e, token$LBRACK);
        if (type->array.len) {
            emitter$emitExpr(e, type->array.len);
        }
        emitter$emitToken(e, token$RBRACK);
        emitter$emitType(e, type->array.elt);
        break;

    case ast$TYPE_FUNC:
        emitter$emitToken(e, token$LPAREN);
        for (ast$Decl **params = type->func.params; params && *params; ) {
            emitter$emitDecl(e, *params);
            params++;
            if (*params != NULL) {
                emitter$emitToken(e, token$COMMA);
                emitter$emitSpace(e);
            }
        }
        emitter$emitToken(e, token$RPAREN);
        if (!is_void(type->func.result)) {
            emitter$emitSpace(e);
            emitter$emitType(e, type->func.result);
        }
        break;

    case ast$TYPE_ENUM:
        emitter$emitToken(e, token$ENUM);
        if (type->enum_.name) {
            emitter$emitSpace(e);
            emitter$emitExpr(e, type->enum_.name);
        }
        if (type->enum_.enums) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$LBRACE);
            emitter$emitNewline(e);
            e->indent++;
            for (ast$Decl **enums = type->enum_.enums; enums && *enums; enums++) {
                ast$Decl *enumerator = *enums;
                emitter$emitTabs(e);
                emitter$emitExpr(e, enumerator->value.name);
                if (enumerator->value.value) {
                    emitter$emitSpace(e);
                    emitter$emitToken(e, token$ASSIGN);
                    emitter$emitSpace(e);
                    emitter$emitExpr(e, enumerator->value.value);
                }
                emitter$emitToken(e, token$COMMA);
                emitter$emitNewline(e);
            }
            e->indent--;
            emitter$emitTabs(e);
            emitter$emitToken(e, token$RBRACE);
        }
        break;

    case ast$EXPR_STAR:
        type = type->star.x;
        if (type->kind == ast$TYPE_FUNC) {
            emitter$emitToken(e, token$FUNC);
            emitter$emitToken(e, token$LPAREN);
            for (ast$Decl **params = type->func.params; params && *params; ) {
                emitter$emitDecl(e, *params);
                params++;
                if (*params != NULL) {
                    emitter$emitToken(e, token$COMMA);
                    emitter$emitSpace(e);
                }
            }
            emitter$emitToken(e, token$RPAREN);
            if (!is_void(type->func.result)) {
                emitter$emitSpace(e);
                emitter$emitType(e, type->func.result);
            }
        } else {
            emitter$emitToken(e, token$MUL);
            emitter$emitType(e, type);
        }
        break;

    case ast$TYPE_STRUCT:
        emitter$emitToken(e, type->struct_.tok);
        if (type->struct_.name) {
            emitter$emitSpace(e);
            emitter$emitExpr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$LBRACE);
            emitter$emitNewline(e);
            e->indent++;
            for (ast$Decl **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emitter$emitTabs(e);
                emitter$emitDecl(e, *fields);
                emitter$emitNewline(e);
            }
            e->indent--;
            emitter$emitTabs(e);
            emitter$emitToken(e, token$RBRACE);
        }
        break;

    default:
        panic("Unknown type: %d", type->kind);
    }
}

extern void emitter$emitDecl(emitter$Emitter *e, ast$Decl *decl) {
    switch (decl->kind) {

    case ast$DECL_ELLIPSIS:
        emitter$emitToken(e, token$ELLIPSIS);
        break;

    case ast$DECL_FIELD:
        if (decl->field.name != NULL) {
            emitter$emitExpr(e, decl->field.name);
            emitter$emitSpace(e);
        }
        emitter$emitType(e, decl->field.type);
        break;

    case ast$DECL_FUNC:
        emitter$emitToken(e, token$FUNC);
        emitter$emitSpace(e);
        emitter$emitExpr(e, decl->func.name);
        emitter$emitType(e, decl->func.type);
        if (decl->func.body) {
            emitter$emitSpace(e);
            emitter$emitStmt(e, decl->func.body);
        }
        break;

    case ast$DECL_IMPORT:
        emitter$emitToken(e, token$IMPORT);
        emitter$emitSpace(e);
        emitter$emitExpr(e, decl->imp.path);
        break;

    case ast$DECL_TYPEDEF:
        emitter$emitToken(e, token$TYPEDEF);
        emitter$emitSpace(e);
        emitter$emitExpr(e, decl->typedef_.name);
        emitter$emitSpace(e);
        emitter$emitType(e, decl->typedef_.type);
        break;

    case ast$DECL_PRAGMA:
        emitter$emitToken(e, token$HASH);
        emitter$emitString(e, decl->pragma.lit);
        break;

    case ast$DECL_VALUE:
        switch (decl->value.kind) {
        case token$VAR:
            emitter$emitToken(e, token$VAR);
            if (decl->value.name) {
                emitter$emitSpace(e);
                emitter$emitExpr(e, decl->value.name);
            }
            // TODO:
            // if (decl->value.value == NULL
            //         || ast$isNil(decl->value.value)
            //         || ast$isVoidPtr(decl->value.value)
            //         || decl->value.value->type == ast$EXPR_COMPOUND) {
            emitter$emitSpace(e);
            emitter$emitType(e, decl->value.type);
            // }
            if (decl->value.value) {
                emitter$emitSpace(e);
                emitter$emitToken(e, token$ASSIGN);
                emitter$emitSpace(e);
                emitter$emitExpr(e, decl->value.value);
            }
            break;
        case token$CONST:
            emitter$emitToken(e, token$HASH);
            emitter$emitString(e, "define");
            emitter$emitSpace(e);
            emitter$emitExpr(e, decl->value.value);
        default:
            panic("bad kind for ast$DECL_VALUE: %s",
                    token$string(decl->value.kind));
            break;
        }
        break;

    default:
        panic("Unknown decl");
        break;
    }
}

extern void emitter$emitFile(emitter$Emitter *e, ast$File *file) {
    if (file->name) {
        e->pkg = file->name->ident.name;
    }
    emitter$emitString(e, "//");
    emitter$emitString(e, file->filename);
    emitter$emitNewline(e);
    if (file->name != NULL) {
        emitter$emitToken(e, token$PACKAGE);
        emitter$emitSpace(e);
        emitter$emitExpr(e, file->name);
        emitter$emitNewline(e);
    }
    for (ast$Decl **imports = file->imports; imports && *imports; imports++) {
        emitter$emitToken(e, token$IMPORT);
        emitter$emitSpace(e);
        emitter$emitExpr(e, (*imports)->imp.path);
        emitter$emitNewline(e);
    }
    for (ast$Decl **decls = file->decls; decls && *decls; decls++) {
        emitter$emitNewline(e);
        emitter$emitDecl(e, *decls);
        emitter$emitNewline(e);
    }
}
