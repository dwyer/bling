#include "bling/parser/parser.h"
#define memdup(src, size) memcpy(malloc((size)), (src), (size))
#define dup(src) (typeof((src)))memdup((src), sizeof(*(src)))
#define alloc(T) (T *)malloc(sizeof(T))

typedef struct {
    scanner_t scanner;
    token_t tok;
    char *lit;
    char *filename;
    scope_t *pkg_scope;
} parser_t;

static void next(parser_t *p);

static void init(parser_t *p, char *filename, char *src) {
    p->filename = filename;
    p->lit = NULL;
    scanner_init(&p->scanner, src);
    next(p);
}

static expr_t *cast_expression(parser_t *p);
static expr_t *expression(parser_t *p);
static expr_t *constant_expression(parser_t *p);
static expr_t *initializer(parser_t *p);

static expr_t *type_specifier(parser_t *p);
static expr_t *struct_or_union_specifier(parser_t *p);
static expr_t *enum_specifier(parser_t *p);
static expr_t *declarator(parser_t *p, expr_t **type_ptr);
static expr_t *pointer(parser_t *p, expr_t *type);
static field_t **parameter_type_list(parser_t *p);
static expr_t *type_name(parser_t *p);
static expr_t *abstract_declarator(parser_t *p, expr_t *type);

static stmt_t *statement(parser_t *p);
static stmt_t *compound_statement(parser_t *p);
static stmt_t *if_statement(parser_t *p);
static stmt_t *switch_statement(parser_t *p);
static stmt_t *while_statement(parser_t *p);
static stmt_t *for_statement(parser_t *p);
static stmt_t *jump_statement(parser_t *p, token_t keyword);
static stmt_t *return_statement(parser_t *p);

static decl_t *parse_decl(parser_t *p);

static field_t *parse_field(parser_t *p);

static const desc_t str_desc = {.size = sizeof(char *)};

static const desc_t expr_desc = {.size = sizeof(expr_t *)};
static const desc_t decl_desc = {.size = sizeof(decl_t *)};
static const desc_t stmt_desc = {.size = sizeof(stmt_t *)};
static const desc_t field_desc = {.size = sizeof(field_t *)};

static void *null_terminated_array(slice_t s) {
    if (len(s) > 0) {
        void *nil_ptr = NULL;
        s = append(s, &nil_ptr);
        return get_ptr(s, 0);
    }
    return NULL;
}

static void declare(parser_t *p, scope_t *s, obj_kind_t kind, expr_t *ident) {
    object_t obj = {
        .kind = kind,
        .name = ident->ident.name,
    };
    scope_insert(s, dup(&obj));
}

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
        {
            object_t *obj = scope_lookup(p->pkg_scope, p->lit);
            return obj && obj->kind == obj_kind_TYPE;
        }
    default:
        return false;
    }
}

static void error(parser_t *p, char *fmt, ...) {
    int line = 1;
    int col = 1;
    int line_offset = 0;
    for (int i = 0; i < p->scanner.offset; i++) {
        if (p->scanner.src[i] == '\n') {
            line++;
            line_offset = i + 1;
            col = 1;
        } else {
            col++;
        }
    }
    fprintf(stderr, "%s:%d:%d: ", p->filename, line, col);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    for (int i = line_offset; ; i++) {
        int ch = p->scanner.src[i];
        fputc(ch, stderr);
        if (!ch || ch == '\n') {
            break;
        }
    }
    fputc('\n', stderr);
    exit(1);
}

static void next(parser_t *p) {
    free(p->lit);
    p->tok = scanner_scan(&p->scanner, &p->lit);
}

static bool accept(parser_t *p, token_t tok0) {
    if (p->tok == tok0) {
        next(p);
        return true;
    }
    return false;
}

static void expect(parser_t *p, token_t tok) {
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

static expr_t *make_ident(char *name) {
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .ident.name = strdup(name),
    };
    return dup(&x);
}

static expr_t *identifier(parser_t *p) {
    expr_t *x = NULL;
    if (p->tok == token_IDENT) {
        x = make_ident(p->lit);
    }
    expect(p, token_IDENT);
    return x;
}

static expr_t *parse_lit(parser_t *p, token_t kind) {
    char *lit = strdup(p->lit);
    expect(p, kind);
    expr_t *x = alloc(expr_t);
    x->type = ast_EXPR_BASIC_LIT;
    x->basic_lit.kind = kind;
    x->basic_lit.value = lit;
    return x;
}

static expr_t *primary_expression(parser_t *p) {
    // primary_expression
    //         : IDENTIFIER
    //         | CONSTANT
    //         | STRING_LITERAL
    //         | '(' expression ')'
    //         ;
    expr_t *x = NULL;
    switch (p->tok) {
    case token_IDENT:
        x = identifier(p);
        break;
    case token_CHAR:
    case token_INT:
    case token_STRING:
        x = parse_lit(p, p->tok);
        break;
    case token_LPAREN:
        expect(p, token_LPAREN);
        x = alloc(expr_t);
        x->type = ast_EXPR_PAREN;
        x->paren.x = expression(p);
        expect(p, token_RPAREN);
        break;
    default:
        error(p, "bad expr: %s: %s", token_string(p->tok), p->lit);
        break;
    }
    return x;
}

static expr_t *postfix_expression(parser_t *p) {
    // postfix_expression
    //         : primary_expression
    //         | postfix_expression '[' expression ']'
    //         | postfix_expression '(' ')'
    //         | postfix_expression '(' argument_expression_list ')'
    //         | postfix_expression '.' IDENTIFIER
    //         | postfix_expression PTR_OP IDENTIFIER
    //         | postfix_expression INC_OP
    //         | postfix_expression DEC_OP
    //         ;
    expr_t *x = primary_expression(p);
    for (;;) {
        expr_t *y;
        switch (p->tok) {
        case token_LBRACK:
            {
                expect(p, token_LBRACK);
                expr_t y = {
                    .type = ast_EXPR_INDEX,
                    .index = {
                        .x = x,
                        .index = expression(p),
                    },
                };
                expect(p, token_RBRACK);
                x = dup(&y);
            }
            break;
        case token_LPAREN:
            {
                slice_t args = {.desc = &expr_desc};
                expect(p, token_LPAREN);
                // argument_expression_list
                //         : assignment_expression
                //         | argument_expression_list ',' assignment_expression
                //         ;
                while (p->tok != token_RPAREN) {
                    expr_t *x = expression(p);
                    args = append(args, &x);
                    if (!accept(p, token_COMMA)) {
                        break;
                    }
                }
                expect(p, token_RPAREN);
                expr_t call = {
                    .type = ast_EXPR_CALL,
                    .call = {
                        .func = x,
                        .args = (expr_t **)null_terminated_array(args),
                    },
                };
                x = dup(&call);
            }
            break;
        case token_ARROW:
        case token_PERIOD:
            {
                token_t tok = p->tok;
                next(p);
                expr_t y = {
                    .type = ast_EXPR_SELECTOR,
                    .selector = {
                        .x = x,
                        .tok = tok,
                        .sel = identifier(p),
                    },
                };
                x = dup(&y);
            }
            break;
        case token_INC:
        case token_DEC:
            {
                expr_t y = {
                    .type = ast_EXPR_INCDEC,
                    .incdec = {
                        .x = x,
                        .tok = p->tok,
                    },
                };
                next(p);
                x = dup(&y);
            }
            break;
        default:
            goto done;
        }
    }
done:
    return x;
}

static expr_t *unary_expression(parser_t *p) {
    // unary_expression
    //         : postfix_expression
    //         | INC_OP unary_expression
    //         | DEC_OP unary_expression
    //         | unary_operator cast_expression
    //         | SIZEOF unary_expression
    //         | SIZEOF '(' type_name ')'
    //         ;
    // unary_operator
    //         : '&'
    //         | '*'
    //         | '+'
    //         | '-'
    //         | '~'
    //         | '!'
    //         ;
    expr_t *x;
    switch (p->tok) {
    case token_ADD:
    case token_AND:
    case token_MUL:
    case token_NOT:
    case token_SUB:
        x = alloc(expr_t);
        x->type = ast_EXPR_UNARY;
        x->unary.op = p->tok;
        next(p);
        x->unary.x = cast_expression(p);
        break;
    case token_SIZEOF:
        x = alloc(expr_t);
        x->type = ast_EXPR_SIZEOF;
        expect(p, token_SIZEOF);
        expect(p, token_LPAREN);
        if (is_type(p)) {
            x->sizeof_.x = type_name(p);
            if (p->tok == token_MUL) {
                declarator(p, &x->sizeof_.x); // TODO assert result == NULL?
            }
        } else {
            x->sizeof_.x = unary_expression(p);
        }
        expect(p, token_RPAREN);
        break;
    default:
        x = postfix_expression(p);
        break;
    }
    return x;
}

static expr_t *cast_expression(parser_t *p) {
    // cast_expression
    //         : unary_expression
    //         | '(' type_name ')' cast_expression
    //         ;
    if (accept(p, token_LPAREN)) {
        if (is_type(p)) {
            expr_t *type = type_name(p);
            expect(p, token_RPAREN);
            expr_t *x = cast_expression(p);
            expr_t y = {
                .type = ast_EXPR_CAST,
                .cast = {
                    .type = type,
                    .expr = x,
                },
            };
            return dup(&y);
        } else {
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .paren = {
                    .x = expression(p),
                },
            };
            expect(p, token_RPAREN);
            return dup(&x);
        }
    }
    return unary_expression(p);
}

static expr_t *_binary_expression(parser_t *p, expr_t *x, token_t op, expr_t *y) {
    expr_t z = {
        .type = ast_EXPR_BINARY,
        .binary = {
            .x = x,
            .op = op,
            .y = y,
        },
    };
    return dup(&z);
}

static expr_t *multiplicative_expression(parser_t *p) {
    // multiplicative_expression
    //         : cast_expression
    //         | multiplicative_expression '*' cast_expression
    //         | multiplicative_expression '/' cast_expression
    //         | multiplicative_expression '%' cast_expression
    //         ;
    expr_t *x = cast_expression(p);
    for (;;) {
        token_t op;
        switch (p->tok) {
        case token_MOD:
        case token_MUL:
        case token_DIV:
            op = p->tok;
            x = _binary_expression(p, x, op, cast_expression(p));
            break;
        default:
            return x;
        }
    }
}

static expr_t *additive_expression(parser_t *p) {
    // additive_expression
    //         : multiplicative_expression
    //         | additive_expression '+' multiplicative_expression
    //         | additive_expression '-' multiplicative_expression
    //         ;
    expr_t *x = multiplicative_expression(p);
    for (;;) {
        token_t op;
        switch (p->tok) {
        case token_ADD:
        case token_SUB:
            op = p->tok;
            next(p);
            x = _binary_expression(p, x, op, multiplicative_expression(p));
            break;
        default:
            return x;
        }
    }
}

static expr_t *shift_expression(parser_t *p) {
    // shift_expression
    //         : additive_expression
    //         | shift_expression LEFT_OP additive_expression
    //         | shift_expression RIGHT_OP additive_expression
    //         ;
    expr_t *x = additive_expression(p);
    for (;;) {
        token_t op;
        switch (p->tok) {
        case token_SHL:
        case token_SHR:
            op = p->tok;
            next(p);
            x = _binary_expression(p, x, op, additive_expression(p));
            break;
        default:
            return x;
        }
    }
}

static expr_t *relational_expression(parser_t *p) {
    // relational_expression
    //         : shift_expression
    //         | relational_expression '<' shift_expression
    //         | relational_expression '>' shift_expression
    //         | relational_expression LE_OP shift_expression
    //         | relational_expression GE_OP shift_expression
    //         ;
    expr_t *x = shift_expression(p);
    for (;;) {
        token_t op;
        switch (p->tok) {
        case token_GT:
        case token_GT_EQUAL:
        case token_LT:
        case token_LT_EQUAL:
            op = p->tok;
            next(p);
            x = _binary_expression(p, x, op, shift_expression(p));
            break;
        default:
            return x;
        }
    }
}

static expr_t *equality_expression(parser_t *p) {
    // equality_expression
    //         : relational_expression
    //         | equality_expression EQ_OP relational_expression
    //         | equality_expression NE_OP relational_expression
    //         ;
    expr_t *x = relational_expression(p);
    for (;;) {
        token_t op;
        switch (p->tok) {
        case token_EQUAL:
        case token_NOT_EQUAL:
            op = p->tok;
            next(p);
            x = _binary_expression(p, x, op, relational_expression(p));
            break;
        default:
            return x;
        }
    }
}

static expr_t *_single_bin_expr(parser_t *p, token_t op, expr_t *(*f)(parser_t *)) {
    expr_t *x = f(p);
    while (accept(p, op)) {
        x = _binary_expression(p, x, op, f(p));
    }
    return x;
}

static expr_t *and_expression(parser_t *p) {
    // and_expression
    //         : equality_expression
    //         | and_expression '&' equality_expression
    //         ;
    return _single_bin_expr(p, token_AND, equality_expression);
}

static expr_t *exclusive_or_expression(parser_t *p) {
    // exclusive_or_expression
    //         : and_expression
    //         | exclusive_or_expression '^' and_expression
    //         ;
    return _single_bin_expr(p, token_XOR, and_expression);
}

static expr_t *inclusive_or_expression(parser_t *p) {
    // inclusive_or_expression
    //         : exclusive_or_expression
    //         | inclusive_or_expression '|' exclusive_or_expression
    //         ;
    return _single_bin_expr(p, token_OR, exclusive_or_expression);
}

static expr_t *logical_and_expression(parser_t *p) {
    // logical_and_expression
    //         : inclusive_or_expression
    //         | logical_and_expression AND_OP inclusive_or_expression
    //         ;
    return _single_bin_expr(p, token_LAND, inclusive_or_expression);
}

static expr_t *logical_or_expression(parser_t *p) {
    // logical_or_expression
    //         : logical_and_expression
    //         | logical_or_expression OR_OP logical_and_expression
    //         ;
    return _single_bin_expr(p, token_LOR, logical_and_expression);
}

static expr_t *conditional_expression(parser_t *p) {
    // conditional_expression
    //         : logical_or_expression
    //         | logical_or_expression '?' expression ':' conditional_expression
    //         ;
    expr_t *x = logical_or_expression(p);
    if (accept(p, token_QUESTION_MARK)) {
        expr_t *consequence = expression(p);
        expect(p, token_COLON);
        expr_t *alternative = conditional_expression(p);
        expr_t conditional = {
            .type = ast_EXPR_COND,
            .conditional = {
                .condition = x,
                .consequence = consequence,
                .alternative = alternative,
            },
        };
        x = dup(&conditional);
    }
    return x;
}

static expr_t *assignment_expression(parser_t *p) {
    // assignment_expression
    //         : conditional_expression
    //         | unary_expression assignment_operator assignment_expression
    //         ;
    expr_t *x = conditional_expression(p);
    switch (p->tok) {
    case token_ADD_ASSIGN:
    case token_ASSIGN:
    case token_DIV_ASSIGN:
    case token_MUL_ASSIGN:
    case token_SUB_ASSIGN:
    case token_XOR_ASSIGN:
        {
            token_t op = p->tok;
            next(p);
            x = _binary_expression(p, x, op, assignment_expression(p));
        }
        break;
    default:
        break;
    }
    return x;
}

static token_t assignment_operator(parser_t *p) {
    // assignment_operator
    //         : '='
    //         | MUL_ASSIGN
    //         | DIV_ASSIGN
    //         | MOD_ASSIGN
    //         | ADD_ASSIGN
    //         | SUB_ASSIGN
    //         | LEFT_ASSIGN
    //         | RIGHT_ASSIGN
    //         | AND_ASSIGN
    //         | XOR_ASSIGN
    //         | OR_ASSIGN
    //         ;
    switch (p->tok) {
    case token_ADD_ASSIGN:
    case token_ASSIGN:
    case token_DIV_ASSIGN:
    case token_MOD_ASSIGN:
    case token_MUL_ASSIGN:
    case token_SHL_ASSIGN:
    case token_SHR_ASSIGN:
    case token_SUB_ASSIGN:
    case token_XOR_ASSIGN:
        return p->tok;
    default:
        return token_ILLEGAL;
    }
}

static expr_t *expression(parser_t *p) {
    // expression : assignment_expression ;
    return assignment_expression(p);
}

static expr_t *constant_expression(parser_t *p) {
    // constant_expression : conditional_expression ;
    return conditional_expression(p);
}

static expr_t *parse_type(parser_t *p) {
    if (accept(p, token_MUL)) {
        expr_t t = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = parse_type(p),
            },
        };
        return dup(&t);
    }
    if (accept(p, token_LBRACK)) {
        expr_t *len = NULL;
        if (p->tok != token_RBRACK) {
            len = constant_expression(p);
        }
        expect(p, token_RBRACK);
        expr_t t = {
            .type = ast_TYPE_ARRAY,
            .array = {
                .elt = parse_type(p),
                .len = len,
            },
        };
        return dup(&t);
    }
    switch (p->tok) {
    case token_ENUM:
        return enum_specifier(p);
    case token_STRUCT:
    case token_UNION:
        return struct_or_union_specifier(p);
    case token_IDENT:
        return identifier(p);
    default:
        error(p, "expected type, got %s", token_string(p->tok));
        return NULL;
    }
}

static field_t **parse_field_list(parser_t *p) {
    slice_t list = make_slice(&field_desc, 0, 0);
    while (p->tok != token_RPAREN) {
        field_t *field = parse_field(p);
        list = append(list, &field);
        if (!accept(p, token_COMMA)) {
            break;
        }
        if (accept(p, token_ELLIPSIS)) {
            field_t *field = alloc(field_t);
            field->type = NULL;
            field->name = NULL;
            list = append(list, &field);
            break;
        }
    }
    return null_terminated_array(list);
}

static decl_t *parse_func_decl(parser_t *p) {
    expect(p, token_FUNC);
    expr_t *name = identifier(p);
    expect(p, token_LPAREN);
    field_t **params = parse_field_list(p);
    expect(p, token_RPAREN);
    expr_t *result = NULL;
    if (p->tok != token_SEMICOLON && p->tok != token_LBRACE) {
        result = parse_type(p);
    }
    stmt_t *body = NULL;
    if (p->tok == token_LBRACE) {
        body = compound_statement(p);
    } else {
        expect(p, token_SEMICOLON);
    }
    expr_t t = {
        .type = ast_TYPE_FUNC,
        .func = {
            .result = result,
            .params = params,
        },
    };
    decl_t d = {
        .type = ast_DECL_FUNC,
        .func = {
            .type = dup(&t),
            .name = name,
            .body = body,
        },
    };
    return dup(&d);
}

static decl_t *parse_type_decl(parser_t *p) {
    expect(p, token_TYPE);
    expr_t *name = identifier(p);
    expr_t *type = parse_type(p);
    expect(p, token_SEMICOLON);
    spec_t s = {
        .type = ast_SPEC_TYPEDEF,
        .typedef_ = {
            .name = name,
            .type = type,
        },
    };
    decl_t d = {
        .type = ast_DECL_GEN,
        .gen = { .spec = dup(&s), },
    };
    declare(p, p->pkg_scope, obj_kind_TYPE, name);
    return dup(&d);
}

static expr_t *get_expr_type(parser_t *p, expr_t *x) {
    switch (x->type) {
    case ast_EXPR_BASIC_LIT:
        switch (x->basic_lit.kind) {
        case token_INT:
            return make_ident("int");
        default:
            error(p, "unknown type");
        }
    default:
        error(p, "unknown type");
        return NULL;
    }
}

static decl_t *parse_var_decl(parser_t *p) {
    expect(p, token_VAR);
    expr_t *name = identifier(p);
    expr_t *type = NULL;
    expr_t *value = NULL;
    if (accept(p, token_ASSIGN)) {
        value = initializer(p);
        type = get_expr_type(p, value);
    } else {
        type = parse_type(p);
        if (accept(p, token_ASSIGN)) {
            value = initializer(p);
        }
    }
    expect(p, token_SEMICOLON);
    spec_t s = {
        .type = ast_SPEC_VALUE,
        .value = {
            .type = type,
            .name = name,
            .value = value,
        },
    };
    decl_t d = {
        .type = ast_DECL_GEN,
        .gen = { .spec = dup(&s), },
    };
    return dup(&d);
}

static decl_t *parse_decl(parser_t *p) {
    switch (p->tok) {
    case token_FUNC:
        return parse_func_decl(p);
    case token_TYPE:
        return parse_type_decl(p);
    case token_VAR:
        return parse_var_decl(p);
    default:
        error(p, "expected declaration, got %s\n", p->lit);
        return NULL;
    }
}

// declaration_specifiers
//         : storage_class_specifier
//         | storage_class_specifier declaration_specifiers
//         | type_specifier
//         | type_specifier declaration_specifiers
//         | type_qualifier
//         | type_qualifier declaration_specifiers
//         ;

// init_declarator_list : init_declarator | init_declarator_list ',' init_declarator ;

// init_declarator : declarator ('=' initializer)? ;

// storage_class_specifier
//         : TYPEDEF
//         | EXTERN
//         | STATIC
//         | AUTO
//         | REGISTER
//         ;

static expr_t *type_specifier(parser_t *p) {
    // type_specifier
    //         : VOID
    //         | CHAR
    //         | SHORT
    //         | INT
    //         | LONG
    //         | FLOAT
    //         | DOUBLE
    //         | SIGNED
    //         | UNSIGNED
    //         | struct_or_union_specifier
    //         | enum_specifier
    //         | TYPE_NAME
    //         ;
    expr_t *x = NULL;
    switch (p->tok) {
    case token_STRUCT:
    case token_UNION:
        x = struct_or_union_specifier(p);
        break;
    case token_ENUM:
        x = enum_specifier(p);
        break;
    case token_IDENT:
        x = identifier(p);
        break;
    default:
        if (p->lit) {
            error(p, "expected type, got %s", p->lit);
        } else {
            error(p, "expected type, got %s", token_string(p->tok));
        }
        break;
    }
    return x;
}

static expr_t *struct_or_union_specifier(parser_t *p) {
    // struct_or_union_specifier
    //         : struct_or_union IDENTIFIER
    //         | struct_or_union IDENTIFIER '{' struct_declaration_list '}'
    //         | struct_or_union '{' struct_declaration_list '}'
    //         ;
    // struct_or_union : STRUCT | UNION ;
    token_t keyword = p->tok;
    expr_t *name = NULL;
    expect(p, keyword);
    if (p->tok == token_IDENT) {
        name = identifier(p);
    }
    field_t **fields = NULL;
    if (accept(p, token_LBRACE)) {
        // struct_declaration_list
        //         : struct_declaration
        //         | struct_declaration_list struct_declaration
        //         ;
        slice_t slice = {.desc = &field_desc};
        for (;;) {
            // struct_declaration : specifier_qualifier_list struct_declarator_list ';' ;
            // struct_declarator_list : declarator | struct_declarator_list ',' declarator ;
            field_t *field = parse_field(p);
            expect(p, token_SEMICOLON);
            slice = append(slice, &field);
            if (p->tok == token_RBRACE) {
                break;
            }
        }
        fields = (field_t **)null_terminated_array(slice);
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

// specifier_qualifier_list
//         : type_specifier specifier_qualifier_list
//         | type_specifier
//         | type_qualifier specifier_qualifier_list
//         | type_qualifier
//         ;

static expr_t *enum_specifier(parser_t *p) {
    // enum_specifier
    //         : ENUM '{' enumerator_list '}'
    //         | ENUM IDENTIFIER '{' enumerator_list '}'
    //         | ENUM IDENTIFIER
    //         ;
    expr_t *name = NULL;
    expect(p, token_ENUM);
    if (p->tok == token_IDENT) {
        name = identifier(p);
    }
    enumerator_t **enums = NULL;
    if (accept(p, token_LBRACE)) {
        // enumerator_list : enumerator | enumerator_list ',' enumerator ;
        desc_t enumerator_desc = {.size=sizeof(enumerator_t *)};
        slice_t list = {.desc=&enumerator_desc};
        for (;;) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            enumerator_t *enumerator = alloc(enumerator_t);
            enumerator->name = identifier(p);
            enumerator->value = NULL;
            if (accept(p, token_ASSIGN)) {
                enumerator->value = constant_expression(p);
            }
            list = append(list, &enumerator);
            if (!accept(p, token_COMMA) || p->tok == token_RBRACE) {
                break;
            }
        }
        enums = (enumerator_t **)null_terminated_array(list);
        expect(p, token_RBRACE);
    }
    expr_t x = {
        .type = ast_TYPE_ENUM,
        .enum_ = {
            .name = name,
            .enumerators = enums,
        },
    };
    return dup(&x);
}

static expr_t *type_qualifier(parser_t *p, expr_t *type) {
    // type_qualifier : CONST | VOLATILE ;
    switch (p->tok) {
    case token_CONST:
        next(p);
        break;
    default:
        break;
    }
    return type;
}


static expr_t *declarator(parser_t *p, expr_t **type_ptr) {
    // declarator : pointer? direct_declarator ;
    if (p->tok == token_MUL) {
        *type_ptr = pointer(p, *type_ptr);
    }
    // direct_declarator
    //         : IDENTIFIER
    //         | '(' declarator ')'
    //         | direct_declarator '[' constant_expression ']'
    //         | direct_declarator '[' ']'
    //         | direct_declarator '(' parameter_type_list ')'
    //         | direct_declarator '(' ')'
    //         ;
    expr_t *name = NULL;
    switch (p->tok) {
    case token_IDENT:
        name = identifier(p);
        break;
    case token_LPAREN:
        expect(p, token_LPAREN);
        name = declarator(p, type_ptr);
        expect(p, token_RPAREN);
        break;
    default:
        break;
    }
    for (;;) {
        if (accept(p, token_LBRACK)) {
            expr_t *len = NULL;
            if (p->tok != token_RBRACK) {
                len = constant_expression(p);
            }
            expr_t type = {
                .type = ast_TYPE_ARRAY,
                .array = {
                    .elt = *type_ptr,
                    .len = len,
                },
            };
            expect(p, token_RBRACK);
            *type_ptr = dup(&type);
        } else if (accept(p, token_LPAREN)) {
            field_t **params = NULL;
            if (p->tok != token_RPAREN) {
                params = parameter_type_list(p);
            }
            expr_t type = {
                .type = ast_TYPE_FUNC,
                .func = {
                    .result = *type_ptr,
                    .params = params,
                },
            };
            expect(p, token_RPAREN);
            *type_ptr = dup(&type);
        } else {
            break;
        }
    }
    return name;
}

static expr_t *pointer(parser_t *p, expr_t *type) {
    // pointer : '*' type_qualifier* pointer? ;
    while (accept(p, token_MUL)) {
        expr_t x = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = type,
            },
        };
        type = dup(&x);
    }
    return type;
}


static field_t **parameter_type_list(parser_t *p) {
    // parameter_type_list : parameter_list (',' ELLIPSIS)? ;
    // parameter_list
    //         : parameter_declaration
    //         | parameter_list ',' parameter_declaration
    //         ;
    slice_t params = {.desc = &field_desc};
    while (p->tok != token_RPAREN) {
        // parameter_declaration
        //         : declaration_specifiers (declarator|abstract_declarator)?
        //         ;
        field_t *param = parse_field(p);
        params = append(params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
        if (accept(p, token_ELLIPSIS)) {
            field_t *param = alloc(field_t);
            param->type = NULL;
            param->name = NULL;
            params = append(params, &param);
            break;
        }
    }
    return (field_t **)null_terminated_array(params);
}

static expr_t *type_name(parser_t *p) {
    // type_name : specifier_qualifier_list abstract_declarator? ;
    expr_t *type = type_specifier(p);
    type = abstract_declarator(p, type);
    expr_t x = {
        .type = ast_TYPE_NAME,
        .type_name = {
            .type = type,
        },
    };
    return dup(&x);
}

static expr_t *abstract_declarator(parser_t *p, expr_t *type) {
    // abstract_declarator
    //         : pointer
    //         | pointer direct_abstract_declarator
    //         | direct_abstract_declarator
    //         ;
    if (p->tok == token_MUL) {
        type = pointer(p, type);
    }
    // direct_abstract_declarator
    //         : '(' abstract_declarator ')'
    //         | '(' parameter_type_list ')'
    //         | '(' ')'
    //         | '[' constant_expression ']'
    //         | '[' ']'
    //         | direct_abstract_declarator '[' constant_expression ']'
    //         | direct_abstract_declarator '[' ']'
    //         | direct_abstract_declarator '(' parameter_type_list ')'
    //         | direct_abstract_declarator '(' ')'
    //         ;
    for (;;) {
        if (accept(p, token_LPAREN)) {
            expect(p, token_RPAREN);
        }
        break;
    }
    return type;
}

static expr_t *initializer(parser_t *p) {
    // initializer
    //         : assignment_expression
    //         | '{' initializer_list '}'
    //         | '{' initializer_list ',' '}'
    //         ;
    if (!accept(p, token_LBRACE)) {
        return assignment_expression(p);
    }
    // initializer_list
    //         : designation? initializer
    //         | initializer_list ',' designation? initializer
    //         ;
    slice_t list = {.desc = &expr_desc};
    while (p->tok != token_RBRACE && p->tok != token_EOF) {
        expr_t *value = NULL;
        // designation : designator_list '=' ;
        // designator_list
        //         : designator
        //         | designator_list designator
        //         ;
        // designator : '.' identifier
        if (accept(p, token_PERIOD)) {
            expr_t *key = identifier(p);
            expect(p, token_ASSIGN);
            expr_t x = {
                .type = ast_EXPR_KEY_VALUE,
                .key_value = {
                    .key = key,
                    .value = initializer(p),
                },
            };
            value = dup(&x);
        } else {
            value = initializer(p);
        }
        list = append(list, &value);
        if (!accept(p, token_COMMA)) {
            break;
        }
    }
    expect(p, token_RBRACE);
    expr_t expr = {
        .type = ast_EXPR_COMPOUND,
        .compound = {
            .list = (expr_t **)null_terminated_array(list),
        },
    };
    return dup(&expr);
}

static stmt_t *statement(parser_t *p) {
    // statement
    //         : declaration
    //         | labeled_statement
    //         | compound_statement
    //         | expression_statement
    //         | if_statement
    //         | switch_statement
    //         | while_statement
    //         | for_statement
    //         | jump_statement
    //         ;
    switch (p->tok) {
    case token_FUNC:
    case token_VAR:
        {
            stmt_t s = {
                .type = ast_STMT_DECL,
                .decl = parse_decl(p),
            };
            return dup(&s);
        }
    default:
        break;
    }
    switch (p->tok) {
    case token_LBRACE:
        return compound_statement(p);
    case token_IF:
        return if_statement(p);
    case token_SWITCH:
        return switch_statement(p);
    case token_BREAK:
    case token_CONTINUE:
    case token_GOTO:
        return jump_statement(p, p->tok);
    case token_FOR:
        return for_statement(p);
    case token_WHILE:
        return while_statement(p);
    case token_RETURN:
        return return_statement(p);
    default:
        break;
    }
    expr_t *x = NULL;
    if (p->tok != token_SEMICOLON) {
        x = expression(p);
    }
    // labeled_statement : IDENTIFIER ':' statement ;
    if (x && x->type == ast_EXPR_IDENT) {
        if (accept(p, token_COLON)) {
            stmt_t stmt = {
                .type = ast_STMT_LABEL,
                .label = {
                    .label = x,
                },
            };
            return dup(&stmt);
        }
    }
    // expression_statement : expression? ';' ;
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .expr = {.x = x},
    };
    expect(p, token_SEMICOLON);
    return dup(&stmt);
}

static stmt_t *compound_statement(parser_t *p) {
    // compound_statement : '{' statement* '}' ;
    stmt_t *stmt = NULL;
    slice_t stmts = {.desc = &stmt_desc};
    expect(p, token_LBRACE);
    while (p->tok != token_RBRACE) {
        stmt = statement(p);
        stmts = append(stmts, &stmt);
    }
    expect(p, token_RBRACE);
    stmt = alloc(stmt_t);
    stmt->type = ast_STMT_BLOCK;
    stmt->block.stmts = (stmt_t **)null_terminated_array(stmts);
    return stmt;
}

static stmt_t *if_statement(parser_t *p) {
    // if_statement
    //         : IF '(' expression ')' compound_statement
    //         | IF '(' expression ')' compound_statement ELSE compound_statement
    //         | IF '(' expression ')' compound_statement ELSE if_statement
    //         ;
    expect(p, token_IF);
    expect(p, token_LPAREN);
    expr_t *cond = expression(p);
    expect(p, token_RPAREN);
    stmt_t *body = compound_statement(p);
    stmt_t *else_ = NULL;
    if (accept(p, token_ELSE)) {
        if (p->tok == token_IF) {
            else_ = if_statement(p);
        } else {
            else_ = compound_statement(p);
        }
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

static stmt_t *switch_statement(parser_t *p) {
    // switch_statement | SWITCH '(' expression ')' '{' case_statement* '}';
    expect(p, token_SWITCH);
    expect(p, token_LPAREN);
    expr_t *tag = expression(p);
    expect(p, token_RPAREN);
    expect(p, token_LBRACE);
    slice_t clauses = {.desc = &stmt_desc};
    while (p->tok == token_CASE || p->tok == token_DEFAULT) {
        // case_statement
        //         | CASE constant_expression ':' statement+
        //         | DEFAULT ':' statement+
        //         ;
        expr_t *expr = NULL;
        if (accept(p, token_CASE)) {
            expr = constant_expression(p);
        } else {
            expect(p, token_DEFAULT);
        }
        expect(p, token_COLON);
        slice_t stmts = {.desc = &stmt_desc};
        bool loop = true;
        while (loop) {
            switch (p->tok) {
            case token_CASE:
            case token_DEFAULT:
            case token_RBRACE:
                loop = false;
                break;
            default:
                break;
            }
            if (loop) {
                stmt_t *stmt = statement(p);
                stmts = append(stmts, &stmt);
            }
        }
        stmt_t stmt = {
            .type = ast_STMT_CASE,
            .case_ = {
                .expr = expr,
                .stmts = (stmt_t **)null_terminated_array(stmts),
            },
        };
        stmt_t *clause = dup(&stmt);
        clauses = append(clauses, &clause);
    }
    expect(p, token_RBRACE);
    stmt_t stmt = {
        .type = ast_STMT_SWITCH,
        .switch_ = {
            .tag = tag,
            .stmts = (stmt_t **)null_terminated_array(clauses),
        },
    };
    return dup(&stmt);
}

static stmt_t *while_statement(parser_t *p) {
    // while_statement : WHILE '(' expression ')' compound_statement ;
    expect(p, token_WHILE);
    expect(p, token_LPAREN);
    expr_t *cond = expression(p);
    expect(p, token_RPAREN);
    stmt_t *body = compound_statement(p);
    stmt_t stmt = {
        .type = ast_STMT_WHILE,
        .while_ = {
            .cond = cond,
            .body = body,
        },
    };
    return dup(&stmt);
}

static stmt_t *for_statement(parser_t *p) {
    // for_statement
    //         : FOR '(' declaration? ';' expression? ';' expression? ')' compound_statement
    //         ;
    expect(p, token_FOR);
    expect(p, token_LPAREN);
    stmt_t *init = NULL;
    if (!accept(p, token_SEMICOLON)) {
        init = statement(p);
    }
    expr_t *cond = NULL;
    if (p->tok != token_SEMICOLON) {
        cond = expression(p);
    }
    expect(p, token_SEMICOLON);
    expr_t *post = NULL;
    if (p->tok != token_RPAREN) {
        post = expression(p);
    }
    expect(p, token_RPAREN);
    stmt_t *body = statement(p);
    stmt_t stmt = {
        .type = ast_STMT_FOR,
        .for_ = {
            .init = init,
            .cond = cond,
            .post = post,
            .body = body,
        },
    };
    return dup(&stmt);
}

static stmt_t *jump_statement(parser_t *p, token_t keyword) {
    // jump_statement
    //         : GOTO IDENTIFIER ';'
    //         | CONTINUE ';'
    //         | BREAK ';'
    //         ;
    expect(p, keyword);
    expr_t *label = NULL;
    if (keyword == token_GOTO) {
        label = identifier(p);
    }
    expect(p, token_SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_JUMP,
        .jump = {
            .keyword = keyword,
            .label = label,
        },
    };
    return dup(&stmt);
}

static stmt_t *return_statement(parser_t *p) {
    // return_statement | RETURN expression? ';' ;
    expr_t *x = NULL;
    expect(p, token_RETURN);
    if (p->tok != token_SEMICOLON) {
        x = expression(p);
    }
    expect(p, token_SEMICOLON);
    stmt_t *stmt = alloc(stmt_t);
    stmt->type = ast_STMT_RETURN;
    stmt->return_.x = x;
    return stmt;
}

static field_t *parse_field(parser_t *p) {
    expr_t *name = identifier(p);
    expr_t *type = parse_type(p);
    field_t field = {
        .type = type,
        .name = name,
    };
    return dup(&field);
}

static file_t *parse_file(parser_t *p) {
    expect(p, token_PACKAGE);
    expr_t *name = identifier(p);
    expect(p, token_SEMICOLON);
    slice_t imports = make_slice(&expr_desc, 0, 0);
    while (accept(p, token_IMPORT)) {
        expr_t *path = parse_lit(p, token_STRING);
        imports = append(imports, &path);
        expect(p, token_SEMICOLON);
    }
    slice_t decls = {.desc = &decl_desc};
    while (p->tok != token_EOF) {
        decl_t *decl = parse_decl(p);
        decls = append(decls, &decl);
    }
    file_t file = {
        .name = name,
        .imports = null_terminated_array(imports),
        .decls = (decl_t **)null_terminated_array(decls),
    };
    return dup(&file);
}

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope) {
    char *src = ioutil_read_file(filename);
    parser_t p;
    init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    file_t *file = parse_file(&p);
    free(src);
    object_t *obj = NULL;
    for (map_iter_t it = map_iter(&pkg_scope->objects); map_iter_next(&it, NULL, &obj);) {
        fprintf(stderr, "%s: %d\n", obj->name, obj->kind);
    }
    return file;
}
