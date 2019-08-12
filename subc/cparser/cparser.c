#include "subc/cparser/cparser.h"

#include "sys/sys.h"

static ast$Expr *cast_expression(parser$Parser *p);
static ast$Expr *expression(parser$Parser *p);
static ast$Expr *constant_expression(parser$Parser *p);
static ast$Expr *initializer(parser$Parser *p);

static ast$Expr *type_specifier(parser$Parser *p);
static ast$Expr *struct_or_union_specifier(parser$Parser *p);
static ast$Expr *enum_specifier(parser$Parser *p);
static ast$Expr *pointer(parser$Parser *p, ast$Expr *type);
static ast$Decl **parameter_type_list(parser$Parser *p);
static ast$Expr *type_name(parser$Parser *p);

static ast$Expr *declarator(parser$Parser *p, ast$Expr **type_ptr);
static ast$Decl *abstract_declarator(parser$Parser *p, ast$Expr *type);

static ast$Stmt *statement(parser$Parser *p);
static ast$Stmt *compound_statement(parser$Parser *p, bool allow_single);

static ast$Expr *specifier_qualifier_list(parser$Parser *p);
static ast$Expr *declaration_specifiers(parser$Parser *p, bool is_top);
static ast$Decl *declaration(parser$Parser *p, bool is_external);

static ast$Decl *parameter_declaration(parser$Parser *p);

static bool is_type(parser$Parser *p) {
    switch (p->tok) {
    case token$CONST:
    case token$ENUM:
    case token$EXTERN:
    case token$SIGNED:
    case token$STATIC:
    case token$STRUCT:
    case token$UNION:
    case token$UNSIGNED:
        return true;
    case token$IDENT:
        {
            ast$Object *obj = ast$Scope_lookup(p->pkg_scope, p->lit);
            return obj && obj->kind == ast$ObjKind_TYPE;
        }
    default:
        return false;
    }
}

static ast$Expr *postfix_expression(parser$Parser *p, ast$Expr *x) {
    // postfix_expression
    //         : primary_expression
    //         | postfix_expression '[' expression ']'
    //         | postfix_expression '(' argument_expression_list? ')'
    //         | postfix_expression '.' IDENTIFIER
    //         | postfix_expression '->' IDENTIFIER
    //         ;
    if (x == NULL) {
        x = parser$parsePrimaryExpr(p);
    }
    for (;;) {
        switch (p->tok) {
        case token$LBRACK:
            {
                parser$expect(p, token$LBRACK);
                ast$Expr y = {
                    .type = ast$EXPR_INDEX,
                    .index = {
                        .x = x,
                        .index = expression(p),
                    },
                };
                parser$expect(p, token$RBRACK);
                x = esc(y);
            }
            break;
        case token$LPAREN:
            {
                utils$Slice args = {.size = sizeof(ast$Expr *)};
                parser$expect(p, token$LPAREN);
                // argument_expression_list
                //         : expression
                //         | argument_expression_list ',' expression
                //         ;
                while (p->tok != token$RPAREN) {
                    ast$Expr *x = expression(p);
                    utils$Slice_append(&args, &x);
                    if (!parser$accept(p, token$COMMA)) {
                        break;
                    }
                }
                parser$expect(p, token$RPAREN);
                ast$Expr call = {
                    .type = ast$EXPR_CALL,
                    .call = {
                        .func = x,
                        .args = utils$Slice_to_nil_array(args),
                    },
                };
                x = esc(call);
            }
            break;
        case token$ARROW:
        case token$PERIOD:
            {
                token$Token tok = p->tok;
                parser$next(p);
                ast$Expr y = {
                    .type = ast$EXPR_SELECTOR,
                    .selector = {
                        .x = x,
                        .tok = tok,
                        .sel = parser$parseIdent(p),
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

static ast$Expr *unary_expression(parser$Parser *p) {
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
            token$Token op = p->tok;
            parser$next(p);
            ast$Expr x = {
                .type = ast$EXPR_UNARY,
                .unary = {
                    .op = op,
                    .x = cast_expression(p),
                },
            };
            return esc(x);
        }
    case token$MUL:
        {
            parser$next(p);
            ast$Expr x = {
                .type = ast$EXPR_STAR,
                .star = {
                    .x = cast_expression(p),
                },
            };
            return esc(x);
        }
    case token$SIZEOF:
        {
            parser$next(p);
            parser$expect(p, token$LPAREN);
            ast$Expr *x = NULL;
            if (is_type(p)) {
                x = type_name(p);
                if (p->tok == token$MUL) {
                    declarator(p, &x); // TODO assert result == NULL?
                }
            } else {
                x = unary_expression(p);
            }
            parser$expect(p, token$RPAREN);
            ast$Expr y = {
                .type = ast$EXPR_SIZEOF,
                .sizeof_ = {
                    .x = x,
                },
            };
            return esc(y);
        }
    case token$DEC:
    case token$INC:
        parser$error(p, p->pos, sys$sprintf("unary `%s` not supported in subc", token$string(p->tok)));
        return NULL;
    default:
        return postfix_expression(p, NULL);
    }
}

static ast$Expr *cast_expression(parser$Parser *p) {
    // cast_expression
    //         : unary_expression
    //         | '(' type_name ')' cast_expression
    //         | '(' type_name ')' initializer
    //         ;
    if (parser$accept(p, token$LPAREN)) {
        if (is_type(p)) {
            ast$Expr *type = type_name(p);
            parser$expect(p, token$RPAREN);
            ast$Expr *x;
            if (p->tok == token$LBRACE) {
                x = initializer(p);
            } else {
                x = cast_expression(p);
            }
            ast$Expr y = {
                .type = ast$EXPR_CAST,
                .cast = {
                    .type = type,
                    .expr = x,
                },
            };
            return esc(y);
        } else {
            ast$Expr x = {
                .type = ast$EXPR_PAREN,
                .paren = {
                    .x = expression(p),
                },
            };
            parser$expect(p, token$RPAREN);
            return postfix_expression(p, esc(x));
        }
    }
    return unary_expression(p);
}

static ast$Expr *binary_expression(parser$Parser *p, int prec1) {
    ast$Expr *x = cast_expression(p);
    for (;;) {
        token$Token op = p->tok;
        int oprec = token$precedence(op);
        if (oprec < prec1) {
            return x;
        }
        parser$expect(p, op);
        ast$Expr *y = binary_expression(p, oprec + 1);
        ast$Expr z = {
            .type = ast$EXPR_BINARY,
            .binary = {
                .x = x,
                .op = op,
                .y = y,
            },
        };
        x = esc(z);
    }
}

static ast$Expr *ternary_expression(parser$Parser *p) {
    // ternary_expression
    //         : binary_expression
    //         | binary_expression '?' expression ':' ternary_expression
    //         ;
    ast$Expr *x = binary_expression(p, token$lowest_prec + 1);
    if (parser$accept(p, token$QUESTION_MARK)) {
        ast$Expr *consequence = expression(p);
        parser$expect(p, token$COLON);
        ast$Expr *alternative = ternary_expression(p);
        ast$Expr conditional = {
            .type = ast$EXPR_COND,
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

static ast$Expr *expression(parser$Parser *p) {
    // expression : ternary_expression ;
    return ternary_expression(p);
}

static ast$Expr *constant_expression(parser$Parser *p) {
    // constant_expression : ternary_expression ;
    return ternary_expression(p);
}

static ast$Expr *struct_or_union_specifier(parser$Parser *p) {
    // struct_or_union_specifier
    //         : struct_or_union IDENTIFIER
    //         | struct_or_union IDENTIFIER '{' struct_declaration_list '}'
    //         | struct_or_union '{' struct_declaration_list '}'
    //         ;
    // struct_or_union : STRUCT | UNION ;
    token$Token keyword = p->tok;
    ast$Expr *name = NULL;
    parser$expect(p, keyword);
    if (p->tok == token$IDENT) {
        name = parser$parseIdent(p);
    }
    ast$Decl **fields = NULL;
    if (parser$accept(p, token$LBRACE)) {
        // struct_declaration_list
        //         : struct_declaration
        //         | struct_declaration_list struct_declaration
        //         ;
        // struct_declaration
        //         : specifier_qualifier_list struct_declarator_list ';'
        //         ;
        utils$Slice fieldSlice = {.size = sizeof(ast$Decl *)};
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
            ast$Expr *type = specifier_qualifier_list(p);
            ast$Expr *name = declarator(p, &type);
            ast$Decl f = {
                .type = ast$DECL_FIELD,
                .field = {
                    .type = type,
                    .name = name,
                },
            };
            parser$expect(p, token$SEMICOLON);
            ast$Decl *field = esc(f);
            utils$Slice_append(&fieldSlice, &field);
            if (p->tok == token$RBRACE) {
                break;
            }
        }
        parser$expect(p, token$RBRACE);
        fields = utils$Slice_to_nil_array(fieldSlice);
    }
    // TODO assert name or fields
    ast$Expr x = {
        .type = ast$TYPE_STRUCT,
        .struct_ = {
            .tok = keyword,
            .name = name,
            .fields = fields,
        },
    };
    return esc(x);
}

static ast$Expr *enum_specifier(parser$Parser *p) {
    // enum_specifier
    //         : ENUM '{' enumerator_list '}'
    //         | ENUM IDENTIFIER '{' enumerator_list '}'
    //         | ENUM IDENTIFIER
    //         ;
    ast$Expr *name = NULL;
    parser$expect(p, token$ENUM);
    if (p->tok == token$IDENT) {
        name = parser$parseIdent(p);
    }
    ast$Decl **enums = NULL;
    if (parser$accept(p, token$LBRACE)) {
        // enumerator_list : enumerator | enumerator_list ',' enumerator ;
        utils$Slice list = {.size = sizeof(ast$Decl *)};
        for (;;) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            ast$Decl decl = {
                .type = ast$DECL_VALUE,
                .value = {
                    .name = parser$parseIdent(p),
                    .kind = token$VAR,
                },
            };
            if (parser$accept(p, token$ASSIGN)) {
                decl.value.value = constant_expression(p);
            }
            ast$Decl *enumerator = esc(decl);
            utils$Slice_append(&list, &enumerator);
            if (!parser$accept(p, token$COMMA) || p->tok == token$RBRACE) {
                break;
            }
        }
        enums = utils$Slice_to_nil_array(list);
        parser$expect(p, token$RBRACE);
    }
    ast$Expr x = {
        .type = ast$TYPE_ENUM,
        .enum_ = {
            .name = name,
            .enums = enums,
        },
    };
    return esc(x);
}

static ast$Expr *declarator(parser$Parser *p, ast$Expr **type_ptr) {
    // declarator : pointer? direct_declarator ;
    if (p->tok == token$MUL) {
        *type_ptr = pointer(p, *type_ptr);
    }
    // direct_declarator
    //         : IDENTIFIER
    //         | '(' declarator ')'
    //         | direct_declarator '[' constant_expression? ']'
    //         | direct_declarator '(' parameter_type_list? ')'
    //         ;
    ast$Expr *name = NULL;
    bool is_ptr = false;
    switch (p->tok) {
    case token$IDENT:
        name = parser$parseIdent(p);
        break;
    case token$LPAREN:
        parser$expect(p, token$LPAREN);
        is_ptr = parser$accept(p, token$MUL);
        if (!is_ptr || p->tok == token$IDENT) {
            name = parser$parseIdent(p);
        }
        parser$expect(p, token$RPAREN);
        break;
    default:
        break;
    }
    if (parser$accept(p, token$LBRACK)) {
        ast$Expr *len = NULL;
        if (p->tok != token$RBRACK) {
            len = constant_expression(p);
        }
        ast$Expr type = {
            .type = ast$TYPE_ARRAY,
            .array = {
                .elt = *type_ptr,
                .len = len,
            },
        };
        parser$expect(p, token$RBRACK);
        *type_ptr = esc(type);
    } else if (parser$accept(p, token$LPAREN)) {
        ast$Decl **params = NULL;
        if (p->tok != token$RPAREN) {
            params = parameter_type_list(p);
        }
        ast$Expr type = {
            .type = ast$TYPE_FUNC,
            .func = {
                .result = *type_ptr,
                .params = params,
            },
        };
        parser$expect(p, token$RPAREN);
        *type_ptr = esc(type);
    }
    if (is_ptr) {
        ast$Expr type = {
            .type = ast$EXPR_STAR,
            .star = {
                .x = *type_ptr,
            }
        };
        *type_ptr = esc(type);
    }
    return name;
}

static ast$Expr *type_qualifier(parser$Parser *p, ast$Expr *type) {
    // type_qualifier_list : type_qualifier+ ;
    if (parser$accept(p, token$CONST)) {
        type->is_const = true;
    }
    return type;
}

static ast$Expr *pointer(parser$Parser *p, ast$Expr *type) {
    // pointer : '*' type_qualifier_list? pointer? ;
    while (parser$accept(p, token$MUL)) {
        ast$Expr x = {
            .type = ast$EXPR_STAR,
            .star = {
                .x = type,
            },
        };
        type = esc(x);
        type = type_qualifier(p, type);
    }
    return type;
}

static ast$Decl **parameter_type_list(parser$Parser *p) {
    // parameter_type_list
    //         : parameter_list
    //         | parameter_list ',' '...'
    //         ;
    // parameter_list
    //         : parameter_declaration
    //         | parameter_list ',' parameter_declaration
    //         ;
    utils$Slice params = {.size = sizeof(ast$Decl *)};
    while (p->tok != token$RPAREN) {
        // parameter_declaration
        //         : declaration_specifiers (declarator | abstract_declarator)?
        //         ;
        ast$Decl *param = parameter_declaration(p);
        utils$Slice_append(&params, &param);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
        if (parser$accept(p, token$ELLIPSIS)) {
            ast$Decl decl = {
                .type = ast$DECL_FIELD,
            };
            ast$Decl *param = esc(decl);
            utils$Slice_append(&params, &param);
            break;
        }
    }
    return utils$Slice_to_nil_array(params);
}

static ast$Expr *type_name(parser$Parser *p) {
    // type_name
    //         : specifier_qualifier_list
    //         | specifier_qualifier_list abstract_declarator
    //         ;
    ast$Expr *type = specifier_qualifier_list(p);
    ast$Decl *decl = abstract_declarator(p, type);
    return decl->field.type;
}

static ast$Decl *abstract_declarator(parser$Parser *p, ast$Expr *type) {
    // abstract_declarator
    //         : pointer? direct_abstract_declarator?
    //         ;
    if (p->tok == token$MUL) {
        type = pointer(p, type);
    }
    // direct_abstract_declarator
    //         : '(' abstract_declarator ')'
    //         | direct_abstract_declarator? '[' constant_expression? ']'
    //         | direct_abstract_declarator? '(' parameter_type_list? ')'
    //         ;
    bool is_ptr = false;
    if (parser$accept(p, token$LPAREN)) {
        parser$expect(p, token$MUL);
        parser$expect(p, token$RPAREN);
        is_ptr = true;
    }
    for (;;) {
        if (parser$accept(p, token$LBRACK)) {
            ast$Expr *len = NULL;
            if (p->tok != token$RBRACK) {
                len = constant_expression(p);
            }
            ast$Expr t = {
                .type = ast$TYPE_ARRAY,
                .array = {
                    .elt = type,
                    .len = len,
                },
            };
            parser$expect(p, token$RBRACK);
            type = esc(t);
        } else if (parser$accept(p, token$LPAREN)) {
            ast$Decl **params = NULL;
            if (p->tok != token$RPAREN) {
                params = parameter_type_list(p);
            }
            ast$Expr t = {
                .type = ast$TYPE_FUNC,
                .func = {
                    .result = type,
                    .params = params,
                },
            };
            parser$expect(p, token$RPAREN);
            type = esc(t);
        } else {
            break;
        }
    }
    if (is_ptr) {
        ast$Expr tmp = {
            .type = ast$EXPR_STAR,
            .star = {
                .x = type,
            },
        };
        type = esc(tmp);
    }
    ast$Decl declarator = {
        .type = ast$DECL_FIELD,
        .field = {
            .type = type,
        },
    };
    return esc(declarator);
}

static ast$Expr *initializer(parser$Parser *p) {
    // initializer
    //         : expression
    //         | '{' initializer_list ','? '}'
    //         ;
    if (!parser$accept(p, token$LBRACE)) {
        return expression(p);
    }
    // initializer_list
    //         : designation? initializer
    //         | initializer_list ',' designation? initializer
    //         ;
    utils$Slice list = {.size = sizeof(ast$Expr *)};
    while (p->tok != token$RBRACE && p->tok != token$EOF) {
        ast$Expr *key = NULL;
        bool isArray = false;
        // designation : designator_list '=' ;
        // designator_list : designator+ ;
        // designator : '.' parser$parseIdent
        if (parser$accept(p, token$PERIOD)) {
            key = parser$parseIdent(p);
            parser$expect(p, token$ASSIGN);
        } else if (parser$accept(p, token$LBRACK)) {
            isArray = true;
            key = expression(p);
            parser$expect(p, token$RBRACK);
            parser$expect(p, token$ASSIGN);
        }
        ast$Expr *value = initializer(p);
        if (key) {
            ast$Expr x = {
                .type = ast$EXPR_KEY_VALUE,
                .key_value = {
                    .key = key,
                    .value = value,
                    .isArray = isArray,
                },
            };
            value = esc(x);
        }
        utils$Slice_append(&list, &value);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
    }
    parser$expect(p, token$RBRACE);
    ast$Expr expr = {
        .type = ast$EXPR_COMPOUND,
        .compound = {
            .list = utils$Slice_to_nil_array(list),
        },
    };
    return esc(expr);
}

static ast$Stmt *simple_statement(parser$Parser *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    ast$Expr *x = expression(p);
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
            parser$next(p);
            ast$Expr *y = expression(p);
            ast$Stmt stmt = {
                .type = ast$STMT_ASSIGN,
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
            parser$next(p);
            ast$Stmt stmt = {
                .type = ast$STMT_POSTFIX,
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
    if (labelOk && x->type == ast$EXPR_IDENT) {
        if (parser$accept(p, token$COLON)) {
            ast$Stmt stmt = {
                .type = ast$STMT_LABEL,
                .label = {
                    .label = x,
                    .stmt = statement(p),
                },
            };
            return esc(stmt);
        }
    }
    // expression_statement : expression? ';' ;
    ast$Stmt stmt = {
        .type = ast$STMT_EXPR,
        .expr = {.x = x},
    };
    return esc(stmt);
}

static ast$Stmt *statement(parser$Parser *p) {
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
        ast$Stmt stmt = {
            .type = ast$STMT_DECL,
            .decl = {
                .decl = declaration(p, false),
            },
        };
        return esc(stmt);
    }

    if (parser$accept(p, token$FOR)) {
        // for_statement
        //         | FOR '(' simple_statement? ';' expression? ';' expression? ')'
        //              compound_statement ;
        parser$expect(p, token$LPAREN);
        ast$Stmt *init = NULL;
        if (!parser$accept(p, token$SEMICOLON)) {
            init = statement(p);
        }
        ast$Expr *cond = NULL;
        if (p->tok != token$SEMICOLON) {
            cond = expression(p);
        }
        parser$expect(p, token$SEMICOLON);
        ast$Stmt *post = NULL;
        if (p->tok != token$RPAREN) {
            post = simple_statement(p, false);
        }
        parser$expect(p, token$RPAREN);
        ast$Stmt *body = compound_statement(p, true);
        ast$Stmt stmt = {
            .type = ast$STMT_ITER,
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

    if (parser$accept(p, token$IF)) {
        // if_statement
        //         : IF '(' expression ')' statement
        //         | IF '(' expression ')' statement ELSE statement
        //         | IF '(' expression ')' statement ELSE if_statement
        //         ;
        parser$expect(p, token$LPAREN);
        ast$Expr *cond = expression(p);
        parser$expect(p, token$RPAREN);
        ast$Stmt *body = compound_statement(p, true);
        ast$Stmt *else_ = NULL;
        if (parser$accept(p, token$ELSE)) {
            if (p->tok == token$IF) {
                else_ = statement(p);
            } else {
                else_ = compound_statement(p, true);
            }
        }
        ast$Stmt stmt = {
            .type = ast$STMT_IF,
            .if_ = {
                .cond = cond,
                .body = body,
                .else_ = else_,
            },
        };
        return esc(stmt);
    }

    if (parser$accept(p, token$RETURN)) {
        // return_statement
        //         : RETURN expression? ';'
        //         ;
        ast$Expr *x = NULL;
        if (p->tok != token$SEMICOLON) {
            x = expression(p);
        }
        parser$expect(p, token$SEMICOLON);
        ast$Stmt stmt = {
            .type = ast$STMT_RETURN,
            .return_ = {
                .x = x,
            },
        };
        return esc(stmt);
    }

    if (parser$accept(p, token$SWITCH)) {
        // switch_statement : SWITCH '(' expression ')' case_statement* ;
        parser$expect(p, token$LPAREN);
        ast$Expr *tag = expression(p);
        parser$expect(p, token$RPAREN);
        parser$expect(p, token$LBRACE);
        utils$Slice clauses = {.size = sizeof(ast$Stmt *)};
        while (p->tok == token$CASE || p->tok == token$DEFAULT) {
            // case_statement
            //         : CASE constant_expression ':' statement+
            //         | DEFAULT ':' statement+
            //         ;
            utils$Slice exprs = {.size=sizeof(ast$Expr *)};
            while (parser$accept(p, token$CASE)) {
                ast$Expr *expr = constant_expression(p);
                parser$expect(p, token$COLON);
                utils$Slice_append(&exprs, &expr);
            }
            if (utils$Slice_len(&exprs) == 0) {
                parser$expect(p, token$DEFAULT);
                parser$expect(p, token$COLON);
            }
            utils$Slice stmts = {.size = sizeof(ast$Stmt *)};
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
                    ast$Stmt *stmt = statement(p);
                    utils$Slice_append(&stmts, &stmt);
                }
            }
            ast$Stmt stmt = {
                .type = ast$STMT_CASE,
                .case_ = {
                    .exprs = utils$Slice_to_nil_array(exprs),
                    .stmts = utils$Slice_to_nil_array(stmts),
                },
            };
            ast$Stmt *clause = esc(stmt);
            utils$Slice_append(&clauses, &clause);
        }
        parser$expect(p, token$RBRACE);
        ast$Stmt stmt = {
            .type = ast$STMT_SWITCH,
            .switch_ = {
                .tag = tag,
                .stmts = utils$Slice_to_nil_array(clauses),
            },
        };
        return esc(stmt);
    }

    if (parser$accept(p, token$WHILE)) {
        // while_statement : WHILE '(' expression ')' statement ;
        parser$expect(p, token$LPAREN);
        ast$Expr *cond = expression(p);
        parser$expect(p, token$RPAREN);
        ast$Stmt *body = compound_statement(p, true);
        ast$Stmt stmt = {
            .type = ast$STMT_ITER,
            .iter = {
                .kind = token$WHILE,
                .cond = cond,
                .body = body,
            },
        };
        return esc(stmt);
    }

    switch (p->tok) {
    case token$BREAK:
    case token$CONTINUE:
    case token$GOTO:
        // jump_statement
        //         : GOTO IDENTIFIER ';'
        //         | CONTINUE ';'
        //         | BREAK ';'
        //         ;
        {
            token$Token keyword = p->tok;
            parser$next(p);
            ast$Expr *label = NULL;
            if (keyword == token$GOTO) {
                label = parser$parseIdent(p);
            }
            parser$expect(p, token$SEMICOLON);
            ast$Stmt stmt = {
                .type = ast$STMT_JUMP,
                .jump = {
                    .keyword = keyword,
                    .label = label,
                },
            };
            return esc(stmt);
        }
    case token$LBRACE:
        return compound_statement(p, false);
    default:
        break;
    }

    if (parser$accept(p, token$SEMICOLON)) {
        ast$Stmt stmt = {
            .type = ast$STMT_EMPTY,
        };
        return esc(stmt);
    }

    ast$Stmt *stmt = simple_statement(p, true);
    if (stmt->type != ast$STMT_LABEL) {
        parser$expect(p, token$SEMICOLON);
    }
    return stmt;
}

static ast$Stmt *compound_statement(parser$Parser *p, bool allow_single) {
    // compound_statement : '{' statement_list? '}' ;
    // statement_list : statement+ ;
    utils$Slice stmts = {.size = sizeof(ast$Stmt *)};
    if (allow_single && p->tok != token$LBRACE) {
        ast$Stmt *stmt = statement(p);
        assert(stmt->type != ast$STMT_DECL);
        utils$Slice_append(&stmts, &stmt);
    } else {
        parser$expect(p, token$LBRACE);
        while (p->tok != token$RBRACE) {
            ast$Stmt *stmt = statement(p);
            utils$Slice_append(&stmts, &stmt);
        }
        parser$expect(p, token$RBRACE);
    }
    ast$Stmt stmt = {
        .type = ast$STMT_BLOCK,
        .block = {
            .stmts = utils$Slice_to_nil_array(stmts),
        },
    };
    return esc(stmt);
}

static ast$Decl *parameter_declaration(parser$Parser *p) {
    ast$Decl decl = {
        .type = ast$DECL_FIELD,
    };
    decl.field.type = declaration_specifiers(p, false);
    decl.field.name = declarator(p, &decl.field.type);
    return esc(decl);
}

static ast$Expr *type_specifier(parser$Parser *p) {
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
    ast$Expr *x = NULL;
    switch (p->tok) {
    case token$SIGNED:
    case token$UNSIGNED:
        parser$error(p, p->pos, sys$sprintf("`%s` is not supported in subc", token$string(p->tok)));
        break;
    case token$STRUCT:
    case token$UNION:
        x = struct_or_union_specifier(p);
        break;
    case token$ENUM:
        x = enum_specifier(p);
        break;
    default:
        if (is_type(p)) {
            x = parser$parseIdent(p);
        } else {
            parser$errorExpected(p, p->pos, "type");
        }
        break;
    }
    return x;
}

static ast$Expr *declaration_specifiers(parser$Parser *p, bool is_top) {
    // declaration_specifiers
    //         : storage_class_specifier? type_qualifier? type_specifier
    //         ;
    if (is_top) {
        // storage_class_specifier : TYPEDEF | EXTERN | STATIC | AUTO | REGISTER ;
        switch (p->tok) {
        case token$EXTERN:
        case token$STATIC:
            parser$next(p);
            break;
        default:
            break;
        }
    }
    // type_qualifier : CONST | VOLATILE ;
    bool is_const = parser$accept(p, token$CONST);
    ast$Expr *type = type_specifier(p);
    if (is_const) {
        type->is_const = is_const;
    }
    return type;
}

static ast$Expr *specifier_qualifier_list(parser$Parser *p) {
    // specifier_qualifier_list
    //         : type_qualifier? type_specifier
    //         ;
    return declaration_specifiers(p, false);
}

static ast$Decl *declaration(parser$Parser *p, bool is_external) {
    // declaration : declaration_specifiers init_declarator_list ';' ;
    if (p->tok == token$HASH) {
        return parser$parsePragma(p);
    }
    if (p->tok == token$TYPEDEF) {
        token$Token keyword = p->tok;
        parser$expect(p, keyword);
        ast$Expr *type = declaration_specifiers(p, true);
        ast$Expr *name = declarator(p, &type);
        parser$expect(p, token$SEMICOLON);
        ast$Decl declref = {
            .type = ast$DECL_TYPEDEF,
            .typedef_ = {
                .name = name,
                .type = type,
            },
        };
        ast$Decl *decl = esc(declref);
        parser$declare(p, p->pkg_scope, decl, ast$ObjKind_TYPE, name);
        return decl;
    }
    ast$Expr *type = declaration_specifiers(p, true);
    ast$Expr *name = declarator(p, &type);
    ast$Expr *value = NULL;
    if (type->type == ast$TYPE_FUNC) {
        ast$Decl decl = {
            .type = ast$DECL_FUNC,
            .func = {
                .type = type,
                .name = name,
            },
        };
        if (is_external && p->tok == token$LBRACE) {
            // function_definition
            //         : declaration_specifiers declarator compound_statement ;
            decl.func.body = compound_statement(p, false);
        } else {
            parser$expect(p, token$SEMICOLON);
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
    if (parser$accept(p, token$ASSIGN)) {
        value = initializer(p);
    }
    parser$expect(p, token$SEMICOLON);
    if (name != NULL) {
        ast$Decl decl = {
            .type = ast$DECL_VALUE,
            .value = {
                .type = type,
                .name = name,
                .value = value,
                .kind = token$VAR,
            },
        };
        return esc(decl);
    } else {
        switch (type->type) {
        case ast$TYPE_STRUCT:
            name = type->struct_.name;
            break;
        default:
            panic("FUCK: %d", type->type);
            break;
        }
        ast$Decl decl = {
            .type = ast$DECL_TYPEDEF,
            .typedef_ = {
                .type = type,
                .name = name,
            },
        };
        return esc(decl);
    }
}

static ast$File *parse_cfile(parser$Parser *p) {
    utils$Slice decls = {.size = sizeof(ast$Decl *)};
    utils$Slice imports = {.size = sizeof(ast$Decl *)};
    ast$Expr *name = NULL;
    while (p->tok == token$HASH) {
        ast$Decl *lit = parser$parsePragma(p);
        utils$Slice_append(&decls, &lit);
    }
    if (parser$accept(p, token$PACKAGE)) {
        parser$expect(p, token$LPAREN);
        name = parser$parseIdent(p);
        parser$expect(p, token$RPAREN);
        parser$expect(p, token$SEMICOLON);
    }
    while (p->tok == token$IMPORT) {
        parser$expect(p, token$IMPORT);
        parser$expect(p, token$LPAREN);
        ast$Expr *path = parser$parseBasicLit(p, token$STRING);
        parser$expect(p, token$RPAREN);
        parser$expect(p, token$SEMICOLON);
        ast$Decl decl = {
            .type = ast$DECL_IMPORT,
            .imp = {
                .path = path,
            },
        };
        ast$Decl *declp = esc(decl);
        utils$Slice_append(&imports, &declp);
    }
    while (p->tok != token$EOF) {
        // translation_unit
        //         : external_declaration+
        //         ;
        ast$Decl *decl = declaration(p, true);
        utils$Slice_append(&decls, &decl);
    }
    ast$File file = {
        .filename = p->file->name,
        .name = name,
        .decls = utils$Slice_to_nil_array(decls),
        .imports = utils$Slice_to_nil_array(imports),
    };
    return esc(file);
}

extern ast$File *cparser$parseFile(const char *filename, ast$Scope *pkg_scope) {
    char *src = ioutil$readFile(filename, NULL);
    parser$Parser p = {};
    parser$init(&p, filename, src);
    p.pkg_scope = pkg_scope;
    p.c_mode = true;
    ast$File *file = parse_cfile(&p);
    file->scope = p.pkg_scope;
    free(src);
    return file;
}
