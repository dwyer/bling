#include "subc/cemitter/cemitter.h"
#include "bling/token/token.h"

static void cemitter$emitDecl(emitter$Emitter *e, ast$Decl *decl);
static void cemitter$emitExpr(emitter$Emitter *e, ast$Expr *expr);
static void cemitter$emitType(emitter$Emitter *e, ast$Expr *type, ast$Expr *name);

static void cemitter$emitExpr(emitter$Emitter *e, ast$Expr *expr) {
    if (!expr) {
        panic("cemitter$emitExpr: expr is NULL");
    }
    switch (expr->type) {

    case ast$EXPR_BASIC_LIT:
        emitter$emitString(e, expr->basic_lit.value);
        break;

    case ast$EXPR_BINARY:
        cemitter$emitExpr(e, expr->binary.x);
        emitter$emitSpace(e);
        emitter$emitToken(e, expr->binary.op);
        emitter$emitSpace(e);
        cemitter$emitExpr(e, expr->binary.y);
        break;

    case ast$EXPR_CALL:
        cemitter$emitExpr(e, expr->call.func);
        emitter$emitToken(e, token$LPAREN);
        for (ast$Expr **args = expr->call.args; args && *args; ) {
            cemitter$emitExpr(e, *args);
            args++;
            if (*args) {
                emitter$emitToken(e, token$COMMA);
                emitter$emitSpace(e);
            }
        }
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_CAST:
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitType(e, expr->cast.type, NULL);
        emitter$emitToken(e, token$RPAREN);
        cemitter$emitExpr(e, expr->cast.expr);
        break;

    case ast$EXPR_COND:
        cemitter$emitExpr(e, expr->conditional.condition);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$QUESTION_MARK);
        emitter$emitSpace(e);
        cemitter$emitExpr(e, expr->conditional.consequence);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$COLON);
        emitter$emitSpace(e);
        cemitter$emitExpr(e, expr->conditional.alternative);
        break;

    case ast$EXPR_COMPOUND:
        emitter$emitToken(e, token$LBRACE);
        emitter$emitNewline(e);
        e->indent++;
        for (ast$Expr **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emitter$emitTabs(e);
            cemitter$emitExpr(e, *exprs);
            emitter$emitToken(e, token$COMMA);
            emitter$emitNewline(e);
        }
        e->indent--;
        emitter$emitTabs(e);
        emitter$emitToken(e, token$RBRACE);
        break;

    case ast$EXPR_IDENT:
        if (expr->ident.pkg) {
            emitter$emitExpr(e, expr->ident.pkg);
            emitter$emitToken(e, token$DOLLAR);
        } else {
            ast$Object *obj = expr->ident.obj;
            if (obj) {
                ast$Scope *scope = obj->scope;
                if (scope) {
                    char *pkg = scope->pkg;
                    if (pkg && !streq(pkg, "main")) {
                        emitter$emitString(e, pkg);
                        emitter$emitToken(e, token$DOLLAR);
                    }
                }
            }
        }
        emitter$emitString(e, expr->ident.name);
        break;

    case ast$EXPR_INDEX:
        cemitter$emitExpr(e, expr->index.x);
        emitter$emitToken(e, token$LBRACK);
        cemitter$emitExpr(e, expr->index.index);
        emitter$emitToken(e, token$RBRACK);
        break;

    case ast$EXPR_KEY_VALUE:
        if (expr->key_value.isArray) {
            emitter$emitToken(e, token$LBRACK);
            cemitter$emitExpr(e, expr->key_value.key);
            emitter$emitToken(e, token$RBRACK);
        } else {
            emitter$emitToken(e, token$PERIOD);
            cemitter$emitExpr(e, expr->key_value.key);
        }
        emitter$emitSpace(e);
        emitter$emitToken(e, token$ASSIGN);
        emitter$emitSpace(e);
        cemitter$emitExpr(e, expr->key_value.value);
        break;

    case ast$EXPR_PAREN:
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitExpr(e, expr->paren.x);
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_SELECTOR:
        cemitter$emitExpr(e, expr->selector.x);
        emitter$emitToken(e, expr->selector.tok);
        cemitter$emitExpr(e, expr->selector.sel);
        break;

    case ast$EXPR_SIZEOF:
        emitter$emitToken(e, token$SIZEOF);
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitType(e, expr->sizeof_.x, NULL);
        emitter$emitToken(e, token$RPAREN);
        break;

    case ast$EXPR_STAR:
        emitter$emitToken(e, token$MUL);
        cemitter$emitExpr(e, expr->star.x);
        break;

    case ast$EXPR_UNARY:
        emitter$emitToken(e, expr->unary.op);
        cemitter$emitExpr(e, expr->unary.x);
        break;

    default:
        panic("Unknown expr: %d", expr->type);
        break;
    }
}

static void cemitter$emitStmt(emitter$Emitter *e, ast$Stmt *stmt) {
    switch (stmt->type) {

    case ast$STMT_ASSIGN:
        cemitter$emitExpr(e, stmt->assign.x);
        emitter$emitSpace(e);
        emitter$emitToken(e, stmt->assign.op);
        emitter$emitSpace(e);
        cemitter$emitExpr(e, stmt->assign.y);
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_BLOCK:
        emitter$emitToken(e, token$LBRACE);
        emitter$emitNewline(e);
        e->indent++;
        for (ast$Stmt **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast$STMT_LABEL:
                break;
            default:
                emitter$emitTabs(e);
                break;
            }
            cemitter$emitStmt(e, *stmts);
            emitter$emitNewline(e);
        }
        e->indent--;
        emitter$emitTabs(e);
        emitter$emitToken(e, token$RBRACE);
        break;

    case ast$STMT_CASE:
        if (stmt->case_.exprs && *stmt->case_.exprs) {
            for (int i = 0; stmt->case_.exprs[i]; i++) {
                if (i > 0) {
                    emitter$emitToken(e, token$COLON);
                    emitter$emitNewline(e);
                    emitter$emitTabs(e);
                }
                emitter$emitToken(e, token$CASE);
                emitter$emitSpace(e);
                cemitter$emitExpr(e, stmt->case_.exprs[i]);
            }
        } else {
            emitter$emitToken(e, token$DEFAULT);
        }
        emitter$emitToken(e, token$COLON);
        emitter$emitNewline(e);
        e->indent++;
        for (ast$Stmt **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            emitter$emitTabs(e);
            cemitter$emitStmt(e, *stmts);
            emitter$emitNewline(e);
        }
        e->indent--;
        break;

    case ast$STMT_DECL:
        cemitter$emitDecl(e, stmt->decl);
        break;

    case ast$STMT_EMPTY:
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_EXPR:
        cemitter$emitExpr(e, stmt->expr.x);
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_IF:
        emitter$emitToken(e, token$IF);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitExpr(e, stmt->if_.cond);
        emitter$emitToken(e, token$RPAREN);
        emitter$emitSpace(e);
        cemitter$emitStmt(e, stmt->if_.body);
        if (stmt->if_.else_) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$ELSE);
            emitter$emitSpace(e);
            cemitter$emitStmt(e, stmt->if_.else_);
        }
        break;

    case ast$STMT_ITER:
        emitter$emitToken(e, stmt->iter.kind);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$LPAREN);
        if (stmt->iter.kind == token$FOR) {
            if (stmt->iter.init) {
                cemitter$emitStmt(e, stmt->iter.init);
                emitter$emitSpace(e);
            } else {
                emitter$emitToken(e, token$SEMICOLON);
                emitter$emitSpace(e);
            }
        }
        if (stmt->iter.cond) {
            cemitter$emitExpr(e, stmt->iter.cond);
        }
        if (stmt->iter.kind == token$FOR) {
            emitter$emitToken(e, token$SEMICOLON);
            emitter$emitSpace(e);
            if (stmt->iter.post) {
                e->skipSemi = true;
                cemitter$emitStmt(e, stmt->iter.post);
                e->skipSemi = false;
            }
        }
        emitter$emitToken(e, token$RPAREN);
        emitter$emitSpace(e);
        cemitter$emitStmt(e, stmt->iter.body);
        break;

    case ast$STMT_JUMP:
        emitter$emitToken(e, stmt->jump.keyword);
        if (stmt->jump.label) {
            emitter$emitSpace(e);
            cemitter$emitExpr(e, stmt->jump.label);
        }
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_LABEL:
        cemitter$emitExpr(e, stmt->label.label);
        emitter$emitToken(e, token$COLON);
        emitter$emitNewline(e);
        emitter$emitTabs(e);
        cemitter$emitStmt(e, stmt->label.stmt);
        break;

    case ast$STMT_POSTFIX:
        cemitter$emitExpr(e, stmt->postfix.x);
        emitter$emitToken(e, stmt->postfix.op);
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_RETURN:
        emitter$emitToken(e, token$RETURN);
        if (stmt->return_.x) {
            emitter$emitSpace(e);
            cemitter$emitExpr(e, stmt->return_.x);
        }
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$STMT_SWITCH:
        emitter$emitToken(e, token$SWITCH);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitExpr(e, stmt->switch_.tag);
        emitter$emitToken(e, token$RPAREN);
        emitter$emitSpace(e);
        emitter$emitToken(e, token$LBRACE);
        emitter$emitNewline(e);
        for (ast$Stmt **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emitter$emitTabs(e);
            cemitter$emitStmt(e, *stmts);
        }
        emitter$emitTabs(e);
        emitter$emitToken(e, token$RBRACE);
        break;

    default:
        panic("Unknown stmt");
        break;
    }
}

static void cemitter$emitType(emitter$Emitter *e, ast$Expr *type, ast$Expr *name) {
    if (type == NULL) {
        panic("cemitter$emitType: type is nil");
    }
    if (type->is_const && type->type != ast$EXPR_STAR) {
        emitter$emitToken(e, token$CONST);
        emitter$emitSpace(e);
    }
    switch (type->type) {
    case ast$TYPE_ARRAY:
        cemitter$emitType(e, type->array.elt, name);
        emitter$emitToken(e, token$LBRACK);
        if (type->array.len) {
            cemitter$emitExpr(e, type->array.len);
        }
        emitter$emitToken(e, token$RBRACK);
        name = NULL;
        break;

    case ast$TYPE_FUNC:
        if (type->func.result != NULL) {
            cemitter$emitType(e, type->func.result, name);
        } else {
            emitter$emitString(e, "void");
            emitter$emitSpace(e);
            cemitter$emitExpr(e, name);
        }
        emitter$emitToken(e, token$LPAREN);
        for (ast$Decl **params = type->func.params; params && *params; ) {
            cemitter$emitDecl(e, *params);
            params++;
            if (*params != NULL) {
                emitter$emitToken(e, token$COMMA);
                emitter$emitSpace(e);
            }
        }
        emitter$emitToken(e, token$RPAREN);
        name = NULL;
        break;

    case ast$TYPE_ENUM:
        emitter$emitToken(e, token$ENUM);
        if (type->enum_.name) {
            emitter$emitSpace(e);
            cemitter$emitExpr(e, type->enum_.name);
        }
        if (type->enum_.enums) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$LBRACE);
            emitter$emitNewline(e);
            e->indent++;
            for (ast$Decl **enums = type->enum_.enums; enums && *enums; enums++) {
                ast$Decl *decl = *enums;
                emitter$emitTabs(e);
                cemitter$emitExpr(e, decl->value.name);
                if (decl->value.value) {
                    emitter$emitSpace(e);
                    emitter$emitToken(e, token$ASSIGN);
                    emitter$emitSpace(e);
                    cemitter$emitExpr(e, decl->value.value);
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
        if (type->type == ast$TYPE_FUNC) {
            cemitter$emitType(e, type->func.result, NULL);
            emitter$emitToken(e, token$LPAREN);
            emitter$emitToken(e, token$MUL);
            if (name != NULL) {
                cemitter$emitExpr(e, name);
            }
            emitter$emitToken(e, token$RPAREN);
            emitter$emitToken(e, token$LPAREN);
            for (ast$Decl **params = type->func.params; params && *params; ) {
                cemitter$emitDecl(e, *params);
                params++;
                if (*params != NULL) {
                    emitter$emitToken(e, token$COMMA);
                    emitter$emitSpace(e);
                }
            }
            emitter$emitToken(e, token$RPAREN);
            name = NULL;
        } else {
            cemitter$emitType(e, type, NULL);
            emitter$emitToken(e, token$MUL);
        }
        break;

    case ast$TYPE_STRUCT:
        emitter$emitToken(e, type->struct_.tok);
        if (type->struct_.name) {
            emitter$emitSpace(e);
            cemitter$emitExpr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$LBRACE);
            emitter$emitNewline(e);
            e->indent++;
            for (ast$Decl **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emitter$emitTabs(e);
                cemitter$emitDecl(e, *fields);
                emitter$emitToken(e, token$SEMICOLON);
                emitter$emitNewline(e);
            }
            e->indent--;
            emitter$emitTabs(e);
            emitter$emitToken(e, token$RBRACE);
        }
        break;

    case ast$EXPR_IDENT:
        cemitter$emitExpr(e, type);
        break;

    default:
        panic("Unknown type: %d", type->type);
    }

    if (type->is_const && type->type == ast$EXPR_STAR) {
        emitter$emitSpace(e);
        emitter$emitToken(e, token$CONST);
    }

    if (name) {
        emitter$emitSpace(e);
        cemitter$emitExpr(e, name);
    }
}

static void cemitter$emitDecl(emitter$Emitter *e, ast$Decl *decl) {
    switch (decl->type) {

    case ast$DECL_FIELD:
        if (decl->field.type == NULL && decl->field.name == NULL) {
            emitter$emitString(e, "...");
        } else {
            cemitter$emitType(e, decl->field.type, decl->field.name);
        }
        break;

    case ast$DECL_FUNC:
        cemitter$emitType(e, decl->func.type, decl->func.name);
        if (decl->func.body) {
            emitter$emitSpace(e);
            cemitter$emitStmt(e, decl->func.body);
        } else {
            emitter$emitToken(e, token$SEMICOLON);
        }
        break;

    case ast$DECL_PRAGMA:
        emitter$emitString(e, "//");
        emitter$emitToken(e, token$HASH);
        emitter$emitString(e, decl->pragma.lit);
        break;

    case ast$DECL_TYPEDEF:
        emitter$emitToken(e, token$TYPEDEF);
        emitter$emitSpace(e);
        cemitter$emitType(e, decl->typedef_.type, decl->typedef_.name);
        emitter$emitToken(e, token$SEMICOLON);
        break;

    case ast$DECL_VALUE:
        cemitter$emitType(e, decl->value.type, decl->value.name);
        if (decl->value.value) {
            emitter$emitSpace(e);
            emitter$emitToken(e, token$ASSIGN);
            emitter$emitSpace(e);
            cemitter$emitExpr(e, decl->value.value);
        }
        emitter$emitToken(e, token$SEMICOLON);
        break;

    default:
        panic("Unknown decl");
        break;
    }
}

extern void cemitter$emitFile(emitter$Emitter *e, ast$File *file) {
    emitter$emitString(e, "//");
    emitter$emitString(e, file->filename);
    emitter$emitNewline(e);
    emitter$emitNewline(e);
    if (file->name != NULL) {
        emitter$emitToken(e, token$PACKAGE);
        emitter$emitToken(e, token$LPAREN);
        cemitter$emitExpr(e, file->name);
        emitter$emitToken(e, token$RPAREN);
        emitter$emitToken(e, token$SEMICOLON);
        emitter$emitNewline(e);
        emitter$emitNewline(e);
    }
    for (ast$Decl **decls = file->decls; decls && *decls; decls++) {
        cemitter$emitDecl(e, *decls);
        emitter$emitNewline(e);
        emitter$emitNewline(e);
    }
}
