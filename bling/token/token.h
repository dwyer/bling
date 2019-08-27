#pragma once
#include "utils/utils.h"

package(token);

import("bytes");
import("sys");
import("utils");

typedef enum {
    token$ILLEGAL = 0,

    token$CHAR,
    token$FLOAT,
    token$IDENT,
    token$INT,
    token$STRING,

    token$_keywordBeg,

    token$ARRAY,
    token$BREAK,
    token$CASE,
    token$CONST,
    token$CONTINUE,
    token$DEFAULT,
    token$ELSE,
    token$ENUM,
    token$ESC,
    token$FALLTHROUGH,
    token$FOR,
    token$FUNC,
    token$GOTO,
    token$IF,
    token$IMPORT,
    token$MAP,
    token$PACKAGE,
    token$RETURN,
    token$SIZEOF,
    token$STRUCT,
    token$SWITCH,
    token$TYPE,
    token$UNION,
    token$VAR,
    token$WHILE,

    // reserved by external C compiler:
    // inline and restrict can be removed when we emit c89
    token$EXTERN,
    token$SIGNED,
    token$STATIC,
    token$TYPEDEF,
    token$UNSIGNED,
    token$_AUTO,
    token$_COMPLEX, // c99
    token$_DO,
    token$_IMAGINARY, // c99
    token$_INLINE, // c99
    token$_LONG,
    token$_REGISTER,
    token$_RESTRICT, // c99
    token$_SHORT,
    token$_TYPEOF,
    token$_VOLATILE,

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

typedef struct token$FileSet token$FileSet;

typedef struct {
    token$FileSet *set;
    char *name;
    int base;
    int size;
    array(int) lines;
    char *src;
} token$File;

typedef struct token$FileSet {
    token$FileSet *fset;
    int base;
    array(token$File *) files;
    token$File *last;
} token$FileSet;

typedef struct {
    char *filename;
    int offset;
    int line;
    int column;
} token$Position;

extern char *token$Position_string(token$Position *p);

extern void             token$File_addLine(token$File *f, int offset);
extern token$Pos        token$File_pos(token$File *f, int offset);
extern token$Position   token$File_position(token$File *f, token$Pos p);
extern char            *token$File_lineString(token$File *f, int line);
extern void             token$FileSet_print(token$FileSet *fset);

extern token$FileSet *token$newFileSet();
extern token$File    *token$FileSet_addFile(token$FileSet *s,
        const char *filename, int base, int size);
extern token$File    *token$FileSet_file(token$FileSet *s, token$Pos pos);
