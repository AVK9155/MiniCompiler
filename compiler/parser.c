#include "compiler.h"

/* ── parser state ── */
static int pos = 0;

static Token *peek(void)       { return &tokens[pos]; }
static Token *advance(void)    { return &tokens[pos++]; }
static int    at_end(void)     { return tokens[pos].type == TOK_EOF; }

static Token *expect(TokenType t, const char *ctx) {
    if (tokens[pos].type == t) return advance();
    report_error(tokens[pos].line,
        "Expected '%s' in %s but got '%s'",
        token_type_str(t), ctx, tokens[pos].value);
    return NULL;
}

/* ── AST helpers ── */
static ASTNode *make_node(NodeType type, int line) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->type  = type;
    n->dtype = DTYPE_UNKNOWN;
    n->line  = line;
    return n;
}

static void node_add_stmt(ASTNode *block, ASTNode *stmt) {
    block->stmts = realloc(block->stmts, sizeof(ASTNode*) * (block->nstmts + 1));
    block->stmts[block->nstmts++] = stmt;
}

/* ── forward declarations ── */
static ASTNode *parse_stmt(void);
static ASTNode *parse_expr(void);
static ASTNode *parse_block(void);

/* ── ERROR RECOVERY: skip to next ';' or '}' ── */
static void sync(void) {
    while (!at_end()) {
        TokenType t = peek()->type;
        if (t == TOK_SEMICOLON) { advance(); return; }
        if (t == TOK_RBRACE)    return;
        advance();
    }
}

/* ─────────────────────────────────────────
   EXPRESSION PARSING  (precedence climbing)
───────────────────────────────────────── */
static ASTNode *parse_primary(void) {
    Token *t = peek();
    int line  = t->line;

    if (t->type == TOK_INT_LIT) {
        advance();
        ASTNode *n = make_node(NODE_INT_LIT, line);
        strncpy(n->name, t->value, MAX_IDENT_LEN-1);
        return n;
    }
    if (t->type == TOK_FLOAT_LIT) {
        advance();
        ASTNode *n = make_node(NODE_FLOAT_LIT, line);
        strncpy(n->name, t->value, MAX_IDENT_LEN-1);
        return n;
    }
    if (t->type == TOK_CHAR_LIT) {
        advance();
        ASTNode *n = make_node(NODE_CHAR_LIT, line);
        strncpy(n->name, t->value, MAX_IDENT_LEN-1);
        return n;
    }
    if (t->type == TOK_IDENT) {
        advance();
        ASTNode *n = make_node(NODE_IDENT, line);
        strncpy(n->name, t->value, MAX_IDENT_LEN-1);
        return n;
    }
    if (t->type == TOK_LPAREN) {
        advance();
        ASTNode *inner = parse_expr();
        expect(TOK_RPAREN, "parenthesised expression");
        return inner;
    }
    if (t->type == TOK_MINUS || t->type == TOK_NOT) {
        advance();
        ASTNode *n = make_node(NODE_UNOP, line);
        strncpy(n->op, t->value, 7);
        n->left = parse_primary();
        return n;
    }
    report_error(line, "Unexpected token '%s' in expression", t->value);
    advance();
    return make_node(NODE_INT_LIT, line); /* dummy */
}

static int binop_prec(TokenType t) {
    switch(t) {
        case TOK_OR:      return 1;
        case TOK_AND:     return 2;
        case TOK_EQ: case TOK_NEQ: return 3;
        case TOK_LT: case TOK_GT:
        case TOK_LEQ: case TOK_GEQ: return 4;
        case TOK_PLUS: case TOK_MINUS: return 5;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 6;
        default: return 0;
    }
}

static ASTNode *parse_binop(int min_prec) {
    ASTNode *left = parse_primary();
    while (1) {
        int prec = binop_prec(peek()->type);
        if (prec <= min_prec) break;
        Token *op = advance();
        ASTNode *right = parse_binop(prec);
        ASTNode *n = make_node(NODE_BINOP, op->line);
        strncpy(n->op, op->value, 7);
        n->left  = left;
        n->right = right;
        left = n;
    }
    return left;
}

static ASTNode *parse_expr(void) {
    return parse_binop(0);
}

/* ─────────────────────────────────────────
   STATEMENT PARSING
───────────────────────────────────────── */
static DataType tok_to_dtype(TokenType t) {
    if (t == TOK_INT)   return DTYPE_INT;
    if (t == TOK_FLOAT) return DTYPE_FLOAT;
    if (t == TOK_CHAR)  return DTYPE_CHAR;
    return DTYPE_UNKNOWN;
}

static ASTNode *parse_var_decl(DataType dtype) {
    int line = peek()->line;
    Token *name_tok = expect(TOK_IDENT, "variable declaration");
    if (!name_tok) { sync(); return NULL; }

    ASTNode *n = make_node(NODE_VAR_DECL, line);
    strncpy(n->name, name_tok->value, MAX_IDENT_LEN-1);
    n->dtype = dtype;

    if (peek()->type == TOK_ASSIGN) {
        advance();
        n->right = parse_expr();
    }
    expect(TOK_SEMICOLON, "variable declaration");
    return n;
}

static ASTNode *parse_stmt(void) {
    Token *t = peek();
    int line  = t->line;

    /* variable declaration */
    if (t->type == TOK_INT || t->type == TOK_FLOAT || t->type == TOK_CHAR) {
        DataType dt = tok_to_dtype(t->type);
        advance();
        return parse_var_decl(dt);
    }

    /* if statement */
    if (t->type == TOK_IF) {
        advance();
        ASTNode *n = make_node(NODE_IF, line);
        expect(TOK_LPAREN, "if condition");
        n->cond = parse_expr();
        expect(TOK_RPAREN, "if condition");
        n->body = parse_block();
        if (peek()->type == TOK_ELSE) {
            advance();
            n->els = (peek()->type == TOK_IF) ? parse_stmt() : parse_block();
        }
        return n;
    }

    /* while statement */
    if (t->type == TOK_WHILE) {
        advance();
        ASTNode *n = make_node(NODE_WHILE, line);
        expect(TOK_LPAREN, "while condition");
        n->cond = parse_expr();
        expect(TOK_RPAREN, "while condition");
        n->body = parse_block();
        return n;
    }

    /* return statement */
    if (t->type == TOK_RETURN) {
        advance();
        ASTNode *n = make_node(NODE_RETURN, line);
        if (peek()->type != TOK_SEMICOLON)
            n->right = parse_expr();
        expect(TOK_SEMICOLON, "return statement");
        return n;
    }

    /* block */
    if (t->type == TOK_LBRACE) return parse_block();

    /* assignment or expression statement */
    if (t->type == TOK_IDENT && pos + 1 < token_count &&
        tokens[pos+1].type == TOK_ASSIGN) {
        ASTNode *n = make_node(NODE_ASSIGN, line);
        strncpy(n->name, t->value, MAX_IDENT_LEN-1);
        advance(); /* ident */
        advance(); /* = */
        n->right = parse_expr();
        expect(TOK_SEMICOLON, "assignment");
        return n;
    }

    /* bare expression statement */
    ASTNode *expr = parse_expr();
    expect(TOK_SEMICOLON, "expression statement");
    return expr;
}

static ASTNode *parse_block(void) {
    int line = peek()->line;
    expect(TOK_LBRACE, "block");
    ASTNode *block = make_node(NODE_BLOCK, line);
    while (!at_end() && peek()->type != TOK_RBRACE) {
        ASTNode *s = parse_stmt();
        if (s) node_add_stmt(block, s);
    }
    expect(TOK_RBRACE, "block");
    return block;
}

/* ─────────────────────────────────────────
   ENTRY POINT
───────────────────────────────────────── */
ASTNode *parse(void) {
    pos = 0;
    ASTNode *root = make_node(NODE_PROGRAM, 1);
    while (!at_end()) {
        ASTNode *s = parse_stmt();
        if (s) node_add_stmt(root, s);
    }
    ast_root = root;
    return root;
}

void free_ast(ASTNode *n) {
    if (!n) return;
    free_ast(n->left);
    free_ast(n->right);
    free_ast(n->cond);
    free_ast(n->body);
    free_ast(n->els);
    for (int i = 0; i < n->nstmts; i++) free_ast(n->stmts[i]);
    free(n->stmts);
    free(n);
}
