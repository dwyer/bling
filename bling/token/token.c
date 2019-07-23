#include "bling/token/token.h"
#include "map/map.h"

static char *tokens[] = {
    [token_ILLEGAL] = "ILLEGAL",

    [token_CHAR] = "CHAR",
    [token_FLOAT] = "FLOAT",
    [token_IDENT] = "IDENT",
    [token_INT] = "INT",
    [token_STRING] = "STRING",
    [token_TYPE_NAME] = "TYPE",

    [token_BREAK] = "break",
    [token_CASE] = "case",
    [token_CONST] = "const",
    [token_CONTINUE] = "continue",
    [token_DEFAULT] = "default",
    [token_ELSE] = "else",
    [token_ENUM] = "enum",
    [token_EXTERN] = "extern",
    [token_FOR] = "for",
    [token_FUNC] = "fun",
    [token_GOTO] = "goto",
    [token_IF] = "if",
    [token_IMPORT] = "import",
    [token_PACKAGE] = "package",
    [token_RETURN] = "return",
    [token_SIGNED] = "signed",
    [token_SIZEOF] = "sizeof",
    [token_STATIC] = "static",
    [token_STRUCT] = "struct",
    [token_SWITCH] = "switch",
    [token_TYPEDEF] = "typedef",
    [token_UNION] = "union",
    [token_UNSIGNED] = "unsigned",
    [token_VAR] = "var",
    [token_WHILE] = "while",

    [token_ARROW] = "->",
    [token_COLON] = ":",
    [token_COMMA] = ",",
    [token_ELLIPSIS] = "...",
    [token_EOF] = "EOF",
    [token_HASH] = "#",
    [token_LBRACE] = "{",
    [token_LBRACK] = "[",
    [token_LPAREN] = "(",
    [token_PERIOD] = ".",
    [token_QUESTION_MARK] = "?",
    [token_RBRACE] = "}",
    [token_RBRACK] = "]",
    [token_RPAREN] = ")",
    [token_SEMICOLON] = ";",

    [token_ADD] = "+",
    [token_ADD_ASSIGN] = "+=",
    [token_AND] = "&",
    [token_AND_ASSIGN] = "&=",
    [token_ASSIGN] = "=",
    [token_BITWISE_NOT] = "~",
    [token_DEC] = "--",
    [token_DIV] = "/",
    [token_DIV_ASSIGN] = "/=",
    [token_EQUAL] = "==",
    [token_GT] = ">",
    [token_GT_EQUAL] = ">=",
    [token_INC] = "++",
    [token_LAND] = "&&",
    [token_LOR] = "||",
    [token_LT] = "<",
    [token_LT_EQUAL] = "<=",
    [token_MOD] = "%",
    [token_MOD_ASSIGN] = "%=",
    [token_MUL] = "*",
    [token_MUL_ASSIGN] = "*=",
    [token_NOT] = "!",
    [token_NOT_EQUAL] = "!=",
    [token_OR] = "|",
    [token_OR_ASSIGN] = "|=",
    [token_SHL] = "<<",
    [token_SHL_ASSIGN] = "<<=",
    [token_SHR] = ">>",
    [token_SHR_ASSIGN] = ">>=",
    [token_SUB] = "-",
    [token_SUB_ASSIGN] = "-=",
    [token_XOR] = "^",
    [token_XOR_ASSIGN] = "^=",
};

extern char *token_string(token_t tok) {
    return tokens[tok];
}

static map_t keywords = {};

extern token_t token_lookup(char *ident) {
    if (keywords.val_size == 0) {
        keywords = map_init(sizeof(token_t));
        for (token_t tok = _token_keyword_beg + 1; tok < _token_keyword_end;
                tok++) {
            char *s = token_string(tok);
            map_set(&keywords, s, &tok);
        }
    }
    token_t tok = token_IDENT;
    map_get(&keywords, ident, &tok);
    return tok;
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
