#include "token.h"

extern char *token_string(token_t tok) {
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

    case token_STRUCT: return "struct";
    case token_TYPEDEF: return "typedef";
    case token_UNION: return "union";

    case token_CHAR: return "$CHAR";
    case token_IDENT: return "$IDENT";
    case token_STRING: return "$STRING";
    case token_TYPE_NAME: return "$TYPE";

    default: print("unknown token: %d", tok); return "???";
    }
}

extern int token_lookup(char *ident) {
    if (!strcmp(ident, "const"))
        return token_CONST;
    if (!strcmp(ident, "else"))
        return token_ELSE;
    if (!strcmp(ident, "enum"))
        return token_ENUM;
    if (!strcmp(ident, "extern"))
        return token_EXTERN;
    if (!strcmp(ident, "for"))
        return token_FOR;
    if (!strcmp(ident, "if"))
        return token_IF;
    if (!strcmp(ident, "static"))
        return token_STATIC;
    if (!strcmp(ident, "struct"))
        return token_STRUCT;
    if (!strcmp(ident, "switch"))
        return token_SWITCH;
    if (!strcmp(ident, "typedef"))
        return token_TYPEDEF;
    if (!strcmp(ident, "union"))
        return token_UNION;
    if (!strcmp(ident, "while"))
        return token_WHILE;
    if (!strcmp(ident, "return"))
        return token_RETURN;
    return token_IDENT;
}
