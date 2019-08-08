#include "bling/scanner/scanner.h"

static void next0(scanner$Scanner *s) {
    s->offset = s->rd_offset;
    if (s->ch == '\n') {
        token$File_addLine(s->file, s->offset);
    }
    s->ch = s->src[s->rd_offset];
    s->rd_offset++;
}

static void skip_whitespace(scanner$Scanner *s) {
    while (s->ch == ' ' || (s->ch == '\n' && !s->insertSemi) || s->ch == '\t') {
        next0(s);
    }
}

static void skip_line(scanner$Scanner *s) {
    while (s->ch >= 0 && s->ch != '\n') {
        next0(s);
    }
}

static bool is_letter(int ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(int ch) {
    return '0' <= ch && ch <= '9';
}

static token$Token switch4(scanner$Scanner *s, token$Token tok0,
        token$Token tok1, int ch2, token$Token tok2, token$Token tok3) {
    if (s->ch == '=') {
        next0(s);
        return tok1;
    }
    if (ch2 && tok2 && s->ch == ch2) {
        next0(s);
        if (tok3 && s->ch == '=') {
            next0(s);
            return tok3;
        }
        return tok2;
    }
    return tok0;
}

static token$Token switch3(scanner$Scanner *s, token$Token tok0,
        token$Token tok1, int ch2, token$Token tok2) {
    return switch4(s, tok0, tok1, ch2, tok2, token$ILLEGAL);
}

static token$Token switch2(scanner$Scanner *s, token$Token tok0,
        token$Token tok1) {
    return switch4(s, tok0, tok1, '\0', token$ILLEGAL, token$ILLEGAL);
}

static char *make_string_slice(scanner$Scanner *s, int start, int end) {
    size_t len = end - start + 1;
    char *lit = (char *)malloc(len);
    strlcpy(lit, &s->src[start], len);
    return lit;
}

static char *scan_ident(scanner$Scanner *s) {
    int offs = s->offset;
    while (is_letter(s->ch) || is_digit(s->ch)
            || (s->dontInsertSemis && s->ch == '$')) {
        next0(s);
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_pragma(scanner$Scanner *s) {
    int offs = s->offset;
    while (s->ch > 0 && s->ch != '\n') {
        next0(s);
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_number(scanner$Scanner *s, token$Token *tokp) {
    int offs = s->offset;
    bool is_float = false;
    while (is_digit(s->ch) || s->ch == '.') {
        if (s->ch == '.') {
            if (!is_float) {
                is_float = true;
            } else {
                // TODO error
            }
        }
        next0(s);
    }
    if (is_float) {
        *tokp = token$FLOAT;
    } else {
        *tokp = token$INT;
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_rune(scanner$Scanner *s) {
    int offs = s->offset;
    int n = 0;
    bool escape = false;
    for (;;) {
        if (n > 0 && s->ch == '\'' && !escape) {
            break;
        }
        escape = s->ch == '\\' && !escape;
        next0(s);
        n++;
    }
    next0(s);
    return make_string_slice(s, offs, s->offset);
}

static char *scan_string(scanner$Scanner *s) {
    int offs = s->offset;
    int n = 0;
    bool escape = false;
    for (;;) {
        if (n > 0 && s->ch == '"' && !escape) {
            break;
        }
        escape = s->ch == '\\' && !escape;
        next0(s);
        n++;
    }
    next0(s);
    return make_string_slice(s, offs, s->offset);
}

static void scanner$error(scanner$Scanner *s, int offs, const char *msg) {
    panic("scanner error: offset %d: %s", offs, msg);
}

static void scan_comment(scanner$Scanner *s) {
    int offs = s->offset - 1;
    switch (s->ch) {
    case '/':
        skip_line(s);
        break;
    case '*':
        next0(s);
        while (s->ch >= 0) {
            int ch = s->ch;
            next0(s);
            if (ch == '*' && s->ch == '/') {
                next0(s);
                return;
            }
        }
        scanner$error(s, offs, "comment not terminated");
        break;
    default:
        panic("not a comment");
        break;
    }
}

extern token$Token scanner$scan(scanner$Scanner *s, token$Pos *pos, char **lit) {
    token$Token tok;
scan_again:
    tok = token$ILLEGAL;
    skip_whitespace(s);
    *pos = s->offset;
    bool insertSemi = false;
    *lit = NULL;
    if (is_letter(s->ch)) {
        *lit = scan_ident(s);
        if (strlen(*lit) > 1) {
            tok = token$lookup(*lit);
            if (tok != token$IDENT) {
                free(*lit);
                *lit = NULL;
            }
            switch (tok) {
            case token$IDENT:
            case token$BREAK:
            case token$CONTINUE:
            case token$RETURN:
                insertSemi = true;
            default:
                break;
            }
        } else {
            insertSemi = true;
            tok = token$IDENT;
        }
    } else if (is_digit(s->ch)) {
        insertSemi = true;
        *lit = scan_number(s, &tok);
    } else if (s->ch == '\'') {
        insertSemi = true;
        *lit = scan_rune(s);
        tok = token$CHAR;
    } else if (s->ch == '"') {
        insertSemi = true;
        *lit = scan_string(s);
        tok = token$STRING;
    } else {
        int ch = s->ch;
        next0(s);
        switch (ch) {
            // structure
        case '\0':
            insertSemi = true;
            tok = token$EOF;
            break;
        case '\n':
            assert(s->insertSemi);
            s->insertSemi = false;
            return token$SEMICOLON;
        case '#':
            tok = token$HASH;
            *lit = scan_pragma(s);
            break;
        case '(':
            tok = token$LPAREN;
            break;
        case ')':
            insertSemi = true;
            tok = token$RPAREN;
            break;
        case ',':
            tok = token$COMMA;
            break;
        case ':':
            tok = token$COLON;
            break;
        case ';':
            tok = token$SEMICOLON;
            break;
        case '?':
            tok = token$QUESTION_MARK;
            break;
        case '[':
            tok = token$LBRACK;
            break;
        case ']':
            insertSemi = true;
            tok = token$RBRACK;
            break;
        case '{':
            tok = token$LBRACE;
            break;
        case '}':
            insertSemi = true;
            tok = token$RBRACE;
            break;

            // operators
        case '~':
            tok = token$BITWISE_NOT;
            break;
        case '!':
            tok = switch2(s, token$NOT, token$NOT_EQUAL);
            break;
        case '$':
            tok = token$DOLLAR;
            break;
        case '%':
            tok = switch2(s, token$MOD, token$MOD_ASSIGN);
            break;
        case '&':
            tok = switch3(s, token$AND, token$AND_ASSIGN, '&', token$LAND);
            break;
        case '*':
            tok = switch2(s, token$MUL, token$MUL_ASSIGN);
            break;
        case '+':
            tok = switch3(s, token$ADD, token$ADD_ASSIGN, '+', token$INC);
            if (tok == token$INC) {
                insertSemi = true;
            }
            break;
        case '-':
            if (s->ch == '>') {
                next0(s);
                tok = token$ARROW;
            } else {
                tok = switch3(s, token$SUB, token$SUB_ASSIGN, '-', token$DEC);
                if (tok == token$DEC) {
                    insertSemi = true;
                }
            }
            break;
        case '.':
            if (s->ch == '.') {
                next0(s);
                if (s->ch == '.') {
                    next0(s);
                    tok = token$ELLIPSIS;
                }
            } else {
                tok = token$PERIOD;
            }
            break;
        case '/':
            if (s->ch == '/' || s->ch == '*') {
                scan_comment(s);
                goto scan_again;
            } else {
                tok = switch2(s, token$DIV, token$DIV_ASSIGN);
            }
            break;
        case '<':
            tok = switch4(s, token$LT, token$LT_EQUAL, '<', token$SHL,
                    token$SHL_ASSIGN);
            break;
        case '=':
            tok = switch2(s, token$ASSIGN, token$EQUAL);
            break;
        case '>':
            tok = switch4(s, token$GT, token$GT_EQUAL, '>', token$SHR,
                    token$SHR_ASSIGN);
            break;
        case '|':
            tok = switch3(s, token$OR, token$OR_ASSIGN, '|', token$LOR);
            break;
        }
    }
    if (!s->dontInsertSemis) {
        s->insertSemi = insertSemi;
    }
    return tok;
}

extern void scanner$init(scanner$Scanner *s, token$File *file, char *src) {
    s->file = file;
    s->src = src;
    s->rd_offset = 0;
    s->offset = 0;
    s->insertSemi = false;
    s->dontInsertSemis = true;
    next0(s);
    s->dontInsertSemis = false;
}
