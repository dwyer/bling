enum {
    token_ILLEGAL = 0,

    token_IDENT,
    token_TYPE_NAME,

    token_CHAR,
    token_INT,
    token_STRING,

    token_CONST,
    token_ELSE,
    token_ENUM,
    token_EXTERN,
    token_FOR,
    token_IF,
    token_RETURN,
    token_STATIC,
    token_STRUCT,
    token_SWITCH,
    token_TYPEDEF,
    token_UNION,
    token_WHILE,

    token_ARROW,
    token_COLON,
    token_COMMA,
    token_EOF,
    token_LBRACE,
    token_LBRACK,
    token_LPAREN,
    token_PERIOD,
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
    token_MUL,
    token_MUL_ASSIGN,
    token_NOT,
    token_NOT_EQUAL,
    token_OR,
    token_OR_ASSIGN,
    token_SUB,
    token_SUB_ASSIGN,
};

char *token_string(int tok)
{
    switch (tok) {

    case token_ARROW: return "->";
    case token_COLON: return ":";
    case token_COMMA: return ",";
    case token_EOF: return "EOF";
    case token_LBRACE: return "{";
    case token_LBRACK: return "[";
    case token_LPAREN: return "(";
    case token_PERIOD: return ".";
    case token_RBRACE: return "}";
    case token_RBRACK: return "]";
    case token_RPAREN: return ")";
    case token_SEMICOLON: return ";";

    case token_ADD: return "+";
    case token_ADD_ASSIGN: return "+=";
    case token_AND: return "&";
    case token_AND_ASSIGN: return "&=";
    case token_ASSIGN: return "=";
    case token_DEC: return "--";
    case token_DIV: return "/";
    case token_DIV_ASSIGN: return "/=";
    case token_EQUAL: return "==";
    case token_GT: return ">";
    case token_GT_EQUAL: return ">=";
    case token_INC: return "++";
    case token_LAND: return "&&";
    case token_LOR: return "||";
    case token_LT: return "<";
    case token_LT_EQUAL: return "<=";
    case token_MUL: return "*";
    case token_MUL_ASSIGN: return "*=";
    case token_NOT: return "!";
    case token_NOT_EQUAL: return "!=";
    case token_OR: return "|";
    case token_OR_ASSIGN: return "|=";
    case token_SUB: return "-";
    case token_SUB_ASSIGN: return "-=";

    case token_STRUCT: return "struct";
    case token_UNION: return "union";
    case token_TYPEDEF: return "typedef";
    case token_TYPE_NAME: return "$TYPE";
    case token_IDENT: return "$IDENT";

    default: return "???";
    }
}
