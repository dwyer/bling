#include "subc/parser/parser.h"
#define dup(src) (typeof(src))memcpy(malloc(sizeof(*src)), src, sizeof(*src))

typedef struct {
    scanner_t scanner;
    token_t tok;
    char *lit;
    char *filename;
    scope_t *top_scope;
    scope_t *pkg_scope;
} parser_t;

static void next(parser_t *p);

static void *slice_to_nil_array(slice_t s) {
    if (len(s)) {
        void *nil = NULL;
        s = append(s, &nil);
    }
    return s.array;
}

static void init(parser_t *p, char *filename, char *src) {
    p->filename = filename;
    p->lit = NULL;
    scanner_init(&p->scanner, src);
    next(p);
}

static void open_scope(parser_t *p) {
    p->top_scope = scope_new(p->top_scope);
}

static void close_scope(parser_t *p) {
    scope_t *top = p->top_scope;
    p->top_scope = top->outer;
    scope_deinit(top);
    free(top);
}

static void declare(parser_t *p, scope_t *s, obj_kind_t kind, char *name) {
    object_t obj = {
        .kind = kind,
        .name = name,
    };
    scope_insert(s, memcpy(malloc(sizeof(obj)), &obj, sizeof(obj)));
}

static expr_t *cast_expression(parser_t *p);
static expr_t *expression(parser_t *p);
static expr_t *constant_expression(parser_t *p);
static expr_t *initializer(parser_t *p);

static expr_t *type_specifier(parser_t *p);
static expr_t *struct_or_union_specifier(parser_t *p);
static expr_t *enum_specifier(parser_t *p);
static expr_t *pointer(parser_t *p, expr_t *type);
static field_t **parameter_type_list(parser_t *p);
static expr_t *type_name(parser_t *p);

static expr_t *declarator(parser_t *p, expr_t **type_ptr);
static field_t *abstract_declarator(parser_t *p, expr_t *type);

static stmt_t *statement(parser_t *p);
static stmt_t *compound_statement(parser_t *p);

static expr_t *specifier_qualifier_list(parser_t *p);
static expr_t *declaration_specifiers(parser_t *p, bool is_top);
static decl_t *declaration(parser_t *p, bool is_external);

static field_t *parse_field(parser_t *p);

static const desc_t str_desc = {.size = sizeof(char *)};

static const desc_t expr_desc = {.size = sizeof(expr_t *)};
static const desc_t decl_desc = {.size = sizeof(decl_t *)};
static const desc_t stmt_desc = {.size = sizeof(stmt_t *)};
static const desc_t field_desc = {.size = sizeof(field_t *)};

static bool is_type(parser_t *p) {
    switch (p->tok) {
    case token_CONST:
    case token_ENUM:
    case token_EXTERN:
    case token_SIGNED:
    case token_STATIC:
    case token_STRUCT:
    case token_UNION:
    case token_UNSIGNED:
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
    panic("panicing");
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
    }
    next(p);
}

static expr_t *identifier(parser_t *p) {
    expr_t *expr = NULL;
    switch (p->tok) {
    case token_IDENT:
        expr = malloc(sizeof(expr_t));
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

static expr_t *primary_expression(parser_t *p) {
    // primary_expression
    //         : IDENTIFIER
    //         | CONSTANT
    //         | STRING_LITERAL
    //         | '(' expression ')'
    //         ;
    switch (p->tok) {
    case token_IDENT:
        return identifier(p);
    case token_CHAR:
    case token_FLOAT:
    case token_INT:
    case token_STRING:
        {
            expr_t x = {
                .type = ast_EXPR_BASIC_LIT,
                .basic_lit = {
                    .kind = p->tok,
                    .value = strdup(p->lit),
                },
            };
            next(p);
            return memcpy(malloc(sizeof(x)), &x, sizeof(x));
        }
    case token_LPAREN:
        {
            expect(p, token_LPAREN);
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .paren = {
                    .x = expression(p),
                },
            };
            expect(p, token_RPAREN);
            return memcpy(malloc(sizeof(x)), &x, sizeof(x));
        }
    default:
        error(p, "bad expr: %s: %s", token_string(p->tok), p->lit);
        return NULL;
    }
}

static expr_t *postfix_expression(parser_t *p, expr_t *x) {
    // postfix_expression
    //         : primary_expression
    //         | postfix_expression '[' expression ']'
    //         | postfix_expression '(' argument_expression_list? ')'
    //         | postfix_expression '.' IDENTIFIER
    //         | postfix_expression '*' IDENTIFIER
    //         | postfix_expression '++'
    //         | postfix_expression '--'
    //         ;
    if (x == NULL) {
        x = primary_expression(p);
    }
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
                x = memcpy(malloc(sizeof(y)), &y, sizeof(y));
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
                        .args = slice_to_nil_array(args),
                    },
                };
                x = memcpy(malloc(sizeof(call)), &call, sizeof(call));
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
                x = memcpy(malloc(sizeof(y)), &y, sizeof(y));
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
                x = memcpy(malloc(sizeof(y)), &y, sizeof(y));
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
        x = malloc(sizeof(expr_t));
        x->type = ast_EXPR_UNARY;
        x->unary.op = p->tok;
        next(p);
        x->unary.x = cast_expression(p);
        break;
    case token_SIZEOF:
        x = malloc(sizeof(expr_t));
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
    case token_DEC:
    case token_INC:
        error(p, "unary `%s` not supported in subc", token_string(p->tok));
        break;
    default:
        x = postfix_expression(p, NULL);
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
            return memcpy(malloc(sizeof(y)), &y, sizeof(y));
        } else {
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .paren = {
                    .x = expression(p),
                },
            };
            expect(p, token_RPAREN);
            return postfix_expression(p, memcpy(malloc(sizeof(x)), &x, sizeof(x)));
        }
    }
    return unary_expression(p);
}

static expr_t *_binary_expression(expr_t *x, token_t op, expr_t *y) {
    expr_t z = {
        .type = ast_EXPR_BINARY,
        .binary = {
            .x = x,
            .op = op,
            .y = y,
        },
    };
    return memcpy(malloc(sizeof(z)), &z, sizeof(z));
}

static expr_t *binary_expression(parser_t *p, int prec1) {
    expr_t *x = cast_expression(p);
    for (;;) {
        token_t op = p->tok;
        int oprec = token_precedence(op);
        if (oprec < prec1) {
            return x;
        }
        expect(p, op);
        x = _binary_expression(x, op, binary_expression(p, oprec + 1));
    }
}

static expr_t *ternary_expression(parser_t *p) {
    // ternary_expression
    //         : binary_expression
    //         | binary_expression '?' expression ':' ternary_expression
    //         ;
    expr_t *x = binary_expression(p, token_lowest_prec + 1);
    if (accept(p, token_QUESTION_MARK)) {
        expr_t *consequence = expression(p);
        expect(p, token_COLON);
        expr_t *alternative = ternary_expression(p);
        expr_t conditional = {
            .type = ast_EXPR_COND,
            .conditional = {
                .condition = x,
                .consequence = consequence,
                .alternative = alternative,
            },
        };
        x = memcpy(malloc(sizeof(conditional)), &conditional, sizeof(conditional));
    }
    return x;
}

static expr_t *assignment_expression(parser_t *p) {
    // assignment_expression
    //         : ternary_expression
    //         | unary_expression assignment_operator assignment_expression
    //         ;
    expr_t *x = ternary_expression(p);
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
            x = _binary_expression(x, op, assignment_expression(p));
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
    // expression
    //         : assignment_expression
    //         | expression ',' assignment_expression
    //         ;
    return assignment_expression(p);
}

static expr_t *constant_expression(parser_t *p) {
    // constant_expression : ternary_expression ;
    return ternary_expression(p);
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
        // struct_declaration
        //         : specifier_qualifier_list struct_declarator_list ';'
        //         ;
        slice_t slice = {.desc = &field_desc};
        for (;;) {
            // struct_declarator_list
            //         : struct_declarator
            //         | struct_declarator_list ',' struct_declarator
            //         ;
            // struct_declarator
            //         : declarator
            //         | ':' constant_expression
            //         | declarator ':' constant_expression
            //         ;
            expr_t *type = specifier_qualifier_list(p);
            expr_t *name = declarator(p, &type);
            field_t f = {
                .type = type,
                .name = name,
            };
            expect(p, token_SEMICOLON);
            field_t *field = memcpy(malloc(sizeof(f)), &f, sizeof(f));
            slice = append(slice, &field);
            if (p->tok == token_RBRACE) {
                break;
            }
        }
        expect(p, token_RBRACE);
        fields = slice_to_nil_array(slice);
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
    return memcpy(malloc(sizeof(x)), &x, sizeof(x));
}

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
            enumerator_t *enumerator = malloc(sizeof(enumerator_t));
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
        enums = slice_to_nil_array(list);
        expect(p, token_RBRACE);
    }
    expr_t x = {
        .type = ast_TYPE_ENUM,
        .enum_ = {
            .name = name,
            .enumerators = enums,
        },
    };
    return memcpy(malloc(sizeof(x)), &x, sizeof(x));
}

static expr_t *declarator(parser_t *p, expr_t **type_ptr) {
    // declarator : pointer? direct_declarator ;
    if (p->tok == token_MUL) {
        *type_ptr = pointer(p, *type_ptr);
    }
    // direct_declarator
    //         : IDENTIFIER
    //         | '(' declarator ')'
    //         | direct_declarator '[' constant_expression? ']'
    //         | direct_declarator '(' parameter_type_list? ')'
    //         ;
    expr_t *name = NULL;
    bool is_ptr = false;
    switch (p->tok) {
    case token_IDENT:
        name = identifier(p);
        break;
    case token_LPAREN:
        expect(p, token_LPAREN);
        is_ptr = accept(p, token_MUL);
        if (!is_ptr || p->tok == token_IDENT) {
            name = identifier(p);
        }
        expect(p, token_RPAREN);
        break;
    default:
        break;
    }
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
        *type_ptr = memcpy(malloc(sizeof(type)), &type, sizeof(type));
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
        *type_ptr = memcpy(malloc(sizeof(type)), &type, sizeof(type));
    }
    if (is_ptr) {
        expr_t type = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = *type_ptr,
            }
        };
        *type_ptr = memcpy(malloc(sizeof(type)), &type, sizeof(type));
    }
    return name;
}

static expr_t *pointer(parser_t *p, expr_t *type) {
    // pointer : '*' type_qualifier_list? pointer? ;
    while (accept(p, token_MUL)) {
        // type_qualifier_list : type_qualifier+ ;
        expr_t x = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = type,
            },
        };
        type = memcpy(malloc(sizeof(x)), &x, sizeof(x));
    }
    return type;
}

static field_t **parameter_type_list(parser_t *p) {
    // parameter_type_list
    //         : parameter_list
    //         | parameter_list ',' '...'
    //         ;
    // parameter_list
    //         : parameter_declaration
    //         | parameter_list ',' parameter_declaration
    //         ;
    slice_t params = {.desc = &field_desc};
    while (p->tok != token_RPAREN) {
        // parameter_declaration
        //         : declaration_specifiers (declarator | abstract_declarator)?
        //         ;
        field_t *param = parse_field(p);
        params = append(params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
        if (accept(p, token_ELLIPSIS)) {
            field_t *param = malloc(sizeof(field_t));
            param->type = NULL;
            param->name = NULL;
            params = append(params, &param);
            break;
        }
    }
    return slice_to_nil_array(params);
}

static expr_t *type_name(parser_t *p) {
    // type_name
    //         : specifier_qualifier_list
    //         | specifier_qualifier_list abstract_declarator
    //         ;
    expr_t *type = specifier_qualifier_list(p);
    field_t *declarator = abstract_declarator(p, type);
    type = declarator->type;
    expr_t x = {
        .type = ast_TYPE_NAME,
        .type_name = {
            .type = type,
        },
    };
    return memcpy(malloc(sizeof(x)), &x, sizeof(x));
}

static field_t *abstract_declarator(parser_t *p, expr_t *type) {
    // abstract_declarator
    //         : pointer? direct_abstract_declarator?
    //         ;
    if (p->tok == token_MUL) {
        type = pointer(p, type);
    }
    // direct_abstract_declarator
    //         : '(' abstract_declarator ')'
    //         | direct_abstract_declarator? '[' constant_expression? ']'
    //         | direct_abstract_declarator? '(' parameter_type_list? ')'
    //         ;
    bool is_ptr = false;
    if (accept(p, token_LPAREN)) {
        expect(p, token_MUL);
        expect(p, token_RPAREN);
        is_ptr = true;
    }
    for (;;) {
        if (accept(p, token_LBRACK)) {
            expr_t *len = NULL;
            if (p->tok != token_RBRACK) {
                len = constant_expression(p);
            }
            expr_t t = {
                .type = ast_TYPE_ARRAY,
                .array = {
                    .elt = type,
                    .len = len,
                },
            };
            expect(p, token_RBRACK);
            type = memcpy(malloc(sizeof(t)), &t, sizeof(t));
        } else if (accept(p, token_LPAREN)) {
            field_t **params = NULL;
            if (p->tok != token_RPAREN) {
                params = parameter_type_list(p);
            }
            expr_t t = {
                .type = ast_TYPE_FUNC,
                .func = {
                    .result = type,
                    .params = params,
                },
            };
            expect(p, token_RPAREN);
            type = memcpy(malloc(sizeof(t)), &t, sizeof(t));
        } else {
            break;
        }
    }
    if (is_ptr) {
        expr_t tmp = {
            .type = ast_TYPE_PTR,
            .ptr = {
                .type = type,
            },
        };
        type = memcpy(malloc(sizeof(tmp)), &tmp, sizeof(tmp));
    }
    field_t declarator = {
        .type = type,
    };
    return memcpy(malloc(sizeof(declarator)), &declarator, sizeof(declarator));
}

static expr_t *initializer(parser_t *p) {
    // initializer
    //         : assignment_expression
    //         | '{' initializer_list ','? '}'
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
        // designator_list : designator+ ;
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
            value = memcpy(malloc(sizeof(x)), &x, sizeof(x));
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
            .list = slice_to_nil_array(list),
        },
    };
    return memcpy(malloc(sizeof(expr)), &expr, sizeof(expr));
}

static stmt_t *statement(parser_t *p) {
    // statement
    //         : declaration
    //         | compound_statement
    //         | if_statement
    //         | switch_statement
    //         | while_statement
    //         | for_statement
    //         | jump_statement
    //         | labeled_statement
    //         | expression_statement
    //         ;

    if (is_type(p)) {
        stmt_t stmt = {
            .type = ast_STMT_DECL,
            .decl = declaration(p, false),
        };
        return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
    }

    if (accept(p, token_FOR)) {
        // for_statement
        //         | FOR '(' simple_statement? ';' expression? ';' expression? ')' 
        //              compound_statement ;
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
        stmt_t *body = compound_statement(p);
        stmt_t stmt = {
            .type = ast_STMT_ITER,
            .iter = {
                .kind = token_FOR,
                .init = init,
                .cond = cond,
                .post = post,
                .body = body,
            },
        };
        return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
    }

    if (accept(p, token_IF)) {
        // if_statement
        //         : IF '(' expression ')' compound_statement
        //         | IF '(' expression ')' compound_statement ELSE compound_statement
        //         | IF '(' expression ')' compound_statement ELSE if_statement
        //         ;
        expect(p, token_LPAREN);
        expr_t *cond = expression(p);
        expect(p, token_RPAREN);
        if (p->tok != token_LBRACE) {
            error(p, "`if` must be followed by a compound_statement");
        }
        stmt_t *body = compound_statement(p);
        stmt_t *else_ = NULL;
        if (accept(p, token_ELSE)) {
            if (p->tok == token_IF) {
                else_ = statement(p);
            } else if (p->tok == token_LBRACE) {
                else_ = compound_statement(p);
            } else {
                error(p, "`else` must be followed by an if_statement or compound_statement");
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
        return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
    }

    if (accept(p, token_RETURN)) {
        // return_statement
        //         | RETURN expression? ';'
        //         ;
        expr_t *x = NULL;
        if (p->tok != token_SEMICOLON) {
            x = expression(p);
        }
        expect(p, token_SEMICOLON);
        stmt_t *stmt = malloc(sizeof(stmt_t));
        stmt->type = ast_STMT_RETURN;
        stmt->return_.x = x;
        return stmt;
    }

    if (accept(p, token_SWITCH)) {
        // switch_statement | SWITCH '(' expression ')' case_statement* ;
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
                expr = expression(p);
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
                    .stmts = slice_to_nil_array(stmts),
                },
            };
            stmt_t *clause = memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
            clauses = append(clauses, &clause);
        }
        expect(p, token_RBRACE);
        stmt_t stmt = {
            .type = ast_STMT_SWITCH,
            .switch_ = {
                .tag = tag,
                .stmts = slice_to_nil_array(clauses),
            },
        };
        return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
    }

    if (accept(p, token_WHILE)) {
        // while_statement : WHILE '(' expression ')' statement ;
        expect(p, token_LPAREN);
        expr_t *cond = expression(p);
        expect(p, token_RPAREN);
        stmt_t *body = statement(p);
        stmt_t stmt = {
            .type = ast_STMT_ITER,
            .iter = {
                .kind = token_WHILE,
                .cond = cond,
                .body = body,
            },
        };
        return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
    }

    switch (p->tok) {
    case token_BREAK:
    case token_CONTINUE:
    case token_GOTO:
        // jump_statement
        //         : GOTO IDENTIFIER ';'
        //         | CONTINUE ';'
        //         | BREAK ';'
        //         ;
        {
            token_t keyword = p->tok;
            next(p);
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
            return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
        }
    case token_LBRACE:
        return compound_statement(p);
    default:
        break;
    }

    expr_t *x = NULL;
    if (p->tok != token_SEMICOLON) {
        x = expression(p);
    }
    // labeled_statement
    //         : IDENTIFIER ':' statement
    //         | CASE constant_expression ':' statement
    //         | DEFAULT ':' statement
    //         ;
    if (x && x->type == ast_EXPR_IDENT) {
        if (accept(p, token_COLON)) {
            stmt_t stmt = {
                .type = ast_STMT_LABEL,
                .label = {
                    .label = x,
                },
            };
            return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
        }
    }
    // expression_statement : expression? ';' ;
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .expr = {.x = x},
    };
    expect(p, token_SEMICOLON);
    return memcpy(malloc(sizeof(stmt)), &stmt, sizeof(stmt));
}

static stmt_t *compound_statement(parser_t *p) {
    // compound_statement : '{' statement_list? '}' ;
    stmt_t *stmt = NULL;
    slice_t stmts = {.desc = &stmt_desc};
    expect(p, token_LBRACE);
    // statement_list : statement+ ;
    while (p->tok != token_RBRACE) {
        stmt = statement(p);
        stmts = append(stmts, &stmt);
    }
    expect(p, token_RBRACE);
    stmt = malloc(sizeof(stmt_t));
    stmt->type = ast_STMT_BLOCK;
    stmt->block.stmts = slice_to_nil_array(stmts);
    return stmt;
}

static field_t *parse_field(parser_t *p) {
    accept(p, token_CONST);
    expr_t *type = type_specifier(p);
    expr_t *name = declarator(p, &type);
    field_t field = {
        .type = type,
        .name = name,
    };
    return memcpy(malloc(sizeof(field)), &field, sizeof(field));
}

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
    case token_SIGNED:
    case token_UNSIGNED:
        error(p, "`%s` is not supported in subc", token_string(p->tok));
        break;
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

static expr_t *declaration_specifiers(parser_t *p, bool is_top) {
    // declaration_specifiers
    //         : storage_class_specifier? type_qualifier? type_specifier
    //         ;
    if (is_top) {
        // storage_class_specifier : TYPEDEF | EXTERN | STATIC | AUTO | REGISTER ;
        switch (p->tok) {
        case token_EXTERN:
        case token_STATIC:
            next(p);
            break;
        default:
            break;
        }
    }
    // type_qualifier : CONST | VOLATILE ;
    switch (p->tok) {
    case token_CONST:
        next(p);
        break;
    default:
        break;
    }
    return type_specifier(p);
}

static expr_t *specifier_qualifier_list(parser_t *p) {
    // specifier_qualifier_list
    //         : type_qualifier? type_specifier
    //         ;
    return declaration_specifiers(p, false);
}

static decl_t *declaration(parser_t *p, bool is_external) {
    // declaration : declaration_specifiers init_declarator_list ';' ;
    if (p->tok == token_TYPEDEF) {
        token_t keyword = p->tok;
        expect(p, keyword);
        expr_t *type = declaration_specifiers(p, true);
        expr_t *ident = declarator(p, &type);
        expect(p, token_SEMICOLON);
        declare(p, p->pkg_scope, obj_kind_TYPE, ident->ident.name);
        spec_t spec = {
            .type = ast_SPEC_TYPEDEF,
            .typedef_ = {
                .name = ident,
                .type = type,
            },
        };
        decl_t decl = {
            .type = ast_DECL_GEN,
            .gen = {.spec = memcpy(malloc(sizeof(spec)), &spec, sizeof(spec))},
        };
        return memcpy(malloc(sizeof(decl)), &decl, sizeof(decl));
    }
    expr_t *type = declaration_specifiers(p, true);
    expr_t *name = declarator(p, &type);
    expr_t *value = NULL;
    if (is_external && p->tok == token_LBRACE) {
        // function_definition
        //         : declaration_specifiers declarator compound_statement ;
        stmt_t *body = compound_statement(p);
        decl_t decl = {
            .type = ast_DECL_FUNC,
            .func = {
                .type = type,
                .name = name,
                .body = body,
            },
        };
        return memcpy(malloc(sizeof(decl)), &decl, sizeof(decl));
    }
    // init_declarator_list
    //         : init_declarator
    //         | init_declarator_list ',' init_declarator
    //         ;
    // init_declarator
    //         : declarator
    //         | declarator '=' initializer
    //         ;
    if (accept(p, token_ASSIGN)) {
        value = initializer(p);
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
        .gen = {
            .spec = memcpy(malloc(sizeof(spec)), &spec, sizeof(spec)),
        },
    };
    return memcpy(malloc(sizeof(decl)), &decl, sizeof(decl));
}

static file_t *parse_file(parser_t *p) {
    slice_t decls = {.desc = &decl_desc};
    expr_t *name = NULL;
    if (accept(p, token_PACKAGE)) {
        expect(p, token_LPAREN);
        name = identifier(p);
        expect(p, token_RPAREN);
        expect(p, token_SEMICOLON);
    }
    while (p->tok != token_EOF) {
        // translation_unit
        //         : external_declaration+
        //         ;
        decl_t *decl = declaration(p, true);
        decls = append(decls, &decl);
    }
    file_t file = {
        .name = name,
        .decls = slice_to_nil_array(decls),
    };
    return memcpy(malloc(sizeof(file)), &file, sizeof(file));
}

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope) {
    char *src = ioutil_read_file(filename);
    parser_t p;
    init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    file_t *file = parse_file(&p);
    free(src);
    return file;
}
