#include "bling/emitter/emit.h"
#include "bling/token/token.h"

extern void emit_string(emitter_t *e, const char *s) {
    strings_Builder_write(&e->builder, s, strlen(s), NULL);
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

extern void emit_token(emitter_t *e, token_t tok) {
    if (e->skipSemi && tok == token_SEMICOLON) {
        return;
    }
    emit_string(e, token_string(tok));
}

extern void print_expr(emitter_t *p, expr_t *expr) {
    if (!expr) {
        panic("print_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        emit_string(p, expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        print_expr(p, expr->binary.x);
        emit_space(p);
        emit_token(p, expr->binary.op);
        emit_space(p);
        print_expr(p, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        print_expr(p, expr->call.func);
        emit_token(p, token_LPAREN);
        for (expr_t **args = expr->call.args; args && *args; ) {
            print_expr(p, *args);
            args++;
            if (*args) {
                emit_token(p, token_COMMA);
                emit_space(p);
            }
        }
        emit_token(p, token_RPAREN);
        break;

    case ast_EXPR_CAST:
        emit_token(p, token_AS);
        emit_token(p, token_LPAREN);
        print_type(p, expr->cast.type);
        emit_token(p, token_RPAREN);
        emit_space(p);
        print_expr(p, expr->cast.expr);
        break;

    case ast_EXPR_COMPOUND:
        emit_token(p, token_LBRACE);
        emit_newline(p);
        p->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emit_tabs(p);
            print_expr(p, *exprs);
            emit_token(p, token_COMMA);
            emit_newline(p);
        }
        p->indent--;
        emit_tabs(p);
        emit_token(p, token_RBRACE);
        break;

    case ast_EXPR_COND:
        print_expr(p, expr->conditional.condition);
        emit_space(p);
        emit_token(p, token_QUESTION_MARK);
        emit_space(p);
        print_expr(p, expr->conditional.consequence);
        emit_space(p);
        emit_token(p, token_COLON);
        emit_space(p);
        print_expr(p, expr->conditional.alternative);
        break;

    case ast_EXPR_IDENT:
        emit_string(p, expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        print_expr(p, expr->index.x);
        emit_token(p, token_LBRACK);
        print_expr(p, expr->index.index);
        emit_token(p, token_RBRACK);
        break;

    case ast_EXPR_KEY_VALUE:
        print_expr(p, expr->key_value.key);
        emit_token(p, token_COLON);
        emit_space(p);
        print_expr(p, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        emit_token(p, token_LPAREN);
        print_expr(p, expr->paren.x);
        emit_token(p, token_RPAREN);
        break;

    case ast_EXPR_SELECTOR:
        print_expr(p, expr->selector.x);
        emit_token(p, expr->selector.tok);
        print_expr(p, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        emit_token(p, token_SIZEOF);
        emit_token(p, token_LPAREN);
        print_type(p, expr->sizeof_.x);
        emit_token(p, token_RPAREN);
        break;

    case ast_EXPR_UNARY:
        emit_token(p, expr->unary.op);
        print_expr(p, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        print_type(p, expr->type_name.type);
        break;

    default:
        panic("Unknown expr");
        break;
    }
}

extern void print_stmt(emitter_t *p, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_ASSIGN:
        print_expr(p, stmt->assign.x);
        emit_space(p);
        emit_token(p, stmt->assign.op);
        emit_space(p);
        print_expr(p, stmt->assign.y);
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_BLOCK:
        emit_token(p, token_LBRACE);
        emit_newline(p);
        p->indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast_STMT_LABEL:
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
        emit_token(p, token_RBRACE);
        break;

    case ast_STMT_CASE:
        if (stmt->case_.expr) {
            emit_token(p, token_CASE);
            emit_space(p);
            print_expr(p, stmt->case_.expr);
        } else {
            emit_token(p, token_DEFAULT);
        }
        emit_token(p, token_COLON);
        emit_newline(p);
        p->indent++;
        for (stmt_t **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(p);
            print_stmt(p, *stmts);
            emit_newline(p);
        }
        p->indent--;
        break;

    case ast_STMT_DECL:
        print_decl(p, stmt->decl);
        break;

    case ast_STMT_EMPTY:
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_EXPR:
        print_expr(p, stmt->expr.x);
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_IF:
        emit_token(p, token_IF);
        emit_space(p);
        print_expr(p, stmt->if_.cond);
        emit_space(p);
        print_stmt(p, stmt->if_.body);
        if (stmt->if_.else_) {
            emit_space(p);
            emit_token(p, token_ELSE);
            emit_space(p);
            print_stmt(p, stmt->if_.else_);
        }
        break;

    case ast_STMT_ITER:
        emit_token(p, stmt->iter.kind);
        emit_space(p);
        if (stmt->iter.kind == token_FOR) {
            if (stmt->iter.init) {
                print_stmt(p, stmt->iter.init);
                emit_space(p);
            } else {
                emit_token(p, token_SEMICOLON);
                emit_space(p);
            }
        }
        if (stmt->iter.cond) {
            print_expr(p, stmt->iter.cond);
        }
        if (stmt->iter.kind == token_FOR) {
            emit_token(p, token_SEMICOLON);
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

    case ast_STMT_JUMP:
        emit_token(p, stmt->jump.keyword);
        if (stmt->jump.label) {
            emit_space(p);
            print_expr(p, stmt->jump.label);
        }
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_LABEL:
        print_expr(p, stmt->label.label);
        emit_token(p, token_COLON);
        emit_tabs(p);
        print_stmt(p, stmt->label.stmt);
        break;

    case ast_STMT_POSTFIX:
        emit_tabs(p);
        print_expr(p, stmt->postfix.x);
        emit_token(p, stmt->postfix.op);
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_RETURN:
        emit_token(p, token_RETURN);
        if (stmt->return_.x) {
            emit_space(p);
            print_expr(p, stmt->return_.x);
        }
        emit_token(p, token_SEMICOLON);
        break;

    case ast_STMT_SWITCH:
        emit_token(p, token_SWITCH);
        emit_space(p);
        print_expr(p, stmt->switch_.tag);
        emit_space(p);
        emit_token(p, token_LBRACE);
        emit_newline(p);
        for (stmt_t **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(p);
            print_stmt(p, *stmts);
        }
        emit_tabs(p);
        emit_token(p, token_RBRACE);
        break;

    default:
        panic("Unknown stmt");
        break;
    }
}

static bool is_void(expr_t *type) {
    return type->type == ast_EXPR_IDENT && streq(type->ident.name, "void");
}

extern void print_type(emitter_t *p, expr_t *type) {
    switch (type->type) {
    case ast_TYPE_ARRAY:
        emit_token(p, token_LBRACK);
        if (type->array.len) {
            print_expr(p, type->array.len);
        }
        emit_token(p, token_RBRACK);
        print_type(p, type->array.elt);
        break;

    case ast_TYPE_FUNC:
        emit_token(p, token_LPAREN);
        for (decl_t **params = type->func.params; params && *params; ) {
            print_decl(p, *params);
            params++;
            if (*params != NULL) {
                emit_token(p, token_COMMA);
                emit_space(p);
            }
        }
        emit_token(p, token_RPAREN);
        if (!is_void(type->func.result)) {
            emit_space(p);
            print_type(p, type->func.result);
        }
        break;

    case ast_TYPE_ENUM:
        emit_token(p, token_ENUM);
        if (type->enum_.name) {
            emit_space(p);
            print_expr(p, type->enum_.name);
        }
        if (type->enum_.enums) {
            emit_space(p);
            emit_token(p, token_LBRACE);
            emit_newline(p);
            p->indent++;
            for (decl_t **enums = type->enum_.enums; enums && *enums; enums++) {
                decl_t *enumerator = *enums;
                emit_tabs(p);
                print_expr(p, enumerator->value.name);
                if (enumerator->value.value) {
                    emit_space(p);
                    emit_token(p, token_ASSIGN);
                    emit_space(p);
                    print_expr(p, enumerator->value.value);
                }
                emit_token(p, token_COMMA);
                emit_newline(p);
            }
            p->indent--;
            emit_tabs(p);
            emit_token(p, token_RBRACE);
        }
        break;

    case ast_TYPE_NAME:
        print_type(p, type->type_name.type);
        break;

    case ast_TYPE_PTR:
        type = type->ptr.type;
        if (type->type == ast_TYPE_FUNC) {
            emit_token(p, token_FUNC);
            emit_token(p, token_LPAREN);
            for (decl_t **params = type->func.params; params && *params; ) {
                print_decl(p, *params);
                params++;
                if (*params != NULL) {
                    emit_token(p, token_COMMA);
                    emit_space(p);
                }
            }
            emit_token(p, token_RPAREN);
            if (!is_void(type->func.result)) {
                emit_space(p);
                print_type(p, type->func.result);
            }
        } else {
            emit_token(p, token_MUL);
            print_type(p, type);
        }
        break;

    case ast_TYPE_QUAL:
        emit_token(p, type->qual.qual);
        emit_space(p);
        print_type(p, type->qual.type);
        break;

    case ast_TYPE_STRUCT:
        emit_token(p, type->struct_.tok);
        if (type->struct_.name) {
            emit_space(p);
            print_expr(p, type->struct_.name);
        }
        if (type->struct_.fields) {
            emit_space(p);
            emit_token(p, token_LBRACE);
            emit_newline(p);
            p->indent++;
            for (decl_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emit_tabs(p);
                print_decl(p, *fields);
                emit_token(p, token_SEMICOLON);
                emit_newline(p);
            }
            p->indent--;
            emit_tabs(p);
            emit_token(p, token_RBRACE);
        }
        break;

    case ast_EXPR_IDENT:
        print_expr(p, type);
        break;

    default:
        panic("Unknown type: %d", type->type);
    }
}

extern void print_decl(emitter_t *p, decl_t *decl) {
    switch (decl->type) {

    case ast_DECL_FIELD:
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

    case ast_DECL_FUNC:
        emit_token(p, token_FUNC);
        emit_space(p);
        print_expr(p, decl->func.name);
        print_type(p, decl->func.type);
        if (decl->func.body) {
            emit_space(p);
            print_stmt(p, decl->func.body);
        } else {
            emit_token(p, token_SEMICOLON);
        }
        break;

    case ast_DECL_TYPEDEF:
        emit_token(p, token_TYPEDEF);
        emit_space(p);
        print_expr(p, decl->typedef_.name);
        emit_space(p);
        print_type(p, decl->typedef_.type);
        emit_token(p, token_SEMICOLON);
        break;

    case ast_DECL_VALUE:
        emit_token(p, token_VAR);
        if (decl->value.name) {
            emit_space(p);
            print_expr(p, decl->value.name);
        }
        emit_space(p);
        print_type(p, decl->value.type);
        if (decl->value.value) {
            emit_space(p);
            emit_token(p, token_ASSIGN);
            emit_space(p);
            print_expr(p, decl->value.value);
        }
        emit_token(p, token_SEMICOLON);
        break;

    default:
        panic("Unknown decl");
        break;
    }
}

extern void printer_print_file(emitter_t *p, file_t *file) {
    emit_string(p, "//");
    emit_string(p, file->filename);
    emit_newline(p);
    for (decl_t **imports = file->imports; imports && *imports; imports++) {
        emit_token(p, token_IMPORT);
        emit_space(p);
        print_expr(p, (*imports)->import.path);
        emit_token(p, token_SEMICOLON);
        emit_newline(p);
    }
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        emit_newline(p);
        print_decl(p, *decls);
        emit_newline(p);
    }
}
