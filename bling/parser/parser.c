#include "bling/parser/parser.h"

#include "bytes/bytes.h"
#include "paths/paths.h"
#include "sys/sys.h"

static bool isTypeName(ast$Expr *x) {
    switch (x->kind) {
    case ast$EXPR_IDENT:
        return true;
    case ast$EXPR_SELECTOR:
        return x->selector.x->kind == ast$EXPR_IDENT;
    default:
        return false;
    }
}

static bool isLiteralType(ast$Expr *x) {
    switch (x->kind) {
    case ast$EXPR_IDENT:
    case ast$TYPE_ARRAY:
    case ast$TYPE_STRUCT:
        return true;
    case ast$EXPR_SELECTOR:
        return x->selector.x->kind == ast$EXPR_IDENT;
    default:
        return false;
    }
}

static ast$Expr *unparen(ast$Expr *x) {
    while (x->kind == ast$EXPR_PAREN) {
        x = x->paren.x;
    }
    return x;
}

static void parser$openScope(parser$Parser *p) {
    p->topScope = ast$Scope_new(p->topScope);
}

static void parser$closeScope(parser$Parser *p) {
    p->topScope = p->topScope->outer;
}

// static ast$Object parser$unresolved = {};

static void parser$tryResolve(parser$Parser *p, ast$Expr *x,
        bool collectUnresolved) {
    return;
    if (x->kind != ast$EXPR_IDENT) {
        return;
    }
    assert(x->ident.obj == NULL);
    for (ast$Scope *s = p->topScope; s != NULL; s = s->outer) {
        ast$Object *obj = ast$Scope_lookup(s, x->ident.name);
        if (obj != NULL) {
            x->ident.obj = obj;
            return;
        }
    }
    if (collectUnresolved) {
        // x->ident.obj = &parser$unresolved;
        utils$Slice_append(&p->unresolved, &x);
    }
}

static void parser$resolve(parser$Parser *p, ast$Expr *x) {
    parser$tryResolve(p, x, true);
}

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
    p->exprLev = 0;
    p->unresolved = utils$Slice_init(sizeof(ast$Expr *));
    parser$next(p);
}

extern void parser$error(parser$Parser *p, token$Pos pos, char *msg) {
    token$Position position = token$File_position(p->file, pos);
    assert(p->topScope);
    assert(p->pkgScope);
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

static ast$Expr *checkExpr(parser$Parser *p, ast$Expr *x) {
    switch (unparen(x)->kind) {
    case ast$EXPR_IDENT:
    case ast$EXPR_BASIC_LIT:
    case ast$EXPR_BINARY:
    case ast$EXPR_CALL:
    case ast$EXPR_CAST:
    case ast$EXPR_COMPOSITE_LIT:
    case ast$EXPR_INDEX:
    case ast$EXPR_SELECTOR:
    case ast$EXPR_SIZEOF:
    case ast$EXPR_STAR:
    case ast$EXPR_TERNARY:
    case ast$EXPR_UNARY:
        break;
    case ast$EXPR_PAREN:
        panic("unreachable");
        break;
    default:
        parser$errorExpected(p, ast$Expr_pos(x), "expression");
        break;
    }
    return x;
}

static ast$Expr *checkExprOrType(parser$Parser *p, ast$Expr *x) {
    return x;
}

static void parser$declare(parser$Parser *p, ast$Decl *decl, void *data,
        ast$Scope *scope, ast$ObjKind kind, ast$Expr *ident) {
    return;
    if (ident->ident.obj != NULL) {
        parser$error(p, decl->pos, sys$sprintf(
                    "identifier already declared or resolved: %s",
                    ident->ident.name));
    }
    ast$Object *obj = ast$newObject(kind, ident->ident.name);
    obj->decl = decl;
    obj->data = NULL;
    obj->scope = scope;
    ident->ident.obj = obj;
    ast$Object *alt = ast$Scope_insert(scope, obj);
    if (alt != NULL) {
        parser$error(p, ast$Expr_pos(ident), sys$sprintf(
                    "%s redeclared in this block", ident->ident.name));
    }
}

static ast$Expr *parseLiteralValue(parser$Parser *p, ast$Expr *type);
static ast$Expr *parseExpr(parser$Parser *p, bool lhs);

const bool LHS = false;

static ast$Expr *tryIdentOrType(parser$Parser *p);
static ast$Expr *tryType(parser$Parser *p);
static ast$Expr *parseType(parser$Parser *p);

static ast$Stmt *parseStmt(parser$Parser *p);
static ast$Stmt *parseBlockStmt(parser$Parser *p);

static ast$Decl *parseDecl(parser$Parser *p);

static ast$Expr *parseRhs(parser$Parser *p) {
    bool old = p->inRhs;
    p->inRhs = true;
    ast$Expr *x = checkExpr(p, parseExpr(p, false));
    p->inRhs = old;
    return x;
}

static ast$Expr *parseRhsOrType(parser$Parser *p) {
    bool old = p->inRhs;
    p->inRhs = true;
    ast$Expr *x = checkExprOrType(p, parseExpr(p, false));
    p->inRhs = old;
    return x;
}

extern ast$Expr *parser$parseBasicLit(parser$Parser *p, token$Token kind) {
    char *value = p->lit;
    p->lit = NULL;
    token$Pos pos = parser$expect(p, kind);
    ast$Expr x = {
        .kind = ast$EXPR_BASIC_LIT,
        .basic = {
            .pos = pos,
            .kind = kind,
            .value = value,
        },
    };
    return esc(x);
}

extern ast$Expr *parser$parseIdent(parser$Parser *p) {
    ast$Expr x = {
        .kind = ast$EXPR_IDENT,
        .ident = {
            .pos = p->pos,
        },
    };
    if (p->tok == token$IDENT) {
        x.ident.name = p->lit;
        p->lit = NULL;
    }
    parser$expect(p, token$IDENT);
    p->numIdents++;
    return esc(x);
}

extern ast$Expr *parser$parseOperand(parser$Parser *p, bool lhs) {
    switch (p->tok) {
    case token$IDENT:
        {
            ast$Expr *x = parser$parseIdent(p);
            parser$resolve(p, x);
            return x;
        }
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
            p->exprLev++;
            ast$Expr x = {
                .kind = ast$EXPR_PAREN,
                .paren = {
                    .pos = pos,
                    .x = parseRhsOrType(p),
                },
            };
            p->exprLev--;
            parser$expect(p, token$RPAREN);
            return esc(x);
        }
    default:
        break;
    }
    ast$Expr *t = tryIdentOrType(p);
    if (t) {
        assert(t->kind != ast$EXPR_IDENT);
        return t;
    }
    parser$errorExpected(p, p->pos, "operand");
    return NULL;
}

static ast$Expr *parseValue(parser$Parser *p, bool keyOk) {
    // initializer
    //         : expression
    //         | '{' initializer_list ','? '}'
    //         ;
    if (p->tok == token$LBRACE) {
        // initializer_list
        //         : designation? initializer
        //         | initializer_list ',' designation? initializer
        //         ;
        return parseLiteralValue(p, NULL);
    }
    ast$Expr *x = checkExpr(p, parseExpr(p, keyOk));
    if (keyOk) {
        if (p->tok != token$COLON) {
            parser$resolve(p, x);
        }
    }
    return x;
}

static ast$Expr *parseElement(parser$Parser *p) {
    ast$Expr *value = parseValue(p, true);
    if (value->kind == ast$EXPR_IDENT && parser$accept(p, token$COLON)) {
        ast$Expr *key = value;
        ast$Expr x = {
            .kind = ast$EXPR_KEY_VALUE,
            .key_value = {
                .key = key,
                .value = parseValue(p, false),
            },
        };
        value = esc(x);
    }
    return value;
}

static ast$Expr **parseElementList(parser$Parser *p) {
    utils$Slice list = {.size = sizeof(ast$Expr *)};
    while (p->tok != token$RBRACE && p->tok != token$EOF) {
        ast$Expr *value = parseElement(p);
        utils$Slice_append(&list, &value);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
    }
    return utils$Slice_to_nil_array(list);
}

static ast$Expr *parseLiteralValue(parser$Parser *p, ast$Expr *type) {
    token$Pos pos = parser$expect(p, token$LBRACE);
    if (type) {
        pos = ast$Expr_pos(type);
    }
    p->exprLev++;
    ast$Expr **list = parseElementList(p);
    p->exprLev--;
    parser$expect(p, token$RBRACE);
    ast$Expr expr = {
        .kind = ast$EXPR_COMPOSITE_LIT,
        .composite = {
            .pos = pos,
            .type = type,
            .list = list,
        },
    };
    return esc(expr);
}

static ast$Expr *parseIndexExpr(parser$Parser *p, ast$Expr *x) {
    parser$expect(p, token$LBRACK);
    p->exprLev++;
    ast$Expr y = {
        .kind = ast$EXPR_INDEX,
        .index = {
            .x = x,
            .index = parseRhs(p),
        },
    };
    p->exprLev--;
    parser$expect(p, token$RBRACK);
    return esc(y);
}

static ast$Expr *parseCallExpr(parser$Parser *p, ast$Expr *x) {
    utils$Slice args = {.size = sizeof(ast$Expr *)};
    parser$expect(p, token$LPAREN);
    p->exprLev++;
    // argument_expression_list
    //         : expression
    //         | argument_expression_list ',' expression
    //         ;
    while (p->tok != token$RPAREN) {
        ast$Expr *x = parseRhsOrType(p);
        utils$Slice_append(&args, &x);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
    }
    p->exprLev--;
    parser$expect(p, token$RPAREN);
    ast$Expr call = {
        .kind = ast$EXPR_CALL,
        .call = {
            .func = x,
            .args = utils$Slice_to_nil_array(args),
        },
    };
    return esc(call);
}

static ast$Expr *parser$parseSelector(parser$Parser *p, ast$Expr *x) {
    ast$Expr y = {
        .kind = ast$EXPR_SELECTOR,
        .selector = {
            .x = x,
            .tok = token$PERIOD,
            .sel = parser$parseIdent(p),
        },
    };
    return esc(y);
}

static ast$Expr *parser$parsePrimaryExpr(parser$Parser *p, bool lhs) {
    ast$Expr *x = parser$parseOperand(p, lhs);
    for (;;) {
        switch (p->tok) {
        case token$PERIOD:
            parser$next(p);
            if (lhs) {
                parser$resolve(p, x);
            }
            x = parser$parseSelector(p, checkExprOrType(p, x));
            break;
        case token$LBRACK:
            if (lhs) {
                parser$resolve(p, x);
            }
            x = parseIndexExpr(p, checkExpr(p, x));
            break;
        case token$LPAREN:
            if (lhs) {
                parser$resolve(p, x);
            }
            x = parseCallExpr(p, checkExprOrType(p, x));
            break;
        case token$LBRACE:
            if (isLiteralType(x) && (p->exprLev >= 0 || !isTypeName(x))) {
                if (lhs) {
                    parser$resolve(p, x);
                }
                x = parseLiteralValue(p, x);
                break;
            } else {
                return x;
            }
        default:
            return x;
        }
    }
}

static ast$Expr *parseUnaryExpr(parser$Parser *p, bool lhs) {
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
    case token$LAND:
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
                .kind = ast$EXPR_UNARY,
                .unary = {
                    .pos = pos,
                    .op = op,
                    .x = checkExpr(p, parseUnaryExpr(p, false)),
                },
            };
            return esc(x);
        }
    case token$MUL:
        {
            token$Pos pos = p->pos;
            parser$next(p);
            ast$Expr x = {
                .kind = ast$EXPR_STAR,
                .star = {
                    .pos = pos,
                    .x = checkExprOrType(p, parseUnaryExpr(p, false)),
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
                .kind = ast$EXPR_SIZEOF,
                .sizeof_ = {
                    .pos = pos,
                    .x = parseType(p),
                },
            };
            parser$expect(p, token$RPAREN);
            return esc(x);
        }
    case token$TYPE:
        {
            token$Pos pos = p->pos;
            parser$expect(p, token$TYPE);
            ast$Expr *type = parseType(p);
            if (p->tok == token$LBRACE) {
                return parseLiteralValue(p, type);
            }
            ast$Expr y = {
                .kind = ast$EXPR_CAST,
                .cast = {
                    .pos = pos,
                    .type = type,
                    .expr = parseUnaryExpr(p, false),
                },
            };
            return esc(y);
        }
    default:
        return parser$parsePrimaryExpr(p, lhs);
    }
}

static ast$Expr *parseBinaryExpr(parser$Parser *p, bool lhs, int prec1) {
    ast$Expr *x = parseUnaryExpr(p, lhs);
    for (;;) {
        token$Token op = p->tok;
        int oprec = token$precedence(op);
        if (oprec < prec1) {
            return x;
        }
        parser$expect(p, op);
        if (lhs) {
            parser$resolve(p, x);
            lhs = false;
        }
        ast$Expr *y = parseBinaryExpr(p, false, oprec + 1);
        ast$Expr z = {
            .kind = ast$EXPR_BINARY,
            .binary = {
                .x = checkExpr(p, x),
                .op = op,
                .y = checkExpr(p, y),
            },
        };
        x = esc(z);
    }
}

static ast$Expr *parseTernaryExpr(parser$Parser *p, bool lhs) {
    // ternary_expression
    //         : binary_expression
    //         | binary_expression '?' expression ':' ternary_expression
    //         ;
    ast$Expr *x = parseBinaryExpr(p, lhs, token$lowest_prec + 1);
    if (parser$accept(p, token$QUESTION_MARK)) {
        ast$Expr *consequence = NULL;
        if (p->tok != token$COLON) {
            consequence = parseExpr(p, false);
        }
        parser$expect(p, token$COLON);
        ast$Expr *alternative = parseTernaryExpr(p, false);
        ast$Expr y = {
            .kind = ast$EXPR_TERNARY,
            .ternary = {
                .cond = x,
                .x = consequence,
                .y = alternative,
            },
        };
        x = esc(y);
    }
    return x;
}

static ast$Expr *parseExpr(parser$Parser *p, bool lhs) {
    return parseTernaryExpr(p, lhs);
}

#pragma mark - types

static ast$Expr *parseTypeName(parser$Parser *p) {
    ast$Expr *x = parser$parseIdent(p);
    if (parser$accept(p, token$PERIOD)) {
        parser$resolve(p, x);
        ast$Expr y = {
            .kind = ast$EXPR_SELECTOR,
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
    p->exprLev++;
    ast$Expr *len = NULL;
    if (p->tok != token$RBRACK) {
        len = parseRhs(p);
    }
    p->exprLev--;
    parser$expect(p, token$RBRACK);
    ast$Expr type = {
        .kind = ast$TYPE_ARRAY,
        .array = {
            .pos = pos,
            .elt = parseType(p),
            .len = len,
        },
    };
    return esc(type);
}

static ast$Decl *parseFieldDecl(parser$Parser *p, ast$Scope *scope) {
    ast$Decl decl = {
        .kind = ast$DECL_FIELD,
        .pos = p->pos,
    };
    if (p->tok == token$UNION) {
        // anonymous union
        decl.field.type = parseType(p);
    } else {
        decl.field.name = parser$parseIdent(p);
        if (p->tok == token$SEMICOLON) {
            decl.field.type = decl.field.name;
            decl.field.name = NULL;
        } else {
            decl.field.type = parseType(p);
        }
    }
    ast$Decl *d = esc(decl);
    if (decl.field.name) {
        parser$declare(p, d, NULL, scope, ast$ObjKind_VAL, decl.field.name);
    }
    parser$resolve(p, decl.field.type);
    parser$expect(p, token$SEMICOLON);
    return d;
}

static ast$Expr *parseStructOrUnionType(parser$Parser *p, token$Token keyword) {
    token$Pos pos = p->pos;
    parser$expect(p, keyword);
    parser$openScope(p);
    ast$Decl **fields = NULL;
    if (parser$accept(p, token$LBRACE)) {
        ast$Scope *scope = ast$Scope_new(p->topScope);
        utils$Slice fieldSlice = {.size = sizeof(ast$Decl *)};
        for (;;) {
            ast$Decl *field = parseFieldDecl(p, scope);
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
        .kind = ast$TYPE_STRUCT,
        .struct_ = {
            .pos = pos,
            .tok = keyword,
            .fields = fields,
        },
    };
    parser$closeScope(p);
    return esc(x);
}

static ast$Expr *parsePointerType(parser$Parser *p) {
    token$Pos pos = p->pos;
    parser$expect(p, token$MUL);
    ast$Expr x = {
        .kind = ast$EXPR_STAR,
        .star = {
            .pos = pos,
            .x = parseType(p),
        },
    };
    return esc(x);
}

static ast$Decl *parseParam(parser$Parser *p, ast$Scope *scope, bool anon) {
    ast$Decl decl = {
        .kind = ast$DECL_FIELD,
        .pos = p->pos,
    };
    if (p->tok == token$IDENT) {
        decl.field.name = parser$parseIdent(p);
    }
    if (decl.field.name != NULL &&
            (p->tok == token$COMMA || p->tok == token$RPAREN)) {
        decl.field.type = decl.field.name;
        decl.field.name = NULL;
    } else {
        decl.field.type = parseType(p);
    }
    ast$Decl *d = esc(decl);
    if (d->field.name) {
        parser$declare(p, d, NULL, scope, ast$ObjKind_VAL, d->field.name);
    }
    parser$resolve(p, d->field.type);
    return d;
}

static ast$Decl **parseParameterList(parser$Parser *p, ast$Scope *scope,
        bool anon) {
    utils$Slice params = utils$Slice_init(sizeof(ast$Decl *));
    for (;;) {
        ast$Decl *param = parseParam(p, scope, false);
        utils$Slice_append(&params, &param);
        if (!parser$accept(p, token$COMMA)) {
            break;
        }
        if (p->tok == token$ELLIPSIS) {
            token$Pos pos = parser$expect(p, token$ELLIPSIS);
            ast$Expr type = {
                .kind = ast$TYPE_ELLIPSIS,
                .ellipsis = {
                    .pos = pos,
                },
            };
            ast$Decl decl = {
                .kind = ast$DECL_FIELD,
                .pos = pos,
                .field = {
                    .type = esc(type),
                },
            };
            ast$Decl *param = esc(decl);
            utils$Slice_append(&params, &param);
            break;
        }
    }
    return utils$Slice_to_nil_array(params);
}

static ast$Decl **parseParameters(parser$Parser *p, ast$Scope *scope,
        bool anon) {
    ast$Decl **params = NULL;
    parser$expect(p, token$LPAREN);
    if (p->tok != token$RPAREN) {
        params = parseParameterList(p, scope, anon);
    }
    parser$expect(p, token$RPAREN);
    return params;
}

static ast$Expr *parseFuncType(parser$Parser *p) {
    token$Pos pos = parser$expect(p, token$FUNC);
    ast$Scope *scope = ast$Scope_new(p->topScope);
    ast$Decl **params = parseParameters(p, scope, false);
    ast$Expr *result = NULL;
    if (p->tok != token$SEMICOLON) {
        result = parseType(p);
    }
    ast$Expr type = {
        .kind = ast$TYPE_FUNC,
        .func = {
            .pos = pos,
            .params = params,
            .result = result,
        },
    };
    ast$Expr ptr = {
        .kind = ast$EXPR_STAR,
        .star = {
            .pos = pos,
            .x = esc(type),
        },
    };
    return esc(ptr);
}

static ast$Expr *parseMapType(parser$Parser *p) {
    token$Pos pos = p->pos;
    parser$expect(p, token$MAP);
    ast$Expr tmp = {
        .kind = ast$TYPE_MAP,
        .map_ = {
            .pos = pos,
            .val = parseType(p),
        },
    };
    return esc(tmp);
}

static ast$Expr *parseEnumType(parser$Parser *p) {
    // enum_specifier
    //         : ENUM '{' enumerator_list '}'
    //         | ENUM IDENTIFIER '{' enumerator_list '}'
    //         | ENUM IDENTIFIER
    //         ;
    token$Pos pos = p->pos;
    parser$expect(p, token$ENUM);
    ast$Decl **enums = NULL;
    if (parser$accept(p, token$LBRACE)) {
        // enumerator_list : enumerator | enumerator_list ',' enumerator ;
        utils$Slice list = {.size = sizeof(ast$Decl *)};
        while (p->tok != token$RBRACE) {
            // enumerator : IDENTIFIER | IDENTIFIER '=' constant_expression ;
            ast$Decl decl = {
                .kind = ast$DECL_VALUE,
                .pos = p->pos,
                .value = {
                    .name = parser$parseIdent(p),
                },
            };
            if (parser$accept(p, token$ASSIGN)) {
                decl.value.value = parseRhs(p);
            }
            ast$Decl *enumerator = esc(decl);
            utils$Slice_append(&list, &enumerator);
            parser$expect(p, token$SEMICOLON);
        }
        enums = utils$Slice_to_nil_array(list);
        parser$expect(p, token$RBRACE);
    }
    ast$Expr x = {
        .kind = ast$TYPE_ENUM,
        .enum_ = {
            .pos = pos,
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

static ast$Expr *tryIdentOrType(parser$Parser *p) {
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
    case token$MAP:
        return parseMapType(p);
    case token$ENUM:
        return parseEnumType(p);
    case token$CONST:
        return parseQualifiedType(p, p->tok);
    default:
        return NULL;
    }
}

static ast$Expr *tryType(parser$Parser *p) {
    ast$Expr *t = tryIdentOrType(p);
    if (t != NULL) {
        parser$resolve(p, t);
    }
    return t;
}

static ast$Expr *parseType(parser$Parser *p) {
    ast$Expr *t = tryType(p);
    if (t == NULL) {
        parser$errorExpected(p, p->pos, "type");
    }
    return t;
}

static ast$Stmt *parseSimpleStmt(parser$Parser *p, bool labelOk) {
    // simple_statement
    //         : labeled_statement
    //         | expression_statement
    //         ;
    ast$Expr *x = parseExpr(p, true);
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
            ast$Expr *y = parseRhs(p);
            ast$Stmt stmt = {
                .kind = ast$STMT_ASSIGN,
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
                .kind = ast$STMT_POSTFIX,
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
    if (labelOk && x->kind == ast$EXPR_IDENT) {
        if (parser$accept(p, token$COLON)) {
            ast$Stmt stmt = {
                .kind = ast$STMT_LABEL,
                .label = {
                    .label = x,
                    .stmt = parseStmt(p),
                },
            };
            return esc(stmt);
        }
    }
    // expression_statement : expression? ';' ;
    ast$Stmt stmt = {
        .kind = ast$STMT_EXPR,
        .expr = {.x = x},
    };
    return esc(stmt);
}

static ast$Stmt *parseForStmt(parser$Parser *p) {
    // for_statement
    //         | FOR simple_statement? ';' expression? ';' expression?
    //              compound_statement ;
    token$Pos pos = parser$expect(p, token$FOR);
    int prevLev = p->exprLev;
    p->exprLev = -1;
    ast$Stmt *init = NULL;
    if (!parser$accept(p, token$SEMICOLON)) {
        init = parseStmt(p);
    }
    ast$Expr *cond = NULL;
    if (p->tok != token$SEMICOLON) {
        cond = parseRhs(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt *post = NULL;
    if (p->tok != token$LBRACE) {
        post = parseSimpleStmt(p, false);
    }
    p->exprLev = prevLev;
    ast$Stmt *body = parseBlockStmt(p);
    ast$Stmt stmt = {
        .kind = ast$STMT_ITER,
        .iter = {
            .pos = pos,
            .kind = token$FOR,
            .init = init,
            .cond = cond,
            .post = post,
            .body = body,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseIfStmt(parser$Parser *p) {
    // if_statement
    //         : IF expression compound_statement
    //         | IF expression compound_statement ELSE compound_statement
    //         | IF expression compound_statement ELSE if_statement
    //         ;
    token$Pos pos = parser$expect(p, token$IF);
    int outer = p->exprLev;
    p->exprLev = -1;
    ast$Expr *cond = parseExpr(p, true);
    p->exprLev = outer;
    if (p->tok != token$LBRACE) {
        parser$error(p, p->pos, "`if` must be followed by a compound_statement");
    }
    ast$Stmt *body = parseBlockStmt(p);
    ast$Stmt *else_ = NULL;
    if (parser$accept(p, token$ELSE)) {
        if (p->tok == token$IF) {
            else_ = parseStmt(p);
        } else if (p->tok == token$LBRACE) {
            else_ = parseBlockStmt(p);
        } else {
            parser$error(p, p->pos,
                    "`else` must be followed by an if_statement or compound_statement");
        }
    }
    ast$Stmt stmt = {
        .kind = ast$STMT_IF,
        .if_ = {
            .pos = pos,
            .cond = cond,
            .body = body,
            .else_ = else_,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseReturnStmt(parser$Parser *p) {
    // return_statement
    //         | RETURN expression? ';'
    //         ;
    token$Pos pos = parser$expect(p, token$RETURN);
    ast$Expr *x = NULL;
    if (p->tok != token$SEMICOLON) {
        x = parseRhs(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .kind = ast$STMT_RETURN,
        .return_ = {
            .pos = pos,
            .x = x,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseSwitchStmt(parser$Parser *p) {
    // switch_statement | SWITCH expression case_statement* ;
    token$Pos pos = parser$expect(p, token$SWITCH);
    int prevLev = p->exprLev;
    p->exprLev = -1;
    ast$Expr *tag = parseExpr(p, true);
    p->exprLev = prevLev;
    parser$expect(p, token$LBRACE);
    parser$openScope(p);
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
                ast$Expr *expr = parseRhs(p);
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
                ast$Stmt *stmt = parseStmt(p);
                utils$Slice_append(&stmts, &stmt);
            }
        }
        ast$Stmt stmt = {
            .kind = ast$STMT_CASE,
            .case_ = {
                .pos = pos,
                .exprs = utils$Slice_to_nil_array(exprs),
                .stmts = utils$Slice_to_nil_array(stmts),
            },
        };
        ast$Stmt *clause = esc(stmt);
        utils$Slice_append(&clauses, &clause);
    }
    parser$closeScope(p);
    parser$expect(p, token$RBRACE);
    parser$accept(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .kind = ast$STMT_SWITCH,
        .switch_ = {
            .pos = pos,
            .tag = tag,
            .stmts = utils$Slice_to_nil_array(clauses),
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseWhileStmt(parser$Parser *p) {
    // while_statement : WHILE expression compound_statement ;
    token$Pos pos = parser$expect(p, token$WHILE);
    int prevLev = p->exprLev;
    p->exprLev = -1;
    ast$Expr *cond = parseExpr(p, true);
    p->exprLev = prevLev;
    ast$Stmt *body = parseBlockStmt(p);
    ast$Stmt stmt = {
        .kind = ast$STMT_ITER,
        .iter = {
            .pos = pos,
            .kind = token$WHILE,
            .cond = cond,
            .body = body,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseJumpStmt(parser$Parser *p, token$Token keyword) {
    // jump_statement
    //         : GOTO IDENTIFIER ';'
    //         | CONTINUE ';'
    //         | BREAK ';'
    //         | FALLTHROUGH ';'
    //         ;
    token$Pos pos = parser$expect(p, keyword);
    ast$Expr *label = NULL;
    if (keyword == token$GOTO) {
        label = parser$parseIdent(p);
    }
    parser$expect(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .kind = ast$STMT_JUMP,
        .jump = {
            .pos = pos,
            .keyword = keyword,
            .label = label,
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseDeclStmt(parser$Parser *p) {
    ast$Stmt stmt = {
        .kind = ast$STMT_DECL,
        .decl = {
            .decl = parseDecl(p),
        },
    };
    return esc(stmt);
}

static ast$Stmt *parseStmt(parser$Parser *p) {
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
    case token$VAR: return parseDeclStmt(p);
    case token$FOR: return parseForStmt(p);
    case token$IF: return parseIfStmt(p);
    case token$RETURN: return parseReturnStmt(p);
    case token$SWITCH: return parseSwitchStmt(p);
    case token$WHILE: return parseWhileStmt(p);
    case token$BREAK:
    case token$CONTINUE:
    case token$FALLTHROUGH:
    case token$GOTO: return parseJumpStmt(p, p->tok);
    case token$LBRACE: return parseBlockStmt(p);
    default:
        break;
    }
    token$Pos pos = p->pos;
    if (parser$accept(p, token$SEMICOLON)) {
        ast$Stmt stmt = {
            .kind = ast$STMT_EMPTY,
            .empty = {
                .pos = pos,
            },
        };
        return esc(stmt);
    }
    ast$Stmt *stmt = parseSimpleStmt(p, true);
    if (stmt->kind != ast$STMT_LABEL) {
        parser$expect(p, token$SEMICOLON);
    }
    return stmt;
}

static ast$Stmt **parseStmtList(parser$Parser *p) {
    utils$Slice stmts = {.size = sizeof(ast$Stmt *)};
    while (p->tok != token$RBRACE) {
        ast$Stmt *stmt = parseStmt(p);
        utils$Slice_append(&stmts, &stmt);
    }
    return utils$Slice_to_nil_array(stmts);
}

static ast$Stmt *parseBody(parser$Parser *p, ast$Scope *scope) {
    token$Pos pos = parser$expect(p, token$LBRACE);
    assert(p->topScope == scope->outer);
    p->topScope = scope;
    ast$Stmt **list = parseStmtList(p);
    parser$closeScope(p);
    parser$expect(p, token$RBRACE);
    parser$accept(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .kind = ast$STMT_BLOCK,
        .block = {
            .pos = pos,
            .stmts = list,
        }
    };
    return esc(stmt);
}

static ast$Stmt *parseBlockStmt(parser$Parser *p) {
    token$Pos pos = parser$expect(p, token$LBRACE);
    parser$openScope(p);
    ast$Stmt **list = parseStmtList(p);
    parser$closeScope(p);
    parser$expect(p, token$RBRACE);
    parser$accept(p, token$SEMICOLON);
    ast$Stmt stmt = {
        .kind = ast$STMT_BLOCK,
        .block = {
            .pos = pos,
            .stmts = list,
        }
    };
    return esc(stmt);
}

extern ast$Decl *parser$parsePragma(parser$Parser *p) {
    token$Pos pos = p->pos;
    char *lit = p->lit;
    p->lit = NULL;
    parser$expect(p, token$HASH);
    ast$Decl decl = {
        .kind = ast$DECL_PRAGMA,
        .pos = pos,
        .pragma = {
            .lit = lit,
        },
    };
    return esc(decl);
}

static ast$Decl *parseValueDecl(parser$Parser *p) {
    token$Token keyword = p->tok;
    token$Pos pos = parser$expect(p, keyword);
    ast$Expr *ident = parser$parseIdent(p);
    ast$Expr *type = NULL;
    if (p->tok != token$ASSIGN) {
        type = parseType(p);
    }
    ast$Expr *value = NULL;
    if (parser$accept(p, token$ASSIGN)) {
        if (p->tok == token$LBRACE) {
            value = parseLiteralValue(p, NULL);
        } else {
            value = parseRhs(p);
        }
    }
    parser$expect(p, token$SEMICOLON);
    ast$Decl decl = {
        .kind = ast$DECL_VALUE,
        .pos = pos,
        .value = {
            .name = ident,
            .type = type,
            .value = value,
            .kind = keyword,
        },
    };
    ast$Decl *d = esc(decl);
    parser$declare(p, d, NULL, p->topScope, ast$ObjKind_VAL, d->value.name);
    return d;
}

static ast$Decl *parseTypeDecl(parser$Parser *p) {
    token$Token keyword = p->tok;
    token$Pos pos = parser$expect(p, keyword);
    ast$Expr *ident = parser$parseIdent(p);
    ast$Expr *type = parseType(p);
    parser$expect(p, token$SEMICOLON);
    ast$Decl decl = {
        .kind = ast$DECL_TYPEDEF,
        .pos = pos,
        .typedef_ = {
            .name = ident,
            .type = type,
        },
    };
    ast$Decl *d = esc(decl);
    parser$declare(p, d, NULL, p->topScope, ast$ObjKind_TYP, ident);
    return d;
}

static ast$Decl *parseFuncDecl(parser$Parser *p) {
    token$Pos pos = parser$expect(p, token$FUNC);
    ast$Decl decl = {
        .kind = ast$DECL_FUNC,
        .pos = pos,
        .func = {
            .name = parser$parseIdent(p),
        },
    };
    ast$Scope *scope = ast$Scope_new(p->topScope);
    ast$Expr type = {
        .kind = ast$TYPE_FUNC,
        .func = {
            .pos = pos,
            .params = parseParameters(p, scope, false),
        },
    };
    if (p->tok != token$LBRACE && p->tok != token$SEMICOLON) {
        type.func.result = parseType(p);
    }
    decl.func.type = esc(type);
    if (p->tok == token$LBRACE) {
        decl.func.body = parseBody(p, scope);
    } else {
        parser$expect(p, token$SEMICOLON);
    }
    ast$Decl *d = esc(decl);
    parser$declare(p, d, NULL, p->topScope, ast$ObjKind_FUN, d->func.name);
    return d;
}

static ast$Decl *parseDecl(parser$Parser *p) {
    switch (p->tok) {
    case token$HASH:
        return parser$parsePragma(p);
    case token$TYPE:
        return parseTypeDecl(p);
    case token$CONST:
    case token$VAR:
        return parseValueDecl(p);
    case token$FUNC:
        return parseFuncDecl(p);
    default:
        parser$error(p, p->pos,
                sys$sprintf("cant handle it: %s", token$string(p->tok)));
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
        ast$Scope *scope, utils$Error **first) {
    utils$Error *err = NULL;
    os$FileInfo **infos = ioutil$readDir(path, &err);
    if (err) {
        utils$Error_move(err, first);
        return NULL;
    }
    utils$Slice files = utils$Slice_init(sizeof(uintptr_t));
    for (int i = 0; infos[i]; i++) {
        char *name = os$FileInfo_name(infos[i]);
        if (isBlingFile(name) && !isTestFile(name)) {
            ast$File *file = parser$parseFile(fset, name, scope);
            utils$Slice_append(&files, &file);
        }
    }
    free(infos);
    return utils$Slice_to_nil_array(files);
}

static ast$File *_parseFile(parser$Parser *p, ast$Scope *scope) {
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
    assert(p->topScope == NULL);
    p->topScope = scope;
    parser$openScope(p);
    p->pkgScope = p->topScope;
    if (name) {
        p->pkgScope->pkg = name->ident.name;
    }
    while (p->tok == token$IMPORT) {
        token$Pos pos = parser$expect(p, token$IMPORT);
        ast$Expr *path = parser$parseBasicLit(p, token$STRING);
        parser$expect(p, token$SEMICOLON);
        ast$Decl decl = {
            .kind = ast$DECL_IMPORT,
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
    parser$closeScope(p);
    assert(p->topScope == scope);
    ast$File file = {
        .filename = p->file->name,
        .name = name,
        .imports = utils$Slice_to_nil_array(imports),
        .decls = utils$Slice_to_nil_array(decls),
        .scope = p->pkgScope,
    };
    return esc(file);
}

extern ast$File *parser$parseFile(token$FileSet *fset, const char *filename,
        ast$Scope *scope) {
    // sys$printf("parsing %s\n", filename);
    utils$Error *err = NULL;
    char *src = ioutil$readFile(filename, &err);
    if (err) {
        panic(sys$sprintf("%s: %s", filename, err->error));
    }
    parser$Parser p = {};
    parser$init(&p, fset, filename, src);
    free(src);
    ast$File *file = _parseFile(&p, scope);
    // print(sys$sprintf("%s: %d/%d unresolved", filename,
    //             utils$Slice_len(&p.unresolved),
    //             p.numIdents));
    return file;
}
