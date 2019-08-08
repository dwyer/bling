#include "bling/parser/parser.h"

#include "bytes/bytes.h"
#include "fmt/fmt.h"
#include "path/path.h"

extern decl_t *parse_pragma(parser_t *p) {
    token$Pos pos = p->pos;
    char *lit = p->lit;
    p->lit = NULL;
    expect(p, token$HASH);
    decl_t decl = {
        .type = ast_DECL_PRAGMA,
        .pos = pos,
        .pragma = {
            .lit = lit,
        },
    };
    return esc(decl);
}

extern void parser_init(parser_t *p, const char *filename, char *src) {
    p->file = token$File_new(filename);
    p->lit = NULL;
    scanner_init(&p->scanner, p->file, src);
    p->scanner.dontInsertSemis = !bytes$hasSuffix(filename, ".bling");
    parser_next(p);
}

extern void parser_declare(parser_t *p, ast_Scope *s, decl_t *decl,
        obj_kind_t kind, expr_t *name) {
    assert(name->type == ast_EXPR_IDENT);
    ast_Object *obj = object_new(kind, name->ident.name);
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

extern void parser_error(parser_t *p, token$Pos pos, char *msg) {
    token$Position position = token$File_position(p->file, pos);
    bytes$Buffer buf = {};
    int i = 0;
    slice$get(&p->file->lines, position.line-1, &i);
    int ch = p->scanner.src[i];
    while (ch > 0 && ch != '\n') {
        bytes$Buffer_writeByte(&buf, ch, NULL);
        i++;
        ch = p->scanner.src[i];
    }
    panic(fmt$sprintf("%s: %s\n%s",
                token$Position_string(&position),
                msg,
                bytes$Buffer_string(&buf)));
}

extern void parser_errorExpected(parser_t *p, token$Pos pos, char *msg) {
    bytes$Buffer buf = {};
    bytes$Buffer_write(&buf, "expected ", -1, NULL);
    bytes$Buffer_write(&buf, msg, -1, NULL);
    if (pos == p->pos) {
        if (p->lit) {
            bytes$Buffer_write(&buf, ", found ", -1, NULL);
            bytes$Buffer_write(&buf, p->lit, -1, NULL);
        } else {
            bytes$Buffer_write(&buf, ", found '", -1, NULL);
            bytes$Buffer_write(&buf, token$string(p->tok), -1, NULL);
            bytes$Buffer_writeByte(&buf, '\'', NULL);
        }
    }
    msg = bytes$Buffer_string(&buf);
    parser_error(p, pos, msg);
    free(msg);
}

extern void parser_next(parser_t *p) {
    p->tok = scanner_scan(&p->scanner, &p->pos, &p->lit);
}

extern bool accept(parser_t *p, token$Token tok0) {
    if (p->tok == tok0) {
        parser_next(p);
        return true;
    }
    return false;
}

extern token$Pos expect(parser_t *p, token$Token tok) {
    token$Pos pos = p->pos;
    if (p->tok != tok) {
        char *lit = p->lit;
        if (lit == NULL) {
            lit = token$string(p->tok);
        }
        parser_errorExpected(p, pos, token$string(tok));
    }
    parser_next(p);
    return pos;
}

extern expr_t *identifier(parser_t *p) {
    expr_t x = {
        .type = ast_EXPR_IDENT,
        .pos = p->pos,
    };
    if (p->tok == token$IDENT) {
        x.ident.name = p->lit;
        p->lit = NULL;
    }
    expect(p, token$IDENT);
    return esc(x);
}

extern expr_t *basic_lit(parser_t *p, token$Token kind) {
    char *value = p->lit;
    p->lit = NULL;
    token$Pos pos = expect(p, kind);
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
    case token$IDENT:
        return identifier(p);
    case token$CHAR:
    case token$FLOAT:
    case token$INT:
    case token$STRING:
        return basic_lit(p, p->tok);
    case token$LPAREN:
        if (p->c_mode) {
            parser_error(p, p->pos, "unreachable");
        } else {
            token$Pos pos = p->pos;
            expect(p, token$LPAREN);
            expr_t x = {
                .type = ast_EXPR_PAREN,
                .pos = pos,
                .paren = {
                    .x = parse_expr(p),
                },
            };
            expect(p, token$RPAREN);
            return esc(x);
        }
    default:
        parser_error(p, p->pos, fmt$sprintf("bad expr: %s: %s", token$string(p->tok), p->lit));
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
        case token$LBRACK:
            {
                expect(p, token$LBRACK);
                expr_t y = {
                    .type = ast_EXPR_INDEX,
                    .pos = x->pos,
                    .index = {
                        .x = x,
                        .index = parse_expr(p),
                    },
                };
                expect(p, token$RBRACK);
                x = esc(y);
            }
            break;
        case token$LPAREN:
            {
                slice$Slice args = {.size = sizeof(expr_t *)};
                expect(p, token$LPAREN);
                // argument_expression_list
                //         : expression
                //         | argument_expression_list ',' expression
                //         ;
                while (p->tok != token$RPAREN) {
                    expr_t *x = parse_expr(p);
                    slice$append(&args, &x);
                    if (!accept(p, token$COMMA)) {
                        break;
                    }
                }
                expect(p, token$RPAREN);
                expr_t call = {
                    .type = ast_EXPR_CALL,
                    .pos = x->pos,
                    .call = {
                        .func = x,
                        .args = slice$to_nil_array(args),
                    },
                };
                x = esc(call);
            }
            break;
        case token$ARROW:
        case token$PERIOD:
            {
                token$Token tok = p->tok;
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
    case token$ADD:
    case token$AND:
    case token$BITWISE_NOT:
    case token$NOT:
    case token$SUB:
        // unary_operator
        //         : '&'
        //         | '*'
        //         | '+'
        //         | '-'
        //         | '~'
        //         | '!'
        //         ;
        {
            token$Pos pos = p->pos;
            token$Token op = p->tok;
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
    case token$MUL:
        {
            token$Pos pos = p->pos;
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
    case token$SIZEOF:
        {
            token$Pos pos = p->pos;
            parser_next(p);
            expect(p, token$LPAREN);
            expr_t x = {
                .type = ast_EXPR_SIZEOF,
                .pos = pos,
                .sizeof_ = {
                    .x = parse_type_spec(p),
                },
            };
            expect(p, token$RPAREN);
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
    token$Pos pos = p->pos;
    if (accept(p, token$LT)) {
        expr_t *type = parse_type_spec(p);
        expect(p, token$GT);
        expr_t *expr = NULL;
        if (p->tok == token$LBRACE) {
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
        token$Token op = p->tok;
        int oprec = token$precedence(op);
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
    expr_t *x = parse_binary_expr(p, token$lowest_prec + 1);
    if (accept(p, token$QUESTION_MARK)) {
        expr_t *consequence = parse_expr(p);
        expect(p, token$COLON);
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
    token$Pos pos = p->pos;
    token$Token keyword = p->tok;
    expr_t *name = NULL;
    expect(p, keyword);
    if (p->tok == token$IDENT) {
        name = identifier(p);
    }
    decl_t **fields = NULL;
    if (accept(p, token$LBRACE)) {
        // struct_declaration_list
        //         : struct_declaration
        //         | struct_declaration_list struct_declaration
        //         ;
        // struct_declaration
        //         : specifier_qualifier_list struct_declarator_list ';'
        //         ;
        slice$Slice slice = {.size = sizeof(decl_t *)};
        for (;;) {
            decl_t decl = {
                .type = ast_DECL_FIELD,
                .pos = p->pos,
            };
            if (p->tok == token$UNION) {
                // anonymous union
                decl.field.type = parse_type_spec(p);
            } else {
                decl.field.name = identifier(p);
                decl.field.type = parse_type_spec(p);
            }
            expect(p, token$SEMICOLON);
            decl_t *field = esc(decl);
            slice$append(&slice, &field);
            if (p->tok == token$RBRACE) {
                break;
            }
        }
        expect(p, token$RBRACE);
        fields = slice$to_nil_array(slice);
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
    token$Pos pos = p->pos;
    expect(p, token$ENUM);
    if (p->tok == token$IDENT) {
        name = identifier(p);
    }
    decl_t **enums = NULL;
    if (accept(p, token$LBRACE)) {
        // enumerator_list : enumerator | enumerator_list ',' enumerator ;
        slice$Slice list = {.size = sizeof(decl_t *)};
        for (;;) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            decl_t decl = {
                .type = ast_DECL_VALUE,
                .pos = p->pos,
                .value = {
                    .name = identifier(p),
                },
            };
            if (accept(p, token$ASSIGN)) {
                decl.value.value = parse_const_expr(p);
            }
            decl_t *enumerator = esc(decl);
            slice$append(&list, &enumerator);
            if (!accept(p, token$COMMA) || p->tok == token$RBRACE) {
                break;
            }
        }
        enums = slice$to_nil_array(list);
        expect(p, token$RBRACE);
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
    token$Pos pos = p->pos;
    expect(p, token$MUL);
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
    slice$Slice params = {.size = sizeof(decl_t *)};
    while (p->tok != token$RPAREN) {
        decl_t *param = parse_field(p, anon);
        slice$append(&params, &param);
        if (!accept(p, token$COMMA)) {
            break;
        }
        token$Pos pos = p->pos;
        if (accept(p, token$ELLIPSIS)) {
            // TODO make this a type
            decl_t decl = {
                .type = ast_DECL_FIELD,
                .pos = pos,
            };
            decl_t *param = esc(decl);
            slice$append(&params, &param);
            break;
        }
    }
    return slice$to_nil_array(params);
}

static expr_t *parse_init_expr(parser_t *p) {
    // initializer
    //         : expression
    //         | '{' initializer_list ','? '}'
    //         ;
    if (p->tok == token$LBRACE) {
        // initializer_list
        //         : designation? initializer
        //         | initializer_list ',' designation? initializer
        //         ;
        token$Pos pos = p->pos;
        expect(p, token$LBRACE);
        slice$Slice list = {.size = sizeof(expr_t *)};
        while (p->tok != token$RBRACE && p->tok != token$EOF) {
            expr_t *value = parse_init_expr(p);
            if (value->type == ast_EXPR_IDENT && accept(p, token$COLON)) {
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
            slice$append(&list, &value);
            if (!accept(p, token$COMMA)) {
                break;
            }
        }
        expect(p, token$RBRACE);
        expr_t expr = {
            .type = ast_EXPR_COMPOUND,
            .pos = pos,
            .compound = {
                .list = slice$to_nil_array(list),
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
    token$Token op = p->tok;
    switch (op) {
    case token$ADD_ASSIGN:
    case token$ASSIGN:
    case token$DIV_ASSIGN:
    case token$MOD_ASSIGN:
    case token$MUL_ASSIGN:
    case token$SHL_ASSIGN:
    case token$SHR_ASSIGN:
    case token$SUB_ASSIGN:
    case token$XOR_ASSIGN:
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
    case token$INC:
    case token$DEC:
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
        if (accept(p, token$COLON)) {
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
    token$Pos pos = expect(p, token$FOR);
    stmt_t *init = NULL;
    if (!accept(p, token$SEMICOLON)) {
        init = parse_stmt(p);
    }
    expr_t *cond = NULL;
    if (p->tok != token$SEMICOLON) {
        cond = parse_expr(p);
    }
    expect(p, token$SEMICOLON);
    stmt_t *post = NULL;
    if (p->tok != token$LBRACE) {
        post = parse_simple_stmt(p, false);
    }
    stmt_t *body = parse_block_stmt(p);
    stmt_t stmt = {
        .type = ast_STMT_ITER,
        .pos = pos,
        .iter = {
            .kind = token$FOR,
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
    token$Pos pos = expect(p, token$IF);
    expr_t *cond = parse_expr(p);
    if (p->tok != token$LBRACE) {
        parser_error(p, p->pos, "`if` must be followed by a compound_statement");
    }
    stmt_t *body = parse_block_stmt(p);
    stmt_t *else_ = NULL;
    if (accept(p, token$ELSE)) {
        if (p->tok == token$IF) {
            else_ = parse_stmt(p);
        } else if (p->tok == token$LBRACE) {
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
    token$Pos pos = expect(p, token$RETURN);
    expr_t *x = NULL;
    if (p->tok != token$SEMICOLON) {
        x = parse_expr(p);
    }
    expect(p, token$SEMICOLON);
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
    token$Pos pos = expect(p, token$SWITCH);
    expr_t *tag = parse_expr(p);
    expect(p, token$LBRACE);
    slice$Slice clauses = {.size = sizeof(stmt_t *)};
    while (p->tok == token$CASE || p->tok == token$DEFAULT) {
        // case_statement
        //         | CASE constant_expression ':' statement+
        //         | DEFAULT ':' statement+
        //         ;
        slice$Slice exprs = {.size=sizeof(expr_t *)};
        token$Pos pos = p->pos;
        if (accept(p, token$CASE)) {
            for (;;) {
                expr_t *expr = parse_const_expr(p);
                slice$append(&exprs, &expr);
                if (!accept(p, token$COMMA)) {
                    break;
                }
            }
        } else {
            expect(p, token$DEFAULT);
        }
        expect(p, token$COLON);
        slice$Slice stmts = {.size = sizeof(stmt_t *)};
        bool loop = true;
        while (loop) {
            switch (p->tok) {
            case token$CASE:
            case token$DEFAULT:
            case token$RBRACE:
                loop = false;
                break;
            default:
                break;
            }
            if (loop) {
                stmt_t *stmt = parse_stmt(p);
                slice$append(&stmts, &stmt);
            }
        }
        stmt_t stmt = {
            .type = ast_STMT_CASE,
            .pos = pos,
            .case_ = {
                .exprs = slice$to_nil_array(exprs),
                .stmts = slice$to_nil_array(stmts),
            },
        };
        stmt_t *clause = esc(stmt);
        slice$append(&clauses, &clause);
    }
    expect(p, token$RBRACE);
    accept(p, token$SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_SWITCH,
        .pos = pos,
        .switch_ = {
            .tag = tag,
            .stmts = slice$to_nil_array(clauses),
        },
    };
    return esc(stmt);
}

static stmt_t *parse_while_stmt(parser_t *p) {
    // while_statement : WHILE expression compound_statement ;
    token$Pos pos = expect(p, token$WHILE);
    expr_t *cond = parse_expr(p);
    stmt_t *body = parse_block_stmt(p);
    stmt_t stmt = {
        .type = ast_STMT_ITER,
        .pos = pos,
        .iter = {
            .kind = token$WHILE,
            .cond = cond,
            .body = body,
        },
    };
    return esc(stmt);
}

static stmt_t *parse_jump_stmt(parser_t *p, token$Token keyword) {
    // jump_statement
    //         : GOTO IDENTIFIER ';'
    //         | CONTINUE ';'
    //         | BREAK ';'
    //         ;
    token$Pos pos = expect(p, keyword);
    expr_t *label = NULL;
    if (keyword == token$GOTO) {
        label = identifier(p);
    }
    expect(p, token$SEMICOLON);
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
    case token$VAR: return parse_decl_stmt(p);
    case token$FOR: return parse_for_stmt(p);
    case token$IF: return parse_if_stmt(p);
    case token$RETURN: return parse_return_stmt(p);
    case token$SWITCH: return parse_switch_stmt(p);
    case token$WHILE: return parse_while_stmt(p);
    case token$BREAK:
    case token$CONTINUE:
    case token$GOTO: return parse_jump_stmt(p, p->tok);
    case token$LBRACE: return parse_block_stmt(p);
    default:
        break;
    }
    token$Pos pos = p->pos;
    if (accept(p, token$SEMICOLON)) {
        stmt_t stmt = {
            .type = ast_STMT_EXPR,
            .pos = pos,
        };
        return esc(stmt);
    }
    stmt_t *stmt = parse_simple_stmt(p, true);
    if (stmt->type != ast_STMT_LABEL) {
        expect(p, token$SEMICOLON);
    }
    return stmt;
}

static stmt_t *parse_block_stmt(parser_t *p) {
    // compound_statement : '{' statement_list? '}' ;
    slice$Slice stmts = {.size = sizeof(stmt_t *)};
    token$Pos pos = expect(p, token$LBRACE);
    // statement_list : statement+ ;
    while (p->tok != token$RBRACE) {
        stmt_t *stmt = parse_stmt(p);
        slice$append(&stmts, &stmt);
    }
    expect(p, token$RBRACE);
    accept(p, token$SEMICOLON);
    stmt_t stmt = {
        .type = ast_STMT_BLOCK,
        .pos = pos,
        .block = {
            .stmts = slice$to_nil_array(stmts),
        }
    };
    return esc(stmt);
}

static decl_t *parse_field(parser_t *p, bool anon) {
    decl_t decl = {
        .type = ast_DECL_FIELD,
        .pos = p->pos,
    };
    if (p->tok == token$IDENT) {
        decl.field.name = identifier(p);
    }
    if (decl.field.name != NULL && (p->tok == token$COMMA || p->tok == token$RPAREN)) {
        decl.field.type = decl.field.name;
        decl.field.name = NULL;
    } else {
        decl.field.type = parse_type_spec(p);
    }
    return esc(decl);
}

static expr_t *parse_func_type(parser_t *p) {
    token$Pos pos = expect(p, token$FUNC);
    expect(p, token$LPAREN);
    decl_t **params = parse_param_type_list(p, false);
    expect(p, token$RPAREN);
    expr_t *result = NULL;
    if (p->tok != token$SEMICOLON) {
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

static expr_t *parse_type_qualifier(parser_t *p, token$Token tok) {
    expect(p, tok);
    expr_t *type = parse_type_spec(p);
    type->is_const = true;
    return type;
}

static expr_t *parse_type_spec(parser_t *p) {
    expr_t *x = NULL;
    switch (p->tok) {
    case token$CONST:
        x = parse_type_qualifier(p, p->tok);
        break;
    case token$IDENT:
        x = identifier(p);
        break;
    case token$MUL:
        x = parse_pointer(p);
        break;
    case token$STRUCT:
    case token$UNION:
        x = parse_struct_or_union_spec(p);
        break;
    case token$ENUM:
        x = parse_enum_spec(p);
        break;
    case token$FUNC:
        x = parse_func_type(p);
        break;
    case token$LBRACK:
        {
            token$Pos pos = expect(p, token$LBRACK);
            expr_t *len = NULL;
            if (p->tok != token$RBRACK) {
                len = parse_const_expr(p);
            }
            expect(p, token$RBRACK);
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
        parser_errorExpected(p, p->pos, "type");
        break;
    }
    return x;
}

static decl_t *parse_decl(parser_t *p, bool is_external) {
    switch (p->tok) {
    case token$HASH:
        return parse_pragma(p);
    case token$TYPEDEF:
        {
            token$Token keyword = p->tok;
            token$Pos pos = expect(p, keyword);
            expr_t *ident = identifier(p);
            expr_t *type = parse_type_spec(p);
            expect(p, token$SEMICOLON);
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
    case token$CONST:
    case token$VAR:
        {
            token$Token tok = p->tok;
            token$Pos pos = expect(p, tok);
            expr_t *ident = identifier(p);
            expr_t *type = NULL;
            if (p->tok != token$ASSIGN) {
                type = parse_type_spec(p);
            }
            expr_t *value = NULL;
            if (accept(p, token$ASSIGN)) {
                value = parse_init_expr(p);
            }
            expect(p, token$SEMICOLON);
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
    case token$FUNC:
        {
            token$Pos pos = expect(p, token$FUNC);
            decl_t decl = {
                .type = ast_DECL_FUNC,
                .pos = pos,
                .func = {
                    .name = identifier(p),
                },
            };
            expect(p, token$LPAREN);
            expr_t type = {
                .type = ast_TYPE_FUNC,
                .pos = pos,
                .func = {
                    .params = parse_param_type_list(p, false),
                },
            };
            expect(p, token$RPAREN);
            if (p->tok != token$LBRACE && p->tok != token$SEMICOLON) {
                type.func.result = parse_type_spec(p);
            }
            decl.func.type = esc(type);
            if (p->tok == token$LBRACE) {
                decl.func.body = parse_block_stmt(p);
            } else {
                expect(p, token$SEMICOLON);
            }
            return esc(decl);
        }
    default:
        parser_error(p, p->pos, fmt$sprintf("cant handle it: %s", token$string(p->tok)));
        return NULL;
    }
}

static bool isBlingFile(const char *name) {
    return bytes$hasSuffix(name, ".bling");
}

static bool isTestFile(const char *name) {
    return path$match("*_test.bling", name);
}

extern ast_File **parser_parseDir(const char *path, error$Error **first) {
    error$Error *err = NULL;
    os$FileInfo **infos = ioutil$readDir(path, &err);
    if (err) {
        error$move(err, first);
        return NULL;
    }
    slice$Slice files = slice$init(sizeof(uintptr_t));
    while (*infos != NULL) {
        char *name = os$FileInfo_name(**infos);
        if (isBlingFile(name) && !isTestFile(name)) {
            ast_File *file = parser_parse_file(name);
            slice$append(&files, &file);
        }
        infos++;
    }
    return slice$to_nil_array(files);
}

static ast_File *parse_file(parser_t *p) {
    expr_t *name = NULL;
    slice$Slice imports = slice$init(sizeof(uintptr_t));
    slice$Slice decls = slice$init(sizeof(decl_t *));
    while (p->tok == token$HASH) {
        decl_t *lit = parse_pragma(p);
        slice$append(&decls, &lit);
    }
    if (accept(p, token$PACKAGE)) {
        name = identifier(p);
        expect(p, token$SEMICOLON);
    }
    while (p->tok == token$IMPORT) {
        token$Pos pos = expect(p, token$IMPORT);
        expr_t *path = basic_lit(p, token$STRING);
        expect(p, token$SEMICOLON);
        decl_t decl = {
            .type = ast_DECL_IMPORT,
            .pos = pos,
            .imp = {
                .path = path,
            },
        };
        decl_t *declp = esc(decl);
        slice$append(&imports, &declp);
    }
    while (p->tok != token$EOF) {
        decl_t *decl = parse_decl(p, true);
        slice$append(&decls, &decl);
    }
    ast_File file = {
        .filename = p->file->name,
        .name = name,
        .imports = slice$to_nil_array(imports),
        .decls = slice$to_nil_array(decls),
    };
    return esc(file);
}

extern ast_File *parser_parse_file(const char *filename) {
    error$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic("%s: %s", filename, err->error);
    }
    parser_t p = {};
    parser_init(&p, filename, src);
    ast_File *file = parse_file(&p);
    free(src);
    return file;
}
