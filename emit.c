void emit_decl(decl_t *decl);
void emit_expr(expr_t *expr);
void emit_type(expr_t *type, expr_t *name);

int emit_indent = 0;
FILE *emit_fp = NULL;
#define emit_printf(fmt, ...) fprintf(emit_fp ? emit_fp : stdout, fmt, ##__VA_ARGS__)

void emit_tabs(void) {
    for (int i = 0; i < emit_indent; i++) {
        emit_printf("\t");
    }
}

void emit_field(field_t *field) {
    emit_type(field->type, field->name);
}

void emit_expr(expr_t *expr)
{
    if (!expr) {
        error("emit_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        emit_printf("%s", expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        emit_expr(expr->binary.x);
        emit_printf(" %s ", token_string(expr->binary.op));
        emit_expr(expr->binary.y);
        break;

    case ast_EXPR_CALL:
        emit_expr(expr->call.func);
        emit_printf("(");
        for (expr_t **args = expr->call.args; args && *args; ) {
            emit_expr(*args);
            args++;
            if (*args) {
                emit_printf(", ");
            }
        }
        emit_printf(")");
        break;

    case ast_EXPR_IDENT:
        emit_printf("%s", expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        emit_expr(expr->index.x);
        emit_printf("[");
        emit_expr(expr->index.index);
        emit_printf("]");
        break;

    case ast_EXPR_INCDEC:
        emit_expr(expr->incdec.x);
        emit_printf("%s", token_string(expr->incdec.tok));
        break;

    case ast_EXPR_SELECTOR:
        emit_expr(expr->selector.x);
        emit_printf(".");
        emit_expr(expr->selector.sel);
        break;

    case ast_EXPR_UNARY:
        emit_printf("%s", token_string(expr->unary.op));
        emit_expr(expr->unary.x);
        break;

    case ast_TYPE_ARRAY:
        emit_expr(expr->array.elt);
        emit_printf("[");
        emit_expr(expr->array.len);
        emit_printf("]");
        break;

    case ast_TYPE_ENUM:
        emit_printf("enum ");
        if (expr->enum_.name) {
            emit_expr(expr->enum_.name);
            emit_printf(" ");
        }
        emit_printf("{\n");
        for (enumerator_t **enumerators = expr->enum_.enumerators;
                enumerators && *enumerators; enumerators++) {
            enumerator_t *enumerator = *enumerators;
            emit_printf("\t");
            emit_expr(enumerator->name);
            if (enumerator->value) {
                emit_printf(" = ");
                emit_expr(enumerator->value);
            }
            emit_printf(",\n");
        }
        emit_printf("}");
        break;

    case ast_TYPE_PTR:
        emit_expr(expr->ptr.type);
        emit_printf("*");
        break;

    case ast_TYPE_STRUCT:
        emit_printf("%s", token_string(expr->struct_.tok));
        if (expr->struct_.name) {
            emit_printf(" ");
            emit_expr(expr->struct_.name);
        }
        if (expr->struct_.fields) {
            emit_printf(" {\n");
            emit_indent++;
            for (field_t **fields = expr->struct_.fields; fields && *fields; fields++) {
                emit_tabs();
                emit_field(*fields);
                emit_printf(";\n");
            }
            emit_indent--;
            emit_tabs();
            emit_printf("}");
        }
        break;

    default:
        emit_printf("/* UNKNOWN EXPR */");
        break;
    }
}

void emit_stmt(stmt_t *stmt)
{
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        emit_printf("{\n");
        emit_indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            emit_tabs();
            emit_stmt(*stmts);
            emit_printf("\n");
        }
        emit_indent--;
        emit_tabs();
        emit_printf("}");
        break;

    case ast_STMT_DECL:
        emit_decl(stmt->decl);
        break;

    case ast_STMT_EXPR:
        emit_expr(stmt->expr.x);
        emit_printf(";");
        break;

    case ast_STMT_IF:
        emit_printf("if (");
        emit_expr(stmt->if_.cond);
        emit_printf(") ");
        emit_stmt(stmt->if_.body);
        if (stmt->if_.else_) {
            emit_printf(" else ");
            emit_stmt(stmt->if_.else_);
        }
        break;

    case ast_STMT_RETURN:
        emit_printf("return");
        if (stmt->return_.x) {
            emit_printf(" ");
            emit_expr(stmt->return_.x);
        }
        emit_printf(";");
        break;

    case ast_STMT_WHILE:
        emit_printf("while (");
        emit_expr(stmt->while_.cond);
        emit_printf(") ");
        emit_stmt(stmt->if_.body);
        break;

    default:
        emit_printf("/* UNKNOWN STMT */;");
        break;
    }
}

void emit_spec(spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        emit_printf("typedef ");
        emit_expr(spec->typedef_.type);
        emit_printf(" ");
        emit_expr(spec->typedef_.name);
        emit_printf(";");
        break;

    case ast_SPEC_VALUE:
        emit_type(spec->value.type, spec->value.name);
        if (spec->value.value) {
            emit_printf(" = ");
            emit_expr(spec->value.value);
        }
        emit_printf(";");
        break;

    default:
        emit_printf("/* UNKNOWN SPEC */");
        break;

    }
}

void emit_type(expr_t *type, expr_t *name) {
    if (type->type == ast_TYPE_FUNC) {
        emit_expr(type->func.result);
    } else {
        emit_expr(type);
    }
    if (name) {
        emit_printf(" ");
        emit_expr(name);
    }
    if (type->type == ast_TYPE_FUNC) {
        emit_printf("(");
        for (field_t **params = type->func.params; params && *params; ) {
            emit_field(*params);
            params++;
            if (*params != NULL) {
                emit_printf(", ");
            }
        }
        emit_printf(")");
    }
}

void emit_decl(decl_t *decl)
{
    switch (decl->type) {

    case ast_DECL_FUNC:
        emit_type(decl->func.type, decl->func.name);
        emit_printf(" ");
        if (decl->func.body) {
            emit_stmt(decl->func.body);
        } else {
            emit_printf(";");
        }
        break;

    case ast_DECL_GEN:
        emit_spec(decl->gen.spec);
        break;

    default:
        emit_printf("/* UNKNOWN DECL */");
        break;
    }
}
