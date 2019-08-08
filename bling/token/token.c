#include "bling/token/token.h"

#include "map/map.h"
#include "sys/sys.h"

static char *tokens[] = {
    [token$ILLEGAL] = "ILLEGAL",

    [token$CHAR] = "CHAR",
    [token$FLOAT] = "FLOAT",
    [token$IDENT] = "IDENT",
    [token$INT] = "INT",
    [token$STRING] = "STRING",

    [token$BREAK] = "break",
    [token$CASE] = "case",
    [token$CONST] = "const",
    [token$CONTINUE] = "continue",
    [token$DEFAULT] = "default",
    [token$ELSE] = "else",
    [token$ENUM] = "enum",
    [token$EXTERN] = "extern",
    [token$FOR] = "for",
    [token$FUNC] = "fun",
    [token$GOTO] = "goto",
    [token$IF] = "if",
    [token$IMPORT] = "import",
    [token$PACKAGE] = "package",
    [token$RETURN] = "return",
    [token$SIGNED] = "signed",
    [token$SIZEOF] = "sizeof",
    [token$STATIC] = "static",
    [token$STRUCT] = "struct",
    [token$SWITCH] = "switch",
    [token$TYPEDEF] = "typedef",
    [token$UNION] = "union",
    [token$UNSIGNED] = "unsigned",
    [token$VAR] = "var",
    [token$WHILE] = "while",

    [token$ARROW] = "->",
    [token$COLON] = ":",
    [token$COMMA] = ",",
    [token$ELLIPSIS] = "...",
    [token$EOF] = "EOF",
    [token$HASH] = "#",
    [token$LBRACE] = "{",
    [token$LBRACK] = "[",
    [token$LPAREN] = "(",
    [token$PERIOD] = ".",
    [token$QUESTION_MARK] = "?",
    [token$RBRACE] = "}",
    [token$RBRACK] = "]",
    [token$RPAREN] = ")",
    [token$SEMICOLON] = ";",

    [token$ADD] = "+",
    [token$ADD_ASSIGN] = "+=",
    [token$AND] = "&",
    [token$AND_ASSIGN] = "&=",
    [token$ASSIGN] = "=",
    [token$BITWISE_NOT] = "~",
    [token$DEC] = "--",
    [token$DIV] = "/",
    [token$DIV_ASSIGN] = "/=",
    [token$DOLLAR] = "$",
    [token$EQUAL] = "==",
    [token$GT] = ">",
    [token$GT_EQUAL] = ">=",
    [token$INC] = "++",
    [token$LAND] = "&&",
    [token$LOR] = "||",
    [token$LT] = "<",
    [token$LT_EQUAL] = "<=",
    [token$MOD] = "%",
    [token$MOD_ASSIGN] = "%=",
    [token$MUL] = "*",
    [token$MUL_ASSIGN] = "*=",
    [token$NOT] = "!",
    [token$NOT_EQUAL] = "!=",
    [token$OR] = "|",
    [token$OR_ASSIGN] = "|=",
    [token$SHL] = "<<",
    [token$SHL_ASSIGN] = "<<=",
    [token$SHR] = ">>",
    [token$SHR_ASSIGN] = ">>=",
    [token$SUB] = "-",
    [token$SUB_ASSIGN] = "-=",
    [token$XOR] = "^",
    [token$XOR_ASSIGN] = "^=",
};

extern char *token$string(token$Token tok) {
    return tokens[tok];
}

static map$Map keywords = {};

extern token$Token token$lookup(char *ident) {
    if (keywords.val_size == 0) {
        keywords = map$init(sizeof(token$Token));
        for (int i = token$_keywordBeg + 1; i < token$_keywordEnd; i++) {
            char *s = token$string(i);
            map$set(&keywords, s, &i);
        }
    }
    token$Token tok = token$IDENT;
    map$get(&keywords, ident, &tok);
    return tok;
}

extern int token$precedence(token$Token op) {
    switch (op) {
    case token$MUL:
    case token$DIV:
    case token$MOD:
        return 10;
    case token$ADD:
    case token$SUB:
        return 9;
    case token$SHL:
    case token$SHR:
        return 8;
    case token$GT:
    case token$GT_EQUAL:
    case token$LT:
    case token$LT_EQUAL:
        return 7;
    case token$EQUAL:
    case token$NOT_EQUAL:
        return 6;
    case token$AND:
        return 5;
    case token$XOR:
        return 4;
    case token$OR:
        return 3;
    case token$LAND:
        return 2;
    case token$LOR:
        return 1;
    default:
        return token$lowest_prec;
    }
}

extern char *token$Position_string(token$Position *p) {
    return sys$sprintf("%s:%d:%d", p->filename, p->line, p->column);
}

extern token$File *token$File_new(const char *filename) {
    token$File file = {
        .name = strdup(filename),
        .lines = utils$init(sizeof(int)),
    };
    int zero = 0;
    utils$append(&file.lines, &zero);
    return esc(file);
}

extern void token$File_addLine(token$File *f, int offset) {
    utils$append(&f->lines, &offset);
}

static int getInt(utils$Slice *a, int i) {
    int x;
    utils$get(a, i, &x);
    return x;
}

static int searchInts(utils$Slice *a, int x) {
    int i = 0;
    int j = utils$len(a);
    while (i < j) {
        int h = i + (j - i) / 2;
        if (getInt(a, h) <= x) {
            i = h + 1;
        } else {
            j = h;
        }
    }
    return i - 1;
}

extern token$Position token$File_position(token$File *f, token$Pos p) {
    int offset = p;
    int i = searchInts(&f->lines, offset);
    token$Position epos = {
        .filename = f->name,
        .offset = offset,
        .line = i + 1,
        .column = offset - getInt(&f->lines, i) + 1,
    };
    return epos;
}
