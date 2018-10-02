#include "cmd/compile/emit.h"
#include "kc/token/token.h"

static void emit_decl(emitter_t *e, decl_t *decl);
static void emit_expr(emitter_t *e, expr_t *expr);
static void emit_type(emitter_t *e, expr_t *type, expr_t *name);

static void emit_printf(emitter_t *e, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(e->fp, fmt, ap);
    va_end(ap);
}

static void emit_tabs(emitter_t *e) {
    for (int i = 0; i < e->indent; i++) {
        emit_printf(e, "\t");
    }
}

static void emit_field(emitter_t *e, field_t *field) {
    if (field->type == NULL && field->name == NULL) {
        emit_printf(e, "%s", token_string(token_ELLIPSIS));
    } else {
        emit_type(e, field->type, field->name);
    }
}

static void emit_expr(emitter_t *e, expr_t *expr) {
    if (!expr) {
        panic("emit_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        emit_printf(e, "%s", expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        emit_expr(e, expr->binary.x);
        emit_printf(e, " %s ", token_string(expr->binary.op));
        emit_expr(e, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        emit_expr(e, expr->call.func);
        emit_printf(e, "(");
        for (expr_t **args = expr->call.args; args && *args; ) {
            emit_expr(e, *args);
            args++;
            if (*args) {
                emit_printf(e, ", ");
            }
        }
        emit_printf(e, ")");
        break;

    case ast_EXPR_IDENT:
        emit_printf(e, "%s", expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        emit_expr(e, expr->index.x);
        emit_printf(e, "[");
        emit_expr(e, expr->index.index);
        emit_printf(e, "]");
        break;

    case ast_EXPR_INCDEC:
        emit_expr(e, expr->incdec.x);
        emit_printf(e, "%s", token_string(expr->incdec.tok));
        break;

    case ast_EXPR_SELECTOR:
        emit_expr(e, expr->selector.x);
        emit_printf(e, ".");
        emit_expr(e, expr->selector.sel);
        break;

    case ast_EXPR_UNARY:
        emit_printf(e, "%s", token_string(expr->unary.op));
        emit_expr(e, expr->unary.x);
        break;

    case ast_TYPE_ARRAY:
        emit_expr(e, expr->array.elt);
        emit_printf(e, "[");
        emit_expr(e, expr->array.len);
        emit_printf(e, "]");
        break;

    case ast_TYPE_ENUM:
        emit_printf(e, "enum ");
        if (expr->enum_.name) {
            emit_expr(e, expr->enum_.name);
            emit_printf(e, " ");
        }
        emit_printf(e, "{\n");
        for (enumerator_t **enumerators = expr->enum_.enumerators;
                enumerators && *enumerators; enumerators++) {
            enumerator_t *enumerator = *enumerators;
            emit_printf(e, "\t");
            emit_expr(e, enumerator->name);
            if (enumerator->value) {
                emit_printf(e, " = ");
                emit_expr(e, enumerator->value);
            }
            emit_printf(e, ",\n");
        }
        emit_printf(e, "}");
        break;

    case ast_TYPE_PTR:
        emit_expr(e, expr->ptr.type);
        emit_printf(e, "*");
        break;

    case ast_TYPE_STRUCT:
        emit_printf(e, "%s", token_string(expr->struct_.tok));
        if (expr->struct_.name) {
            emit_printf(e, " ");
            emit_expr(e, expr->struct_.name);
        }
        if (expr->struct_.fields) {
            emit_printf(e, " {\n");
            e->indent++;
            for (field_t **fields = expr->struct_.fields; fields && *fields; fields++) {
                emit_tabs(e);
                emit_field(e, *fields);
                emit_printf(e, ";\n");
            }
            e->indent--;
            emit_tabs(e);
            emit_printf(e, "}");
        }
        break;

    default:
        emit_printf(e, "/* UNKNOWN EXPR */");
        break;
    }
}

static void emit_stmt(emitter_t *e, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        emit_printf(e, "{\n");
        e->indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            emit_tabs(e);
            emit_stmt(e, *stmts);
            emit_printf(e, "\n");
        }
        e->indent--;
        emit_tabs(e);
        emit_printf(e, "}");
        break;

    case ast_STMT_DECL:
        emit_decl(e, stmt->decl);
        break;

    case ast_STMT_EXPR:
        emit_expr(e, stmt->expr.x);
        emit_printf(e, ";");
        break;

    case ast_STMT_IF:
        emit_printf(e, "if (");
        emit_expr(e, stmt->if_.cond);
        emit_printf(e, ") ");
        emit_stmt(e, stmt->if_.body);
        if (stmt->if_.else_) {
            emit_printf(e, " else ");
            emit_stmt(e, stmt->if_.else_);
        }
        break;

    case ast_STMT_RETURN:
        emit_printf(e, "return");
        if (stmt->return_.x) {
            emit_printf(e, " ");
            emit_expr(e, stmt->return_.x);
        }
        emit_printf(e, ";");
        break;

    case ast_STMT_WHILE:
        emit_printf(e, "while (");
        emit_expr(e, stmt->while_.cond);
        emit_printf(e, ") ");
        emit_stmt(e, stmt->if_.body);
        break;

    default:
        emit_printf(e, "/* UNKNOWN STMT */;");
        break;
    }
}

static void emit_spec(emitter_t *e, spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        emit_printf(e, "typedef ");
        emit_expr(e, spec->typedef_.type);
        emit_printf(e, " ");
        emit_expr(e, spec->typedef_.name);
        emit_printf(e, ";");
        break;

    case ast_SPEC_VALUE:
        emit_type(e, spec->value.type, spec->value.name);
        if (spec->value.value) {
            emit_printf(e, " = ");
            emit_expr(e, spec->value.value);
        }
        emit_printf(e, ";");
        break;

    default:
        emit_printf(e, "/* UNKNOWN SPEC */");
        break;

    }
}

static void emit_type(emitter_t *e, expr_t *type, expr_t *name) {
    if (type->type == ast_TYPE_FUNC) {
        emit_expr(e, type->func.result);
    } else {
        emit_expr(e, type);
    }
    if (name) {
        emit_printf(e, " ");
        emit_expr(e, name);
    }
    if (type->type == ast_TYPE_FUNC) {
        emit_printf(e, "(");
        for (field_t **params = type->func.params; params && *params; ) {
            emit_field(e, *params);
            params++;
            if (*params != NULL) {
                emit_printf(e, ", ");
            }
        }
        emit_printf(e, ")");
    }
}

static void emit_decl(emitter_t *e, decl_t *decl) {
    switch (decl->type) {

    case ast_DECL_FUNC:
        emit_type(e, decl->func.type, decl->func.name);
        emit_printf(e, " ");
        if (decl->func.body) {
            emit_stmt(e, decl->func.body);
        } else {
            emit_printf(e, ";");
        }
        break;

    case ast_DECL_GEN:
        emit_spec(e, decl->gen.spec);
        break;

    default:
        emit_printf(e, "/* UNKNOWN DECL */");
        break;
    }
}

extern void emitter_emit_file(emitter_t *e, file_t *file) {
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        emit_decl(e, *decls);
        emit_printf(e, "\n\n");
    }
}
