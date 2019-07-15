#include "subc/emitter/emit.h"
#include "bling/token/token.h"

static void emit_c_decl(emitter_t *e, decl_t *decl);
static void emit_c_expr(emitter_t *e, expr_t *expr);
static void emit_c_type(emitter_t *e, expr_t *type, expr_t *name);

static void emit_c_expr(emitter_t *e, expr_t *expr) {
    if (!expr) {
        panic("emit_c_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        emit_string(e, expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        emit_c_expr(e, expr->binary.x);
        emit_space(e);
        emit_token(e, expr->binary.op);
        emit_space(e);
        emit_c_expr(e, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        emit_c_expr(e, expr->call.func);
        emit_token(e, token_LPAREN);
        for (expr_t **args = expr->call.args; args && *args; ) {
            emit_c_expr(e, *args);
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
        emit_c_type(e, expr->cast.type, NULL);
        emit_token(e, token_RPAREN);
        emit_c_expr(e, expr->cast.expr);
        break;

    case ast_EXPR_COMPOUND:
        emit_token(e, token_LBRACE);
        emit_newline(e);
        e->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emit_tabs(e);
            emit_c_expr(e, *exprs);
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
        emit_c_expr(e, expr->index.x);
        emit_token(e, token_LBRACK);
        emit_c_expr(e, expr->index.index);
        emit_token(e, token_RBRACK);
        break;

    case ast_EXPR_INCDEC:
        emit_c_expr(e, expr->incdec.x);
        emit_token(e, expr->incdec.tok);
        break;

    case ast_EXPR_KEY_VALUE:
        emit_token(e, token_PERIOD);
        emit_c_expr(e, expr->key_value.key);
        emit_space(e);
        emit_token(e, token_ASSIGN);
        emit_space(e);
        emit_c_expr(e, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        emit_token(e, token_LPAREN);
        emit_c_expr(e, expr->paren.x);
        emit_token(e, token_RPAREN);
        break;

    case ast_EXPR_SELECTOR:
        emit_c_expr(e, expr->selector.x);
        emit_token(e, expr->selector.tok);
        emit_c_expr(e, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        emit_token(e, token_SIZEOF);
        emit_token(e, token_LPAREN);
        emit_c_type(e, expr->sizeof_.x, NULL);
        emit_token(e, token_RPAREN);
        break;

    case ast_EXPR_UNARY:
        emit_token(e, expr->unary.op);
        emit_c_expr(e, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        emit_c_expr(e, expr->type_name.type);
        break;

    default:
        panic("Unknown expr: %d", expr->type);
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
            emit_c_expr(e, stmt->case_.expr);
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
        emit_c_decl(e, stmt->decl);
        break;

    case ast_STMT_EXPR:
        emit_c_expr(e, stmt->expr.x);
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_IF:
        emit_token(e, token_IF);
        emit_space(e);
        emit_token(e, token_LPAREN);
        emit_c_expr(e, stmt->if_.cond);
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
            emit_c_expr(e, stmt->iter.cond);
        }
        if (stmt->iter.kind == token_FOR) {
            emit_token(e, token_SEMICOLON);
            emit_space(e);
            if (stmt->iter.post) {
                emit_c_expr(e, stmt->iter.post);
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
            emit_c_expr(e, stmt->jump.label);
        }
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_LABEL:
        emit_c_expr(e, stmt->label.label);
        emit_token(e, token_COLON);
        break;

    case ast_STMT_RETURN:
        emit_token(e, token_RETURN);
        if (stmt->return_.x) {
            emit_space(e);
            emit_c_expr(e, stmt->return_.x);
        }
        emit_token(e, token_SEMICOLON);
        break;

    case ast_STMT_SWITCH:
        emit_token(e, token_SWITCH);
        emit_space(e);
        emit_token(e, token_LPAREN);
        emit_c_expr(e, stmt->switch_.tag);
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

static void emit_c_type(emitter_t *e, expr_t *type, expr_t *name) {
    if (type == NULL) {
        panic("emit_c_type: type is nil");
    }
    switch (type->type) {
    case ast_TYPE_ARRAY:
        emit_c_type(e, type->array.elt, name);
        emit_token(e, token_LBRACK);
        if (type->array.len) {
            emit_c_expr(e, type->array.len);
        }
        emit_token(e, token_RBRACK);
        name = NULL;
        break;

    case ast_TYPE_FUNC:
        if (type->func.result != NULL) {
            emit_c_type(e, type->func.result, name);
        } else {
            emit_string(e, "void");
            emit_space(e);
            emit_c_expr(e, name);
        }
        emit_token(e, token_LPAREN);
        for (decl_t **params = type->func.params; params && *params; ) {
            emit_c_decl(e, *params);
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
            emit_c_expr(e, type->enum_.name);
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
                emit_c_expr(e, enumerator->name);
                if (enumerator->value) {
                    emit_space(e);
                    emit_token(e, token_ASSIGN);
                    emit_space(e);
                    emit_c_expr(e, enumerator->value);
                }
                emit_token(e, token_COMMA);
                emit_newline(e);
            }
            e->indent--;
            emit_tabs(e);
            emit_token(e, token_RBRACE);
        }
        break;

    case ast_TYPE_NAME:
        emit_c_expr(e, type->type_name.type);
        break;

    case ast_TYPE_PTR:
        type = type->ptr.type;
        if (type->type == ast_TYPE_FUNC) {
            emit_c_type(e, type->func.result, NULL);
            emit_token(e, token_LPAREN);
            emit_token(e, token_MUL);
            if (name != NULL) {
                emit_c_expr(e, name);
            }
            emit_token(e, token_RPAREN);
            emit_token(e, token_LPAREN);
            for (decl_t **params = type->func.params; params && *params; ) {
                emit_c_decl(e, *params);
                params++;
                if (*params != NULL) {
                    emit_token(e, token_COMMA);
                    emit_space(e);
                }
            }
            emit_token(e, token_RPAREN);
            name = NULL;
        } else {
            emit_c_type(e, type, NULL);
            emit_token(e, token_MUL);
        }
        break;

    case ast_TYPE_QUAL:
        emit_token(e, type->qual.qual);
        emit_space(e);
        emit_c_type(e, type->qual.type, name);
        name = NULL;
        break;

    case ast_TYPE_STRUCT:
        emit_token(e, type->struct_.tok);
        if (type->struct_.name) {
            emit_space(e);
            emit_c_expr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            emit_space(e);
            emit_token(e, token_LBRACE);
            emit_newline(e);
            e->indent++;
            for (decl_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emit_tabs(e);
                emit_c_decl(e, *fields);
                emit_token(e, token_SEMICOLON);
                emit_newline(e);
            }
            e->indent--;
            emit_tabs(e);
            emit_token(e, token_RBRACE);
        }
        break;

    case ast_EXPR_IDENT:
        emit_c_expr(e, type);
        break;

    default:
        panic("Unknown type: %d", type->type);
    }

    if (name) {
        emit_space(e);
        emit_c_expr(e, name);
    }
}

static void emit_c_decl(emitter_t *e, decl_t *decl) {
    if (decl->store != token_ILLEGAL) {
        emit_token(e, decl->store);
    }
    switch (decl->type) {

    case ast_DECL_FIELD:
        if (decl->field.type == NULL && decl->field.name == NULL) {
            emit_string(e, "...");
        } else {
            emit_c_type(e, decl->field.type, decl->field.name);
        }
        break;

    case ast_DECL_FUNC:
        emit_c_type(e, decl->func.type, decl->func.name);
        if (decl->func.body) {
            emit_space(e);
            emit_stmt(e, decl->func.body);
        } else {
            emit_token(e, token_SEMICOLON);
        }
        break;

    case ast_DECL_TYPEDEF:
        emit_token(e, token_TYPEDEF);
        emit_space(e);
        emit_c_type(e, decl->typedef_.type, decl->typedef_.name);
        emit_token(e, token_SEMICOLON);
        break;

    case ast_DECL_VALUE:
        emit_c_type(e, decl->value.type, decl->value.name);
        if (decl->value.value) {
            emit_space(e);
            emit_token(e, token_ASSIGN);
            emit_space(e);
            emit_c_expr(e, decl->value.value);
        }
        emit_token(e, token_SEMICOLON);
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
        emit_c_decl(e, *decls);
        emit_newline(e);
        emit_newline(e);
    }
}
