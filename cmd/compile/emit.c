#include "cmd/compile/emit.h"
#include "subc/token/token.h"

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

static void emit_token(emitter_t *e, token_t tok) {
    emit_printf(e, "%s", token_string(tok));
}

static void emit_field(emitter_t *e, field_t *field) {
    if (field->type == NULL && field->name == NULL) {
        emit_printf(e, "...");
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
        emit_printf(e, " ");
        emit_token(e, expr->binary.op);
        emit_printf(e, " ");
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

    case ast_EXPR_CAST:
        emit_printf(e, "(");
        emit_expr(e, expr->cast.type);
        emit_printf(e, ")");
        emit_expr(e, expr->cast.expr);
        break;

    case ast_EXPR_COMPOUND:
        emit_printf(e, "{\n");
        e->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            emit_tabs(e);
            emit_expr(e, *exprs);
            emit_printf(e, ",\n");
        }
        e->indent--;
        emit_tabs(e);
        emit_printf(e, "}");
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
        emit_token(e, expr->incdec.tok);
        break;

    case ast_EXPR_KEY_VALUE:
        emit_printf(e, ".");
        emit_expr(e, expr->key_value.key);
        emit_printf(e, " = ");
        emit_expr(e, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        emit_printf(e, "(");
        emit_expr(e, expr->paren.x);
        emit_printf(e, ")");
        break;

    case ast_EXPR_SELECTOR:
        emit_expr(e, expr->selector.x);
        emit_token(e, expr->selector.tok);
        emit_expr(e, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        emit_printf(e, "sizeof(");
        emit_expr(e, expr->sizeof_.x);
        emit_printf(e, ")");
        break;

    case ast_EXPR_UNARY:
        emit_token(e, expr->unary.op);
        emit_expr(e, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        emit_type(e, expr->type_name.type, NULL);
        break;

    default:
        emit_printf(e, "/* [UNKNOWN EXPR] */");
        break;
    }
}

static void emit_stmt(emitter_t *e, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        emit_printf(e, "{\n");
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
            emit_printf(e, "\n");
        }
        e->indent--;
        emit_tabs(e);
        emit_printf(e, "}");
        break;

    case ast_STMT_CASE:
        if (stmt->case_.expr) {
            emit_printf(e, "case ");
            emit_expr(e, stmt->case_.expr);
        } else {
            emit_printf(e, "default");
        }
        emit_printf(e, ":\n");
        e->indent++;
        for (stmt_t **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(e);
            emit_stmt(e, *stmts);
            emit_printf(e, "\n");
        }
        e->indent--;
        break;

    case ast_STMT_DECL:
        emit_decl(e, stmt->decl);
        break;

    case ast_STMT_EXPR:
        emit_expr(e, stmt->expr.x);
        emit_printf(e, ";");
        break;

    case ast_STMT_FOR:
        emit_printf(e, "for (");
        if (stmt->for_.init) {
            emit_stmt(e, stmt->for_.init);
            emit_printf(e, " ");
        } else {
            emit_printf(e, "; ");
        }
        if (stmt->for_.cond) {
            emit_expr(e, stmt->for_.cond);
        }
        emit_printf(e, "; ");
        if (stmt->for_.post) {
            emit_expr(e, stmt->for_.post);
        }
        emit_printf(e, ") ");
        emit_stmt(e, stmt->for_.body);
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

    case ast_STMT_JUMP:
        emit_token(e, stmt->jump.keyword);
        if (stmt->jump.label) {
            emit_printf(e, " ");
            emit_expr(e, stmt->jump.label);
        }
        emit_printf(e, ";");
        break;

    case ast_STMT_LABEL:
        emit_expr(e, stmt->label.label);
        emit_printf(e, ":");
        break;

    case ast_STMT_RETURN:
        emit_printf(e, "return");
        if (stmt->return_.x) {
            emit_printf(e, " ");
            emit_expr(e, stmt->return_.x);
        }
        emit_printf(e, ";");
        break;

    case ast_STMT_SWITCH:
        emit_printf(e, "switch (");
        emit_expr(e, stmt->switch_.tag);
        emit_printf(e, ") {\n");
        for (stmt_t **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            emit_tabs(e);
            emit_stmt(e, *stmts);
        }
        emit_tabs(e);
        emit_printf(e, "}");
        break;

    case ast_STMT_WHILE:
        emit_printf(e, "while (");
        emit_expr(e, stmt->while_.cond);
        emit_printf(e, ") ");
        emit_stmt(e, stmt->while_.body);
        break;

    default:
        emit_printf(e, "/* [UNKNOWN STMT] */;");
        break;
    }
}

static void emit_spec(emitter_t *e, spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        emit_printf(e, "typedef ");
        emit_type(e, spec->typedef_.type, spec->typedef_.name);
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
        emit_printf(e, "/* [UNKNOWN SPEC] */");
        break;

    }
}

static void emit_type(emitter_t *e, expr_t *type, expr_t *name) {
    switch (type->type) {
    case ast_TYPE_ARRAY:
        emit_type(e, type->array.elt, name);
        emit_printf(e, "[");
        if (type->array.len) {
            emit_expr(e, type->array.len);
        }
        emit_printf(e, "]");
        name = NULL;
        break;

    case ast_TYPE_FUNC:
        emit_type(e, type->func.result, name);
        emit_printf(e, "(");
        for (field_t **params = type->func.params; params && *params; ) {
            emit_field(e, *params);
            params++;
            if (*params != NULL) {
                emit_printf(e, ", ");
            }
        }
        emit_printf(e, ")");
        name = NULL;
        break;

    case ast_TYPE_ENUM:
        emit_printf(e, "enum");
        if (type->enum_.name) {
            emit_printf(e, " ");
            emit_expr(e, type->enum_.name);
        }
        if (type->enum_.enumerators) {
            emit_printf(e, " {\n");
            e->indent++;
            for (enumerator_t **enumerators = type->enum_.enumerators;
                    enumerators && *enumerators; enumerators++) {
                enumerator_t *enumerator = *enumerators;
                emit_tabs(e);
                emit_expr(e, enumerator->name);
                if (enumerator->value) {
                    emit_printf(e, " = ");
                    emit_expr(e, enumerator->value);
                }
                emit_printf(e, ",\n");
            }
            e->indent--;
            emit_tabs(e);
            emit_printf(e, "}");
        }
        break;

    case ast_TYPE_PTR:
        emit_type(e, type->ptr.type, NULL);
        emit_printf(e, "*");
        break;

    case ast_TYPE_STRUCT:
        emit_token(e, type->struct_.tok);
        if (type->struct_.name) {
            emit_printf(e, " ");
            emit_expr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            emit_printf(e, " {\n");
            e->indent++;
            for (field_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                emit_tabs(e);
                emit_field(e, *fields);
                emit_printf(e, ";\n");
            }
            e->indent--;
            emit_tabs(e);
            emit_printf(e, "}");
        }
        break;

    case ast_EXPR_IDENT:
        emit_expr(e, type);
        break;

    default:
        emit_printf(e, "/* [UNKNOWN TYPE] */");
    }

    if (name) {
        emit_printf(e, " ");
        emit_expr(e, name);
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
        emit_printf(e, "/* [UNKNOWN DECL] */");
        break;
    }
}

extern void emitter_emit_file(emitter_t *e, file_t *file) {
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        emit_decl(e, *decls);
        emit_printf(e, "\n\n");
    }
}
