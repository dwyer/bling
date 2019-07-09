#include "bling/emitter/emit.h"
#include "bling/token/token.h"

static void print_decl(printer_t *p, decl_t *decl);
static void print_expr(printer_t *p, expr_t *expr);
static void print_type(printer_t *p, expr_t *type);

static void print_string(printer_t *p, const char *s) {
    fputs(s, p->fp);
}

static void print_space(printer_t *p) {
    print_string(p, " ");
}

static void print_newline(printer_t *p) {
    print_string(p, "\n");
}

static void print_tabs(printer_t *p) {
    for (int i = 0; i < p->indent; i++) {
        print_string(p, "    ");
    }
}

static void print_token(printer_t *p, token_t tok) {
    print_string(p, token_string(tok));
}

static void print_field(printer_t *p, field_t *field) {
    if (field->type == NULL && field->name == NULL) {
        print_string(p, "...");
    } else {
        if (field->is_const) {
            print_token(p, token_CONST);
            print_space(p);
        }
        if (field->name != NULL) {
            print_expr(p, field->name);
            print_space(p);
        }
        print_type(p, field->type);
    }
}

static void print_expr(printer_t *p, expr_t *expr) {
    if (!expr) {
        panic("print_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        print_string(p, expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        print_expr(p, expr->binary.x);
        print_space(p);
        print_token(p, expr->binary.op);
        print_space(p);
        print_expr(p, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        print_expr(p, expr->call.func);
        print_token(p, token_LPAREN);
        for (expr_t **args = expr->call.args; args && *args; ) {
            print_expr(p, *args);
            args++;
            if (*args) {
                print_token(p, token_COMMA);
                print_space(p);
            }
        }
        print_token(p, token_RPAREN);
        break;

    case ast_EXPR_CAST:
        print_expr(p, expr->cast.expr);
        print_space(p);
        print_token(p, token_AS);
        print_space(p);
        print_expr(p, expr->cast.type);
        break;

    case ast_EXPR_COMPOUND:
        print_token(p, token_LBRACE);
        print_newline(p);
        p->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            print_tabs(p);
            print_expr(p, *exprs);
            print_token(p, token_COMMA);
            print_newline(p);
        }
        p->indent--;
        print_tabs(p);
        print_token(p, token_RBRACE);
        break;

    case ast_EXPR_IDENT:
        print_string(p, expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        print_expr(p, expr->index.x);
        print_token(p, token_LBRACK);
        print_expr(p, expr->index.index);
        print_token(p, token_RBRACK);
        break;

    case ast_EXPR_INCDEC:
        print_expr(p, expr->incdec.x);
        print_token(p, expr->incdec.tok);
        break;

    case ast_EXPR_KEY_VALUE:
        print_expr(p, expr->key_value.key);
        print_token(p, token_COLON);
        print_space(p);
        print_expr(p, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        print_token(p, token_LPAREN);
        print_expr(p, expr->paren.x);
        print_token(p, token_RPAREN);
        break;

    case ast_EXPR_SELECTOR:
        print_expr(p, expr->selector.x);
        print_token(p, expr->selector.tok);
        print_expr(p, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        print_token(p, token_SIZEOF);
        print_token(p, token_LPAREN);
        print_type(p, expr->sizeof_.x);
        print_token(p, token_RPAREN);
        break;

    case ast_EXPR_UNARY:
        print_token(p, expr->unary.op);
        print_expr(p, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        print_type(p, expr->type_name.type);
        break;

    default:
        print_string(p, "/* [UNKNOWN EXPR] */");
        break;
    }
}

static void print_stmt(printer_t *p, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        print_token(p, token_LBRACE);
        print_newline(p);
        p->indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast_STMT_LABEL:
                break;
            default:
                print_tabs(p);
                break;
            }
            print_stmt(p, *stmts);
            print_newline(p);
        }
        p->indent--;
        print_tabs(p);
        print_token(p, token_RBRACE);
        break;

    case ast_STMT_CASE:
        if (stmt->case_.expr) {
            print_token(p, token_CASE);
            print_space(p);
            print_expr(p, stmt->case_.expr);
        } else {
            print_token(p, token_DEFAULT);
        }
        print_token(p, token_COLON);
        print_newline(p);
        p->indent++;
        for (stmt_t **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            print_tabs(p);
            print_stmt(p, *stmts);
            print_newline(p);
        }
        p->indent--;
        break;

    case ast_STMT_DECL:
        print_decl(p, stmt->decl);
        break;

    case ast_STMT_EXPR:
        print_expr(p, stmt->expr.x);
        print_token(p, token_SEMICOLON);
        break;

    case ast_STMT_IF:
        print_token(p, token_IF);
        print_space(p);
        print_expr(p, stmt->if_.cond);
        print_space(p);
        print_stmt(p, stmt->if_.body);
        if (stmt->if_.else_) {
            print_space(p);
            print_token(p, token_ELSE);
            print_space(p);
            print_stmt(p, stmt->if_.else_);
        }
        break;

    case ast_STMT_ITER:
        print_token(p, stmt->iter.kind);
        print_space(p);
        if (stmt->iter.kind == token_FOR) {
            if (stmt->iter.init) {
                print_stmt(p, stmt->iter.init);
                print_space(p);
            } else {
                print_token(p, token_SEMICOLON);
                print_space(p);
            }
        }
        if (stmt->iter.cond) {
            print_expr(p, stmt->iter.cond);
        }
        if (stmt->iter.kind == token_FOR) {
            print_token(p, token_SEMICOLON);
            print_space(p);
            if (stmt->iter.post) {
                print_expr(p, stmt->iter.post);
            }
        }
        print_space(p);
        print_stmt(p, stmt->iter.body);
        break;

    case ast_STMT_JUMP:
        print_token(p, stmt->jump.keyword);
        if (stmt->jump.label) {
            print_space(p);
            print_expr(p, stmt->jump.label);
        }
        print_token(p, token_SEMICOLON);
        break;

    case ast_STMT_LABEL:
        print_expr(p, stmt->label.label);
        print_token(p, token_COLON);
        break;

    case ast_STMT_RETURN:
        print_token(p, token_RETURN);
        if (stmt->return_.x) {
            print_space(p);
            print_expr(p, stmt->return_.x);
        }
        print_token(p, token_SEMICOLON);
        break;

    case ast_STMT_SWITCH:
        print_token(p, token_SWITCH);
        print_space(p);
        print_expr(p, stmt->switch_.tag);
        print_space(p);
        print_token(p, token_LBRACE);
        print_newline(p);
        for (stmt_t **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            print_tabs(p);
            print_stmt(p, *stmts);
        }
        print_tabs(p);
        print_token(p, token_RBRACE);
        break;

    default:
        print_string(p, "/* [UNKNOWN STMT] */;");
        break;
    }
}

static void print_spec(printer_t *p, spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        print_token(p, token_TYPEDEF);
        print_space(p);
        print_expr(p, spec->typedef_.name);
        print_space(p);
        print_type(p, spec->typedef_.type);
        print_token(p, token_SEMICOLON);
        break;

    case ast_SPEC_VALUE:
        print_token(p, token_VAR);
        if (spec->value.name) {
            print_space(p);
            print_expr(p, spec->value.name);
        }
        print_space(p);
        print_type(p, spec->value.type);
        if (spec->value.value) {
            print_space(p);
            print_token(p, token_ASSIGN);
            print_space(p);
            if (spec->value.value->type == ast_EXPR_COMPOUND) {
                print_type(p, spec->value.type);
            }
            print_expr(p, spec->value.value);
        }
        print_token(p, token_SEMICOLON);
        break;

    default:
        print_string(p, "/* [UNKNOWN SPEC] */");
        break;

    }
}

extern bool streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

extern bool strneq(const char *a, const char *b) {
    return !streq(a, b);
}

static bool is_void(expr_t *type) {
    return type->type == ast_EXPR_IDENT && streq(type->ident.name, "void");
}

static void print_type(printer_t *p, expr_t *type) {
    switch (type->type) {
    case ast_TYPE_ARRAY:
        print_token(p, token_LBRACK);
        if (type->array.len) {
            print_expr(p, type->array.len);
        }
        print_token(p, token_RBRACK);
        print_type(p, type->array.elt);
        break;

    case ast_TYPE_FUNC:
        print_token(p, token_LPAREN);
        for (field_t **params = type->func.params; params && *params; ) {
            print_field(p, *params);
            params++;
            if (*params != NULL) {
                print_token(p, token_COMMA);
                print_space(p);
            }
        }
        print_token(p, token_RPAREN);
        if (!is_void(type->func.result)) {
            print_space(p);
            print_type(p, type->func.result);
        }
        break;

    case ast_TYPE_ENUM:
        print_token(p, token_ENUM);
        if (type->enum_.name) {
            print_space(p);
            print_expr(p, type->enum_.name);
        }
        if (type->enum_.enumerators) {
            print_space(p);
            print_token(p, token_LBRACE);
            print_newline(p);
            p->indent++;
            for (enumerator_t **enumerators = type->enum_.enumerators;
                    enumerators && *enumerators; enumerators++) {
                enumerator_t *enumerator = *enumerators;
                print_tabs(p);
                print_expr(p, enumerator->name);
                if (enumerator->value) {
                    print_space(p);
                    print_token(p, token_ASSIGN);
                    print_space(p);
                    print_expr(p, enumerator->value);
                }
                print_token(p, token_COMMA);
                print_newline(p);
            }
            p->indent--;
            print_tabs(p);
            print_token(p, token_RBRACE);
        }
        break;

    case ast_TYPE_NAME:
        print_type(p, type->type_name.type);
        break;

    case ast_TYPE_PTR:
        type = type->ptr.type;
        if (type->type == ast_TYPE_FUNC) {
            print_token(p, token_FUNC);
            print_token(p, token_LPAREN);
            for (field_t **params = type->func.params; params && *params; ) {
                print_field(p, *params);
                params++;
                if (*params != NULL) {
                    print_token(p, token_COMMA);
                    print_space(p);
                }
            }
            print_token(p, token_RPAREN);
            if (!is_void(type->func.result)) {
                print_space(p);
                print_type(p, type->func.result);
            }
        } else {
            print_token(p, token_MUL);
            print_type(p, type);
        }
        break;

    case ast_TYPE_QUAL:
        //print_token(p, type->qual.qual);
        //print_space(p);
        print_type(p, type->qual.type);
        break;

    case ast_TYPE_STRUCT:
        print_token(p, type->struct_.tok);
        if (type->struct_.name) {
            print_space(p);
            print_expr(p, type->struct_.name);
        }
        if (type->struct_.fields) {
            print_space(p);
            print_token(p, token_LBRACE);
            print_newline(p);
            p->indent++;
            for (field_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                print_tabs(p);
                print_field(p, *fields);
                print_token(p, token_SEMICOLON);
                print_newline(p);
            }
            p->indent--;
            print_tabs(p);
            print_token(p, token_RBRACE);
        }
        break;

    case ast_EXPR_IDENT:
        print_expr(p, type);
        break;

    default:
        panic("Unknown type: %d", type->type);
    }
}

static void print_decl(printer_t *p, decl_t *decl) {
    switch (decl->type) {

    case ast_DECL_FUNC:
        print_token(p, token_FUNC);
        print_space(p);
        print_expr(p, decl->func.name);
        print_type(p, decl->func.type);
        if (decl->func.body) {
            print_space(p);
            print_stmt(p, decl->func.body);
        } else {
            print_token(p, token_SEMICOLON);
        }
        break;

    case ast_DECL_GEN:
        print_spec(p, decl->gen.spec);
        break;

    default:
        print_string(p, "/* [UNKNOWN DECL] */");
        break;
    }
}

extern void printer_print_file(printer_t *p, file_t *file) {
    print_string(p, "// ");
    print_string(p, file->filename);
    print_newline(p);
    print_newline(p);
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        print_decl(p, *decls);
        print_newline(p);
        print_newline(p);
    }
}
