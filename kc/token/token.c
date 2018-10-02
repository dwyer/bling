#include "token.h"

#define num_tokens 59

static struct {
    int tok;
    char *s;
} tokens[num_tokens] = {

    {token_BREAK, "break"},
    {token_CASE, "case"},
    {token_CONST, "const"},
    {token_CONTINUE, "continue"},
    {token_DEFAULT, "default"},
    {token_ELSE, "else"},
    {token_ENUM, "enum"},
    {token_EXTERN, "extern"},
    {token_FOR, "for"},
    {token_GOTO, "goto"},
    {token_IF, "if"},
    {token_RETURN, "return"},
    {token_STATIC, "static"},
    {token_STRUCT, "struct"},
    {token_SWITCH, "switch"},
    {token_TYPEDEF, "typedef"},
    {token_UNION, "union"},
    {token_WHILE, "which"},

    {token_ARROW, "->"},
    {token_COLON, ":"},
    {token_COMMA, ","},
    {token_ELLIPSIS, "..."},
    {token_EOF, "EOF"},
    {token_LBRACE, "{"},
    {token_LBRACK, "["},
    {token_LPAREN, "("},
    {token_PERIOD, "."},
    {token_RBRACE, "}"},
    {token_RBRACK, "]"},
    {token_RPAREN, ")"},
    {token_SEMICOLON, ";"},

    {token_ADD, "+"},
    {token_ADD_ASSIGN, "+="},
    {token_AND, "&"},
    {token_AND_ASSIGN, "&="},
    {token_ASSIGN, "="},
    {token_DEC, "--"},
    {token_DIV, "/"},
    {token_DIV_ASSIGN, "/="},
    {token_EQUAL, "=="},
    {token_GT, ">"},
    {token_GT_EQUAL, ">="},
    {token_INC, "++"},
    {token_LAND, "&&"},
    {token_LOR, "||"},
    {token_LT, "<"},
    {token_LT_EQUAL, "<="},
    {token_MUL, "*"},
    {token_MUL_ASSIGN, "*="},
    {token_NOT, "!"},
    {token_NOT_EQUAL, "!="},
    {token_OR, "|"},
    {token_OR_ASSIGN, "|="},
    {token_SUB, "-"},
    {token_SUB_ASSIGN, "-="},

    {token_CHAR, "$CHAR"},
    {token_IDENT, "$IDENT"},
    {token_STRING, "$STRING"},
    {token_TYPE_NAME, "$TYPE"},
};

extern char *token_string(token_t tok) {
    //for (int i = 0; i < num_tokens; i++) {
    //    if (tok == tokens[i].tok) {
    //        return tokens[i].s;
    //    }
    //}

    switch (tok) {

    case token_ARROW: return "->";
    case token_COLON: return ":";
    case token_COMMA: return ",";
    case token_ELLIPSIS: return "...";
    case token_EOF: return "EOF";
    case token_LBRACE: return "{";
    case token_LBRACK: return "[";
    case token_LPAREN: return "(";
    case token_PERIOD: return ".";
    case token_RBRACE: return "}";
    case token_RBRACK: return "]";
    case token_RPAREN: return ")";
    case token_SEMICOLON: return ";";

    case token_ADD: return "+";
    case token_ADD_ASSIGN: return "+=";
    case token_AND: return "&";
    case token_AND_ASSIGN: return "&=";
    case token_ASSIGN: return "=";
    case token_DEC: return "--";
    case token_DIV: return "/";
    case token_DIV_ASSIGN: return "/=";
    case token_EQUAL: return "==";
    case token_GT: return ">";
    case token_GT_EQUAL: return ">=";
    case token_INC: return "++";
    case token_LAND: return "&&";
    case token_LOR: return "||";
    case token_LT: return "<";
    case token_LT_EQUAL: return "<=";
    case token_MUL: return "*";
    case token_MUL_ASSIGN: return "*=";
    case token_NOT: return "!";
    case token_NOT_EQUAL: return "!=";
    case token_OR: return "|";
    case token_OR_ASSIGN: return "|=";
    case token_SUB: return "-";
    case token_SUB_ASSIGN: return "-=";

    case token_BREAK: return "break";
    case token_CONST: return "const";
    case token_CONTINUE: return "continue";
    case token_ELSE: return "else";
    case token_ENUM: return "enum";
    case token_EXTERN: return "extern";
    case token_FOR: return "for";
    case token_GOTO: return "goto";
    case token_IF: return "if";
    case token_RETURN: return "return";
    case token_STATIC: return "static";
    case token_STRUCT: return "struct";
    case token_SWITCH: return "switch";
    case token_TYPEDEF: return "typedef";
    case token_UNION: return "union";
    case token_WHILE: return "which";

    case token_CHAR: return "$CHAR";
    case token_IDENT: return "$IDENT";
    case token_STRING: return "$STRING";
    case token_TYPE_NAME: return "$TYPE";

    default: print("unknown token: %d", tok); return "???";
    }
}

extern int token_lookup(char *ident) {
    //for (int i = 0; i < num_tokens; i++) {
    //    if (!strcmp(ident, tokens[i].s)) {
    //        return tokens[i].tok;
    //    }
    //}
    if (!strcmp(ident, "break")) {
        return token_BREAK;
    }
    if (!strcmp(ident, "case")) {
        return token_CASE;
    }
    if (!strcmp(ident, "const")) {
        return token_CONST;
    }
    if (!strcmp(ident, "continue")) {
        return token_CONTINUE;
    }
    if (!strcmp(ident, "default")) {
        return token_DEFAULT;
    }
    if (!strcmp(ident, "else")) {
        return token_ELSE;
    }
    if (!strcmp(ident, "enum")) {
        return token_ENUM;
    }
    if (!strcmp(ident, "extern")) {
        return token_EXTERN;
    }
    if (!strcmp(ident, "for")) {
        return token_FOR;
    }
    if (!strcmp(ident, "goto")) {
        return token_GOTO;
    }
    if (!strcmp(ident, "if")) {
        return token_IF;
    }
    if (!strcmp(ident, "static")) {
        return token_STATIC;
    }
    if (!strcmp(ident, "struct")) {
        return token_STRUCT;
    }
    if (!strcmp(ident, "switch")) {
        return token_SWITCH;
    }
    if (!strcmp(ident, "typedef")) {
        return token_TYPEDEF;
    }
    if (!strcmp(ident, "union")) {
        return token_UNION;
    }
    if (!strcmp(ident, "while")) {
        return token_WHILE;
    }
    if (!strcmp(ident, "return")) {
        return token_RETURN;
    }
    return token_IDENT;
}
