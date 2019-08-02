#include "bling/parser/parser.h"

#include "bytes/bytes.h"
#include "fmt/fmt.h"
#include "path/path.h"

extern decl_t *parse_pragma(parser_t *p) {
    pos_t pos = p->pos;
    char *lit = p->lit;
    p->lit = NULL;
    expect(p, token_HASH);
    decl_t decl = {
        .type = ast_DECL_PRAGMA,
        .pos = pos,
        .pragma = {
            .lit = lit,
        },
    };
    return esc(decl);
}

extern void parser_init(parser_t *p, char *filename, char *src) {
    p->file = token_File_new(filename);
    p->lit = NULL;
    scanner_init(&p->scanner, p->file, src);
    p->scanner.dontInsertSemis = !path_matchExt(".bling", filename);
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

extern void parser_error(parser_t *p, pos_t pos, char *fmt, ...) {
    token_Position position = token_File_position(p->file, pos);
    buffer_t buf = {};
    int i = 0;
    slice_get(&p->file->lines, position.line-1, &i);
    int ch = p->scanner.src[i];
    while (ch > 0 && ch != '\n') {
        buffer_writeByte(&buf, ch, NULL);
        i++;
        ch = p->scanner.src[i];
    }
    panic(fmt_sprintf("%s: %s\n%s",
                token_Position_string(&position),
                fmt,
                buffer_string(&buf)));
}

extern void parser_next(parser_t *p) {
    p->tok = scanner_scan(&p->scanner, &p->pos, &p->lit);
}

extern bool accept(parser_t *p, token_t tok0) {
    if (p->tok == tok0) {
        parser_next(p);
        return true;
    }
    return false;
}

extern pos_t expect(parser_t *p, token_t tok) {
    pos_t pos = p->pos;
    if (p->tok != tok) {
        char *lit = p->lit;
        if (lit == NULL) {
            lit = token_string(p->tok);
        }
        parser_error(p, pos, fmt_sprintf("expected `%s`, got `%s`", token_string(tok), lit));
    }
    parser_next(p);
    return pos;
}

extern expr_t *identifier(parser_t *p) {
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .pos = p->pos,
    };
    if (p->tok == token_IDENT) {
        x.ident.name = p->lit;
        p->lit = NULL;
    }
    expect(p, token_IDENT);
    return esc(x);
}

extern expr_t *basic_lit(parser_t *p, token_t kind) {
    char *value = p->lit;
    p->lit = NULL;
    pos_t pos = expect(p, kind);
    expr_t x = {
        .type = ast_EXPR_BASIC_LIT,
        .pos = pos,
        .basic_lit = {
            .kind = kind,
            .value = value,
        },
    };
    return esc(x);
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
            parser_error(p, p->pos, "unreachable");
        } else {
            pos_t pos = p->pos;
            expect(p, token_LPAREN);
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .pos = pos,
                .paren = {
                    .x = parse_expr(p),
                },
            };
            expect(p, token_RPAREN);
            return esc(x);
        }
    default:
        parser_error(p, p->pos, fmt_sprintf("bad expr: %s: %s", token_string(p->tok), p->lit));
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
                    .pos = x->pos,
                    .index = {
                        .x = x,
                        .index = parse_expr(p),
                    },
                };
                expect(p, token_RBRACK);
                x = esc(y);
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
                    .pos = x->pos,
                    .call = {
                        .func = x,
                        .args = slice_to_nil_array(args),
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
                    .pos = x->pos,
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

static expr_t *parse_unary_expr(parser_t *p) {
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
            pos_t pos = p->pos;
            token_t op = p->tok;
            parser_next(p);
            expr_t x = {
                .type = ast_EXPR_UNARY,
                .pos = pos,
                .unary = {
                    .op = op,
                    .x = parse_cast_expr(p),
                },
            };
            return esc(x);
        }
    case token_MUL:
        {
            pos_t pos = p->pos;
            parser_next(p);
            expr_t x = {
                .type = ast_EXPR_STAR,
                .pos = pos,
                .star = {
                    .x = parse_cast_expr(p),
                },
            };
            return esc(x);
        }
    case token_SIZEOF:
        {
            pos_t pos = p->pos;
            parser_next(p);
            expect(p, token_LPAREN);
            expr_t x = {
                .type = ast_EXPR_SIZEOF,
                .pos = pos,
                .sizeof_ = {
                    .x = parse_type_spec(p),
                },
            };
            expect(p, token_RPAREN);
            return esc(x);
        }
    default:
        return parse_postfix_expr(p);
    }
}

static expr_t *parse_cast_expr(parser_t *p) {
    // cast_expression
    //         : unary_expression
    //         | '<' type_name '>' cast_expression
    //         | '<' type_name '>' initializer
    //         ;
    pos_t pos = p->pos;
    if (accept(p, token_LT)) {
        expr_t *type = parse_type_spec(p);
        expect(p, token_GT);
        expr_t *expr = NULL;
        if (p->tok == token_LBRACE) {
            expr = parse_init_expr(p);
        } else {
            expr = parse_cast_expr(p);
        }
        expr_t y = {
            .type = ast_EXPR_CAST,
            .pos = pos,
            .cast = {
                .type = type,
                .expr = expr,
            },
        };
        return esc(y);
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
            .pos = x->pos,
            .binary = {
                .x = x,
                .op = op,
                .y = y,
            },
        };
        x = esc(z);
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
            .pos = x->pos,
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
    pos_t pos = p->pos;
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
                .pos = p->pos,
            };
            if (p->tok == token_UNION) {
                // anonymous union
                decl.field.type = parse_type_spec(p);
            } else {
                decl.field.name = identifier(p);
                decl.field.type = parse_type_spec(p);
            }
            expect(p, token_SEMICOLON);
            decl_t *field = esc(decl);
            slice = append(slice, &field);
            if (p->tok == token_RBRACE) {
                break;
            }
        }
        expect(p, token_RBRACE);
        fields = slice_to_nil_array(slice);
    }
    // TODO assert(name || fields)
    expr_t x = {
        .type = ast_TYPE_STRUCT,
        .pos = pos,
        .struct_ = {
            .tok = keyword,
            .name = name,
            .fields = fields,
        },
    };
    return esc(x);
}

static expr_t *parse_enum_spec(parser_t *p) {
    // enum_specifier
    //         : ENUM '{' enumerator_list '}'
    //         | ENUM IDENTIFIER '{' enumerator_list '}'
    //         | ENUM IDENTIFIER
    //         ;
    expr_t *name = NULL;
    pos_t pos = p->pos;
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
                .type = ast_DECL_VALUE,
                .pos = p->pos,
                .value = {
                    .name = identifier(p),
                },
            };
            if (accept(p, token_ASSIGN)) {
                decl.value.value = parse_const_expr(p);
            }
            decl_t *enumerator = esc(decl);
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
        .pos = pos,
        .enum_ = {
            .name = name,
            .enums = enums,
        },
    };
    return esc(x);
}

static expr_t *parse_pointer(parser_t *p) {
    pos_t pos = p->pos;
    expect(p, token_MUL);
    expr_t x = {
        .type = ast_EXPR_STAR,
        .pos = pos,
        .star = {
            .x = parse_type_spec(p),
        },
    };
    return esc(x);
}

static decl_t **parse_param_type_list(parser_t *p, bool anon) {
    slice_t params = {.size = sizeof(decl_t *)};
    while (p->tok != token_RPAREN) {
        decl_t *param = parse_field(p, anon);
        params = append(params, &param);
        if (!accept(p, token_COMMA)) {
            break;
        }
        pos_t pos = p->pos;
        if (accept(p, token_ELLIPSIS)) {
            // TODO make this a type
            decl_t decl = {
                .type = ast_DECL_FIELD,
                .pos = pos,
            };
            decl_t *param = esc(decl);
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
        pos_t pos = p->pos;
        expect(p, token_LBRACE);
        slice_t list = {.size = sizeof(expr_t *)};
        while (p->tok != token_RBRACE && p->tok != token_EOF) {
            expr_t *value = parse_init_expr(p);
            if (value->type == ast_EXPR_IDENT && accept(p, token_COLON)) {
                expr_t *key = value;
                expr_t x = {
                    .type = ast_EXPR_KEY_VALUE,
                    .pos = key->pos,
                    .key_value = {
                        .key = key,
                        .value = parse_init_expr(p),
                    },
                };
                value = esc(x);
            }
            list = append(list, &value);
            if (!accept(p, token_COMMA)) {
                break;
            }
        }
        expect(p, token_RBRACE);
        expr_t expr = {
            .type = ast_EXPR_COMPOUND,
            .pos = pos,
            .compound = {
                .list = slice_to_nil_array(list),
            },
        };
        return esc(expr);
    }
    return parse_expr(p);
}

static stmt_t *parse_simple_stmt(parser_t *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    expr_t *x = parse_expr(p);
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
            expr_t *y = parse_expr(p);
            stmt_t stmt = {
                .type = ast_STMT_ASSIGN,
                .pos = x->pos,
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
                .pos = x->pos,
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
                .pos = x->pos,
                .label = {
                    .label = x,
                    .stmt = parse_stmt(p),
                },
            };
            return esc(stmt);
        }
    }
    // expression_statement : expression? ';' ;
    stmt_t stmt = {
        .type = ast_STMT_EXPR,
        .pos = x->pos,
        .expr = {.x = x},
    };
    return esc(stmt);
}

static stmt_t *parse_for_stmt(parser_t *p) {
    // for_statement
    //         | FOR simple_statement? ';' expression? ';' expression?
    //              compound_statement ;
    pos_t pos = expect(p, token_FOR);
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
        .pos = pos,
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

static stmt_t *parse_if_stmt(parser_t *p) {
    // if_statement
    //         : IF expression compound_statement
    //         | IF expression compound_statement ELSE compound_statement
    //         | IF expression compound_statement ELSE if_statement
    //         ;
    pos_t pos = expect(p, token_IF);
    expr_t *cond = parse_expr(p);
    if (p->tok != token_LBRACE) {
        parser_error(p, p->pos, "`if` must be followed by a compound_statement");
    }
    stmt_t *body = parse_block_stmt(p);
    stmt_t *else_ = NULL;
    if (accept(p, token_ELSE)) {
        if (p->tok == token_IF) {
            else_ = parse_stmt(p);
        } else if (p->tok == token_LBRACE) {
            else_ = parse_block_stmt(p);
        } else {
            parser_error(p, p->pos, "`else` must be followed by an if_statement or compound_statement");
        }
    }
    stmt_t stmt = {
        .type = ast_STMT_IF,
        .pos = pos,
        .if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return esc(stmt);
}

static stmt_t *parse_return_stmt(parser_t *p) {
    // return_statement
    //         | RETURN expression? ';'
    //         ;
    pos_t pos = expect(p, token_RETURN);
    expr_t *x = NULL;
    if (p->tok != token_SEMICOLON) {
        x = parse_expr(p);
    }
    expect(p, token_SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_RETURN,
        .pos = pos,
        .return_ = {
            .x = x,
        },
    };
    return esc(stmt);
}

static stmt_t *parse_switch_stmt(parser_t *p) {
    // switch_statement | SWITCH expression case_statement* ;
    pos_t pos = expect(p, token_SWITCH);
    expr_t *tag = parse_expr(p);
    expect(p, token_LBRACE);
    slice_t clauses = {.size = sizeof(stmt_t *)};
    while (p->tok == token_CASE || p->tok == token_DEFAULT) {
        // case_statement
        //         | CASE constant_expression ':' statement+
        //         | DEFAULT ':' statement+
        //         ;
        slice_t exprs = {.size=sizeof(expr_t *)};
        pos_t pos = p->pos;
        if (accept(p, token_CASE)) {
            for (;;) {
                expr_t *expr = parse_const_expr(p);
                exprs = append(exprs, &expr);
                if (!accept(p, token_COMMA)) {
                    break;
                }
            }
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
            .pos = pos,
            .case_ = {
                .exprs = slice_to_nil_array(exprs),
                .stmts = slice_to_nil_array(stmts),
            },
        };
        stmt_t *clause = esc(stmt);
        clauses = append(clauses, &clause);
    }
    expect(p, token_RBRACE);
    accept(p, token_SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_SWITCH,
        .pos = pos,
        .switch_ = {
            .tag = tag,
            .stmts = slice_to_nil_array(clauses),
        },
    };
    return esc(stmt);
}

static stmt_t *parse_while_stmt(parser_t *p) {
    // while_statement : WHILE expression compound_statement ;
    pos_t pos = expect(p, token_WHILE);
    expr_t *cond = parse_expr(p);
    stmt_t *body = parse_block_stmt(p);
    stmt_t stmt = {
        .type = ast_STMT_ITER,
        .pos = pos,
        .iter = {
            .kind = token_WHILE,
            .cond = cond,
            .body = body,
        },
    };
    return esc(stmt);
}

static stmt_t *parse_jump_stmt(parser_t *p, token_t keyword) {
    // jump_statement
    //         : GOTO IDENTIFIER ';'
    //         | CONTINUE ';'
    //         | BREAK ';'
    //         ;
    pos_t pos = expect(p, keyword);
    expr_t *label = NULL;
    if (keyword == token_GOTO) {
        label = identifier(p);
    }
    expect(p, token_SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_JUMP,
        .pos = pos,
        .jump = {
            .keyword = keyword,
            .label = label,
        },
    };
    return esc(stmt);
}

static stmt_t *parse_decl_stmt(parser_t *p) {
    stmt_t stmt = {
        .type = ast_STMT_DECL,
        .pos = p->pos,
        .decl = parse_decl(p, false),
    };
    return esc(stmt);
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
    switch (p->tok) {
    case token_VAR: return parse_decl_stmt(p);
    case token_FOR: return parse_for_stmt(p);
    case token_IF: return parse_if_stmt(p);
    case token_RETURN: return parse_return_stmt(p);
    case token_SWITCH: return parse_switch_stmt(p);
    case token_WHILE: return parse_while_stmt(p);
    case token_BREAK:
    case token_CONTINUE:
    case token_GOTO: return parse_jump_stmt(p, p->tok);
    case token_LBRACE: return parse_block_stmt(p);
    default:
        break;
    }
    pos_t pos = p->pos;
    if (accept(p, token_SEMICOLON)) {
        stmt_t stmt = {
            .type = ast_STMT_EXPR,
            .pos = pos,
        };
        return esc(stmt);
    }
    stmt_t *stmt = parse_simple_stmt(p, true);
    if (stmt->type != ast_STMT_LABEL) {
        expect(p, token_SEMICOLON);
    }
    return stmt;
}

static stmt_t *parse_block_stmt(parser_t *p) {
    // compound_statement : '{' statement_list? '}' ;
    slice_t stmts = {.size = sizeof(stmt_t *)};
    pos_t pos = expect(p, token_LBRACE);
    // statement_list : statement+ ;
    while (p->tok != token_RBRACE) {
        stmt_t *stmt = parse_stmt(p);
        stmts = append(stmts, &stmt);
    }
    expect(p, token_RBRACE);
    accept(p, token_SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_BLOCK,
        .pos = pos,
        .block = {
            .stmts = slice_to_nil_array(stmts),
        }
    };
    return esc(stmt);
}

static decl_t *parse_field(parser_t *p, bool anon) {
    decl_t decl = {
        .type = ast_DECL_FIELD,
        .pos = p->pos,
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
    return esc(decl);
}

static expr_t *parse_func_type(parser_t *p) {
    pos_t pos = expect(p, token_FUNC);
    expect(p, token_LPAREN);
    decl_t **params = parse_param_type_list(p, false);
    expect(p, token_RPAREN);
    expr_t *result = NULL;
    if (p->tok != token_SEMICOLON) {
        result = parse_type_spec(p);
    }
    expr_t type = {
        .type = ast_TYPE_FUNC,
        .pos = pos,
        .func = {
            .params = params,
            .result = result,
        },
    };
    expr_t ptr = {
        .type = ast_EXPR_STAR,
        .pos = type.pos,
        .star = {
            .x = esc(type),
        },
    };
    return esc(ptr);
}

static expr_t *parse_type_qualifier(parser_t *p, token_t tok) {
    expect(p, tok);
    expr_t *type = parse_type_spec(p);
    type->is_const = true;
    return type;
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
            pos_t pos = expect(p, token_LBRACK);
            expr_t *len = NULL;
            if (p->tok != token_RBRACK) {
                len = parse_const_expr(p);
            }
            expect(p, token_RBRACK);
            expr_t type = {
                .type = ast_TYPE_ARRAY,
                .pos = pos,
                .array = {
                    .elt = parse_type_spec(p),
                    .len = len,
                },
            };
            x = esc(type);
        }
        break;
    default:
        parser_error(p, p->pos, fmt_sprintf("expected type, got %s", token_string(p->tok)));
        break;
    }
    return x;
}

static decl_t *parse_decl(parser_t *p, bool is_external) {
    switch (p->tok) {
    case token_HASH:
        return parse_pragma(p);
    case token_TYPEDEF:
        {
            token_t keyword = p->tok;
            pos_t pos = expect(p, keyword);
            expr_t *ident = identifier(p);
            expr_t *type = parse_type_spec(p);
            expect(p, token_SEMICOLON);
            decl_t decl = {
                .type = ast_DECL_TYPEDEF,
                .pos = pos,
                .typedef_ = {
                    .name = ident,
                    .type = type,
                },
            };
            return esc(decl);
        }
    case token_CONST:
    case token_VAR:
        {
            token_t tok = p->tok;
            pos_t pos = expect(p, tok);
            expr_t *ident = identifier(p);
            expr_t *type = NULL;
            if (p->tok != token_ASSIGN) {
                type = parse_type_spec(p);
            }
            expr_t *value = NULL;
            if (accept(p, token_ASSIGN)) {
                value = parse_init_expr(p);
            }
            expect(p, token_SEMICOLON);
            decl_t decl = {
                .type = ast_DECL_VALUE,
                .pos = pos,
                .value = {
                    .name = ident,
                    .type = type,
                    .value = value,
                    .kind = tok,
                },
            };
            return esc(decl);
        }
    case token_FUNC:
        {
            pos_t pos = expect(p, token_FUNC);
            decl_t decl = {
                .type = ast_DECL_FUNC,
                .pos = pos,
                .func = {
                    .name = identifier(p),
                },
            };
            expect(p, token_LPAREN);
            expr_t type = {
                .type = ast_TYPE_FUNC,
                .pos = pos,
                .func = {
                    .params = parse_param_type_list(p, false),
                },
            };
            expect(p, token_RPAREN);
            if (p->tok != token_LBRACE && p->tok != token_SEMICOLON) {
                type.func.result = parse_type_spec(p);
            }
            decl.func.type = esc(type);
            if (p->tok == token_LBRACE) {
                decl.func.body = parse_block_stmt(p);
            } else {
                expect(p, token_SEMICOLON);
            }
            return esc(decl);
        }
    default:
        parser_error(p, p->pos, fmt_sprintf("cant handle it: %s", token_string(p->tok)));
        return NULL;
    }
}

static bool isBlingFile(const char *name) {
    return path_matchExt(".bling", name);
}

static bool isTestFile(const char *name) {
    return path_match("*_test.bling", name);
}

extern ast_File **parser_parseDir(const char *path, error_t **first) {
    error_t *err = NULL;
    os_FileInfo **infos = ioutil_read_dir(path, &err);
    if (err) {
        error_move(err, first);
        return NULL;
    }
    slice_t files = slice_init(sizeof(uintptr_t));
    while (*infos != NULL) {
        char *name = os_FileInfo_name(**infos);
        if (isBlingFile(name) && !isTestFile(name)) {
            ast_File *file = parser_parse_file(name);
            files = append(files, &file);
        }
        infos++;
    }
    return slice_to_nil_array(files);
}

static ast_File *parse_file(parser_t *p) {
    expr_t *name = NULL;
    slice_t imports = slice_init(sizeof(uintptr_t));
    slice_t decls = slice_init(sizeof(decl_t *));
    while (p->tok == token_HASH) {
        decl_t *lit = parse_pragma(p);
        decls = append(decls, &lit);
    }
    if (accept(p, token_PACKAGE)) {
        expect(p, token_LPAREN);
        name = identifier(p);
        expect(p, token_RPAREN);
        expect(p, token_SEMICOLON);
    }
    while (p->tok == token_IMPORT) {
        pos_t pos = expect(p, token_IMPORT);
        expr_t *path = basic_lit(p, token_STRING);
        expect(p, token_SEMICOLON);
        decl_t decl = {
            .type = ast_DECL_IMPORT,
            .pos = pos,
            .imp = {
                .path = path,
            },
        };
        decl_t *declp = esc(decl);
        imports = append(imports, &declp);
    }
    while (p->tok != token_EOF) {
        decl_t *decl = parse_decl(p, true);
        decls = append(decls, &decl);
    }
    ast_File file = {
        .filename = p->file->name,
        .name = name,
        .imports = slice_to_nil_array(imports),
        .decls = slice_to_nil_array(decls),
    };
    return esc(file);
}

extern ast_File *parser_parse_file(char *filename) {
    error_t *err = NULL;
    char *src = ioutil_read_file(filename, &err);
    if (err) {
        panic("%s: %s", filename, err->error);
    }
    parser_t p = {};
    parser_init(&p, filename, src);
    ast_File *file = parse_file(&p);
    file->scope = p.pkg_scope;
    free(src);
    return file;
}
