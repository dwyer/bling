#include "kc/scanner/scanner.h"
#include "kc/token/token.h"

void next(scanner_t *s) {
    s->ch = s->src[s->rd_offset];
    s->rd_offset++;
}

void skip_whitespace(scanner_t *s) {
    while (s->ch == ' ' || s->ch == '\n' || s->ch == '\t') {
        next(s);
    }
}

void skip_line(scanner_t *s) {
    while (s->ch && s->ch != '\n') {
        next(s);
    }
}

int switch3(scanner_t *s, int tok0, int tok1, int ch2, int tok2) {
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

int switch2(scanner_t *s, int tok0, int tok1) {
    return switch3(s, tok0, tok1, '\0', token_ILLEGAL);
}

bool isletter(int ch) {
    return ch == '_' || isalpha(ch);
}

int scan(scanner_t *s, char **lit) {
    int tok;
scan_again:
    tok = token_ILLEGAL;
    s->offset = s->rd_offset;
    skip_whitespace(s);
    s->lit[0] = '\0';
    if (isletter(s->ch)) {
        int n = 0;
        while (isletter(s->ch) || isdigit(s->ch)) {
            s->lit[n++] = s->ch;
            next(s);
        }
        s->lit[n] = '\0';
        if (!strcmp(s->lit, "const")) tok = token_CONST;
        else if (!strcmp(s->lit, "else")) tok = token_ELSE;
        else if (!strcmp(s->lit, "enum")) tok = token_ENUM;
        else if (!strcmp(s->lit, "extern")) tok = token_EXTERN;
        else if (!strcmp(s->lit, "for")) tok = token_FOR;
        else if (!strcmp(s->lit, "if")) tok = token_IF;
        else if (!strcmp(s->lit, "static")) tok = token_STATIC;
        else if (!strcmp(s->lit, "struct")) tok = token_STRUCT;
        else if (!strcmp(s->lit, "switch")) tok = token_SWITCH;
        else if (!strcmp(s->lit, "typedef")) tok = token_TYPEDEF;
        else if (!strcmp(s->lit, "union")) tok = token_UNION;
        else if (!strcmp(s->lit, "while")) tok = token_WHILE;
        else if (!strcmp(s->lit, "return")) tok = token_RETURN;
        else tok = token_IDENT;
    } else if (isdigit(s->ch)) {
        int n = 0;
        while (isdigit(s->ch)) {
            s->lit[n++] = s->ch;
            next(s);
        }
        s->lit[n] = '\0';
        tok = token_INT;
    } else if (s->ch == '\'') {
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
        tok = token_CHAR;
        next(s);
    } else if (s->ch == '"') {
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
        tok = token_STRING;
        next(s);
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

void scan_init(scanner_t *s) {
    s->src = NULL;
    s->rd_offset = 0;
    s->offset = 0;
}
