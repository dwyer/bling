#include "token.h"

extern char *token_string(token_t tok) {
    switch (tok) {
    case token_ILLEGAL: return "ILLEGAL";

    case token_CHAR: return "CHAR";
    case token_FLOAT: return "FLOAT";
    case token_IDENT: return "IDENT";
    case token_INT: return "INT";
    case token_STRING: return "STRING";
    case token_TYPE_NAME: return "TYPE";

    case token_AS: return "$as";
    case token_BREAK: return "break";
    case token_CASE: return "case";
    case token_CONST: return "const";
    case token_CONTINUE: return "continue";
    case token_DEFAULT: return "default";
    case token_ELSE: return "else";
    case token_ENUM: return "enum";
    case token_EXTERN: return "extern";
    case token_FOR: return "for";
    case token_FUNC: return "$func";
    case token_GOTO: return "goto";
    case token_IF: return "if";
    case token_IMPORT: return "$import";
    case token_PACKAGE: return "$package";
    case token_RETURN: return "return";
    case token_SIGNED: return "signed";
    case token_SIZEOF: return "sizeof";
    case token_STATIC: return "static";
    case token_STRUCT: return "struct";
    case token_SWITCH: return "switch";
    case token_TYPE: return "$type";
    case token_TYPEDEF: return "typedef";
    case token_UNION: return "union";
    case token_UNSIGNED: return "unsigned";
    case token_VAR: return "$var";
    case token_WHILE: return "while";

    case token_ARROW: return "->";
    case token_COLON: return ":";
    case token_COMMA: return ",";
    case token_ELLIPSIS: return "...";
    case token_EOF: return "EOF";
    case token_HASH: return "#";
    case token_LBRACE: return "{";
    case token_LBRACK: return "[";
    case token_LPAREN: return "(";
    case token_PERIOD: return ".";
    case token_QUESTION_MARK: return "?";
    case token_RBRACE: return "}";
    case token_RBRACK: return "]";
    case token_RPAREN: return ")";
    case token_SEMICOLON: return ";";

    case token_ADD: return "+";
    case token_ADD_ASSIGN: return "+=";
    case token_AND: return "&";
    case token_AND_ASSIGN: return "&=";
    case token_ASSIGN: return "=";
    case token_BITWISE_NOT: return "~";
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
    case token_MOD: return "%";
    case token_MOD_ASSIGN: return "%=";
    case token_MUL: return "*";
    case token_MUL_ASSIGN: return "*=";
    case token_NOT: return "!";
    case token_NOT_EQUAL: return "!=";
    case token_OR: return "|";
    case token_OR_ASSIGN: return "|=";
    case token_SHL: return "<<";
    case token_SHL_ASSIGN: return "<<=";
    case token_SHR: return ">>";
    case token_SHR_ASSIGN: return ">>=";
    case token_SUB: return "-";
    case token_SUB_ASSIGN: return "-=";
    case token_XOR: return "^";
    case token_XOR_ASSIGN: return "^=";
    }
    return "???";
}

extern token_t token_lookup(char *ident) {
    for (token_t tok = token_AS; tok <= token_WHILE; tok++) {
        char *s = token_string(tok);
        if (streq(ident, s)) {
            return tok;
        }
    }
    return token_IDENT;
}

extern int token_precedence(token_t op) {
    switch (op) {
    case token_MOD:
    case token_MUL:
    case token_DIV:
        return 10;
    case token_ADD:
    case token_SUB:
        return 9;
    case token_SHL:
    case token_SHR:
        return 8;
    case token_GT:
    case token_GT_EQUAL:
    case token_LT:
    case token_LT_EQUAL:
        return 7;
    case token_EQUAL:
    case token_NOT_EQUAL:
        return 6;
    case token_AND:
        return 5;
    case token_XOR:
        return 4;
    case token_OR:
        return 3;
    case token_LAND:
        return 2;
    case token_LOR:
        return 1;
    default:
        return token_lowest_prec;
    }
}
