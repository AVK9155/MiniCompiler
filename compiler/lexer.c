#include <ctype.h>
#include <stdarg.h>
#include "compiler.h"

/* ── error helper ── */
void report_error(int line, const char *fmt, ...) {
    if (error_count >= MAX_ERRORS) return;
    va_list ap;
    va_start(ap, fmt);
    char buf[240];
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    snprintf(error_msgs[error_count++], 256, "Line %d: %s", line, buf);
}

void print_errors(void) {
    for (int i = 0; i < error_count; i++)
        fprintf(stderr, "ERROR: %s\n", error_msgs[i]);
}

/* ── keyword table ── */
static struct { const char *word; TokenType type; } keywords[] = {
    {"int",    TOK_INT},
    {"float",  TOK_FLOAT},
    {"char",   TOK_CHAR},
    {"if",     TOK_IF},
    {"else",   TOK_ELSE},
    {"while",  TOK_WHILE},
    {"for",    TOK_FOR},
    {"return", TOK_RETURN},
    {NULL, TOK_UNKNOWN}
};

static TokenType keyword_lookup(const char *s) {
    for (int i = 0; keywords[i].word; i++)
        if (strcmp(s, keywords[i].word) == 0)
            return keywords[i].type;
    return TOK_IDENT;
}

const char *token_type_str(TokenType t) {
    switch(t) {
        case TOK_INT_LIT:   return "INT_LIT";
        case TOK_FLOAT_LIT: return "FLOAT_LIT";
        case TOK_CHAR_LIT:  return "CHAR_LIT";
        case TOK_IDENT:     return "IDENT";
        case TOK_INT:       return "KEYWORD";
        case TOK_FLOAT:     return "KEYWORD";
        case TOK_CHAR:      return "KEYWORD";
        case TOK_IF:        return "KEYWORD";
        case TOK_ELSE:      return "KEYWORD";
        case TOK_WHILE:     return "KEYWORD";
        case TOK_FOR:       return "KEYWORD";
        case TOK_RETURN:    return "KEYWORD";
        case TOK_PLUS:      return "PLUS";
        case TOK_MINUS:     return "MINUS";
        case TOK_STAR:      return "STAR";
        case TOK_SLASH:     return "SLASH";
        case TOK_PERCENT:   return "PERCENT";
        case TOK_EQ:        return "EQ";
        case TOK_NEQ:       return "NEQ";
        case TOK_LT:        return "LT";
        case TOK_GT:        return "GT";
        case TOK_LEQ:       return "LEQ";
        case TOK_GEQ:       return "GEQ";
        case TOK_AND:       return "AND";
        case TOK_OR:        return "OR";
        case TOK_NOT:       return "NOT";
        case TOK_ASSIGN:    return "ASSIGN";
        case TOK_LPAREN:    return "LPAREN";
        case TOK_RPAREN:    return "RPAREN";
        case TOK_LBRACE:    return "LBRACE";
        case TOK_RBRACE:    return "RBRACE";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_COMMA:     return "COMMA";
        case TOK_EOF:       return "EOF";
        default:            return "UNKNOWN";
    }
}

static void add_token(TokenType type, const char *val, int line) {
    if (token_count >= MAX_TOKENS) return;
    tokens[token_count].type = type;
    strncpy(tokens[token_count].value, val, MAX_IDENT_LEN - 1);
    tokens[token_count].line = line;
    token_count++;
}

void lexical_analysis(const char *src) {
    token_count = 0;
    int i = 0, line = 1;
    int len = (int)strlen(src);

    while (i < len) {
        /* skip whitespace */
        if (src[i] == '\n') { line++; i++; continue; }
        if (isspace(src[i])) { i++; continue; }

        /* single-line comment */
        if (src[i] == '/' && src[i+1] == '/') {
            while (i < len && src[i] != '\n') i++;
            continue;
        }

        /* multi-line comment */
        if (src[i] == '/' && src[i+1] == '*') {
            i += 2;
            while (i < len - 1 && !(src[i] == '*' && src[i+1] == '/')) {
                if (src[i] == '\n') line++;
                i++;
            }
            i += 2;
            continue;
        }

        /* number literal */
        if (isdigit(src[i])) {
            int start = i;
            int is_float = 0;
            while (i < len && isdigit(src[i])) i++;
            if (i < len && src[i] == '.') { is_float = 1; i++; while (i < len && isdigit(src[i])) i++; }
            char buf[MAX_IDENT_LEN] = {0};
            strncpy(buf, src + start, i - start);
            add_token(is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, buf, line);
            continue;
        }

        /* char literal */
        if (src[i] == '\'') {
            i++;
            char buf[4] = {0};
            if (i < len && src[i] != '\'') { buf[0] = src[i]; i++; }
            if (i < len && src[i] == '\'') i++;
            add_token(TOK_CHAR_LIT, buf, line);
            continue;
        }

        /* identifier / keyword */
        if (isalpha(src[i]) || src[i] == '_') {
            int start = i;
            while (i < len && (isalnum(src[i]) || src[i] == '_')) i++;
            char buf[MAX_IDENT_LEN] = {0};
            strncpy(buf, src + start, i - start);
            add_token(keyword_lookup(buf), buf, line);
            continue;
        }

        /* two-char operators */
        if (src[i] == '=' && src[i+1] == '=') { add_token(TOK_EQ,  "==", line); i+=2; continue; }
        if (src[i] == '!' && src[i+1] == '=') { add_token(TOK_NEQ, "!=", line); i+=2; continue; }
        if (src[i] == '<' && src[i+1] == '=') { add_token(TOK_LEQ, "<=", line); i+=2; continue; }
        if (src[i] == '>' && src[i+1] == '=') { add_token(TOK_GEQ, ">=", line); i+=2; continue; }
        if (src[i] == '&' && src[i+1] == '&') { add_token(TOK_AND, "&&", line); i+=2; continue; }
        if (src[i] == '|' && src[i+1] == '|') { add_token(TOK_OR,  "||", line); i+=2; continue; }

        /* single-char tokens */
        char ch[2] = {src[i], 0};
        switch (src[i]) {
            case '+': add_token(TOK_PLUS,      ch, line); break;
            case '-': add_token(TOK_MINUS,     ch, line); break;
            case '*': add_token(TOK_STAR,      ch, line); break;
            case '/': add_token(TOK_SLASH,     ch, line); break;
            case '%': add_token(TOK_PERCENT,   ch, line); break;
            case '<': add_token(TOK_LT,        ch, line); break;
            case '>': add_token(TOK_GT,        ch, line); break;
            case '!': add_token(TOK_NOT,       ch, line); break;
            case '=': add_token(TOK_ASSIGN,    ch, line); break;
            case '(': add_token(TOK_LPAREN,    ch, line); break;
            case ')': add_token(TOK_RPAREN,    ch, line); break;
            case '{': add_token(TOK_LBRACE,    ch, line); break;
            case '}': add_token(TOK_RBRACE,    ch, line); break;
            case ';': add_token(TOK_SEMICOLON, ch, line); break;
            case ',': add_token(TOK_COMMA,     ch, line); break;
            default:
                report_error(line, "Unknown character '%c'", src[i]);
                add_token(TOK_UNKNOWN, ch, line);
        }
        i++;
    }
    add_token(TOK_EOF, "", line);
}
