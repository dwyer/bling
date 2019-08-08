#include "bling/emitter/emitter.h"
#include "bling/token/token.h"

extern char *emitter_string(emitter_t *e) {
    return bytes$Buffer_string(&e->buf);
}

extern void emit_string(emitter_t *e, const char *s) {
    bytes$Buffer_write(&e->buf, s, strlen(s), NULL);
}

extern void emit_space(emitter_t *e) {
    emit_string(e, " ");
}

extern void emit_newline(emitter_t *e) {
    emit_string(e, "\n");
}

extern void emit_tabs(emitter_t *e) {
    for (int i = 0; i < e->indent; i++) {
        emit_string(e, "    ");
    }
}

extern void emit_token(emitter_t *e, token$Token tok) {
    if (e->skipSemi && tok == token$SEMICOLON) {
        return;
    }
    emit_string(e, token$string(tok));
}

extern void print_expr(emitter_t *p, ast$Expr *expr) {
    if (!expr) {
        panic("print_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast$EXPR_BASIC_LIT:
        emit_string(p, expr->basic_lit.value);
        break;

    case ast$EXPR_BINARY:
        print_expr(p, expr->binary.x);
        emit_space(p);
        emit_token(p, expr->binary.op);
        emit_space(p);
        print_expr(p, expr->binary.y);
        break;

    case ast$EXPR_CALL:
        print_expr(p, expr->call.func);
        emit_token(p, token$LPAREN);
        for (ast$Expr **args = expr->call.args; args && *args; ) {
            print_expr(p, *args);
            args++;
            if (*args) {
                emit_token(p, token$COMMA);
                emit_space(p);
            }
        }
        emit_token(p, token$RPAREN);
        break;

    case ast$EXPR_CAST:
        emit_token(p, token$LT);
        print_type(p, expr->cast.type);
        emit_token(p, token$GT);
        emit_space(p);
        print_expr(p, expr->cast.expr);
        break;

    case ast$EXPR_COMPOUND:
        emit_token(p, token$LBRACE);
        emit_newline(p);
        p->indent++;
        for (ast$Expr **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emit_tabs(p);
            print_expr(p, *exprs);
            emit_token(p, token$COMMA);
            emit_newline(p);
        }
        p->indent--;
        emit_tabs(p);
        emit_token(p, token$RBRACE);
        break;

    case ast$EXPR_COND:
        print_expr(p, expr->conditional.condition);
        emit_space(p);
        emit_token(p, token$QUESTION_MARK);
        emit_space(p);
        print_expr(p, expr->conditional.consequence);
        emit_space(p);
        emit_token(p, token$COLON);
        emit_space(p);
        print_expr(p, expr->conditional.alternative);
        break;

    case ast$EXPR_IDENT:
        emit_string(p, expr->ident.name);
        break;

    case ast$EXPR_INDEX:
        print_expr(p, expr->index.x);
        emit_token(p, token$LBRACK);
        print_expr(p, expr->index.index);
        emit_token(p, token$RBRACK);
        break;

    case ast$EXPR_KEY_VALUE:
        print_expr(p, expr->key_value.key);
        emit_token(p, token$COLON);
        emit_space(p);
        print_expr(p, expr->key_value.value);
        break;

    case ast$EXPR_PAREN:
        emit_token(p, token$LPAREN);
        print_expr(p, expr->paren.x);
        emit_token(p, token$RPAREN);
        break;

    case ast$EXPR_SELECTOR:
        print_expr(p, expr->selector.x);
        emit_token(p, token$PERIOD);
        print_expr(p, expr->selector.sel);
        break;

    case ast$EXPR_SIZEOF:
        emit_token(p, token$SIZEOF);
        emit_token(p, token$LPAREN);
        print_type(p, expr->sizeof_.x);
        emit_token(p, token$RPAREN);
        break;

    case ast$EXPR_STAR:
        emit_token(p, token$MUL);
        print_expr(p, expr->star.x);
        break;

    case ast$EXPR_UNARY:
        emit_token(p, expr->unary.op);
        print_expr(p, expr->unary.x);
        break;

    default:
        panic("Unknown expr");
        break;
    }
}

extern void print_stmt(emitter_t *p, ast$Stmt *stmt) {
    switch (stmt->type) {

    case ast$STMT_ASSIGN:
        print_expr(p, stmt->assign.x);
        emit_space(p);
        emit_token(p, stmt->assign.op);
        emit_space(p);
        print_expr(p, stmt->assign.y);
        break;

    case ast$STMT_BLOCK:
        emit_token(p, token$LBRACE);
        emit_newline(p);
        p->indent++;
        for (ast$Stmt **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast$STMT_LABEL:
                break;
            default:
                emit_tabs(p);
                break;
            }
            print_stmt(p, *stmts);
            emit_newline(p);
        }
        p->indent--;
        emit_tabs(p);
        emit_token(p, token$RBRACE);
        break;

    case ast$STMT_CASE:
        if (stmt->case_.exprs && *stmt->case_.exprs) {
            emit_token(p, token$CASE);
            emit_space(p);
            for (int i = 0; stmt->case_.exprs[i]; i++) {
                if (i > 0) {
                    emit_token(p, token$COMMA);
                    emit_space(p);
                }
                print_expr(p, stmt->case_.exprs[i]);
            }
        } else {
            emit_token(p, token$DEFAULT);
        }
        emit_token(p, token$COLON);
        emit_newline(p);
        p->indent++;
        for (ast$Stmt **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(p);
            print_stmt(p, *stmts);
            emit_newline(p);
        }
        p->indent--;
        break;

    case ast$STMT_DECL:
        print_decl(p, stmt->decl);
        break;

    case ast$STMT_EMPTY:
        emit_token(p, token$SEMICOLON);
        break;

    case ast$STMT_EXPR:
        print_expr(p, stmt->expr.x);
        break;

    case ast$STMT_IF:
        emit_token(p, token$IF);
        emit_space(p);
        print_expr(p, stmt->if_.cond);
        emit_space(p);
        print_stmt(p, stmt->if_.body);
        if (stmt->if_.else_) {
            emit_space(p);
            emit_token(p, token$ELSE);
            emit_space(p);
            print_stmt(p, stmt->if_.else_);
        }
        break;

    case ast$STMT_ITER:
        emit_token(p, stmt->iter.kind);
        emit_space(p);
        if (stmt->iter.kind == token$FOR) {
            if (stmt->iter.init) {
                print_stmt(p, stmt->iter.init);
            }
            emit_token(p, token$SEMICOLON);
            emit_space(p);
        }
        if (stmt->iter.cond) {
            print_expr(p, stmt->iter.cond);
        }
        if (stmt->iter.kind == token$FOR) {
            emit_token(p, token$SEMICOLON);
            emit_space(p);
            if (stmt->iter.post) {
                p->skipSemi = true;
                print_stmt(p, stmt->iter.post);
                p->skipSemi = false;
            }
        }
        emit_space(p);
        print_stmt(p, stmt->iter.body);
        break;

    case ast$STMT_JUMP:
        emit_token(p, stmt->jump.keyword);
        if (stmt->jump.label) {
            emit_space(p);
            print_expr(p, stmt->jump.label);
        }
        break;

    case ast$STMT_LABEL:
        print_expr(p, stmt->label.label);
        emit_token(p, token$COLON);
        emit_newline(p);
        emit_tabs(p);
        print_stmt(p, stmt->label.stmt);
        break;

    case ast$STMT_POSTFIX:
        print_expr(p, stmt->postfix.x);
        emit_token(p, stmt->postfix.op);
        break;

    case ast$STMT_RETURN:
        emit_token(p, token$RETURN);
        if (stmt->return_.x) {
            emit_space(p);
            print_expr(p, stmt->return_.x);
        }
        break;

    case ast$STMT_SWITCH:
        emit_token(p, token$SWITCH);
        emit_space(p);
        print_expr(p, stmt->switch_.tag);
        emit_space(p);
        emit_token(p, token$LBRACE);
        emit_newline(p);
        for (ast$Stmt **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(p);
            print_stmt(p, *stmts);
        }
        emit_tabs(p);
        emit_token(p, token$RBRACE);
        break;

    default:
        panic("Unknown stmt");
        break;
    }
}

static bool is_void(ast$Expr *type) {
    return type == NULL || (type->type == ast$EXPR_IDENT && streq(type->ident.name, "void"));
}

extern void print_type(emitter_t *p, ast$Expr *type) {
    if (type->is_const) {
        emit_token(p, token$CONST);
        emit_space(p);
    }
    switch (type->type) {
    case ast$TYPE_ARRAY:
        emit_token(p, token$LBRACK);
        if (type->array.len) {
            print_expr(p, type->array.len);
        }
        emit_token(p, token$RBRACK);
        print_type(p, type->array.elt);
        break;

    case ast$TYPE_FUNC:
        emit_token(p, token$LPAREN);
        for (ast$Decl **params = type->func.params; params && *params; ) {
            print_decl(p, *params);
            params++;
            if (*params != NULL) {
                emit_token(p, token$COMMA);
                emit_space(p);
            }
        }
        emit_token(p, token$RPAREN);
        if (!is_void(type->func.result)) {
            emit_space(p);
            print_type(p, type->func.result);
        }
        break;

    case ast$TYPE_ENUM:
        emit_token(p, token$ENUM);
        if (type->enum_.name) {
            emit_space(p);
            print_expr(p, type->enum_.name);
        }
        if (type->enum_.enums) {
            emit_space(p);
            emit_token(p, token$LBRACE);
            emit_newline(p);
            p->indent++;
            for (ast$Decl **enums = type->enum_.enums; enums && *enums; enums++) {
                ast$Decl *enumerator = *enums;
                emit_tabs(p);
                print_expr(p, enumerator->value.name);
                if (enumerator->value.value) {
                    emit_space(p);
                    emit_token(p, token$ASSIGN);
                    emit_space(p);
                    print_expr(p, enumerator->value.value);
                }
                emit_token(p, token$COMMA);
                emit_newline(p);
            }
            p->indent--;
            emit_tabs(p);
            emit_token(p, token$RBRACE);
        }
        break;

    case ast$EXPR_STAR:
        type = type->star.x;
        if (type->type == ast$TYPE_FUNC) {
            emit_token(p, token$FUNC);
            emit_token(p, token$LPAREN);
            for (ast$Decl **params = type->func.params; params && *params; ) {
                print_decl(p, *params);
                params++;
                if (*params != NULL) {
                    emit_token(p, token$COMMA);
                    emit_space(p);
                }
            }
            emit_token(p, token$RPAREN);
            if (!is_void(type->func.result)) {
                emit_space(p);
                print_type(p, type->func.result);
            }
        } else {
            emit_token(p, token$MUL);
            print_type(p, type);
        }
        break;

    case ast$TYPE_STRUCT:
        emit_token(p, type->struct_.tok);
        if (type->struct_.name) {
            emit_space(p);
            print_expr(p, type->struct_.name);
        }
        if (type->struct_.fields) {
            emit_space(p);
            emit_token(p, token$LBRACE);
            emit_newline(p);
            p->indent++;
            for (ast$Decl **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emit_tabs(p);
                print_decl(p, *fields);
                emit_newline(p);
            }
            p->indent--;
            emit_tabs(p);
            emit_token(p, token$RBRACE);
        }
        break;

    case ast$EXPR_IDENT:
        print_expr(p, type);
        break;

    default:
        panic("Unknown type: %d", type->type);
    }
}

extern void print_decl(emitter_t *p, ast$Decl *decl) {
    switch (decl->type) {

    case ast$DECL_FIELD:
        if (decl->field.type == NULL && decl->field.name == NULL) {
            emit_string(p, "...");
        } else {
            if (decl->field.name != NULL) {
                print_expr(p, decl->field.name);
                emit_space(p);
            }
            print_type(p, decl->field.type);
        }
        break;

    case ast$DECL_FUNC:
        emit_token(p, token$FUNC);
        emit_space(p);
        print_expr(p, decl->func.name);
        print_type(p, decl->func.type);
        if (decl->func.body) {
            emit_space(p);
            print_stmt(p, decl->func.body);
        }
        break;

    case ast$DECL_TYPEDEF:
        emit_token(p, token$TYPEDEF);
        emit_space(p);
        print_expr(p, decl->typedef_.name);
        emit_space(p);
        print_type(p, decl->typedef_.type);
        break;

    case ast$DECL_PRAGMA:
        emit_token(p, token$HASH);
        emit_string(p, decl->pragma.lit);
        break;

    case ast$DECL_VALUE:
        switch (decl->value.kind) {
        case token$VAR:
            emit_token(p, token$VAR);
            if (decl->value.name) {
                emit_space(p);
                print_expr(p, decl->value.name);
            }
            // TODO:
            // if (decl->value.value == NULL
            //         || ast$isNil(decl->value.value)
            //         || ast$isVoidPtr(decl->value.value)
            //         || decl->value.value->type == ast$EXPR_COMPOUND) {
            emit_space(p);
            print_type(p, decl->value.type);
            // }
            if (decl->value.value) {
                emit_space(p);
                emit_token(p, token$ASSIGN);
                emit_space(p);
                print_expr(p, decl->value.value);
            }
            break;
        case token$CONST:
            emit_token(p, token$HASH);
            emit_string(p, "define");
            emit_space(p);
            print_expr(p, decl->value.value);
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

extern void printer_print_file(emitter_t *p, ast$File *file) {
    emit_string(p, "//");
    emit_string(p, file->filename);
    emit_newline(p);
    if (file->name != NULL) {
        emit_token(p, token$PACKAGE);
        emit_space(p);
        print_expr(p, file->name);
        emit_newline(p);
    }
    for (ast$Decl **imports = file->imports; imports && *imports; imports++) {
        emit_token(p, token$IMPORT);
        emit_space(p);
        print_expr(p, (*imports)->imp.path);
        emit_newline(p);
    }
    for (ast$Decl **decls = file->decls; decls && *decls; decls++) {
        emit_newline(p);
        print_decl(p, *decls);
        emit_newline(p);
    }
}
