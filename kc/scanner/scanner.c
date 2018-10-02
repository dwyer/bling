#include "kc/scanner/scanner.h"
#include "kc/token/token.h"

char *src;
int rd_offset;
int offset;
int ch;
char lit[BUFSIZ];
int tok;

void next(void)
{
    ch = src[rd_offset];
    rd_offset++;
}

void skip_whitespace(void)
{
    while (ch == ' ' || ch == '\n' || ch == '\t')
        next();
}

void skip_line(void)
{
    while (ch && ch != '\n')
        next();
}

int switch3(int tok0, int tok1, int ch2, int tok2)
{
    if (ch == '=') {
        next();
        return tok1;
    }
    if (ch && ch == ch2) {
        next();
        return tok2;
    }
    return tok0;
}

int switch2(int tok0, int tok1)
{
    return switch3(tok0, tok1, '\0', token_ILLEGAL);
}

int isletter(int ch) {
    return ch == '_' || isalpha(ch);
}

void scan(void)
{
    offset = rd_offset;
    skip_whitespace();
    tok = token_ILLEGAL;
    lit[0] = '\0';
    if (isletter(ch)) {
        int n = 0;
        while (isletter(ch) || isdigit(ch)) {
            lit[n++] = ch;
            next();
        }
        lit[n] = '\0';
        if (!strcmp(lit, "const")) tok = token_CONST;
        else if (!strcmp(lit, "else")) tok = token_ELSE;
        else if (!strcmp(lit, "enum")) tok = token_ENUM;
        else if (!strcmp(lit, "extern")) tok = token_EXTERN;
        else if (!strcmp(lit, "for")) tok = token_FOR;
        else if (!strcmp(lit, "if")) tok = token_IF;
        else if (!strcmp(lit, "static")) tok = token_STATIC;
        else if (!strcmp(lit, "struct")) tok = token_STRUCT;
        else if (!strcmp(lit, "switch")) tok = token_SWITCH;
        else if (!strcmp(lit, "typedef")) tok = token_TYPEDEF;
        else if (!strcmp(lit, "union")) tok = token_UNION;
        else if (!strcmp(lit, "while")) tok = token_WHILE;
        else if (!strcmp(lit, "return")) tok = token_RETURN;
        else tok = token_IDENT;
    } else if (isdigit(ch)) {
        int n = 0;
        while (isdigit(ch)) {
            lit[n++] = ch;
            next();
        }
        lit[n] = '\0';
        tok = token_INT;
    } else if (ch == '\'') {
        int n = 0;
        int escape = 0;
        while (1) {
            lit[n++] = ch;
            if (n > 1 && ch == '\'' && !escape)
                break;
            escape = ch == '\\' && !escape;
            next();
        }
        lit[n] = '\0';
        tok = token_CHAR;
        next();
    } else if (ch == '"') {
        int n = 0;
        int escape = 0;
        for (;;) {
            lit[n++] = ch;
            if (n > 1 && ch == '"' && !escape)
                break;
            escape = ch == '\\' && !escape;
            next();
        }
        lit[n] = '\0';
        tok = token_STRING;
        next();
    } else if (ch == '.') {
        next();
        if (ch == '.') {
            next();
            if (ch == '.') {
                next();
                tok = token_ELLIPSIS;
            }
        } else {
            tok = token_PERIOD;
        }
    } else if (ch == '/') {
        next();
        if (ch == '/') {
            skip_line();
            scan();
        } else {
            tok = switch2(token_DIV, token_DIV_ASSIGN);
        }
    } else {
        int ch0 = ch;
        next();
        switch (ch0) {
            // structure
        case '\0': tok = token_EOF; break;
        case '#': skip_line(); scan(); break;
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
        case '!': tok = switch2(token_NOT, token_NOT_EQUAL); break;
        case '&': tok = switch3(token_AND, token_AND_ASSIGN, '&', token_LAND); break;
        case '*': tok = switch2(token_MUL, token_MUL_ASSIGN); break;
        case '+': tok = switch3(token_ADD, token_ADD_ASSIGN, '+', token_INC); break;
        case '-': tok = switch3(token_SUB, token_SUB_ASSIGN, '-', token_DEC); break;
        case '<': tok = switch2(token_LT, token_LT_EQUAL); break;
        case '=': tok = switch2(token_ASSIGN, token_EQUAL); break;
        case '>': tok = switch2(token_GT, token_GT_EQUAL); break;
        case '|': tok = switch3(token_OR, token_OR_ASSIGN, '|', token_LOR); break;
        }
    }
}

void scan_init(void) {
    src = NULL;
    rd_offset = 0;
    offset = 0;
}
