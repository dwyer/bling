#include "bling/parser/parser.h"

#include "path/path.h"

extern void parser_init(parser_t *p, char *filename, char *src) {
    p->filename = filename;
    p->lit = NULL;
    scanner_init(&p->scanner, src);
    parser_next(p);
}

extern void parser_declare(parser_t *p, scope_t *s, decl_t *decl,
        obj_kind_t kind, expr_t *name) {
    assert(name->type == ast_EXPR_IDENT);
    object_t *obj = object_new(kind, name->ident.name);
    obj->decl = decl;
    scope_insert(s, obj);
}

static expr_t *parse_cast_expr(parser_t *p);
static expr_t *parse_expr(parser_t *p);
static expr_t *parse_const_expr(parser_t *p);
static expr_t *parse_init_expr(parser_t *p);

static expr_t *parse_type_spec(parser_t *p);
static expr_t *parse_struct_or_union_spec(parser_t *p);
static expr_t *parse_enum_spec(parser_t *p);
static expr_t *parse_pointer(parser_t *p);
static decl_t **parse_param_type_list(parser_t *p, bool anon);

static stmt_t *parse_stmt(parser_t *p);
static stmt_t *parse_block_stmt(parser_t *p);

static decl_t *parse_decl(parser_t *p, bool is_external);
static decl_t *parse_field(parser_t *p, bool anon);

extern void parser_error(parser_t *p, char *fmt, ...) {
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

extern void parser_next(parser_t *p) {
    free(p->lit);
    p->tok = scanner_scan(&p->scanner, &p->lit);
}

extern bool accept(parser_t *p, token_t tok0) {
    if (p->tok == tok0) {
        parser_next(p);
        return true;
    }
    return false;
}

extern void expect(parser_t *p, token_t tok) {
    if (p->tok != tok) {
        char *lit = p->lit;
        if (lit == NULL) {
            lit = token_string(p->tok);
        }
        parser_error(p, "expected `%s`, got `%s`", token_string(tok), lit);
    }
    parser_next(p);
}

extern expr_t *identifier(parser_t *p) {
    expr_t x = {
        .type = ast_EXPR_IDENT,
    };
    if (p->tok == token_IDENT) {
        x.ident.name = strdup(p->lit);
    }
    expect(p, token_IDENT);
    return memdup(&x, sizeof(expr_t));
}

static expr_t *basic_lit(parser_t *p, token_t kind) {
    char *value = p->lit;
    expect(p, kind);
    expr_t x = {
        .type = ast_EXPR_BASIC_LIT,
        .basic_lit = {
            .kind = kind,
            .value = strdup(value),
        },
    };
    return memdup(&x, sizeof(expr_t));
}

extern expr_t *primary_expression(parser_t *p) {
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
        return basic_lit(p, p->tok);
    case token_LPAREN:
        if (p->c_mode) {
            parser_error(p, "unreachable");
        } else {
            expect(p, token_LPAREN);
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .paren = {
                    .x = parse_expr(p),
                },
            };
            expect(p, token_RPAREN);
            return memdup(&x, sizeof(expr_t));
        }
    default:
        parser_error(p, "bad expr: %s: %s", token_string(p->tok), p->lit);
        return NULL;
    }
}

static expr_t *parse_postfix_expr(parser_t *p) {
    // postfix_expression
    //         : primary_expression
    //         | postfix_expression '[' expression ']'
    //         | postfix_expression '(' argument_expression_list? ')'
    //         | postfix_expression '.' IDENTIFIER
    //         | postfix_expression '->' IDENTIFIER
    //         ;
    expr_t *x = primary_expression(p);
    for (;;) {
        switch (p->tok) {
        case token_LBRACK:
            {
                expect(p, token_LBRACK);
                expr_t y = {
                    .type = ast_EXPR_INDEX,
                    .index = {
                        .x = x,
                        .index = parse_expr(p),
                    },
                };
                expect(p, token_RBRACK);
                x = memdup(&y, sizeof(expr_t));
            }
            break;
        case token_LPAREN:
            {
                slice_t args = {.size = sizeof(expr_t *)};
                expect(p, token_LPAREN);
                // argument_expression_list
                //         : expression
                //         | argument_expression_list ',' expression
                //         ;
                while (p->tok != token_RPAREN) {
                    expr_t *x = parse_expr(p);
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
                x = memdup(&call, sizeof(expr_t));
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
                x = memdup(&y, sizeof(expr_t));
            }
            break;
        default:
            goto done;
        }
    }
done:
    return x;
}

static expr_t *parse_unary_expr(parser_t *p) {
    // unary_expression
    //         : postfix_expression
    //         | unary_operator cast_expression
    //         | SIZEOF unary_expression
    //         | SIZEOF '(' type_name ')'
    //         ;
    expr_t *x = NULL;
    switch (p->tok) {
    case token_ADD:
    case token_AND:
    case token_MUL:
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
        x = malloc(sizeof(expr_t));
        x->type = ast_EXPR_UNARY;
        x->unary.op = p->tok;
        parser_next(p);
        x->unary.x = parse_cast_expr(p);
        break;
    case token_SIZEOF:
        x = malloc(sizeof(expr_t));
        x->type = ast_EXPR_SIZEOF;
        expect(p, token_SIZEOF);
        expect(p, token_LPAREN);
        x->sizeof_.x = parse_type_spec(p);
        expect(p, token_RPAREN);
        break;
    case token_DEC:
    case token_INC:
        parser_error(p, "unary `%s` not supported in subc", token_string(p->tok));
        break;
    default:
        x = parse_postfix_expr(p);
        break;
    }
    return x;
}

static expr_t *parse_cast_expr(parser_t *p) {
    // cast_expression
    //         : unary_expression
    //         | AS '(' cast_expression ',' type_name ')'
    //         ;
    if (accept(p, token_AS)) {
        expect(p, token_LPAREN);
        expr_t *type = parse_type_spec(p);
        expect(p, token_RPAREN);
        expr_t *expr = parse_cast_expr(p);
        expr_t y = {
            .type = ast_EXPR_CAST,
            .cast = {
                .type = type,
                .expr = expr,
            },
        };
        return memdup(&y, sizeof(expr_t));
    }
    return parse_unary_expr(p);
}

static expr_t *parse_binary_expr(parser_t *p, int prec1) {
    expr_t *x = parse_cast_expr(p);
    for (;;) {
        token_t op = p->tok;
        int oprec = token_precedence(op);
        if (oprec < prec1) {
            return x;
        }
        expect(p, op);
        expr_t *y = parse_binary_expr(p, oprec + 1);
        expr_t z = {
            .type = ast_EXPR_BINARY,
            .binary = {
                .x = x,
                .op = op,
                .y = y,
            },
        };
        x = memdup(&z, sizeof(expr_t));
    }
}

static expr_t *parse_ternary_expr(parser_t *p) {
    // ternary_expression
    //         : binary_expression
    //         | binary_expression '?' expression ':' ternary_expression
    //         ;
    expr_t *x = parse_binary_expr(p, token_lowest_prec + 1);
    if (accept(p, token_QUESTION_MARK)) {
        expr_t *consequence = parse_expr(p);
        expect(p, token_COLON);
        expr_t *alternative = parse_ternary_expr(p);
        expr_t conditional = {
            .type = ast_EXPR_COND,
            .conditional = {
                .condition = x,
                .consequence = consequence,
                .alternative = alternative,
            },
        };
        x = memdup(&conditional, sizeof(expr_t));
    }
    return x;
}

static expr_t *parse_expr(parser_t *p) {
    // expression : ternary_expression ;
    return parse_ternary_expr(p);
}

static expr_t *parse_const_expr(parser_t *p) {
    // constant_expression : ternary_expression ;
    return parse_ternary_expr(p);
}

static expr_t *parse_struct_or_union_spec(parser_t *p) {
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
        slice_t slice = {.size = sizeof(decl_t *)};
        for (;;) {
            decl_t decl = {
                .type = ast_DECL_FIELD,
            };
            if (p->tok == token_UNION) {
                // anonymous union
                decl.field.type = parse_type_spec(p);
            } else {
                decl.field.name = identifier(p);
                decl.field.type = parse_type_spec(p);
            }
            expect(p, token_SEMICOLON);
            decl_t *field = memdup(&decl, sizeof(decl_t));
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
    return memdup(&x, sizeof(expr_t));
}

static expr_t *parse_enum_spec(parser_t *p) {
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
        slice_t list = {.size = sizeof(decl_t *)};
        for (;;) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            decl_t decl = {
                .type = ast_DECL_ENUM,
                .enum_ = {
                    .name = identifier(p),
                },
            };
            if (accept(p, token_ASSIGN)) {
                decl.enum_.value = parse_const_expr(p);
            }
            decl_t *enumerator = memdup(&decl, sizeof(decl_t));
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
            .enums = enums,
        },
    };
    return memdup(&x, sizeof(expr_t));
}

static expr_t *parse_pointer(parser_t *p) {
    expect(p, token_MUL);
    expr_t x = {
        .type = ast_TYPE_PTR,
        .ptr = {
            .type = parse_type_spec(p),
        },
    };
    return memdup(&x, sizeof(expr_t));
}

static decl_t **parse_param_type_list(parser_t *p, bool anon) {
    slice_t params = {.size = sizeof(decl_t *)};
    while (p->tok != token_RPAREN) {
        decl_t *param = parse_field(p, anon);
        params = append(params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
        if (accept(p, token_ELLIPSIS)) {
            // TODO make this a type
            decl_t decl = {
                .type = ast_DECL_FIELD,
            };
            decl_t *param = memdup(&decl, sizeof(decl_t));
            params = append(params, &param);
            break;
        }
    }
    return slice_to_nil_array(params);
}

static expr_t *parse_init_expr(parser_t *p) {
    // initializer
    //         : expression
    //         | '{' initializer_list ','? '}'
    //         ;
    if (p->tok == token_LBRACE) {
        // initializer_list
        //         : designation? initializer
        //         | initializer_list ',' designation? initializer
        //         ;
        expect(p, token_LBRACE);
        slice_t list = {.size = sizeof(expr_t *)};
        while (p->tok != token_RBRACE && p->tok != token_EOF) {
            expr_t *value = parse_init_expr(p);
            if (value->type == ast_EXPR_IDENT && accept(p, token_COLON)) {
                expr_t *key = value;
                expr_t x = {
                    .type = ast_EXPR_KEY_VALUE,
                    .key_value = {
                        .key = key,
                        .value = parse_init_expr(p),
                    },
                };
                value = memdup(&x, sizeof(expr_t));
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
        return memdup(&expr, sizeof(expr_t));
    }
    return parse_expr(p);
}

static stmt_t *parse_simple_stmt(parser_t *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    expr_t *x = parse_expr(p);
    // assignment_expression
    //         : expression
    //         | unary_expression assignment_operator expression
    //         | unary_expression INC_OP
    //         | unary_expression DEC_OP
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
            expr_t *y = parse_expr(p);
            expr_t z = {
                .type = ast_EXPR_BINARY,
                .binary = {
                    .x = x,
                    .op = op,
                    .y = y,
                },
            };
            x = memdup(&z, sizeof(expr_t));
            stmt_t stmt = {
                .type = ast_STMT_EXPR,
                .expr = {
                    .x = x,
                },
            };
            return memdup(&stmt, sizeof(stmt_t));
        }
    case token_INC:
    case token_DEC:
        {
            parser_next(p);
            expr_t y = {
                .type = ast_EXPR_INCDEC,
                .incdec = {
                    .x = x,
                    .tok = op,
                },
            };
            x = memdup(&y, sizeof(expr_t));
            stmt_t stmt = {
                .type = ast_STMT_EXPR,
                .expr = {
                    .x = x,
                },
            };
            return memdup(&stmt, sizeof(stmt_t));
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
                    .stmt = parse_stmt(p),
                },
            };
            return memdup(&stmt, sizeof(stmt_t));
        }
    }
    // expression_statement : expression? ';' ;
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .expr = {.x = x},
    };
    return memdup(&stmt, sizeof(stmt_t));
}

static stmt_t *parse_stmt(parser_t *p) {
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

    if (p->tok == token_VAR) {
        stmt_t stmt = {
            .type = ast_STMT_DECL,
            .decl = parse_decl(p, false),
        };
        return memdup(&stmt, sizeof(stmt_t));
    }

    if (accept(p, token_FOR)) {
        // for_statement
        //         | FOR simple_statement? ';' expression? ';' expression?
        //              compound_statement ;
        stmt_t *init = NULL;
        if (!accept(p, token_SEMICOLON)) {
            init = parse_stmt(p);
        }
        expr_t *cond = NULL;
        if (p->tok != token_SEMICOLON) {
            cond = parse_expr(p);
        }
        expect(p, token_SEMICOLON);
        stmt_t *post = NULL;
        if (p->tok != token_LBRACE) {
            post = parse_simple_stmt(p, false);
        }
        stmt_t *body = parse_block_stmt(p);
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
        return memdup(&stmt, sizeof(stmt_t));
    }

    if (accept(p, token_IF)) {
        // if_statement
        //         : IF expression compound_statement
        //         | IF expression compound_statement ELSE compound_statement
        //         | IF expression compound_statement ELSE if_statement
        //         ;
        expr_t *cond = parse_expr(p);
        if (p->tok != token_LBRACE) {
            parser_error(p, "`if` must be followed by a compound_statement");
        }
        stmt_t *body = parse_block_stmt(p);
        stmt_t *else_ = NULL;
        if (accept(p, token_ELSE)) {
            if (p->tok == token_IF) {
                else_ = parse_stmt(p);
            } else if (p->tok == token_LBRACE) {
                else_ = parse_block_stmt(p);
            } else {
                parser_error(p, "`else` must be followed by an if_statement or compound_statement");
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
        return memdup(&stmt, sizeof(stmt_t));
    }

    if (accept(p, token_RETURN)) {
        // return_statement
        //         | RETURN expression? ';'
        //         ;
        expr_t *x = NULL;
        if (p->tok != token_SEMICOLON) {
            x = parse_expr(p);
        }
        expect(p, token_SEMICOLON);
        stmt_t *stmt = malloc(sizeof(stmt_t));
        stmt->type = ast_STMT_RETURN;
        stmt->return_.x = x;
        return stmt;
    }

    if (accept(p, token_SWITCH)) {
        // switch_statement | SWITCH expression case_statement* ;
        expr_t *tag = parse_expr(p);
        expect(p, token_LBRACE);
        slice_t clauses = {.size = sizeof(stmt_t *)};
        while (p->tok == token_CASE || p->tok == token_DEFAULT) {
            // case_statement
            //         | CASE constant_expression ':' statement+
            //         | DEFAULT ':' statement+
            //         ;
            expr_t *expr = NULL;
            if (accept(p, token_CASE)) {
                expr = parse_const_expr(p);
            } else {
                expect(p, token_DEFAULT);
            }
            expect(p, token_COLON);
            slice_t stmts = {.size = sizeof(stmt_t *)};
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
                    stmt_t *stmt = parse_stmt(p);
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
            stmt_t *clause = memdup(&stmt, sizeof(stmt_t));
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
        return memdup(&stmt, sizeof(stmt_t));
    }

    if (accept(p, token_WHILE)) {
        // while_statement : WHILE expression compound_statement ;
        expr_t *cond = parse_expr(p);
        stmt_t *body = parse_block_stmt(p);
        stmt_t stmt = {
            .type = ast_STMT_ITER,
            .iter = {
                .kind = token_WHILE,
                .cond = cond,
                .body = body,
            },
        };
        return memdup(&stmt, sizeof(stmt_t));
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
            return memdup(&stmt, sizeof(stmt_t));
        }
    case token_LBRACE:
        return parse_block_stmt(p);
    default:
        break;
    }

    if (accept(p, token_SEMICOLON)) {
        stmt_t stmt = {
            .type = ast_STMT_EXPR,
        };
        return memdup(&stmt, sizeof(stmt_t));
    }

    stmt_t *stmt = parse_simple_stmt(p, true);
    if (stmt->type != ast_STMT_LABEL) {
        expect(p, token_SEMICOLON);
    }
    return stmt;
}

static stmt_t *parse_block_stmt(parser_t *p) {
    // compound_statement : '{' statement_list? '}' ;
    stmt_t *stmt = NULL;
    slice_t stmts = {.size = sizeof(stmt_t *)};
    expect(p, token_LBRACE);
    // statement_list : statement+ ;
    while (p->tok != token_RBRACE) {
        stmt = parse_stmt(p);
        stmts = append(stmts, &stmt);
    }
    expect(p, token_RBRACE);
    stmt = malloc(sizeof(stmt_t));
    stmt->type = ast_STMT_BLOCK;
    stmt->block.stmts = slice_to_nil_array(stmts);
    return stmt;
}

static decl_t *parse_field(parser_t *p, bool anon) {
    decl_t decl = {
        .type = ast_DECL_FIELD,
    };
    if (p->tok == token_IDENT) {
        decl.field.name = identifier(p);
    }
    if (decl.field.name != NULL && (p->tok == token_COMMA || p->tok == token_RPAREN)) {
        decl.field.type = decl.field.name;
        decl.field.name = NULL;
    } else {
        decl.field.type = parse_type_spec(p);
    }
    return memdup(&decl, sizeof(decl_t));
}

static expr_t *parse_func_type(parser_t *p) {
    expect(p, token_FUNC);
    expect(p, token_LPAREN);
    decl_t **params = parse_param_type_list(p, false);
    expect(p, token_RPAREN);
    expr_t *result = NULL;
    if (p->tok != token_SEMICOLON) {
        result = parse_type_spec(p);
    }
    expr_t type = {
        .type = ast_TYPE_FUNC,
        .func = {
            .params = params,
            .result = result,
        },
    };
    expr_t ptr = {
        .type = ast_TYPE_PTR,
        .ptr = {
            .type = memdup(&type, sizeof(expr_t)),
        },
    };
    return memdup(&ptr, sizeof(expr_t));
}

static expr_t *parse_type_qualifier(parser_t *p, token_t tok) {
    expect(p, tok);
    expr_t x = {
        .type = ast_TYPE_QUAL,
        .qual = {
            .qual = tok,
            .type = parse_type_spec(p),
        },
    };
    return memdup(&x, sizeof(expr_t));
}

static expr_t *parse_type_spec(parser_t *p) {
    expr_t *x = NULL;
    switch (p->tok) {
    case token_CONST:
        x = parse_type_qualifier(p, p->tok);
        break;
    case token_IDENT:
        x = identifier(p);
        break;
    case token_MUL:
        x = parse_pointer(p);
        break;
    case token_STRUCT:
    case token_UNION:
        x = parse_struct_or_union_spec(p);
        break;
    case token_ENUM:
        x = parse_enum_spec(p);
        break;
    case token_FUNC:
        x = parse_func_type(p);
        break;
    case token_LBRACK:
        {
            expect(p, token_LBRACK);
            expr_t *len = NULL;
            if (p->tok != token_RBRACK) {
                len = parse_const_expr(p);
            }
            expect(p, token_RBRACK);
            expr_t type = {
                .type = ast_TYPE_ARRAY,
                .array = {
                    .elt = parse_type_spec(p),
                    .len = len,
                },
            };
            x = memdup(&type, sizeof(expr_t));
        }
        break;
    default:
        parser_error(p, "expected type, got %s", token_string(p->tok));
        break;
    }
    return x;
}

static decl_t *parse_decl(parser_t *p, bool is_external) {
    switch (p->tok) {
    case token_TYPEDEF:
        {
            token_t keyword = p->tok;
            expect(p, keyword);
            expr_t *ident = identifier(p);
            expr_t *type = parse_type_spec(p);
            expect(p, token_SEMICOLON);
            decl_t decl = {
                .type = ast_DECL_TYPEDEF,
                .typedef_ = {
                    .name = ident,
                    .type = type,
                },
            };
            return memdup(&decl, sizeof(decl_t));
        }
    case token_VAR:
        {
            expect(p, token_VAR);
            expr_t *ident = identifier(p);
            expr_t *type = NULL;
            if (p->is_experiemental) {
                if (p->tok != token_ASSIGN) {
                    type = parse_type_spec(p);
                }
            } else {
                type = parse_type_spec(p);
            }
            expr_t *value = NULL;
            if (accept(p, token_ASSIGN)) {
                value = parse_init_expr(p);
            }
            expect(p, token_SEMICOLON);
            decl_t decl = {
                .type = ast_DECL_VALUE,
                .value = {
                    .name = ident,
                    .type = type,
                    .value = value,
                },
            };
            return memdup(&decl, sizeof(decl_t));
        }
    case token_FUNC:
        {
            decl_t decl = {.type = ast_DECL_FUNC};
            expect(p, token_FUNC);
            decl.func.name = identifier(p);
            expect(p, token_LPAREN);
            expr_t type = {.type = ast_TYPE_FUNC};
            type.func.params = parse_param_type_list(p, false);
            expect(p, token_RPAREN);
            if (p->tok != token_LBRACE && p->tok != token_SEMICOLON) {
                type.func.result = parse_type_spec(p);
            }
            decl.func.type = memdup(&type, sizeof(expr_t));
            if (p->tok == token_LBRACE) {
                decl.func.body = parse_block_stmt(p);
            } else {
                expect(p, token_SEMICOLON);
            }
            return memdup(&decl, sizeof(decl_t));
        }
    default:
        parser_error(p, "cant handle it: %s", token_string(p->tok));
        return NULL;
    }
}

static bool isBlingFile(const char *name) {
    return path_matchExt(".bling", name);
}

static bool isTestFile(const char *name) {
    return path_match("*_test.bling", name);
}

static void parser_import(parser_t *p, const char *dirname, slice_t *decls) {
    for (int i = 0; i < len(p->pkg_scope->filenames); i++) {
        char *s = NULL;
        slice_get(&p->pkg_scope->filenames, i, &s);
        if (streq(dirname, s)) {
            return;
        }
    }
    p->pkg_scope->filenames = append(p->pkg_scope->filenames, &dirname);
    error_t *err = NULL;
    os_FileInfo **infos = ioutil_read_dir(dirname, &err);
    while (*infos != NULL) {
        char *name = os_FileInfo_name(**infos);
        if (isBlingFile(name) && !isTestFile(name)) {
            file_t *file = parser_parse_file(name, p->pkg_scope);
            for (int i = 0; file->decls[i] != NULL; i++) {
                *decls = append(*decls, &file->decls[i]);
            }
        }
        infos++;
    }
}

static file_t *parse_file(parser_t *p) {
    slice_t decls = {.size = sizeof(decl_t *)};
    expr_t *name = NULL;
    if (accept(p, token_PACKAGE)) {
        expect(p, token_LPAREN);
        name = identifier(p);
        expect(p, token_RPAREN);
        expect(p, token_SEMICOLON);
        if (streq(name->ident.name, "experimental")) {
            p->is_experiemental = true;
        }
    }
    parser_import(p, "builtin", &decls);
    while (p->tok == token_IMPORT) {
        expect(p, token_IMPORT);
        expr_t *path = basic_lit(p, token_STRING);
        expect(p, token_SEMICOLON);
        const char *lit = path->basic_lit.value;
        int n = strlen(lit) - 2;
        char *dirname = malloc(n + 1);
        for (int i = 0; i < n; i++) {
            dirname[i] = lit[i+1];
        }
        dirname[n] = '\0';
        parser_import(p, dirname, &decls);
    }
    while (p->tok != token_EOF) {
        decl_t *decl = parse_decl(p, true);
        decls = append(decls, &decl);
    }
    file_t file = {
        .filename = p->filename,
        .name = name,
        .decls = slice_to_nil_array(decls),
    };
    return memdup(&file, sizeof(file_t));
}

extern file_t *parser_parse_file(char *filename, scope_t *pkg_scope) {
    char *src = ioutil_read_file(filename, NULL);
    parser_t p = {};
    parser_init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    file_t *file = parse_file(&p);
    file->scope = p.pkg_scope;
    free(src);
    return file;
}
