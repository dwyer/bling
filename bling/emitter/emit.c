#include "bling/emitter/emit.h"
#include "subc/token/token.h"

static void print_decl(printer_t *e, decl_t *decl);
static void print_expr(printer_t *e, expr_t *expr);
static void print_type(printer_t *e, expr_t *type);

static void print_string(printer_t *e, const char *s) {
    fputs(s, e->fp);
}

static void print_space(printer_t *e) {
    print_string(e, " ");
}

static void print_newline(printer_t *e) {
    print_string(e, "\n");
}

static void print_tabs(printer_t *e) {
    for (int i = 0; i < e->indent; i++) {
        print_string(e, "    ");
    }
}

static void print_token(printer_t *e, token_t tok) {
    print_string(e, token_string(tok));
}

static void print_field(printer_t *e, field_t *field) {
    if (field->type == NULL && field->name == NULL) {
        print_string(e, "...");
    } else {
        if (field->is_const) {
            print_token(e, token_CONST);
            print_space(e);
        }
        print_type(e, field->type);
        if (field->name != NULL) {
            print_space(e);
            print_expr(e, field->name);
        }
    }
}

static void print_expr(printer_t *e, expr_t *expr) {
    if (!expr) {
        panic("print_expr: expr is NULL");
    }
    switch (expr->type) {

    case ast_EXPR_BASIC_LIT:
        print_string(e, expr->basic_lit.value);
        break;

    case ast_EXPR_BINARY:
        print_expr(e, expr->binary.x);
        print_space(e);
        print_token(e, expr->binary.op);
        print_space(e);
        print_expr(e, expr->binary.y);
        break;

    case ast_EXPR_CALL:
        print_expr(e, expr->call.func);
        print_token(e, token_LPAREN);
        for (expr_t **args = expr->call.args; args && *args; ) {
            print_expr(e, *args);
            args++;
            if (*args) {
                print_token(e, token_COMMA);
                print_space(e);
            }
        }
        print_token(e, token_RPAREN);
        break;

    case ast_EXPR_CAST:
        print_token(e, token_LPAREN);
        print_expr(e, expr->cast.type);
        print_token(e, token_RPAREN);
        print_expr(e, expr->cast.expr);
        break;

    case ast_EXPR_COMPOUND:
        print_token(e, token_LBRACE);
        print_newline(e);
        e->indent++;
        for (expr_t **exprs = expr->compound.list; exprs && *exprs; exprs++) {
            print_tabs(e);
            print_expr(e, *exprs);
            print_token(e, token_COMMA);
            print_newline(e);
        }
        e->indent--;
        print_tabs(e);
        print_token(e, token_RBRACE);
        break;

    case ast_EXPR_IDENT:
        print_string(e, expr->ident.name);
        break;

    case ast_EXPR_INDEX:
        print_expr(e, expr->index.x);
        print_token(e, token_LBRACK);
        print_expr(e, expr->index.index);
        print_token(e, token_RBRACK);
        break;

    case ast_EXPR_INCDEC:
        print_expr(e, expr->incdec.x);
        print_token(e, expr->incdec.tok);
        break;

    case ast_EXPR_KEY_VALUE:
        print_token(e, token_PERIOD);
        print_expr(e, expr->key_value.key);
        print_space(e);
        print_token(e, token_ASSIGN);
        print_space(e);
        print_expr(e, expr->key_value.value);
        break;

    case ast_EXPR_PAREN:
        print_token(e, token_LPAREN);
        print_expr(e, expr->paren.x);
        print_token(e, token_RPAREN);
        break;

    case ast_EXPR_SELECTOR:
        print_expr(e, expr->selector.x);
        print_token(e, expr->selector.tok);
        print_expr(e, expr->selector.sel);
        break;

    case ast_EXPR_SIZEOF:
        print_token(e, token_SIZEOF);
        print_token(e, token_LPAREN);
        print_expr(e, expr->sizeof_.x);
        print_token(e, token_RPAREN);
        break;

    case ast_EXPR_UNARY:
        print_token(e, expr->unary.op);
        print_expr(e, expr->unary.x);
        break;

    case ast_TYPE_NAME:
        print_type(e, expr->type_name.type);
        break;

    default:
        print_string(e, "/* [UNKNOWN EXPR] */");
        break;
    }
}

static void print_stmt(printer_t *e, stmt_t *stmt) {
    switch (stmt->type) {

    case ast_STMT_BLOCK:
        print_token(e, token_LBRACE);
        print_newline(e);
        e->indent++;
        for (stmt_t **stmts = stmt->block.stmts; stmts && *stmts; stmts++) {
            switch ((*stmts)->type) {
            case ast_STMT_LABEL:
                break;
            default:
                print_tabs(e);
                break;
            }
            print_stmt(e, *stmts);
            print_newline(e);
        }
        e->indent--;
        print_tabs(e);
        print_token(e, token_RBRACE);
        break;

    case ast_STMT_CASE:
        if (stmt->case_.expr) {
            print_token(e, token_CASE);
            print_space(e);
            print_expr(e, stmt->case_.expr);
        } else {
            print_token(e, token_DEFAULT);
        }
        print_token(e, token_COLON);
        print_newline(e);
        e->indent++;
        for (stmt_t **stmts = stmt->case_.stmts; stmts && *stmts; stmts++) {
            print_tabs(e);
            print_stmt(e, *stmts);
            print_newline(e);
        }
        e->indent--;
        break;

    case ast_STMT_DECL:
        print_decl(e, stmt->decl);
        break;

    case ast_STMT_EXPR:
        print_expr(e, stmt->expr.x);
        print_token(e, token_SEMICOLON);
        break;

    case ast_STMT_IF:
        print_token(e, token_IF);
        print_space(e);
        print_expr(e, stmt->if_.cond);
        print_space(e);
        print_stmt(e, stmt->if_.body);
        if (stmt->if_.else_) {
            print_space(e);
            print_token(e, token_ELSE);
            print_space(e);
            print_stmt(e, stmt->if_.else_);
        }
        break;

    case ast_STMT_ITER:
        print_token(e, stmt->iter.kind);
        print_space(e);
        if (stmt->iter.kind == token_FOR) {
            if (stmt->iter.init) {
                print_stmt(e, stmt->iter.init);
                print_space(e);
            } else {
                print_token(e, token_SEMICOLON);
                print_space(e);
            }
        }
        if (stmt->iter.cond) {
            print_expr(e, stmt->iter.cond);
        }
        if (stmt->iter.kind == token_FOR) {
            print_token(e, token_SEMICOLON);
            print_space(e);
            if (stmt->iter.post) {
                print_expr(e, stmt->iter.post);
            }
        }
        print_space(e);
        print_stmt(e, stmt->iter.body);
        break;

    case ast_STMT_JUMP:
        print_token(e, stmt->jump.keyword);
        if (stmt->jump.label) {
            print_space(e);
            print_expr(e, stmt->jump.label);
        }
        print_token(e, token_SEMICOLON);
        break;

    case ast_STMT_LABEL:
        print_expr(e, stmt->label.label);
        print_token(e, token_COLON);
        break;

    case ast_STMT_RETURN:
        print_token(e, token_RETURN);
        if (stmt->return_.x) {
            print_space(e);
            print_expr(e, stmt->return_.x);
        }
        print_token(e, token_SEMICOLON);
        break;

    case ast_STMT_SWITCH:
        print_token(e, token_SWITCH);
        print_space(e);
        print_expr(e, stmt->switch_.tag);
        print_space(e);
        print_token(e, token_LBRACE);
        print_newline(e);
        for (stmt_t **stmts = stmt->switch_.stmts; stmts && *stmts; stmts++) {
            print_tabs(e);
            print_stmt(e, *stmts);
        }
        print_tabs(e);
        print_token(e, token_RBRACE);
        break;

    default:
        print_string(e, "/* [UNKNOWN STMT] */;");
        break;
    }
}

static void print_spec(printer_t *e, spec_t *spec) {
    switch (spec->type) {

    case ast_SPEC_TYPEDEF:
        print_token(e, token_TYPEDEF);
        print_space(e);
        print_expr(e, spec->typedef_.name);
        print_space(e);
        print_type(e, spec->typedef_.type);
        print_token(e, token_SEMICOLON);
        break;

    case ast_SPEC_VALUE:
        print_token(e, token_VAR);
        if (spec->value.name) {
            print_space(e);
            print_expr(e, spec->value.name);
        }
        print_space(e);
        print_type(e, spec->value.type);
        if (spec->value.value) {
            print_space(e);
            print_token(e, token_ASSIGN);
            print_space(e);
            print_expr(e, spec->value.value);
        }
        print_token(e, token_SEMICOLON);
        break;

    default:
        print_string(e, "/* [UNKNOWN SPEC] */");
        break;

    }
}

static void print_type(printer_t *e, expr_t *type) {
    switch (type->type) {
    case ast_TYPE_ARRAY:
        print_token(e, token_LBRACK);
        if (type->array.len) {
            print_expr(e, type->array.len);
        }
        print_token(e, token_RBRACK);
        print_type(e, type->array.elt);
        break;

    case ast_TYPE_FUNC:
        print_token(e, token_LPAREN);
        for (field_t **params = type->func.params; params && *params; ) {
            print_field(e, *params);
            params++;
            if (*params != NULL) {
                print_token(e, token_COMMA);
                print_space(e);
            }
        }
        print_token(e, token_RPAREN);
        print_space(e);
        print_type(e, type->func.result);
        break;

    case ast_TYPE_ENUM:
        print_token(e, token_ENUM);
        if (type->enum_.name) {
            print_space(e);
            print_expr(e, type->enum_.name);
        }
        if (type->enum_.enumerators) {
            print_space(e);
            print_token(e, token_LBRACE);
            print_newline(e);
            e->indent++;
            for (enumerator_t **enumerators = type->enum_.enumerators;
                    enumerators && *enumerators; enumerators++) {
                enumerator_t *enumerator = *enumerators;
                print_tabs(e);
                print_expr(e, enumerator->name);
                if (enumerator->value) {
                    print_space(e);
                    print_token(e, token_ASSIGN);
                    print_space(e);
                    print_expr(e, enumerator->value);
                }
                print_token(e, token_COMMA);
                print_newline(e);
            }
            e->indent--;
            print_tabs(e);
            print_token(e, token_RBRACE);
        }
        break;

    case ast_TYPE_PTR:
        type = type->ptr.type;
        if (type->type == ast_TYPE_FUNC) {
            print_type(e, type->func.result);
            print_token(e, token_LPAREN);
            print_token(e, token_MUL);
            print_token(e, token_RPAREN);
            print_token(e, token_LPAREN);
            for (field_t **params = type->func.params; params && *params; ) {
                print_field(e, *params);
                params++;
                if (*params != NULL) {
                    print_token(e, token_COMMA);
                    print_space(e);
                }
            }
            print_token(e, token_RPAREN);
        } else {
            print_token(e, token_MUL);
            print_type(e, type);
        }
        break;

    case ast_TYPE_QUAL:
        print_token(e, type->qual.qual);
        print_space(e);
        print_type(e, type->qual.type);
        break;

    case ast_TYPE_STRUCT:
        print_token(e, type->struct_.tok);
        if (type->struct_.name) {
            print_space(e);
            print_expr(e, type->struct_.name);
        }
        if (type->struct_.fields) {
            print_space(e);
            print_token(e, token_LBRACE);
            print_newline(e);
            e->indent++;
            for (field_t **fields = type->struct_.fields; fields && *fields;
                    fields++) {
                print_tabs(e);
                print_field(e, *fields);
                print_token(e, token_SEMICOLON);
                print_newline(e);
            }
            e->indent--;
            print_tabs(e);
            print_token(e, token_RBRACE);
        }
        break;

    case ast_EXPR_IDENT:
        print_expr(e, type);
        break;

    default:
        panic("/* [UNKNOWN TYPE] */");
    }
}

static void print_decl(printer_t *e, decl_t *decl) {
    switch (decl->type) {

    case ast_DECL_FUNC:
        print_token(e, token_FUNC);
        print_space(e);
        print_expr(e, decl->func.name);
        print_type(e, decl->func.type);
        if (decl->func.body) {
            print_space(e);
            print_stmt(e, decl->func.body);
        } else {
            print_token(e, token_SEMICOLON);
        }
        break;

    case ast_DECL_GEN:
        print_spec(e, decl->gen.spec);
        break;

    default:
        print_string(e, "/* [UNKNOWN DECL] */");
        break;
    }
}

extern void printer_print_file(printer_t *e, file_t *file) {
    print_string(e, "// ");
    print_string(e, file->filename);
    print_newline(e);
    print_newline(e);
    for (decl_t **decls = file->decls; decls && *decls; decls++) {
        print_decl(e, *decls);
        print_newline(e);
        print_newline(e);
    }
}
