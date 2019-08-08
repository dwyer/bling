#include "subc/parser/parser.h"

#include "fmt/fmt.h"

static expr_t *cast_expression(parser_t *p);
static expr_t *expression(parser_t *p);
static expr_t *constant_expression(parser_t *p);
static expr_t *initializer(parser_t *p);

static expr_t *type_specifier(parser_t *p);
static expr_t *struct_or_union_specifier(parser_t *p);
static expr_t *enum_specifier(parser_t *p);
static expr_t *pointer(parser_t *p, expr_t *type);
static decl_t **parameter_type_list(parser_t *p);
static expr_t *type_name(parser_t *p);

static expr_t *declarator(parser_t *p, expr_t **type_ptr);
static decl_t *abstract_declarator(parser_t *p, expr_t *type);

static stmt_t *statement(parser_t *p);
static stmt_t *compound_statement(parser_t *p, bool allow_single);

static expr_t *specifier_qualifier_list(parser_t *p);
static expr_t *declaration_specifiers(parser_t *p, bool is_top);
static decl_t *declaration(parser_t *p, bool is_external);

static decl_t *parameter_declaration(parser_t *p);

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
            ast_Object *obj = scope_lookup(p->pkg_scope, p->lit);
            return obj && obj->kind == obj_kind_TYPE;
        }
    default:
        return false;
    }
}

static expr_t *postfix_expression(parser_t *p, expr_t *x) {
    // postfix_expression
    //         : primary_expression
    //         | postfix_expression '[' expression ']'
    //         | postfix_expression '(' argument_expression_list? ')'
    //         | postfix_expression '.' IDENTIFIER
    //         | postfix_expression '->' IDENTIFIER
    //         ;
    if (x == NULL) {
        x = primary_expression(p);
    }
    for (;;) {
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
                x = esc(y);
            }
            break;
        case token_LPAREN:
            {
                slice$Slice args = {.size = sizeof(expr_t *)};
                expect(p, token_LPAREN);
                // argument_expression_list
                //         : expression
                //         | argument_expression_list ',' expression
                //         ;
                while (p->tok != token_RPAREN) {
                    expr_t *x = expression(p);
                    slice$append(&args, &x);
                    if (!accept(p, token_COMMA)) {
                        break;
                    }
                }
                expect(p, token_RPAREN);
                expr_t call = {
                    .type = ast_EXPR_CALL,
                    .call = {
                        .func = x,
                        .args = slice$to_nil_array(args),
                    },
                };
                x = esc(call);
            }
            break;
        case token_ARROW:
        case token_PERIOD:
            {
                token_t tok = p->tok;
                parser_next(p);
                expr_t y = {
                    .type = ast_EXPR_SELECTOR,
                    .selector = {
                        .x = x,
                        .tok = tok,
                        .sel = identifier(p),
                    },
                };
                x = esc(y);
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
    switch (p->tok) {
    case token_ADD:
    case token_AND:
    case token_BITWISE_NOT:
    case token_NOT:
    case token_SUB:
        // unary_operator
        //         : '&'
        //         | '*'
        //         | '+'
        //         | '-'
        //         | '~'
        //         | '!'
        //         ;
        {
            token_t op = p->tok;
            parser_next(p);
            expr_t x = {
                .type = ast_EXPR_UNARY,
                .unary = {
                    .op = op,
                    .x = cast_expression(p),
                },
            };
            return esc(x);
        }
    case token_MUL:
        {
            parser_next(p);
            expr_t x = {
                .type = ast_EXPR_STAR,
                .star = {
                    .x = cast_expression(p),
                },
            };
            return esc(x);
        }
    case token_SIZEOF:
        {
            parser_next(p);
            expect(p, token_LPAREN);
            expr_t *x = NULL;
            if (is_type(p)) {
                x = type_name(p);
                if (p->tok == token_MUL) {
                    declarator(p, &x); // TODO assert result == NULL?
                }
            } else {
                x = unary_expression(p);
            }
            expect(p, token_RPAREN);
            expr_t y = {
                .type = ast_EXPR_SIZEOF,
                .sizeof_ = {
                    .x = x,
                },
            };
            return esc(y);
        }
    case token_DEC:
    case token_INC:
        parser_error(p, p->pos, fmt$sprintf("unary `%s` not supported in subc", token_string(p->tok)));
        return NULL;
    default:
        return postfix_expression(p, NULL);
    }
}

static expr_t *cast_expression(parser_t *p) {
    // cast_expression
    //         : unary_expression
    //         | '(' type_name ')' cast_expression
    //         | '(' type_name ')' initializer
    //         ;
    if (accept(p, token_LPAREN)) {
        if (is_type(p)) {
            expr_t *type = type_name(p);
            expect(p, token_RPAREN);
            expr_t *x;
            if (p->tok == token_LBRACE) {
                x = initializer(p);
            } else {
                x = cast_expression(p);
            }
            expr_t y = {
                .type = ast_EXPR_CAST,
                .cast = {
                    .type = type,
                    .expr = x,
                },
            };
            return esc(y);
        } else {
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .paren = {
                    .x = expression(p),
                },
            };
            expect(p, token_RPAREN);
            return postfix_expression(p, esc(x));
        }
    }
    return unary_expression(p);
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
        expr_t *y = binary_expression(p, oprec + 1);
        expr_t z = {
            .type = ast_EXPR_BINARY,
            .binary = {
                .x = x,
                .op = op,
                .y = y,
            },
        };
        x = esc(z);
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
        x = esc(conditional);
    }
    return x;
}

static expr_t *expression(parser_t *p) {
    // expression : ternary_expression ;
    return ternary_expression(p);
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
    decl_t **fields = NULL;
    if (accept(p, token_LBRACE)) {
        // struct_declaration_list
        //         : struct_declaration
        //         | struct_declaration_list struct_declaration
        //         ;
        // struct_declaration
        //         : specifier_qualifier_list struct_declarator_list ';'
        //         ;
        slice$Slice slice = {.size = sizeof(decl_t *)};
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
            decl_t f = {
                .type = ast_DECL_FIELD,
                .field = {
                    .type = type,
                    .name = name,
                },
            };
            expect(p, token_SEMICOLON);
            decl_t *field = esc(f);
            slice$append(&slice, &field);
            if (p->tok == token_RBRACE) {
                break;
            }
        }
        expect(p, token_RBRACE);
        fields = slice$to_nil_array(slice);
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
    return esc(x);
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
    decl_t **enums = NULL;
    if (accept(p, token_LBRACE)) {
        // enumerator_list : enumerator | enumerator_list ',' enumerator ;
        slice$Slice list = {.size = sizeof(decl_t *)};
        for (;;) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            decl_t decl = {
                .type = ast_DECL_VALUE,
                .value = {
                    .name = identifier(p),
                    .kind = token_VAR,
                },
            };
            if (accept(p, token_ASSIGN)) {
                decl.value.value = constant_expression(p);
            }
            decl_t *enumerator = esc(decl);
            slice$append(&list, &enumerator);
            if (!accept(p, token_COMMA) || p->tok == token_RBRACE) {
                break;
            }
        }
        enums = slice$to_nil_array(list);
        expect(p, token_RBRACE);
    }
    expr_t x = {
        .type = ast_TYPE_ENUM,
        .enum_ = {
            .name = name,
            .enums = enums,
        },
    };
    return esc(x);
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
        *type_ptr = esc(type);
    } else if (accept(p, token_LPAREN)) {
        decl_t **params = NULL;
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
        *type_ptr = esc(type);
    }
    if (is_ptr) {
        expr_t type = {
            .type = ast_EXPR_STAR,
            .star = {
                .x = *type_ptr,
            }
        };
        *type_ptr = esc(type);
    }
    return name;
}

static expr_t *type_qualifier(parser_t *p, expr_t *type) {
    // type_qualifier_list : type_qualifier+ ;
    if (accept(p, token_CONST)) {
        type->is_const = true;
    }
    return type;
}

static expr_t *pointer(parser_t *p, expr_t *type) {
    // pointer : '*' type_qualifier_list? pointer? ;
    while (accept(p, token_MUL)) {
        expr_t x = {
            .type = ast_EXPR_STAR,
            .star = {
                .x = type,
            },
        };
        type = esc(x);
        type = type_qualifier(p, type);
    }
    return type;
}

static decl_t **parameter_type_list(parser_t *p) {
    // parameter_type_list
    //         : parameter_list
    //         | parameter_list ',' '...'
    //         ;
    // parameter_list
    //         : parameter_declaration
    //         | parameter_list ',' parameter_declaration
    //         ;
    slice$Slice params = {.size = sizeof(decl_t *)};
    while (p->tok != token_RPAREN) {
        // parameter_declaration
        //         : declaration_specifiers (declarator | abstract_declarator)?
        //         ;
        decl_t *param = parameter_declaration(p);
        slice$append(&params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
        if (accept(p, token_ELLIPSIS)) {
            decl_t decl = {
                .type = ast_DECL_FIELD,
            };
            decl_t *param = esc(decl);
            slice$append(&params, &param);
            break;
        }
    }
    return slice$to_nil_array(params);
}

static expr_t *type_name(parser_t *p) {
    // type_name
    //         : specifier_qualifier_list
    //         | specifier_qualifier_list abstract_declarator
    //         ;
    expr_t *type = specifier_qualifier_list(p);
    decl_t *decl = abstract_declarator(p, type);
    return decl->field.type;
}

static decl_t *abstract_declarator(parser_t *p, expr_t *type) {
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
            type = esc(t);
        } else if (accept(p, token_LPAREN)) {
            decl_t **params = NULL;
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
            type = esc(t);
        } else {
            break;
        }
    }
    if (is_ptr) {
        expr_t tmp = {
            .type = ast_EXPR_STAR,
            .star = {
                .x = type,
            },
        };
        type = esc(tmp);
    }
    decl_t declarator = {
        .type = ast_DECL_FIELD,
        .field = {
            .type = type,
        },
    };
    return esc(declarator);
}

static expr_t *initializer(parser_t *p) {
    // initializer
    //         : expression
    //         | '{' initializer_list ','? '}'
    //         ;
    if (!accept(p, token_LBRACE)) {
        return expression(p);
    }
    // initializer_list
    //         : designation? initializer
    //         | initializer_list ',' designation? initializer
    //         ;
    slice$Slice list = {.size = sizeof(expr_t *)};
    while (p->tok != token_RBRACE && p->tok != token_EOF) {
        expr_t *key = NULL;
        bool isArray = false;
        // designation : designator_list '=' ;
        // designator_list : designator+ ;
        // designator : '.' identifier
        if (accept(p, token_PERIOD)) {
            key = identifier(p);
            expect(p, token_ASSIGN);
        } else if (accept(p, token_LBRACK)) {
            isArray = true;
            key = expression(p);
            expect(p, token_RBRACK);
            expect(p, token_ASSIGN);
        }
        expr_t *value = initializer(p);
        if (key) {
            expr_t x = {
                .type = ast_EXPR_KEY_VALUE,
                .key_value = {
                    .key = key,
                    .value = value,
                    .isArray = isArray,
                },
            };
            value = esc(x);
        }
        slice$append(&list, &value);
        if (!accept(p, token_COMMA)) {
            break;
        }
    }
    expect(p, token_RBRACE);
    expr_t expr = {
        .type = ast_EXPR_COMPOUND,
        .compound = {
            .list = slice$to_nil_array(list),
        },
    };
    return esc(expr);
}

static stmt_t *simple_statement(parser_t *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    expr_t *x = expression(p);
    // assignment_statement
    //         : expression
    //         | expression assignment_operator expression
    //         | expression INC_OP
    //         | expression DEC_OP
    //         ;
    token_t op = p->tok;
    switch (op) {
    case token_ADD_ASSIGN:
    case token_ASSIGN:
    case token_DIV_ASSIGN:
    case token_MOD_ASSIGN:
    case token_MUL_ASSIGN:
    case token_SHL_ASSIGN:
    case token_SHR_ASSIGN:
    case token_SUB_ASSIGN:
    case token_XOR_ASSIGN:
        {
            parser_next(p);
            expr_t *y = expression(p);
            stmt_t stmt = {
                .type = ast_STMT_ASSIGN,
                .assign = {
                    .x = x,
                    .op = op,
                    .y = y,
                },
            };
            return esc(stmt);
        }
    case token_INC:
    case token_DEC:
        {
            parser_next(p);
            stmt_t stmt = {
                .type = ast_STMT_POSTFIX,
                .postfix = {
                    .x = x,
                    .op = op,
                },
            };
            return esc(stmt);
        }
    default:
        break;
    }
    // labeled_statement
    //         : IDENTIFIER ':' statement
    //         | CASE constant_expression ':' statement
    //         | DEFAULT ':' statement
    //         ;
    if (labelOk && x->type == ast_EXPR_IDENT) {
        if (accept(p, token_COLON)) {
            stmt_t stmt = {
                .type = ast_STMT_LABEL,
                .label = {
                    .label = x,
                    .stmt = statement(p),
                },
            };
            return esc(stmt);
        }
    }
    // expression_statement : expression? ';' ;
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .expr = {.x = x},
    };
    return esc(stmt);
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
    //         | simple_statement
    //         ;

    if (is_type(p)) {
        stmt_t stmt = {
            .type = ast_STMT_DECL,
            .decl = declaration(p, false),
        };
        return esc(stmt);
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
        stmt_t *post = NULL;
        if (p->tok != token_RPAREN) {
            post = simple_statement(p, false);
        }
        expect(p, token_RPAREN);
        stmt_t *body = compound_statement(p, true);
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
        return esc(stmt);
    }

    if (accept(p, token_IF)) {
        // if_statement
        //         : IF '(' expression ')' statement
        //         | IF '(' expression ')' statement ELSE statement
        //         | IF '(' expression ')' statement ELSE if_statement
        //         ;
        expect(p, token_LPAREN);
        expr_t *cond = expression(p);
        expect(p, token_RPAREN);
        stmt_t *body = compound_statement(p, true);
        stmt_t *else_ = NULL;
        if (accept(p, token_ELSE)) {
            if (p->tok == token_IF) {
                else_ = statement(p);
            } else {
                else_ = compound_statement(p, true);
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
        return esc(stmt);
    }

    if (accept(p, token_RETURN)) {
        // return_statement
        //         : RETURN expression? ';'
        //         ;
        expr_t *x = NULL;
        if (p->tok != token_SEMICOLON) {
            x = expression(p);
        }
        expect(p, token_SEMICOLON);
        stmt_t stmt = {
            .type = ast_STMT_RETURN,
            .return_ = {
                .x = x,
            },
        };
        return esc(stmt);
    }

    if (accept(p, token_SWITCH)) {
        // switch_statement : SWITCH '(' expression ')' case_statement* ;
        expect(p, token_LPAREN);
        expr_t *tag = expression(p);
        expect(p, token_RPAREN);
        expect(p, token_LBRACE);
        slice$Slice clauses = {.size = sizeof(stmt_t *)};
        while (p->tok == token_CASE || p->tok == token_DEFAULT) {
            // case_statement
            //         : CASE constant_expression ':' statement+
            //         | DEFAULT ':' statement+
            //         ;
            slice$Slice exprs = {.size=sizeof(expr_t *)};
            while (accept(p, token_CASE)) {
                expr_t *expr = constant_expression(p);
                expect(p, token_COLON);
                slice$append(&exprs, &expr);
            }
            if (slice$len(&exprs) == 0) {
                expect(p, token_DEFAULT);
                expect(p, token_COLON);
            }
            slice$Slice stmts = {.size = sizeof(stmt_t *)};
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
                    slice$append(&stmts, &stmt);
                }
            }
            stmt_t stmt = {
                .type = ast_STMT_CASE,
                .case_ = {
                    .exprs = slice$to_nil_array(exprs),
                    .stmts = slice$to_nil_array(stmts),
                },
            };
            stmt_t *clause = esc(stmt);
            slice$append(&clauses, &clause);
        }
        expect(p, token_RBRACE);
        stmt_t stmt = {
            .type = ast_STMT_SWITCH,
            .switch_ = {
                .tag = tag,
                .stmts = slice$to_nil_array(clauses),
            },
        };
        return esc(stmt);
    }

    if (accept(p, token_WHILE)) {
        // while_statement : WHILE '(' expression ')' statement ;
        expect(p, token_LPAREN);
        expr_t *cond = expression(p);
        expect(p, token_RPAREN);
        stmt_t *body = compound_statement(p, true);
        stmt_t stmt = {
            .type = ast_STMT_ITER,
            .iter = {
                .kind = token_WHILE,
                .cond = cond,
                .body = body,
            },
        };
        return esc(stmt);
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
            parser_next(p);
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
            return esc(stmt);
        }
    case token_LBRACE:
        return compound_statement(p, false);
    default:
        break;
    }

    if (accept(p, token_SEMICOLON)) {
        stmt_t stmt = {
            .type = ast_STMT_EMPTY,
        };
        return esc(stmt);
    }

    stmt_t *stmt = simple_statement(p, true);
    if (stmt->type != ast_STMT_LABEL) {
        expect(p, token_SEMICOLON);
    }
    return stmt;
}

static stmt_t *compound_statement(parser_t *p, bool allow_single) {
    // compound_statement : '{' statement_list? '}' ;
    // statement_list : statement+ ;
    slice$Slice stmts = {.size = sizeof(stmt_t *)};
    if (allow_single && p->tok != token_LBRACE) {
        stmt_t *stmt = statement(p);
        assert(stmt->type != ast_STMT_DECL);
        slice$append(&stmts, &stmt);
    } else {
        expect(p, token_LBRACE);
        while (p->tok != token_RBRACE) {
            stmt_t *stmt = statement(p);
            slice$append(&stmts, &stmt);
        }
        expect(p, token_RBRACE);
    }
    stmt_t stmt = {
        .type = ast_STMT_BLOCK,
        .block = {
            .stmts = slice$to_nil_array(stmts),
        },
    };
    return esc(stmt);
}

static decl_t *parameter_declaration(parser_t *p) {
    decl_t decl = {
        .type = ast_DECL_FIELD,
    };
    decl.field.type = declaration_specifiers(p, false);
    decl.field.name = declarator(p, &decl.field.type);
    return esc(decl);
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
        parser_error(p, p->pos, fmt$sprintf("`%s` is not supported in subc", token_string(p->tok)));
        break;
    case token_STRUCT:
    case token_UNION:
        x = struct_or_union_specifier(p);
        break;
    case token_ENUM:
        x = enum_specifier(p);
        break;
    default:
        if (is_type(p)) {
            x = identifier(p);
        } else {
            parser_errorExpected(p, p->pos, "type");
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
            parser_next(p);
            break;
        default:
            break;
        }
    }
    // type_qualifier : CONST | VOLATILE ;
    bool is_const = accept(p, token_CONST);
    expr_t *type = type_specifier(p);
    if (is_const) {
        type->is_const = is_const;
    }
    return type;
}

static expr_t *specifier_qualifier_list(parser_t *p) {
    // specifier_qualifier_list
    //         : type_qualifier? type_specifier
    //         ;
    return declaration_specifiers(p, false);
}

static decl_t *declaration(parser_t *p, bool is_external) {
    // declaration : declaration_specifiers init_declarator_list ';' ;
    if (p->tok == token_HASH) {
        return parse_pragma(p);
    }
    if (p->tok == token_TYPEDEF) {
        token_t keyword = p->tok;
        expect(p, keyword);
        expr_t *type = declaration_specifiers(p, true);
        expr_t *name = declarator(p, &type);
        expect(p, token_SEMICOLON);
        decl_t declref = {
            .type = ast_DECL_TYPEDEF,
            .typedef_ = {
                .name = name,
                .type = type,
            },
        };
        decl_t *decl = esc(declref);
        parser_declare(p, p->pkg_scope, decl, obj_kind_TYPE, name);
        return decl;
    }
    expr_t *type = declaration_specifiers(p, true);
    expr_t *name = declarator(p, &type);
    expr_t *value = NULL;
    if (type->type == ast_TYPE_FUNC) {
        decl_t decl = {
            .type = ast_DECL_FUNC,
            .func = {
                .type = type,
                .name = name,
            },
        };
        if (is_external && p->tok == token_LBRACE) {
            // function_definition
            //         : declaration_specifiers declarator compound_statement ;
            decl.func.body = compound_statement(p, false);
        } else {
            expect(p, token_SEMICOLON);
        }
        return esc(decl);
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
    if (name != NULL) {
        decl_t decl = {
            .type = ast_DECL_VALUE,
            .value = {
                .type = type,
                .name = name,
                .value = value,
                .kind = token_VAR,
            },
        };
        return esc(decl);
    } else {
        switch (type->type) {
        case ast_TYPE_STRUCT:
            name = type->struct_.name;
            break;
        default:
            panic("FUCK: %d", type->type);
            break;
        }
        decl_t decl = {
            .type = ast_DECL_TYPEDEF,
            .typedef_ = {
                .type = type,
                .name = name,
            },
        };
        return esc(decl);
    }
}

static ast_File *parse_cfile(parser_t *p) {
    slice$Slice decls = {.size = sizeof(decl_t *)};
    slice$Slice imports = {.size = sizeof(decl_t *)};
    expr_t *name = NULL;
    while (p->tok == token_HASH) {
        decl_t *lit = parse_pragma(p);
        slice$append(&decls, &lit);
    }
    if (accept(p, token_PACKAGE)) {
        expect(p, token_LPAREN);
        name = identifier(p);
        expect(p, token_RPAREN);
        expect(p, token_SEMICOLON);
    }
    while (p->tok == token_IMPORT) {
        expect(p, token_IMPORT);
        expect(p, token_LPAREN);
        expr_t *path = basic_lit(p, token_STRING);
        expect(p, token_RPAREN);
        expect(p, token_SEMICOLON);
        decl_t decl = {
            .type = ast_DECL_IMPORT,
            .imp = {
                .path = path,
            },
        };
        decl_t *declp = esc(decl);
        slice$append(&imports, &declp);
    }
    while (p->tok != token_EOF) {
        // translation_unit
        //         : external_declaration+
        //         ;
        decl_t *decl = declaration(p, true);
        slice$append(&decls, &decl);
    }
    ast_File file = {
        .filename = p->file->name,
        .name = name,
        .decls = slice$to_nil_array(decls),
        .imports = slice$to_nil_array(imports),
    };
    return esc(file);
}

extern ast_File *parser_parse_cfile(const char *filename, ast_Scope *pkg_scope) {
    char *src = ioutil$readFile(filename, NULL);
    parser_t p = {};
    parser_init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    p.c_mode = true;
    ast_File *file = parse_cfile(&p);
    file->scope = p.pkg_scope;
    free(src);
    return file;
}
