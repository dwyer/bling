#include "kc/parser/parser.h"

#define memdup(src, size) memcpy(malloc((size)), (src), (size))
#define dup(src) memdup((src), sizeof(*(src)))

#define error(p, fmt, ...) \
    panic("%s:%d:%d " fmt "\n", p->filename, line(p), col(p), ## __VA_ARGS__);

typedef struct {
    scanner_t scanner;
    int tok;
    char *lit;
    char *filename;
} parser_t;

static expr_t *parse_const_expr(parser_t *p);
static expr_t *parse_declarator(parser_t *p, expr_t **type_ptr);
static decl_t *parse_decl(parser_t *p);
static expr_t *parse_expr(parser_t *p);
static expr_t *parse_type(parser_t *p);
static stmt_t *parse_stmt(parser_t *p);

static slice_t types = {.size=sizeof(char *)};
static void *nil_ptr = NULL;

static bool is_type(parser_t *p) {
    switch (p->tok) {
    case token_CONST:
    case token_ENUM:
    case token_EXTERN:
    case token_STATIC:
    case token_STRUCT:
    case token_UNION:
        return true;
    case token_IDENT:
        for (int i = 0; i < len(types); i++) {
            if (!strcmp(*(char **)get_ptr(types, i), p->lit)) {
                return true;
            }
        }
    default:
        return false;
    }
}

static int line(parser_t *p) {
    int n = 1;
    for (int i = 0; i < p->scanner.offset; i++)
        if (p->scanner.src[i] == '\n')
            n++;
    return n;
}

static int col(parser_t *p) {
    int n = 1;
    for (int i = 0; i < p->scanner.offset; i++) {
        if (p->scanner.src[i] == '\n')
            n = 1;
        else
            n++;
    }
    return n;
}

static void next(parser_t *p) {
    free(p->lit);
    p->tok = scanner_scan(&p->scanner, &p->lit);
}

static void append_type(char *st) {
    types = append(types, &st);
}

static bool accept(parser_t *p, int tok0) {
    if (p->tok == tok0) {
        next(p);
        return true;
    }
    return false;
}

static void expect(parser_t *p, int tok) {
    if (p->tok != tok) {
        char *lit = p->lit;
        if (lit == NULL) {
            lit = token_string(p->tok);
        }
        error(p, "expected `%s`, got `%s`", token_string(tok), lit);
        exit(1);
    }
    next(p);
}

static expr_t *parse_ident(parser_t *p) {
    expr_t *expr = NULL;
    switch (p->tok) {
    case token_IDENT:
        expr = malloc(sizeof(*expr));
        expr->type = ast_EXPR_IDENT;
        expr->ident.name = strdup(p->lit);
        break;
    default:
        expect(p, token_IDENT);
        break;
    }
    next(p);
    return expr;
}

static expr_t *parse_primary_expr(parser_t *p) {
    expr_t *x = NULL;
    switch (p->tok) {
    case token_IDENT:
        x = parse_ident(p);
        break;
    case token_CHAR:
    case token_INT:
    case token_STRING:
        x = malloc(sizeof(*x));
        x->type = ast_EXPR_BASIC_LIT;
        x->basic_lit.kind = p->tok;
        x->basic_lit.value = strdup(p->lit);
        next(p);
        break;
    default:
        error(p, "bad expr: %s: %s", token_string(p->tok), p->lit);
        break;
    }
    return x;
}

static expr_t *parse_call_expr(parser_t *p, expr_t *func) {
    slice_t args = {.size = sizeof(expr_t *)};
    expect(p, token_LPAREN);
    while (p->tok != token_RPAREN) {
        expr_t *x = parse_expr(p);
        args = append(args, &x);
        if (!accept(p, token_COMMA)) {
            args = append(args, &nil_ptr);
            break;
        }
    }
    expect(p, token_RPAREN);
    expr_t call = {
        .type = ast_EXPR_CALL,
        .call = {
            .func = func,
            .args = args.array,
        },
    };
    return dup(&call);
}

static expr_t *parse_postfix_expr(parser_t *p) {
    expr_t *x = parse_primary_expr(p);
    for (;;) {
        expr_t *y;
        switch (p->tok) {
        case token_LPAREN:
            x = parse_call_expr(p, x);
            break;
        case token_LBRACK:
            next(p);
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_INDEX;
            y->index.x = x;
            y->index.index = parse_expr(p);
            x = y;
            expect(p, token_RBRACK);
            break;
        case token_PERIOD:
            next(p);
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_SELECTOR;
            y->selector.x = x;
            y->selector.sel = parse_ident(p);
            x = y;
            break;
        case token_INC:
        case token_DEC:
            y = malloc(sizeof(*y));
            y->type = ast_EXPR_INCDEC;
            y->incdec.x = x;
            y->incdec.tok = p->tok;
            x = y;
            next(p);
            break;
        default:
            goto done;
        }
    }
done:
    return x;
}

static expr_t *parse_unary_expr(parser_t *p) {
    expr_t *x;
    switch (p->tok) {
    case token_ADD:
    case token_AND:
    case token_MUL:
    case token_NOT:
    case token_SUB:
        x = malloc(sizeof(*x));
        x->type = ast_EXPR_UNARY;
        x->unary.op = p->tok;
        next(p);
        x->unary.x = parse_unary_expr(p); // TODO: parse_cast_expr
        break;
    default:
        x = parse_postfix_expr(p);
        break;
    }
    return x;
}

static expr_t *parse_expr(parser_t *p) {
    expr_t *x = parse_unary_expr(p);
    expr_t *y;
    switch (p->tok) {
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
        y->binary.op = p->tok;
        next(p);
        y->binary.y = parse_expr(p);
        x = y;
        break;
    }
    return x;
}

static field_t *parse_field(parser_t *p) {
    expr_t *type = parse_type(p);
    expr_t *name = parse_declarator(p, &type);
    field_t field = {
        .type = type,
        .name = name,
    };
    return dup(&field);
}

static expr_t *parse_struct_or_union(parser_t *p) {
    int keyword = p->tok;
    expr_t *name = NULL;
    expect(p, keyword);
    if (p->tok == token_IDENT) {
        name = parse_ident(p);
    }
    field_t **fields = NULL;
    if (accept(p, token_LBRACE)) {
        slice_t slice = {.size = sizeof(field_t *)};
        for (;;) {
            field_t *field = parse_field(p);
            expect(p, token_SEMICOLON);
            slice = append(slice, &field);
            if (p->tok == token_RBRACE) {
                slice = append(slice, &nil_ptr);
                break;
            }
        }
        fields = slice.array;
        expect(p, token_RBRACE);
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
    return dup(&x);
}

static expr_t *parse_enum(parser_t *p) {
    expr_t *name = NULL;
    slice_t enums = {.size=sizeof(enumerator_t *)};
    int cap = 0;
    int len = 0;
    expect(p, token_ENUM);
    if (p->tok == token_IDENT) {
        name = parse_ident(p);
    }
    expect(p, token_LBRACE);
    for (;;) {
        enumerator_t *enumerator = malloc(sizeof(*enumerator));
        enumerator->name = parse_ident(p);
        enumerator->value = NULL;
        if (p->tok == token_ASSIGN) {
            next(p);
            enumerator->value = parse_const_expr(p);
        }
        expect(p, token_COMMA);
        enums = append(enums, &enumerator);
        if (p->tok == token_RBRACE) {
            enumerator = NULL;
            enums = append(enums, &enumerator);
            break;
        }
    }
    expect(p, token_RBRACE);
    expr_t *x = malloc(sizeof(*x));
    x->type = ast_TYPE_ENUM;
    x->enum_.name = name;
    x->enum_.enumerators = enums.array;
    return x;
}

static expr_t *parse_type(parser_t *p) {
    expr_t *x = NULL;
    switch (p->tok) {
    case token_STRUCT:
    case token_UNION:
        x = parse_struct_or_union(p);
        break;
    case token_ENUM:
        x = parse_enum(p);
        break;
    case token_IDENT:
        x = parse_ident(p);
        break;
    default:
        error(p, "expected type, got %s", p->lit ? p->lit : token_string(p->tok));
        break;
    }
    return x;
}

static stmt_t *parse_compound_stmt(parser_t *p) {
    stmt_t *stmt = NULL;
    slice_t stmts = {.size = sizeof(stmt_t *)};
    expect(p, token_LBRACE);
    while (p->tok != token_RBRACE) {
        stmt = parse_stmt(p);
        stmts = append(stmts, &stmt);
    }
    stmts = append(stmts, &nil_ptr);
    expect(p, token_RBRACE);
    stmt = malloc(sizeof(*stmt));
    stmt->type = ast_STMT_BLOCK;
    stmt->block.stmts = stmts.array;
    return stmt;
}

static stmt_t *parse_return_stmt(parser_t *p) {
    expr_t *x = NULL;
    expect(p, token_RETURN);
    if (p->tok != token_SEMICOLON)
        x = parse_expr(p);
    expect(p, token_SEMICOLON);
    stmt_t *stmt = malloc(sizeof(*stmt));
    stmt->type = ast_STMT_RETURN;
    stmt->return_.x = x;
    return stmt;
}

static stmt_t *parse_if_stmt(parser_t *p) {
    expect(p, token_IF);
    expect(p, token_LPAREN);
    expr_t *cond = parse_expr(p);
    expect(p, token_RPAREN);
    stmt_t *body = parse_compound_stmt(p);
    stmt_t *else_ = NULL;
    if (accept(p, token_ELSE)) {
        else_ = parse_stmt(p);
    }
    stmt_t stmt = {
        .type = ast_STMT_IF,
        .if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return dup(&stmt);
}

static stmt_t *parse_while_stmt(parser_t *p) {
    expect(p, token_WHILE);
    expect(p, token_LPAREN);
    expr_t *cond = parse_expr(p);
    expect(p, token_RPAREN);
    stmt_t *body = parse_stmt(p);
    stmt_t stmt = {
        .type = ast_STMT_WHILE,
        .while_ = {
            .cond = cond,
            .body = body,
        },
    };
    return dup(&stmt);
}

static stmt_t *parse_switch_stmt(parser_t *p) {
    expect(p, token_SWITCH);
    expect(p, token_LPAREN);
    expr_t *tag = parse_expr(p);
    expect(p, token_RPAREN);
    stmt_t *body = parse_compound_stmt(p);
    stmt_t stmt = {
        .type = ast_STMT_SWITCH,
        .switch_ = {
            .tag = tag,
            .body = body,
        },
    };
    return dup(&stmt);
}

static stmt_t *parse_case_stmt(parser_t *p) {
    expr_t *expr = NULL;
    if (accept(p, token_CASE)) {
        expr = parse_expr(p);
    } else {
        expect(p, token_DEFAULT);
    }
    expect(p, token_COLON);
    stmt_t stmt = {
        .type = ast_STMT_CASE,
        .case_ = {
            .expr = expr,
        },
    };
    return dup(&stmt);
}

static stmt_t *parse_stmt(parser_t *p) {
    if (is_type(p)) {
        stmt_t stmt = {
            .type = ast_STMT_DECL,
            .decl = parse_decl(p),
        };
        return dup(&stmt);
    }
    switch (p->tok) {
    case token_LBRACE:
        return parse_compound_stmt(p);
    case token_CASE:
    case token_DEFAULT:
        return parse_case_stmt(p);
    case token_IF:
        return parse_if_stmt(p);
    case token_SWITCH:
        return parse_switch_stmt(p);
    case token_WHILE:
        return parse_while_stmt(p);
    case token_RETURN:
        return parse_return_stmt(p);
    }
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .expr.x = NULL,
    };
    if (p->tok != token_SEMICOLON) {
        stmt.expr.x = parse_expr(p);
    }
    expect(p, token_SEMICOLON);
    return dup(&stmt);
}

static expr_t *parse_const_expr(parser_t *p) {
    return parse_expr(p); // TODO change
}

static field_t **parse_parameter_type_list(parser_t *p) {
    if (p->tok == token_IDENT && !strcmp(p->lit, "void")) {
        next(p);
        return NULL;
    }
    slice_t params = {.size=sizeof(field_t *)};
    while (p->tok != token_RPAREN) {
        if (accept(p, token_ELLIPSIS)) {
            field_t *param = malloc(sizeof(*param));
            param->type = NULL;
            param->name = NULL;
            params = append(params, &param);
            break;
        }
        field_t *param = parse_field(p);
        params = append(params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
    }
    if (len(params)) {
        params = append(params, &nil_ptr);
    }
    return params.array;
}

static expr_t *parse_declarator(parser_t *p, expr_t **type_ptr) {
    while (accept(p, token_MUL)) {
        expr_t type = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = *type_ptr,
            }
        };
        *type_ptr = dup(&type);
    }
    expr_t *name = NULL;
    if (p->tok == token_IDENT) {
        name = parse_ident(p);
    }
    switch (p->tok) {
    case token_LBRACK:
        {
            expect(p, token_LBRACK);
            expr_t type = {
                .type = ast_TYPE_ARRAY,
                .array = {
                    .elt = *type_ptr,
                    .len = parse_const_expr(p),
                },
            };
            expect(p, token_RBRACK);
            *type_ptr = dup(&type);
        }
        break;
    case token_LPAREN:
        {
            expect(p, token_LPAREN);
            expr_t type = {
                .type = ast_TYPE_FUNC,
                .func = {
                    .result = *type_ptr,
                    .params = parse_parameter_type_list(p),
                },
            };
            expect(p, token_RPAREN);
            *type_ptr = dup(&type);
        }
        break;
    }
    return name;
}

static spec_t *parse_typedef_spec(parser_t *p) {
    expr_t *type = parse_type(p);
    expr_t *ident = parse_ident(p);
    spec_t spec = {
        .type = ast_SPEC_TYPEDEF,
        .typedef_.name = ident,
        .typedef_.type = type,
    };
    types = append(types, &ident->ident.name);
    return dup(&spec);
}

static decl_t *parse_gen_decl(parser_t *p, int keyword, spec_t *(*f)(parser_t *p)) {
    expect(p, keyword);
    spec_t *spec = f(p);
    expect(p, token_SEMICOLON);
    decl_t *decl = malloc(sizeof(*decl));
    decl->type = ast_DECL_GEN;
    decl->gen.spec = spec;
    return decl;
}

static decl_t *parse_decl(parser_t *p) {
    if (p->tok == token_TYPEDEF) {
        return parse_gen_decl(p, p->tok, parse_typedef_spec);
    }
    switch (p->tok) {
    case token_EXTERN:
    case token_STATIC:
        next(p);
    }
    expr_t *type = parse_type(p);
    expr_t *name = parse_declarator(p, &type);
    expr_t *value = NULL;
    if (p->tok == token_LBRACE) {
        stmt_t *body = parse_compound_stmt(p);
        decl_t decl = {
            .type = ast_DECL_FUNC,
            .func = {
                .type = type,
                .name = name,
                .body = body,
            },
        };
        return dup(&decl);
    }
    if (accept(p, token_ASSIGN)) {
        value = parse_expr(p);
    }
    expect(p, token_SEMICOLON);
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
        .gen.spec = dup(&spec),
    };
    return dup(&decl);
}

static file_t *parse_file(parser_t *p) {
    slice_t decls = {.size = sizeof(decl_t *)};
    append_type("FILE");
    append_type("char");
    append_type("float");
    append_type("int");
    append_type("size_t");
    append_type("va_list");
    append_type("void");
    next(p);
    while (p->tok != token_EOF) {
        decl_t *decl = parse_decl(p);
        decls = append(decls, &decl);
    }
    decls = append(decls, &nil_ptr);
    file_t file = {
        .decls = decls.array,
    };
    return dup(&file);
}

extern file_t *parser_parse_file(char *filename) {
    char *src = ioutil_read_file(filename);
    parser_t parser = {.filename=filename};
    scanner_init(&parser.scanner, src);
    file_t *file = parse_file(&parser);
    free(src);
    return file;
}
