#include "subc/emitter/emit.h"
#include "subc/token/token.h"

static void emit_decl(emitter_t *e, decl_t *decl);
static void emit_expr(emitter_t *e, expr_t *expr);
static void emit_type(emitter_t *e, expr_t *type, expr_t *name);

static void emit_string(emitter_t *e, const char *s) {
    fputs(s, e->fp);
}

static void emit_space(emitter_t *e) {
    emit_string(e, " ");
}

static void emit_newline(emitter_t *e) {
    emit_string(e, "\n");
}

static void emit_tabs(emitter_t *e) {
    for (int i = 0; i < e->indent; i++) {
        emit_string(e, "    ");
    }
}

static void emit_token(emitter_t *e, token_t tok) {
    emit_string(e, token_string(tok));
}

static void emit_field(emitter_t *e, field_t *field) {
    if (field->type == NULL && field->name == NULL) {
        emit_string(e, "...");
    } else {
        if (field->is_const) {
            emit_token(e, token_CONST);
            emit_space(e);
        }
        emit_type(e, field->type, field->name);
    }
}

static void emit_expr(emitter_t *e, expr_t *expr) {
    if (!expr) {
        panic("emit_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        emit_string(e, expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        emit_expr(e, expr->binary.x);
        emit_space(e);
        emit_token(e, expr->binary.op);
        emit_space(e);
        emit_expr(e, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        emit_expr(e, expr->call.func);
        emit_token(e, token_LPAREN);
        for (expr_t **args = expr->call.args; args && *args; ) {
            emit_expr(e, *args);
            args++;
            if (*args) {
                emit_token(e, token_COMMA);
                emit_space(e);
            }
        }
        emit_token(e, token_RPAREN);
        break;

    case ast_EXPR_CAST:
        emit_token(e, token_LPAREN);
        emit_expr(e, expr->cast.type);
        emit_token(e, token_RPAREN);
        emit_expr(e, expr->cast.expr);
        break;

    case ast_EXPR_COMPOUND:
        emit_token(e, token_LBRACE);
        emit_newline(e);
        e->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emit_tabs(e);
            emit_expr(e, *exprs);
            emit_token(e, token_COMMA);
            emit_newline(e);
        }
        e->indent--;
        emit_tabs(e);
        emit_token(e, token_RBRACE);
        break;

    case ast_EXPR_IDENT:
        emit_string(e, expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        emit_expr(e, expr->index.x);
        emit_token(e, token_LBRACK);
        emit_expr(e, expr->index.index);
        emit_token(e, token_RBRACK);
        break;

    case ast_EXPR_INCDEC:
        emit_expr(e, expr->incdec.x);
        emit_token(e, expr->incdec.tok);
        break;

    case ast_EXPR_KEY_VALUE:
        emit_token(e, token_PERIOD);
        emit_expr(e, expr->key_value.key);
        emit_space(e);
        emit_token(e, token_ASSIGN);
        emit_space(e);
        emit_expr(e, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        emit_token(e, token_LPAREN);
        emit_expr(e, expr->paren.x);
        emit_token(e, token_RPAREN);
        break;

    case ast_EXPR_SELECTOR:
        emit_expr(e, expr->selector.x);
        emit_token(e, expr->selector.tok);
        emit_expr(e, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        emit_token(e, token_SIZEOF);
        emit_token(e, token_LPAREN);
        emit_expr(e, expr->sizeof_.x);
        emit_token(e, token_RPAREN);
        break;

    case ast_EXPR_UNARY:
        emit_token(e, expr->unary.op);
        emit_expr(e, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        emit_type(e, expr->type_name.type, NULL);
        break;

    default:
        emit_string(e, "/* [UNKNOWN EXPR] */");
        break;
    }
}

static void emit_stmt(emitter_t *e, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        emit_token(e, token_LBRACE);
        emit_newline(e);
        e->indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast_STMT_LABEL:
                break;
            default:
                emit_tabs(e);
                break;
            }
            emit_stmt(e, *stmts);
            emit_newline(e);
        }
        e->indent--;
        emit_tabs(e);
        emit_token(e, token_RBRACE);
        break;

    case ast_STMT_CASE:
        if (stmt->case_.expr) {
            emit_token(e, token_CASE);
            emit_space(e);
            emit_expr(e, stmt->case_.expr);
        } else {
            emit_token(e, token_DEFAULT);
        }
        emit_token(e, token_COLON);
        emit_newline(e);
        e->indent++;
        for (stmt_t **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(e);
            emit_stmt(e, *stmts);
            emit_newline(e);
        }
        e->indent--;
        break;

    case ast_STMT_DECL:
        emit_decl(e, stmt->decl);
        break;

    case ast_STMT_EXPR:
        emit_expr(e, stmt->expr.x);
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_IF:
        emit_token(e, token_IF);
        emit_space(e);
        emit_token(e, token_LPAREN);
        emit_expr(e, stmt->if_.cond);
        emit_token(e, token_RPAREN);
        emit_space(e);
        emit_stmt(e, stmt->if_.body);
        if (stmt->if_.else_) {
            emit_space(e);
            emit_token(e, token_ELSE);
            emit_space(e);
            emit_stmt(e, stmt->if_.else_);
        }
        break;

    case ast_STMT_ITER:
        emit_token(e, stmt->iter.kind);
        emit_space(e);
        emit_token(e, token_LPAREN);
        if (stmt->iter.kind == token_FOR) {
            if (stmt->iter.init) {
                emit_stmt(e, stmt->iter.init);
                emit_space(e);
            } else {
                emit_token(e, token_SEMICOLON);
                emit_space(e);
            }
        }
        if (stmt->iter.cond) {
            emit_expr(e, stmt->iter.cond);
        }
        if (stmt->iter.kind == token_FOR) {
            emit_token(e, token_SEMICOLON);
            emit_space(e);
            if (stmt->iter.post) {
                emit_expr(e, stmt->iter.post);
            }
        }
        emit_token(e, token_RPAREN);
        emit_space(e);
        emit_stmt(e, stmt->iter.body);
        break;

    case ast_STMT_JUMP:
        emit_token(e, stmt->jump.keyword);
        if (stmt->jump.label) {
            emit_space(e);
            emit_expr(e, stmt->jump.label);
        }
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_LABEL:
        emit_expr(e, stmt->label.label);
        emit_token(e, token_COLON);
        break;

    case ast_STMT_RETURN:
        emit_token(e, token_RETURN);
        if (stmt->return_.x) {
            emit_space(e);
            emit_expr(e, stmt->return_.x);
        }
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_SWITCH:
        emit_token(e, token_SWITCH);
        emit_space(e);
        emit_token(e, token_LPAREN);
        emit_expr(e, stmt->switch_.tag);
        emit_token(e, token_RPAREN);
        emit_space(e);
        emit_token(e, token_LBRACE);
        emit_newline(e);
        for (stmt_t **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(e);
            emit_stmt(e, *stmts);
        }
        emit_tabs(e);
        emit_token(e, token_RBRACE);
        break;

    default:
        emit_string(e, "/* [UNKNOWN STMT] */;");
        break;
    }
}

static void emit_spec(emitter_t *e, spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        emit_token(e, token_TYPEDEF);
        emit_space(e);
        emit_type(e, spec->typedef_.type, spec->typedef_.name);
        emit_token(e, token_SEMICOLON);
        break;

    case ast_SPEC_VALUE:
        emit_type(e, spec->value.type, spec->value.name);
        if (spec->value.value) {
            emit_space(e);
            emit_token(e, token_ASSIGN);
            emit_space(e);
            emit_expr(e, spec->value.value);
        }
        emit_token(e, token_SEMICOLON);
        break;

    default:
        emit_string(e, "/* [UNKNOWN SPEC] */");
        break;

    }
}

static void emit_type(emitter_t *e, expr_t *type, expr_t *name) {
    switch (type->type) {
    case ast_TYPE_ARRAY:
        emit_type(e, type->array.elt, name);
        emit_token(e, token_LBRACK);
        if (type->array.len) {
            emit_expr(e, type->array.len);
        }
        emit_token(e, token_RBRACK);
        name = NULL;
        break;

    case ast_TYPE_FUNC:
        emit_type(e, type->func.result, name);
        emit_token(e, token_LPAREN);
        for (field_t **params = type->func.params; params && *params; ) {
            emit_field(e, *params);
            params++;
            if (*params != NULL) {
                emit_token(e, token_COMMA);
                emit_space(e);
            }
        }
        emit_token(e, token_RPAREN);
        name = NULL;
        break;

    case ast_TYPE_ENUM:
        emit_token(e, token_ENUM);
        if (type->enum_.name) {
            emit_space(e);
            emit_expr(e, type->enum_.name);
        }
        if (type->enum_.enumerators) {
            emit_space(e);
            emit_token(e, token_LBRACE);
            emit_newline(e);
            e->indent++;
            for (enumerator_t **enumerators = type->enum_.enumerators;
                    enumerators && *enumerators; enumerators++) {
                enumerator_t *enumerator = *enumerators;
                emit_tabs(e);
                emit_expr(e, enumerator->name);
                if (enumerator->value) {
                    emit_space(e);
                    emit_token(e, token_ASSIGN);
                    emit_space(e);
                    emit_expr(e, enumerator->value);
                }
                emit_token(e, token_COMMA);
                emit_newline(e);
            }
            e->indent--;
            emit_tabs(e);
            emit_token(e, token_RBRACE);
        }
        break;

    case ast_TYPE_PTR:
        type = type->ptr.type;
        if (type->type == ast_TYPE_FUNC) {
            emit_type(e, type->func.result, NULL);
            emit_token(e, token_LPAREN);
            emit_token(e, token_MUL);
            if (name != NULL) {
                emit_expr(e, name);
            }
            emit_token(e, token_RPAREN);
            emit_token(e, token_LPAREN);
            for (field_t **params = type->func.params; params && *params; ) {
                emit_field(e, *params);
                params++;
                if (*params != NULL) {
                    emit_token(e, token_COMMA);
                    emit_space(e);
                }
            }
            emit_token(e, token_RPAREN);
            name = NULL;
        } else {
            emit_type(e, type, NULL);
            emit_token(e, token_MUL);
        }
        break;

    case ast_SPEC_STORAGE:
        emit_token(e, type->store.store);
        emit_space(e);
        emit_type(e, type->store.type, name);
        name = NULL;
        break;

    case ast_TYPE_QUAL:
        emit_token(e, type->qual.qual);
        emit_space(e);
        emit_type(e, type->qual.type, name);
        name = NULL;
        break;

    case ast_TYPE_STRUCT:
        emit_token(e, type->struct_.tok);
        if (type->struct_.name) {
            emit_space(e);
            emit_expr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            emit_space(e);
            emit_token(e, token_LBRACE);
            emit_newline(e);
            e->indent++;
            for (field_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emit_tabs(e);
                emit_field(e, *fields);
                emit_token(e, token_SEMICOLON);
                emit_newline(e);
            }
            e->indent--;
            emit_tabs(e);
            emit_token(e, token_RBRACE);
        }
        break;

    case ast_EXPR_IDENT:
        emit_expr(e, type);
        break;

    default:
        panic("/* [UNKNOWN TYPE] */");
    }

    if (name) {
        emit_space(e);
        emit_expr(e, name);
    }
}

static void emit_decl(emitter_t *e, decl_t *decl) {
    switch (decl->type) {

    case ast_DECL_FUNC:
        emit_type(e, decl->func.type, decl->func.name);
        emit_space(e);
        if (decl->func.body) {
            emit_stmt(e, decl->func.body);
        } else {
            emit_token(e, token_SEMICOLON);
        }
        break;

    case ast_DECL_GEN:
        emit_spec(e, decl->gen.spec);
        break;

    default:
        emit_string(e, "/* [UNKNOWN DECL] */");
        break;
    }
}

extern void emitter_emit_file(emitter_t *e, file_t *file) {
    emit_string(e, "// ");
    emit_string(e, file->filename);
    emit_newline(e);
    emit_newline(e);
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        emit_decl(e, *decls);
        emit_newline(e);
        emit_newline(e);
    }
}