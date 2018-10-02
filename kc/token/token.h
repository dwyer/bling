#pragma once
#include "builtin/builtin.h"

typedef enum {
    token_ILLEGAL = 0,

    token_IDENT,
    token_TYPE_NAME,

    token_CHAR,
    token_INT,
    token_STRING,

    token_CONST,
    token_ELSE,
    token_ENUM,
    token_EXTERN,
    token_FOR,
    token_IF,
    token_RETURN,
    token_STATIC,
    token_STRUCT,
    token_SWITCH,
    token_TYPEDEF,
    token_UNION,
    token_WHILE,

    token_ARROW,
    token_COLON,
    token_COMMA,
    token_ELLIPSIS,
    token_EOF,
    token_LBRACE,
    token_LBRACK,
    token_LPAREN,
    token_PERIOD,
    token_RBRACE,
    token_RBRACK,
    token_RPAREN,
    token_SEMICOLON,

    // operators
    token_ADD,
    token_ADD_ASSIGN,
    token_AND,
    token_AND_ASSIGN,
    token_ASSIGN,
    token_DEC,
    token_DIV,
    token_DIV_ASSIGN,
    token_EQUAL,
    token_GT,
    token_GT_EQUAL,
    token_INC,
    token_LAND,
    token_LOR,
    token_LT,
    token_LT_EQUAL,
    token_MUL,
    token_MUL_ASSIGN,
    token_NOT,
    token_NOT_EQUAL,
    token_OR,
    token_OR_ASSIGN,
    token_SUB,
    token_SUB_ASSIGN,
} token_t;

char *token_string(token_t tok);
