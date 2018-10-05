#include "token.h"

#define num_tokens 60

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
    {token_SIZEOF, "sizeof"},
    {token_STATIC, "static"},
    {token_STRUCT, "struct"},
    {token_SWITCH, "switch"},
    {token_TYPEDEF, "typedef"},
    {token_UNION, "union"},
    {token_WHILE, "while"},

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
    for (int i = 0; i < num_tokens; i++) {
        if (tok == tokens[i].tok) {
            return tokens[i].s;
        }
    }
    print("unknown token: %d", tok);
    return "???";
}

extern int token_lookup(char *ident) {
    for (int i = 0; i < 19; i++) {
        if (!strcmp(ident, tokens[i].s)) {
            return tokens[i].tok;
        }
    }
    return token_IDENT;
}
