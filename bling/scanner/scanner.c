#include "bling/scanner/scanner.h"

static void next0(scanner_t *s) {
    s->offset = s->rd_offset;
    if (s->ch == '\n') {
        token_File_addLine(s->file, s->offset);
    }
    s->ch = s->src[s->rd_offset];
    s->rd_offset++;
}

static void skip_whitespace(scanner_t *s) {
    while (s->ch == ' ' || (s->ch == '\n' && !s->insertSemi) || s->ch == '\t') {
        next0(s);
    }
}

static void skip_line(scanner_t *s) {
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

static token_t switch4(scanner_t *s, token_t tok0, token_t tok1, int ch2,
        token_t tok2, token_t tok3) {
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

static token_t switch3(scanner_t *s, token_t tok0, token_t tok1, int ch2,
        token_t tok2) {
    return switch4(s, tok0, tok1, ch2, tok2, token_ILLEGAL);
}

static token_t switch2(scanner_t *s, token_t tok0, token_t tok1) {
    return switch4(s, tok0, tok1, '\0', token_ILLEGAL, token_ILLEGAL);
}

static char *make_string_slice(scanner_t *s, int start, int end) {
    size_t len = end - start + 1;
    char *lit = (char *)malloc(len);
    strlcpy(lit, &s->src[start], len);
    return lit;
}

static char *scan_ident(scanner_t *s) {
    int offs = s->offset;
    while (is_letter(s->ch) || is_digit(s->ch)) {
        next0(s);
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_pragma(scanner_t *s) {
    int offs = s->offset;
    while (s->ch > 0 && s->ch != '\n') {
        next0(s);
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_number(scanner_t *s, token_t *tokp) {
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
        *tokp = token_FLOAT;
    } else {
        *tokp = token_INT;
    }
    return make_string_slice(s, offs, s->offset);
}

static char *scan_rune(scanner_t *s) {
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

static char *scan_string(scanner_t *s) {
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

static void scanner_error(scanner_t *s, int offs, const char *msg) {
    panic("scanner error: offset %d: %s", offs, msg);
}

static void scan_comment(scanner_t *s) {
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
        scanner_error(s, offs, "comment not terminated");
        break;
    default:
        panic("not a comment");
        break;
    }
}

extern token_t scanner_scan(scanner_t *s, pos_t *pos, char **lit) {
    token_t tok;
scan_again:
    tok = token_ILLEGAL;
    skip_whitespace(s);
    *pos = s->offset;
    bool insertSemi = false;
    *lit = NULL;
    if (is_letter(s->ch)) {
        *lit = scan_ident(s);
        if (strlen(*lit) > 1) {
            tok = token_lookup(*lit);
            if (tok != token_IDENT) {
                free(*lit);
                *lit = NULL;
            }
            switch (tok) {
            case token_IDENT:
            case token_BREAK:
            case token_CONTINUE:
            case token_RETURN:
                insertSemi = true;
            default:
                break;
            }
        } else {
            insertSemi = true;
            tok = token_IDENT;
        }
    } else if (is_digit(s->ch)) {
        insertSemi = true;
        *lit = scan_number(s, &tok);
    } else if (s->ch == '\'') {
        insertSemi = true;
        *lit = scan_rune(s);
        tok = token_CHAR;
    } else if (s->ch == '"') {
        insertSemi = true;
        *lit = scan_string(s);
        tok = token_STRING;
    } else {
        int ch = s->ch;
        next0(s);
        switch (ch) {
            // structure
        case '\0':
            insertSemi = true;
            tok = token_EOF;
            break;
        case '\n':
            assert(s->insertSemi);
            s->insertSemi = false;
            return token_SEMICOLON;
        case '#':
            tok = token_HASH;
            *lit = scan_pragma(s);
            break;
        case '(':
            tok = token_LPAREN;
            break;
        case ')':
            insertSemi = true;
            tok = token_RPAREN;
            break;
        case ',':
            tok = token_COMMA;
            break;
        case ':':
            tok = token_COLON;
            break;
        case ';':
            tok = token_SEMICOLON;
            break;
        case '?':
            tok = token_QUESTION_MARK;
            break;
        case '[':
            tok = token_LBRACK;
            break;
        case ']':
            insertSemi = true;
            tok = token_RBRACK;
            break;
        case '{':
            tok = token_LBRACE;
            break;
        case '}':
            insertSemi = true;
            tok = token_RBRACE;
            break;

            // operators
        case '~':
            tok = token_BITWISE_NOT;
            break;
        case '!':
            tok = switch2(s, token_NOT, token_NOT_EQUAL);
            break;
        case '$':
            tok = token_DOLLAR;
            break;
        case '%':
            tok = switch2(s, token_MOD, token_MOD_ASSIGN);
            break;
        case '&':
            tok = switch3(s, token_AND, token_AND_ASSIGN, '&', token_LAND);
            break;
        case '*':
            tok = switch2(s, token_MUL, token_MUL_ASSIGN);
            break;
        case '+':
            tok = switch3(s, token_ADD, token_ADD_ASSIGN, '+', token_INC);
            if (tok == token_INC) {
                insertSemi = true;
            }
            break;
        case '-':
            if (s->ch == '>') {
                next0(s);
                tok = token_ARROW;
            } else {
                tok = switch3(s, token_SUB, token_SUB_ASSIGN, '-', token_DEC);
                if (tok == token_DEC) {
                    insertSemi = true;
                }
            }
            break;
        case '.':
            if (s->ch == '.') {
                next0(s);
                if (s->ch == '.') {
                    next0(s);
                    tok = token_ELLIPSIS;
                }
            } else {
                tok = token_PERIOD;
            }
            break;
        case '/':
            if (s->ch == '/' || s->ch == '*') {
                scan_comment(s);
                goto scan_again;
            } else {
                tok = switch2(s, token_DIV, token_DIV_ASSIGN);
            }
            break;
        case '<':
            tok = switch4(s, token_LT, token_LT_EQUAL, '<', token_SHL,
                    token_SHL_ASSIGN);
            break;
        case '=':
            tok = switch2(s, token_ASSIGN, token_EQUAL);
            break;
        case '>':
            tok = switch4(s, token_GT, token_GT_EQUAL, '>', token_SHR,
                    token_SHR_ASSIGN);
            break;
        case '|':
            tok = switch3(s, token_OR, token_OR_ASSIGN, '|', token_LOR);
            break;
        }
    }
    if (!s->dontInsertSemis) {
        s->insertSemi = insertSemi;
    }
    return tok;
}

extern void scanner_init(scanner_t *s, token_File *file, char *src) {
    s->file = file;
    s->src = src;
    s->rd_offset = 0;
    s->offset = 0;
    s->insertSemi = false;
    s->dontInsertSemis = true;
    next0(s);
    s->dontInsertSemis = false;
}
