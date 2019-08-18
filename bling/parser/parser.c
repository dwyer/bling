#include "bling/parser/parser.h"

#include "bytes/bytes.h"
#include "paths/paths.h"
#include "sys/sys.h"

extern void parser$next(parser$Parser *p) {
    p->tok = scanner$scan(&p->scanner, &p->pos, &p->lit);
}

extern void parser$init(parser$Parser *p, token$FileSet *fset,
        const char *filename, char *src) {
    assert(fset);
    p->file = token$FileSet_addFile(fset, filename, -1, strlen(src));
    p->lit = NULL;
    scanner$init(&p->scanner, p->file, src);
    p->scanner.dontInsertSemis = !bytes$hasSuffix(filename, ".bling");
    parser$next(p);
}

extern void parser$error(parser$Parser *p, token$Pos pos, char *msg) {
    token$Position position = token$File_position(p->file, pos);
    panic(sys$sprintf("%s: %s\n%s",
                token$Position_string(&position),
                msg,
                token$File_lineString(p->file, position.line)));
}

extern void parser$errorExpected(parser$Parser *p, token$Pos pos, char *msg) {
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
    parser$error(p, pos, msg);
    free(msg);
}

extern bool parser$accept(parser$Parser *p, token$Token tok0) {
    if (p->tok == tok0) {
        parser$next(p);
        return true;
    }
    return false;
}

extern token$Pos parser$expect(parser$Parser *p, token$Token tok) {
    token$Pos pos = p->pos;
    if (p->tok != tok) {
        char *lit = p->lit;
        if (lit == NULL) {
            lit = token$string(p->tok);
        }
        parser$errorExpected(p, pos, token$string(tok));
    }
    parser$next(p);
    return pos;
}

extern void parser$declare(parser$Parser *p, ast$Scope *s, ast$Decl *decl,
        ast$ObjKind kind, ast$Expr *name) {
    assert(name->type == ast$EXPR_IDENT);
    ast$Object *obj = ast$newObject(kind, name->ident.name);
    obj->decl = decl;
    obj->scope = s;
    ast$Scope_insert(s, obj);
}

static ast$Expr *parseExpr(parser$Parser *p);
static ast$Expr *parseInitExpr(parser$Parser *p);

static ast$Expr *parseType(parser$Parser *p);

static ast$Stmt *parse_stmt(parser$Parser *p);
static ast$Stmt *parse_block_stmt(parser$Parser *p);

static ast$Decl *parseDecl(parser$Parser *p);
static ast$Decl *parse_field(parser$Parser *p, bool anon);

extern ast$Expr *parser$parseBasicLit(parser$Parser *p, token$Token kind) {
    char *value = p->lit;
    p->lit = NULL;
    token$Pos pos = parser$expect(p, kind);
    ast$Expr x = {
        .type = ast$EXPR_BASIC_LIT,
        .pos = pos,
        .basic_lit = {
            .kind = kind,
            .value = value,
        },
    };
    return esc(x);
}

extern ast$Expr *parser$parseIdent(parser$Parser *p) {
    ast$Expr x = {
        .type = ast$EXPR_IDENT,
        .pos = p->pos,
    };
    if (p->tok == token$IDENT) {
        x.ident.name = p->lit;
        p->lit = NULL;
    }
    parser$expect(p, token$IDENT);
    return esc(x);
}

extern ast$Expr *parser$parseOperand(parser$Parser *p) {
    switch (p->tok) {
    case token$IDENT:
        return parser$parseIdent(p);
    case token$CHAR:
    case token$FLOAT:
    case token$INT:
    case token$STRING:
        return parser$parseBasicLit(p, p->tok);
    case token$LPAREN:
        if (p->c_mode) {
            parser$error(p, p->pos, "unreachable");
        } else {
            token$Pos pos = p->pos;
            parser$expect(p, token$LPAREN);
            ast$Expr x = {
                .type = ast$EXPR_PAREN,
                .pos = pos,
                .paren = {
                    .x = parseExpr(p),
                },
            };
            parser$expect(p, token$RPAREN);
            return esc(x);
        }
    default:
        parser$errorExpected(p, p->pos, "operand");
        return NULL;
    }
}

static ast$Expr *parser$parsePrimaryExpr(parser$Parser *p) {
    ast$Expr *x = parser$parseOperand(p);
    for (;;) {
        switch (p->tok) {
        case token$PERIOD:
            {
                token$Token tok = p->tok;
                parser$next(p);
                ast$Expr y = {
                    .type = ast$EXPR_SELECTOR,
                    .pos = x->pos,
                    .selector = {
                        .x = x,
                        .tok = tok,
                        .sel = parser$parseIdent(p),
                    },
                };
                x = esc(y);
            }
            break;
        case token$LBRACK:
            {
                parser$expect(p, token$LBRACK);
                ast$Expr y = {
                    .type = ast$EXPR_INDEX,
                    .pos = x->pos,
                    .index = {
                        .x = x,
                        .index = parseExpr(p),
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
                    ast$Expr *x = parseExpr(p);
                    utils$Slice_append(&args, &x);
                    if (!parser$accept(p, token$COMMA)) {
                        break;
                    }
                }
                parser$expect(p, token$RPAREN);
                ast$Expr call = {
                    .type = ast$EXPR_CALL,
                    .pos = x->pos,
                    .call = {
                        .func = x,
                        .args = utils$Slice_to_nil_array(args),
                    },
                };
                x = esc(call);
            }
            break;
        default:
            return x;
        }
    }
}

static ast$Expr *parseUnaryExpr(parser$Parser *p) {
    // unary_expression
    //         : postfix_expression
    //         | unary_operator cast$expression
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
            parser$next(p);
            ast$Expr x = {
                .type = ast$EXPR_UNARY,
                .pos = pos,
                .unary = {
                    .op = op,
                    .x = parseUnaryExpr(p),
                },
            };
            return esc(x);
        }
    case token$MUL:
        {
            token$Pos pos = p->pos;
            parser$next(p);
            ast$Expr x = {
                .type = ast$EXPR_STAR,
                .pos = pos,
                .star = {
                    .x = parseUnaryExpr(p),
                },
            };
            return esc(x);
        }
    case token$SIZEOF:
        {
            token$Pos pos = p->pos;
            parser$next(p);
            parser$expect(p, token$LPAREN);
            ast$Expr x = {
                .type = ast$EXPR_SIZEOF,
                .pos = pos,
                .sizeof_ = {
                    .x = parseType(p),
                },
            };
            parser$expect(p, token$RPAREN);
            return esc(x);
        }
    case token$LT:
        {
            token$Pos pos = p->pos;
            parser$expect(p, token$LT);
            ast$Expr *type = parseType(p);
            parser$expect(p, token$GT);
            ast$Expr *expr = NULL;
            if (p->tok == token$LBRACE) {
                expr = parseInitExpr(p);
            } else {
                expr = parseUnaryExpr(p);
            }
            ast$Expr y = {
                .type = ast$EXPR_CAST,
                .pos = pos,
                .cast = {
                    .type = type,
                    .expr = expr,
                },
            };
            return esc(y);
        }
    default:
        return parser$parsePrimaryExpr(p);
    }
}

static ast$Expr *parseBinaryExpr(parser$Parser *p, int prec1) {
    ast$Expr *x = parseUnaryExpr(p);
    for (;;) {
        token$Token op = p->tok;
        int oprec = token$precedence(op);
        if (oprec < prec1) {
            return x;
        }
        parser$expect(p, op);
        ast$Expr *y = parseBinaryExpr(p, oprec + 1);
        ast$Expr z = {
            .type = ast$EXPR_BINARY,
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

static ast$Expr *parseTernaryExpr(parser$Parser *p) {
    // ternary_expression
    //         : binary_expression
    //         | binary_expression '?' expression ':' ternary_expression
    //         ;
    ast$Expr *x = parseBinaryExpr(p, token$lowest_prec + 1);
    if (parser$accept(p, token$QUESTION_MARK)) {
        ast$Expr *consequence = parseTernaryExpr(p);
        parser$expect(p, token$COLON);
        ast$Expr *alternative = parseTernaryExpr(p);
        ast$Expr conditional = {
            .type = ast$EXPR_COND,
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

static ast$Expr *parseExpr(parser$Parser *p) {
    return parseTernaryExpr(p);
}

#pragma mark - types

static ast$Expr *parseTypeName(parser$Parser *p) {
    ast$Expr *x = parser$parseIdent(p);
    if (parser$accept(p, token$PERIOD)) {
        ast$Expr y = {
            .type = ast$EXPR_SELECTOR,
            .pos = x->pos,
            .selector = {
                .x = x,
                .tok = token$DOLLAR,
                .sel = parser$parseIdent(p),
            },
        };
        x = esc(y);
    }
    return x;
}

static ast$Expr *parseArrayType(parser$Parser *p) {
    token$Pos pos = parser$expect(p, token$LBRACK);
    ast$Expr *len = NULL;
    if (p->tok != token$RBRACK) {
        len = parseExpr(p);
    }
    parser$expect(p, token$RBRACK);
    ast$Expr type = {
        .type = ast$TYPE_ARRAY,
        .pos = pos,
        .array = {
            .elt = parseType(p),
            .len = len,
        },
    };
    return esc(type);
}

static ast$Expr *parseStructOrUnionType(parser$Parser *p, token$Token keyword) {
    // struct_or_union_specifier
    //         : struct_or_union IDENTIFIER
    //         | struct_or_union IDENTIFIER '{' struct_declaration_list '}'
    //         | struct_or_union '{' struct_declaration_list '}'
    //         ;
    // struct_or_union : STRUCT | UNION ;
    token$Pos pos = p->pos;
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
            ast$Decl decl = {
                .type = ast$DECL_FIELD,
                .pos = p->pos,
            };
            if (p->tok == token$UNION) {
                // anonymous union
                decl.field.type = parseType(p);
            } else {
                decl.field.name = parser$parseIdent(p);
                decl.field.type = parseType(p);
            }
            parser$expect(p, token$SEMICOLON);
            ast$Decl *field = esc(decl);
            utils$Slice_append(&fieldSlice, &field);
            if (p->tok == token$RBRACE) {
                break;
            }
        }
        parser$expect(p, token$RBRACE);
        fields = utils$Slice_to_nil_array(fieldSlice);
    }
    // TODO assert(name || fields)
    ast$Expr x = {
        .type = ast$TYPE_STRUCT,
        .pos = pos,
        .struct_ = {
            .tok = keyword,
            .name = name,
            .fields = fields,
        },
    };
    return esc(x);
}

static ast$Expr *parsePointerType(parser$Parser *p) {
    token$Pos pos = p->pos;
    parser$expect(p, token$MUL);
    ast$Expr x = {
        .type = ast$EXPR_STAR,
        .pos = pos,
        .star = {
            .x = parseType(p),
        },
    };
    return esc(x);
}

static ast$Decl **parseParameterList(parser$Parser *p, bool anon) {
    utils$Slice params = utils$Slice_init(sizeof(ast$Decl *));
    for (;;) {
        ast$Decl *param = parse_field(p, false);
        utils$Slice_append(&params, &param);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
        if (p->tok == token$ELLIPSIS) {
            ast$Decl decl = {
                .type = ast$DECL_ELLIPSIS,
                .pos = p->pos,
            };
            parser$next(p);
            ast$Decl *param = esc(decl);
            utils$Slice_append(&params, &param);
            break;
        }
    }
    return utils$Slice_to_nil_array(params);
}

static ast$Decl **parseParameters(parser$Parser *p, bool anon) {
    ast$Decl **params = NULL;
    parser$expect(p, token$LPAREN);
    if (p->tok != token$RPAREN) {
        params = parseParameterList(p, anon);
    }
    parser$expect(p, token$RPAREN);
    return params;
}

static ast$Expr *parseFuncType(parser$Parser *p) {
    token$Pos pos = parser$expect(p, token$FUNC);
    ast$Decl **params = parseParameters(p, false);
    ast$Expr *result = NULL;
    if (p->tok != token$SEMICOLON) {
        result = parseType(p);
    }
    ast$Expr type = {
        .type = ast$TYPE_FUNC,
        .pos = pos,
        .func = {
            .params = params,
            .result = result,
        },
    };
    ast$Expr ptr = {
        .type = ast$EXPR_STAR,
        .pos = type.pos,
        .star = {
            .x = esc(type),
        },
    };
    return esc(ptr);
}

static ast$Expr *parseEnumType(parser$Parser *p) {
    // enum_specifier
    //         : ENUM '{' enumerator_list '}'
    //         | ENUM IDENTIFIER '{' enumerator_list '}'
    //         | ENUM IDENTIFIER
    //         ;
    ast$Expr *name = NULL;
    token$Pos pos = p->pos;
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
                .pos = p->pos,
                .value = {
                    .name = parser$parseIdent(p),
                },
            };
            if (parser$accept(p, token$ASSIGN)) {
                decl.value.value = parseExpr(p);
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
        .pos = pos,
        .enum_ = {
            .name = name,
            .enums = enums,
        },
    };
    return esc(x);
}

static ast$Expr *parseQualifiedType(parser$Parser *p, token$Token tok) {
    parser$expect(p, tok);
    ast$Expr *type = parseType(p);
    type->is_const = true;
    return type;
}

static ast$Expr *tryType(parser$Parser *p) {
    switch (p->tok) {
    case token$IDENT:
        return parseTypeName(p);
    case token$LBRACK:
        return parseArrayType(p);
    case token$STRUCT:
    case token$UNION:
        return parseStructOrUnionType(p, p->tok);
    case token$MUL:
        return parsePointerType(p);
    case token$FUNC:
        return parseFuncType(p);
    case token$ENUM:
        return parseEnumType(p);
    case token$CONST:
        return parseQualifiedType(p, p->tok);
    default:
        return NULL;
    }
}

static ast$Expr *parseType(parser$Parser *p) {
    ast$Expr *t = tryType(p);
    if (t == NULL) {
        parser$errorExpected(p, p->pos, "type");
    }
    return t;
}

static ast$Expr *parseInitExpr(parser$Parser *p) {
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
        parser$expect(p, token$LBRACE);
        utils$Slice list = {.size = sizeof(ast$Expr *)};
        while (p->tok != token$RBRACE && p->tok != token$EOF) {
            ast$Expr *value = parseInitExpr(p);
            if (value->type == ast$EXPR_IDENT && parser$accept(p, token$COLON)) {
                ast$Expr *key = value;
                ast$Expr x = {
                    .type = ast$EXPR_KEY_VALUE,
                    .pos = key->pos,
                    .key_value = {
                        .key = key,
                        .value = parseInitExpr(p),
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
            .pos = pos,
            .compound = {
                .list = utils$Slice_to_nil_array(list),
            },
        };
        return esc(expr);
    }
    return parseExpr(p);
}

static ast$Stmt *parse_simple_stmt(parser$Parser *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    ast$Expr *x = parseExpr(p);
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
            ast$Expr *y = parseExpr(p);
            ast$Stmt stmt = {
                .type = ast$STMT_ASSIGN,
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
            parser$next(p);
            ast$Stmt stmt = {
                .type = ast$STMT_POSTFIX,
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
    if (labelOk && x->type == ast$EXPR_IDENT) {
        if (parser$accept(p, token$COLON)) {
            ast$Stmt stmt = {
                .type = ast$STMT_LABEL,
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
    ast$Stmt stmt = {
        .type = ast$STMT_EXPR,
        .pos = x->pos,
        .expr = {.x = x},
    };
    return esc(stmt);
}

static ast$Stmt *parse_for_stmt(parser$Parser *p) {
    // for_statement
    //         | FOR simple_statement? ';' expression? ';' expression?
    //              compound_statement ;
    token$Pos pos = parser$expect(p, token$FOR);
    ast$Stmt *init = NULL;
    if (!parser$accept(p, token$SEMICOLON)) {
        init = parse_stmt(p);
    }
    ast$Expr *cond = NULL;
    if (p->tok != token$SEMICOLON) {
        cond = parseExpr(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt *post = NULL;
    if (p->tok != token$LBRACE) {
        post = parse_simple_stmt(p, false);
    }
    ast$Stmt *body = parse_block_stmt(p);
    ast$Stmt stmt = {
        .type = ast$STMT_ITER,
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

static ast$Stmt *parse_if_stmt(parser$Parser *p) {
    // if_statement
    //         : IF expression compound_statement
    //         | IF expression compound_statement ELSE compound_statement
    //         | IF expression compound_statement ELSE if_statement
    //         ;
    token$Pos pos = parser$expect(p, token$IF);
    ast$Expr *cond = parseExpr(p);
    if (p->tok != token$LBRACE) {
        parser$error(p, p->pos, "`if` must be followed by a compound_statement");
    }
    ast$Stmt *body = parse_block_stmt(p);
    ast$Stmt *else_ = NULL;
    if (parser$accept(p, token$ELSE)) {
        if (p->tok == token$IF) {
            else_ = parse_stmt(p);
        } else if (p->tok == token$LBRACE) {
            else_ = parse_block_stmt(p);
        } else {
            parser$error(p, p->pos, "`else` must be followed by an if_statement or compound_statement");
        }
    }
    ast$Stmt stmt = {
        .type = ast$STMT_IF,
        .pos = pos,
        .if_ = {
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_return_stmt(parser$Parser *p) {
    // return_statement
    //         | RETURN expression? ';'
    //         ;
    token$Pos pos = parser$expect(p, token$RETURN);
    ast$Expr *x = NULL;
    if (p->tok != token$SEMICOLON) {
        x = parseExpr(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .type = ast$STMT_RETURN,
        .pos = pos,
        .return_ = {
            .x = x,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_switch_stmt(parser$Parser *p) {
    // switch_statement | SWITCH expression case_statement* ;
    token$Pos pos = parser$expect(p, token$SWITCH);
    ast$Expr *tag = parseExpr(p);
    parser$expect(p, token$LBRACE);
    utils$Slice clauses = {.size = sizeof(ast$Stmt *)};
    while (p->tok == token$CASE || p->tok == token$DEFAULT) {
        // case_statement
        //         | CASE constant_expression ':' statement+
        //         | DEFAULT ':' statement+
        //         ;
        utils$Slice exprs = {.size=sizeof(ast$Expr *)};
        token$Pos pos = p->pos;
        if (parser$accept(p, token$CASE)) {
            for (;;) {
                ast$Expr *expr = parseExpr(p);
                utils$Slice_append(&exprs, &expr);
                if (!parser$accept(p, token$COMMA)) {
                    break;
                }
            }
        } else {
            parser$expect(p, token$DEFAULT);
        }
        parser$expect(p, token$COLON);
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
                ast$Stmt *stmt = parse_stmt(p);
                utils$Slice_append(&stmts, &stmt);
            }
        }
        ast$Stmt stmt = {
            .type = ast$STMT_CASE,
            .pos = pos,
            .case_ = {
                .exprs = utils$Slice_to_nil_array(exprs),
                .stmts = utils$Slice_to_nil_array(stmts),
            },
        };
        ast$Stmt *clause = esc(stmt);
        utils$Slice_append(&clauses, &clause);
    }
    parser$expect(p, token$RBRACE);
    parser$accept(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .type = ast$STMT_SWITCH,
        .pos = pos,
        .switch_ = {
            .tag = tag,
            .stmts = utils$Slice_to_nil_array(clauses),
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_while_stmt(parser$Parser *p) {
    // while_statement : WHILE expression compound_statement ;
    token$Pos pos = parser$expect(p, token$WHILE);
    ast$Expr *cond = parseExpr(p);
    ast$Stmt *body = parse_block_stmt(p);
    ast$Stmt stmt = {
        .type = ast$STMT_ITER,
        .pos = pos,
        .iter = {
            .kind = token$WHILE,
            .cond = cond,
            .body = body,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_jump_stmt(parser$Parser *p, token$Token keyword) {
    // jump_statement
    //         : GOTO IDENTIFIER ';'
    //         | CONTINUE ';'
    //         | BREAK ';'
    //         ;
    token$Pos pos = parser$expect(p, keyword);
    ast$Expr *label = NULL;
    if (keyword == token$GOTO) {
        label = parser$parseIdent(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .type = ast$STMT_JUMP,
        .pos = pos,
        .jump = {
            .keyword = keyword,
            .label = label,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_decl_stmt(parser$Parser *p) {
    ast$Stmt stmt = {
        .type = ast$STMT_DECL,
        .pos = p->pos,
        .decl = {
            .decl = parseDecl(p),
        },
    };
    return esc(stmt);
}

static ast$Stmt *parse_stmt(parser$Parser *p) {
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
    if (parser$accept(p, token$SEMICOLON)) {
        ast$Stmt stmt = {
            .type = ast$STMT_EXPR,
            .pos = pos,
        };
        return esc(stmt);
    }
    ast$Stmt *stmt = parse_simple_stmt(p, true);
    if (stmt->type != ast$STMT_LABEL) {
        parser$expect(p, token$SEMICOLON);
    }
    return stmt;
}

static ast$Stmt *parse_block_stmt(parser$Parser *p) {
    // compound_statement : '{' statement_list? '}' ;
    utils$Slice stmts = {.size = sizeof(ast$Stmt *)};
    token$Pos pos = parser$expect(p, token$LBRACE);
    // statement_list : statement+ ;
    while (p->tok != token$RBRACE) {
        ast$Stmt *stmt = parse_stmt(p);
        utils$Slice_append(&stmts, &stmt);
    }
    parser$expect(p, token$RBRACE);
    parser$accept(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .type = ast$STMT_BLOCK,
        .pos = pos,
        .block = {
            .stmts = utils$Slice_to_nil_array(stmts),
        }
    };
    return esc(stmt);
}

static ast$Decl *parse_field(parser$Parser *p, bool anon) {
    ast$Decl decl = {
        .type = ast$DECL_FIELD,
        .pos = p->pos,
    };
    if (p->tok == token$IDENT) {
        decl.field.name = parser$parseIdent(p);
    }
    if (decl.field.name != NULL && (p->tok == token$COMMA || p->tok == token$RPAREN)) {
        decl.field.type = decl.field.name;
        decl.field.name = NULL;
    } else {
        decl.field.type = parseType(p);
    }
    return esc(decl);
}

extern ast$Decl *parser$parsePragma(parser$Parser *p) {
    token$Pos pos = p->pos;
    char *lit = p->lit;
    p->lit = NULL;
    parser$expect(p, token$HASH);
    ast$Decl decl = {
        .type = ast$DECL_PRAGMA,
        .pos = pos,
        .pragma = {
            .lit = lit,
        },
    };
    return esc(decl);
}

static ast$Decl *parseDecl(parser$Parser *p) {
    switch (p->tok) {
    case token$HASH:
        return parser$parsePragma(p);
    case token$TYPEDEF:
        {
            token$Token keyword = p->tok;
            token$Pos pos = parser$expect(p, keyword);
            ast$Expr *ident = parser$parseIdent(p);
            ast$Expr *type = parseType(p);
            parser$expect(p, token$SEMICOLON);
            ast$Decl decl = {
                .type = ast$DECL_TYPEDEF,
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
            token$Token keyword = p->tok;
            token$Pos pos = parser$expect(p, keyword);
            ast$Expr *ident = parser$parseIdent(p);
            ast$Expr *type = NULL;
            if (p->tok != token$ASSIGN) {
                type = parseType(p);
            }
            ast$Expr *value = NULL;
            if (parser$accept(p, token$ASSIGN)) {
                value = parseInitExpr(p);
            }
            parser$expect(p, token$SEMICOLON);
            ast$Decl decl = {
                .type = ast$DECL_VALUE,
                .pos = pos,
                .value = {
                    .name = ident,
                    .type = type,
                    .value = value,
                    .kind = keyword,
                },
            };
            return esc(decl);
        }
    case token$FUNC:
        {
            token$Pos pos = parser$expect(p, token$FUNC);
            ast$Decl decl = {
                .type = ast$DECL_FUNC,
                .pos = pos,
                .func = {
                    .name = parser$parseIdent(p),
                },
            };
            ast$Expr type = {
                .type = ast$TYPE_FUNC,
                .pos = pos,
                .func = {
                    .params = parseParameters(p, false),
                },
            };
            if (p->tok != token$LBRACE && p->tok != token$SEMICOLON) {
                type.func.result = parseType(p);
            }
            decl.func.type = esc(type);
            if (p->tok == token$LBRACE) {
                decl.func.body = parse_block_stmt(p);
            } else {
                parser$expect(p, token$SEMICOLON);
            }
            return esc(decl);
        }
    default:
        parser$error(p, p->pos, sys$sprintf("cant handle it: %s", token$string(p->tok)));
        return NULL;
    }
}

static bool isBlingFile(const char *name) {
    return bytes$hasSuffix(name, ".bling");
}

static bool isTestFile(const char *name) {
    return paths$match("*_test.bling", name);
}

extern ast$File **parser$parseDir(token$FileSet *fset, const char *path,
        utils$Error **first) {
    utils$Error *err = NULL;
    os$FileInfo **infos = ioutil$readDir(path, &err);
    if (err) {
        utils$Error_move(err, first);
        return NULL;
    }
    utils$Slice files = utils$Slice_init(sizeof(uintptr_t));
    while (*infos != NULL) {
        char *name = os$FileInfo_name(**infos);
        if (isBlingFile(name) && !isTestFile(name)) {
            ast$File *file = parser$parseFile(fset, name);
            utils$Slice_append(&files, &file);
        }
        infos++;
    }
    return utils$Slice_to_nil_array(files);
}

static ast$File *_parse_file(parser$Parser *p) {
    ast$Expr *name = NULL;
    utils$Slice imports = utils$Slice_init(sizeof(uintptr_t));
    utils$Slice decls = utils$Slice_init(sizeof(ast$Decl *));
    while (p->tok == token$HASH) {
        ast$Decl *lit = parser$parsePragma(p);
        utils$Slice_append(&decls, &lit);
    }
    if (parser$accept(p, token$PACKAGE)) {
        name = parser$parseIdent(p);
        parser$expect(p, token$SEMICOLON);
    }
    while (p->tok == token$IMPORT) {
        token$Pos pos = parser$expect(p, token$IMPORT);
        ast$Expr *path = parser$parseBasicLit(p, token$STRING);
        parser$expect(p, token$SEMICOLON);
        ast$Decl decl = {
            .type = ast$DECL_IMPORT,
            .pos = pos,
            .imp = {
                .path = path,
            },
        };
        ast$Decl *declp = esc(decl);
        utils$Slice_append(&imports, &declp);
    }
    while (p->tok != token$EOF) {
        ast$Decl *decl = parseDecl(p);
        utils$Slice_append(&decls, &decl);
    }
    ast$File file = {
        .filename = p->file->name,
        .name = name,
        .imports = utils$Slice_to_nil_array(imports),
        .decls = utils$Slice_to_nil_array(decls),
    };
    return esc(file);
}

extern ast$File *parser$parseFile(token$FileSet *fset, const char *filename) {
    utils$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic("%s: %s", filename, err->error);
    }
    parser$Parser p = {};
    parser$init(&p, fset, filename, src);
    ast$File *file = _parse_file(&p);
    free(src);
    return file;
}
