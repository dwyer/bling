#include "kc/scanner/scanner.h"
#include "kc/token/token.h"

static void next(scanner_t *s) {
    s->ch = s->src[s->rd_offset];
    s->rd_offset++;
}

static void skip_whitespace(scanner_t *s) {
    while (s->ch == ' ' || s->ch == '\n' || s->ch == '\t') {
        next(s);
    }
}

static void skip_line(scanner_t *s) {
    while (s->ch && s->ch != '\n') {
        next(s);
    }
}

static bool is_letter(int ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

static bool is_digit(int ch) {
    return '0' <= ch && ch <= '9';
}

static int switch3(scanner_t *s, int tok0, int tok1, int ch2, int tok2) {
    if (s->ch == '=') {
        next(s);
        return tok1;
    }
    if (s->ch && s->ch == ch2) {
        next(s);
        return tok2;
    }
    return tok0;
}

static int switch2(scanner_t *s, int tok0, int tok1) {
    return switch3(s, tok0, tok1, '\0', token_ILLEGAL);
}

static char *scan_ident(scanner_t *s) {
    int n = 0;
    while (is_letter(s->ch) || is_digit(s->ch)) {
        s->lit[n++] = s->ch;
        next(s);
    }
    s->lit[n] = '\0';
    return s->lit;
}

static int scan_number(scanner_t *s, char **litp) {
    int n = 0;
    while (is_digit(s->ch)) {
        (*litp)[n++] = s->ch;
        next(s);
    }
    (*litp)[n] = '\0';
    return token_INT;
}

static void scan_rune(scanner_t *s) {
    int n = 0;
    int escape = 0;
    while (1) {
        s->lit[n++] = s->ch;
        if (n > 1 && s->ch == '\'' && !escape)
            break;
        escape = s->ch == '\\' && !escape;
        next(s);
    }
    s->lit[n] = '\0';
    next(s);
}

static void scan_string(scanner_t *s) {
    int n = 0;
    int escape = 0;
    for (;;) {
        s->lit[n++] = s->ch;
        if (n > 1 && s->ch == '"' && !escape)
            break;
        escape = s->ch == '\\' && !escape;
        next(s);
    }
    s->lit[n] = '\0';
    next(s);
}

extern int scanner_scan(scanner_t *s, char **lit) {
    int tok;
scan_again:
    tok = token_ILLEGAL;
    s->offset = s->rd_offset;
    skip_whitespace(s);
    s->lit[0] = '\0';
    if (is_letter(s->ch)) {
        *lit = scan_ident(s);
        tok = token_lookup(*lit);
    } else if (is_digit(s->ch)) {
        tok = scan_number(s, lit);
    } else if (s->ch == '\'') {
        scan_rune(s);
        tok = token_CHAR;
    } else if (s->ch == '"') {
        scan_string(s);
        tok = token_STRING;
    } else if (s->ch == '.') {
        next(s);
        if (s->ch == '.') {
            next(s);
            if (s->ch == '.') {
                next(s);
                tok = token_ELLIPSIS;
            }
        } else {
            tok = token_PERIOD;
        }
    } else if (s->ch == '/') {
        next(s);
        if (s->ch == '/') {
            skip_line(s);
            goto scan_again;
        } else {
            tok = switch2(s, token_DIV, token_DIV_ASSIGN);
        }
    } else {
        int ch0 = s->ch;
        next(s);
        switch (ch0) {
            // structure
        case '\0': tok = token_EOF; break;
        case '#': skip_line(s); goto scan_again; break;
        case '(': tok = token_LPAREN; break;
        case ')': tok = token_RPAREN; break;
        case ',': tok = token_COMMA; break;
        case ':': tok = token_COLON; break;
        case ';': tok = token_SEMICOLON; break;
        case '[': tok = token_LBRACK; break;
        case ']': tok = token_RBRACK; break;
        case '{': tok = token_LBRACE; break;
        case '}': tok = token_RBRACE; break;
                   // operators
        case '!': tok = switch2(s, token_NOT, token_NOT_EQUAL); break;
        case '&': tok = switch3(s, token_AND, token_AND_ASSIGN, '&', token_LAND); break;
        case '*': tok = switch2(s, token_MUL, token_MUL_ASSIGN); break;
        case '+': tok = switch3(s, token_ADD, token_ADD_ASSIGN, '+', token_INC); break;
        case '-': tok = switch3(s, token_SUB, token_SUB_ASSIGN, '-', token_DEC); break;
        case '<': tok = switch2(s, token_LT, token_LT_EQUAL); break;
        case '=': tok = switch2(s, token_ASSIGN, token_EQUAL); break;
        case '>': tok = switch2(s, token_GT, token_GT_EQUAL); break;
        case '|': tok = switch3(s, token_OR, token_OR_ASSIGN, '|', token_LOR); break;
        }
    }
    *lit = s->lit;
    return tok;
}

extern void scanner_init(scanner_t *s, char *src) {
    s->src = src;
    s->rd_offset = 0;
    s->offset = 0;
    next(s);
}
