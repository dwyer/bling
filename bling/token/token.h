#pragma once
#include "builtin/builtin.h"

$package(token);

typedef enum {
    token_ILLEGAL = 0,

    token_IDENT,
    token_TYPE_NAME,

    token_CHAR,
    token_FLOAT,
    token_INT,
    token_STRING,

    token_BREAK,
    token_CASE,
    token_CONST,
    token_CONTINUE,
    token_DEFAULT,
    token_ELSE,
    token_ENUM,
    token_EXTERN,
    token_FOR,
    token_FUNC,
    token_GOTO,
    token_IF,
    token_IMPORT,
    token_PACKAGE,
    token_RETURN,
    token_SIGNED,
    token_SIZEOF,
    token_STATIC,
    token_STRUCT,
    token_SWITCH,
    token_TYPE,
    token_TYPEDEF,
    token_UNION,
    token_UNSIGNED,
    token_VAR,
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
    token_QUESTION_MARK,
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
    token_MOD,
    token_MOD_ASSIGN,
    token_MUL,
    token_MUL_ASSIGN,
    token_NOT,
    token_NOT_EQUAL,
    token_OR,
    token_OR_ASSIGN,
    token_SHL,
    token_SHL_ASSIGN,
    token_SHR,
    token_SHR_ASSIGN,
    token_SUB,
    token_SUB_ASSIGN,
    token_XOR,
    token_XOR_ASSIGN,
} token_t;

extern char *token_string(token_t tok);
extern token_t token_lookup(char *ident);

enum {
    token_lowest_prec = 0,
    token_unary_prec = 11,
    token_highest_prec = 12,
};

extern int token_precedence(token_t op);
