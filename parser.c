#include "kc.h"

#include "token.h"

#include "scanner.h"
#include "ast.h"
#include "emit.h"

#define error(fmt, ...) do { \
    fprintf(stderr, "%d:%d " fmt "\n", line(), col(), ## __VA_ARGS__); \
    void *buf[1000]; \
    int n = backtrace(buf, 1000); \
    backtrace_symbols_fd(buf, n, 2); \
    exit(1); \
} while (0)

expr_t *parse_declarator(expr_t **type);
decl_t *parse_decl(void);
expr_t *parse_expr(void);
expr_t *parse_type(void);
stmt_t *parse_stmt(void);

static slice_t types = {.size=sizeof(char *)};
void *nil_ptr = NULL;

bool is_type(void) {
    switch (tok) {
    case token_CONST:
    case token_ENUM:
    case token_EXTERN:
    case token_STATIC:
    case token_STRUCT:
    case token_UNION:
        return true;
    case token_IDENT:
        for (int i = 0; i < len(types); i++) {
            if (!strcmp(*(char **)get_ptr(types, i), lit)) {
                return true;
            }
        }
    default:
        return false;
    }
}

static int line(void)
{
    int n = 1;
    for (int i = 0; i < offset; i++)
        if (src[i] == '\n')
            n++;
    return n;
}

static int col(void)
{
    int n = 1;
    for (int i = 0; i < offset; i++) {
        if (src[i] == '\n')
            n = 1;
        else
            n++;
    }
    return n;
}

void append_type(char *st) {
    types = append(types, &st);
}

bool accept(int tok0) {
    if (tok == tok0) {
        scan();
        return true;
    }
    return false;
}

void expect(int tok0) {
    if (tok != tok0) {
        char *s = lit;
        if (!*s)
            s = token_string(tok);
        error("expected `%s`, got %s", token_string(tok0), s);
        exit(1);
    }
    scan();
}

expr_t *parse_ident(void)
{
    expr_t *expr = NULL;
    switch (tok) {
    case token_IDENT:
        expr = malloc(sizeof(*expr));
        expr->type = ast_EXPR_IDENT;
        expr->ident.name = strdup(lit);
        break;
    default:
        expect(token_IDENT);
        break;
    }
    scan();
    return expr;
}

expr_t *parse_primary_expr(void)
{
    expr_t *x = NULL;
    switch (tok) {
    case token_IDENT:
        x = parse_ident();
        break;
    case token_INT:
        x = malloc(sizeof(*x));
        x->type = ast_EXPR_BASIC_LIT;
        x->basic_lit.kind = tok;
        x->basic_lit.value = strdup(lit);
        scan();
        break;
    default:
        error("bad expr: %s: %s", token_string(tok), lit);
        break;
    }
    return x;
}

expr_t *parse_call_expr(expr_t *func) {
    slice_t args = {.size = sizeof(expr_t *)};
    expect(token_LPAREN);
    while (tok != token_RPAREN) {
        expr_t *x = parse_expr();
        args = append(args, &x);
        if (!accept(token_COMMA)) {
            args = append(args, &nil_ptr);
            break;
        }
    }
    expect(token_RPAREN);
    expr_t call = {
        .type = ast_EXPR_CALL,
        .call = {
            .func = func,
            .args = args.array,
        },
    };
    return copy(&call);
}

expr_t *parse_postfix_expr(void)
{
    expr_t *x = parse_primary_expr();
    for (;;) {
        expr_t *y;
        switch (tok) {
        case token_LPAREN:
            x = parse_call_expr(x);
            break;
        case token_LBRACK:
            scan();
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_INDEX;
            y->index.x = x;
            y->index.index = parse_expr();
            x = y;
            expect(token_RBRACK);
            break;
        case token_PERIOD:
            scan();
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_SELECTOR;
            y->selector.x = x;
            y->selector.sel = parse_ident();
            x = y;
            break;
        case token_INC:
        case token_DEC:
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_INCDEC;
            y->incdec.x = x;
            y->incdec.tok = tok;
            x = y;
            scan();
            break;
        default:
            goto done;
        }
    }
done:
    return x;
}

expr_t *parse_unary_expr(void)
{
    expr_t *x;
    switch (tok) {
    case token_ADD:
    case token_AND:
    case token_MUL:
    case token_NOT:
    case token_SUB:
        x = malloc(sizeof(*x));
        x->type = ast_EXPR_UNARY;
        x->unary.op = tok;
        scan();
        x->unary.x = parse_unary_expr(); // TODO: parse_cast_expr
        break;
    default:
        x = parse_postfix_expr();
        break;
    }
    return x;
}

expr_t *parse_expr(void)
{
    expr_t *x = parse_unary_expr();
    expr_t *y;
    switch (tok) {
    case token_ADD:
    case token_ADD_ASSIGN:
    case token_ASSIGN:
    case token_DIV:
    case token_DIV_ASSIGN:
    case token_EQUAL:
    case token_GT:
    case token_GT_EQUAL:
    case token_LT:
    case token_LT_EQUAL:
    case token_MUL:
    case token_MUL_ASSIGN:
    case token_SUB:
    case token_SUB_ASSIGN:
        y = malloc(sizeof(*y));
        y->type = ast_EXPR_BINARY;
        y->binary.x = x;
        y->binary.op = tok;
        scan();
        y->binary.y = parse_expr();
        x = y;
        break;
    }
    return x;
}

field_t *parse_field(void) {
    expr_t *type = parse_type();
    expr_t *name = parse_declarator(&type);
    field_t field = {
        .type = type,
        .name = name,
    };
    return copy(&field);
}

expr_t *parse_struct_or_union(int keyword)
{
    expr_t *name = NULL;
    expect(keyword);
    if (tok == token_IDENT) {
        name = parse_ident();
    }
    field_t **fields = NULL;
    if (accept(token_LBRACE)) {
        slice_t slice = {.size = sizeof(field_t *)};
        for (;;) {
            field_t *field = parse_field();
            expect(token_SEMICOLON);
            slice = append(slice, &field);
            if (tok == token_RBRACE) {
                slice = append(slice, &nil_ptr);
                break;
            }
        }
        fields = slice.array;
        expect(token_RBRACE);
    }
    // TODO assert name or fields
    expr_t x = {
        .type = ast_TYPE_STRUCT,
        .struct_ = {
            .tok = keyword,
            .name = name,
            .fields = fields,
        },
    };
    return copy(&x);
}

expr_t *parse_enum(void)
{
    expr_t *name = NULL;
    slice_t enums = {.size=sizeof(enumerator_t *)};
    int cap = 0;
    int len = 0;
    expect(token_ENUM);
    if (tok == token_IDENT) {
        name = parse_ident();
    }
    expect(token_LBRACE);
    for (;;) {
        enumerator_t *enumerator = malloc(sizeof(*enumerator));
        enumerator->name = parse_ident();
        enumerator->value = NULL;
        if (tok == token_ASSIGN) {
            scan();
            enumerator->value = parse_expr();
        }
        expect(token_COMMA);
        enums = append(enums, &enumerator);
        if (tok == token_RBRACE) {
            enumerator = NULL;
            enums = append(enums, &enumerator);
            break;
        }
    }
    expect(token_RBRACE);
    expr_t *x = malloc(sizeof(*x));
    x->type = ast_TYPE_ENUM;
    x->enum_.name = name;
    x->enum_.enumerators = enums.array;
    return x;
}

expr_t *parse_type(void)
{
    expr_t *x = NULL;
    switch (tok) {
    case token_STRUCT:
    case token_UNION:
        x = parse_struct_or_union(tok);
        break;
    case token_ENUM:
        x = parse_enum();
        break;
    case token_IDENT:
        x = parse_ident();
        break;
    default:
        error("expected type, got %s", *lit ? lit : token_string(tok));
        break;
    }
    return x;
}

stmt_t *parse_compound_stmt(void)
{
    stmt_t *stmt = NULL;
    slice_t stmts = {.size = sizeof(stmt_t *)};
    expect(token_LBRACE);
    while (tok != token_RBRACE) {
        stmt = parse_stmt();
        stmts = append(stmts, &stmt);
    }
    stmts = append(stmts, &nil_ptr);
    expect(token_RBRACE);
    stmt = malloc(sizeof(*stmt));
    stmt->type = ast_STMT_BLOCK;
    stmt->block.stmts = stmts.array;
    return stmt;
}

stmt_t *parse_return_stmt(void)
{
    expr_t *x = NULL;
    expect(token_RETURN);
    if (tok != token_SEMICOLON)
        x = parse_expr();
    expect(token_SEMICOLON);
    stmt_t *stmt = malloc(sizeof(*stmt));
    stmt->type = ast_STMT_RETURN;
    stmt->return_.x = x;
    return stmt;
}

stmt_t *parse_if_stmt(void) {
    expect(token_IF);
    expect(token_LPAREN);
    expr_t *cond = parse_expr();
    expect(token_RPAREN);
    stmt_t *body = parse_compound_stmt();
    stmt_t *else_ = NULL;
    if (accept(token_ELSE)) {
        else_ = parse_stmt();
    }
    stmt_t stmt = {
        .type = ast_STMT_IF,
        .if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return copy(&stmt);
}

stmt_t *parse_while_stmt(void)
{
    expect(token_WHILE);
    expect(token_LPAREN);
    expr_t *cond = parse_expr();
    expect(token_RPAREN);
    stmt_t *body = parse_stmt();
    stmt_t stmt = {
        .type = ast_STMT_WHILE,
        .while_ = {
            .cond = cond,
            .body = body,
        },
    };
    return copy(&stmt);
}

stmt_t *parse_stmt(void)
{
    if (is_type()) {
        stmt_t stmt = {
            .type = ast_STMT_DECL,
            .decl = parse_decl(),
        };
        return copy(&stmt);
    }
    switch (tok) {
    case token_LBRACE:
        return parse_compound_stmt();
    case token_IF:
        return parse_if_stmt();
    case token_WHILE:
        return parse_while_stmt();
    case token_RETURN:
        return parse_return_stmt();
    default:
        {
            stmt_t stmt = {
                .type = ast_STMT_EXPR,
                .expr.x = NULL,
            };
            if (tok != token_SEMICOLON) {
                stmt.expr.x = parse_expr();
            }
            expect(token_SEMICOLON);
            return copy(&stmt);
        }
    }
}

expr_t *parse_const_expr(void)
{
    return parse_expr(); // TODO change
}

field_t **parse_parameter_type_list(void) {
    if (tok == token_IDENT && !strcmp(lit, "void")) {
        scan();
        return NULL;
    }
    slice_t params = {.size=sizeof(field_t *)};
    while (tok != token_RPAREN) {
        field_t *param = parse_field();
        params = append(params, &param);
        if (!accept(token_COMMA)) {
            break;
        }
    }
    if (len(params)) {
        params = append(params, &nil_ptr);
    }
    return params.array;
}

expr_t *parse_declarator(expr_t **type_ptr)
{
    while (accept(token_MUL)) {
        expr_t type = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = *type_ptr,
            }
        };
        *type_ptr = copy(&type);
    }
    expr_t *name = NULL;
    if (tok == token_IDENT) {
        name = parse_ident();
    }
    switch (tok) {
    case token_LBRACK:
        {
            expect(token_LBRACK);
            expr_t type = {
                .type = ast_TYPE_ARRAY,
                .array = {
                    .elt = *type_ptr,
                    .len = parse_const_expr(),
                },
            };
            expect(token_RBRACK);
            *type_ptr = copy(&type);
        }
        break;
    case token_LPAREN:
        {
            expect(token_LPAREN);
            expr_t type = {
                .type = ast_TYPE_FUNC,
                .func = {
                    .result = *type_ptr,
                    .params = parse_parameter_type_list(),
                },
            };
            expect(token_RPAREN);
            *type_ptr = copy(&type);
        }
        break;
    }
    return name;
}

spec_t *parse_typedef_spec(void) {
    expr_t *type = parse_type();
    expr_t *ident = parse_ident();
    spec_t spec = {
        .type = ast_SPEC_TYPEDEF,
        .typedef_.name = ident,
        .typedef_.type = type,
    };
    types = append(types, &ident->ident.name);
    return copy(&spec);
}

decl_t *parse_gen_decl(int keyword, spec_t *(*f)(void)) {
    expect(keyword);
    spec_t *spec = f();
    expect(token_SEMICOLON);
    decl_t *decl = malloc(sizeof(*decl));
    decl->type = ast_DECL_GEN;
    decl->gen.spec = spec;
    return decl;
}

decl_t *parse_decl(void) {
    if (tok == token_TYPEDEF) {
        return parse_gen_decl(tok, parse_typedef_spec);
    }
    expr_t *type = parse_type();
    expr_t *name = parse_declarator(&type);
    expr_t *value = NULL;
    if (tok == token_LBRACE) {
        stmt_t *body = parse_compound_stmt();
        decl_t decl = {
            .type = ast_DECL_FUNC,
            .func = {
                .type = type,
                .name = name,
                .body = body,
            },
        };
        return copy(&decl);
    }
    if (accept(token_ASSIGN)) {
        value = parse_expr();
    }
    expect(token_SEMICOLON);
    spec_t spec = {
        .type = ast_SPEC_VALUE,
        .value = {
            .type = type,
            .name = name,
            .value = value,
        },
    };
    decl_t decl = {
        .type = ast_DECL_GEN,
        .gen.spec = copy(&spec),
    };
    return copy(&decl);
}

decl_t **parse_file(void) {
    slice_t decls = {.size = sizeof(decl_t *)};
    append_type("FILE");
    append_type("bool");
    append_type("char");
    append_type("float");
    append_type("int");
    append_type("size_t");
    append_type("void");
    next();
    scan();
    while (tok != token_EOF) {
        decl_t *decl = parse_decl();
        if (!decl)
            break;
        decls = append(decls, &decl);
        emit_decl(decl);
        printf("\n\n");
    }
    decls = append(decls, &nil_ptr);
    return decls.array;
}

