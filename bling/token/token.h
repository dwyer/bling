#pragma once
#include "slice/slice.h"

package(token);

import("fmt");
import("map");
import("slice");

typedef enum {
    token$ILLEGAL = 0,

    token$CHAR,
    token$FLOAT,
    token$IDENT,
    token$INT,
    token$STRING,

    token$_keywordBeg,
    token$BREAK,
    token$CASE,
    token$CONST,
    token$CONTINUE,
    token$DEFAULT,
    token$ELSE,
    token$ENUM,
    token$EXTERN,
    token$FOR,
    token$FUNC,
    token$GOTO,
    token$IF,
    token$IMPORT,
    token$PACKAGE,
    token$RETURN,
    token$SIGNED,
    token$SIZEOF,
    token$STATIC,
    token$STRUCT,
    token$SWITCH,
    token$TYPEDEF,
    token$UNION,
    token$UNSIGNED,
    token$VAR,
    token$WHILE,
    token$_keywordEnd,

    token$ARROW,
    token$COLON,
    token$COMMA,
    token$ELLIPSIS,
    token$EOF,
    token$HASH,
    token$LBRACE,
    token$LBRACK,
    token$LPAREN,
    token$PERIOD,
    token$QUESTION_MARK,
    token$RBRACE,
    token$RBRACK,
    token$RPAREN,
    token$SEMICOLON,

    // operators
    token$ADD,
    token$ADD_ASSIGN,
    token$AND,
    token$AND_ASSIGN,
    token$ASSIGN,
    token$BITWISE_NOT,
    token$DEC,
    token$DIV,
    token$DIV_ASSIGN,
    token$DOLLAR,
    token$EQUAL,
    token$GT,
    token$GT_EQUAL,
    token$INC,
    token$LAND,
    token$LOR,
    token$LT,
    token$LT_EQUAL,
    token$MOD,
    token$MOD_ASSIGN,
    token$MUL,
    token$MUL_ASSIGN,
    token$NOT,
    token$NOT_EQUAL,
    token$OR,
    token$OR_ASSIGN,
    token$SHL,
    token$SHL_ASSIGN,
    token$SHR,
    token$SHR_ASSIGN,
    token$SUB,
    token$SUB_ASSIGN,
    token$XOR,
    token$XOR_ASSIGN,
} token$Token;

typedef int token$Pos;

extern char *token$string(token$Token tok);
extern token$Token token$lookup(char *ident);

typedef enum {
    token$lowest_prec = 0,
    token$unary_prec = 11,
    token$highest_prec = 12,
} token$Prec;

extern int token$precedence(token$Token op);

typedef struct {
    char *name;
    slice$Slice lines;
} token$File;

typedef struct {
    char *filename;
    int offset;
    int line;
    int column;
} token$Position;

extern char *token$Position_string(token$Position *p);

extern token$File      *token$File_new(const char *filename);
extern void             token$File_addLine(token$File *f, int offset);
extern token$Position   token$File_position(token$File *f, token$Pos p);
