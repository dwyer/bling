#include "bling/token/token.h"

#include "bytes/bytes.h"
#include "sys/sys.h"
#include "utils/utils.h"

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
    [token$ESC] = "esc",
    [token$EXTERN] = "extern",
    [token$FALLTHROUGH] = "fallthrough",
    [token$FOR] = "for",
    [token$FUNC] = "fun",
    [token$GOTO] = "goto",
    [token$IF] = "if",
    [token$IMPORT] = "import",
    [token$MAP] = "map",
    [token$PACKAGE] = "package",
    [token$RETURN] = "return",
    [token$SIGNED] = "signed",
    [token$SIZEOF] = "sizeof",
    [token$STATIC] = "static",
    [token$STRUCT] = "struct",
    [token$SWITCH] = "switch",
    [token$TYPEDEF] = "typedef",
    [token$TYPE] = "typ",
    [token$UNION] = "union",
    [token$UNSIGNED] = "unsigned",
    [token$VAR] = "var",
    [token$WHILE] = "while",
    [token$_AUTO] = "auto",
    [token$_COMPLEX] = "_Complex",
    [token$_DO] = "do",
    [token$_IMAGINARY] = "_Imaginary",
    [token$_INLINE] = "inline",
    [token$_LONG] = "long",
    [token$_REGISTER] = "register",
    [token$_RESTRICT] = "restrict",
    [token$_SHORT] = "short",
    [token$_TYPEOF] = "typeof",
    [token$_VOLATILE] = "volatile",

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

static utils$Map keywords = {};

extern token$Token token$lookup(char *ident) {
    if (utils$Map_len(&keywords) == 0) {
        keywords = utils$Map_make(sizeof(token$Token));
        for (int i = token$_keywordBeg + 1; i < token$_keywordEnd; i++) {
            char *s = token$string(i);
            utils$Map_set(&keywords, s, &i);
        }
    }
    token$Token tok = token$IDENT;
    utils$Map_get(&keywords, ident, &tok);
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

static int getInt(utils$Slice *a, int i) {
    int x;
    utils$Slice_get(a, i, &x);
    return x;
}

extern void token$File_addLine(token$File *f, int offset) {
    int i = utils$Slice_len(&f->lines);
    if ((i == 0 || getInt(&f->lines, i-1) < offset) && offset < f->size) {
        utils$Slice_append(&f->lines, &offset);
    }
}

extern token$Pos token$File_pos(token$File *f, int offset) {
    return f->base + offset;
}

static int searchInts(utils$Slice *a, int x) {
    int i = 0;
    int j = utils$Slice_len(a);
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

extern token$Position token$_File_position(token$File *f, token$Pos p,
        bool adjusted) {
    int offset = p - f->base;
    int i = searchInts(&f->lines, offset);
    token$Position pos = {
        .filename = f->name,
        .offset = offset,
        .line = i + 1,
        .column = offset - getInt(&f->lines, i) + 1,
    };
    return pos;
}

static const token$Pos token$noPos = 0;

extern token$Position token$File_positionFor(token$File *f, token$Pos p,
        bool adjusted) {
    token$Position pos = {};
    if (p != token$noPos) {
        if (p < f->base || p > f->base + f->size) {
            panic("illegal Pos value");
        }
        pos = token$_File_position(f, p, adjusted);
    }
    return pos;
}

extern token$Position token$File_position(token$File *f, token$Pos p) {
    return token$File_positionFor(f, p, true);
}

extern char *token$File_lineString(token$File *f, int line) {
    if (line < 1) {
        return NULL;
    }
    bytes$Buffer buf = {};
    int i = 0;
    utils$Slice_get(&f->lines, line-1, &i);
    int ch = f->src[i];
    while (ch > 0 && ch != '\n') {
        bytes$Buffer_writeByte(&buf, ch, NULL);
        i++;
        ch = f->src[i];
    }
    return bytes$Buffer_string(&buf);
}

extern token$FileSet *token$newFileSet() {
    token$FileSet fset = {
        .base = 1,
        .files = utils$Slice_make(sizeof(token$File *)),
    };
    return esc(fset);
}

extern token$File *token$FileSet_addFile(token$FileSet *s,
        const char *filename, int base, int size) {
    if (base < 0) {
        base = s->base;
    }
    if (base < s->base || size < 0) {
        panic("illegal base or size");
    }
    utils$Slice lines = utils$Slice_make(sizeof(int));
    int zero = 0;
    utils$Slice_append(&lines, &zero);
    token$File file = {
        .name = strdup(filename),
        .base = base,
        .size = size,
        .lines = lines,
    };
    token$File *f = esc(file);
    base += size + 1;
    s->base = base;
    utils$Slice_append(&s->files, &f);
    s->last = f;
    return f;
}

static int searchFiles(utils$Slice *files, int x) {
    for (int i = 0; i < utils$Slice_len(files); i++) {
        token$File *f;
        utils$Slice_get(files, i, &f);
        if (f->base <= x && x <= f->base + f->size) {
            return i;
        }
    }
    return -1;
}

extern token$File *token$FileSet_file(token$FileSet *s, token$Pos p) {
    token$File *f = s->last;
    if (f != NULL && f->base <= p && p <= f->base + f->size) {
        return f;
    }
    int i = searchFiles(&s->files, p);
    if (i >= 0) {
        utils$Slice_get(&s->files, i, &f);
        if (p <= f->base + f->size) {
            s->last = f;
            return f;
        }
    }
    return NULL;
}

extern void token$FileSet_print(token$FileSet *fset) {
    for (int i = 0; i < utils$Slice_len(&fset->files); i++) {
        token$File *f;
        utils$Slice_get(&fset->files, i, &f);
        char *s = sys$sprintf("%d: %s (base=%d, size=%d)", i, f->name, f->base, f->size);
        print(s);
        free(s);
    }
}
