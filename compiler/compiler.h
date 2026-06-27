#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─────────────────────────────────────────
   LIMITS
───────────────────────────────────────── */
#define MAX_TOKENS      512
#define MAX_SYMBOLS     128
#define MAX_IR          512
#define MAX_ERRORS      64
#define MAX_IDENT_LEN   64
#define MAX_SCOPE_DEPTH 16

/* ─────────────────────────────────────────
   TOKEN TYPES
───────────────────────────────────────── */
typedef enum {
    /* literals */
    TOK_INT_LIT, TOK_FLOAT_LIT, TOK_CHAR_LIT,
    /* identifiers */
    TOK_IDENT,
    /* keywords */
    TOK_INT, TOK_FLOAT, TOK_CHAR,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR,
    TOK_RETURN,
    /* operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LEQ, TOK_GEQ,
    TOK_AND, TOK_OR, TOK_NOT,
    TOK_ASSIGN,
    /* punctuation */
    TOK_LPAREN, TOK_RPAREN,
    TOK_LBRACE, TOK_RBRACE,
    TOK_SEMICOLON, TOK_COMMA,
    /* special */
    TOK_EOF, TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char      value[MAX_IDENT_LEN];
    int       line;
} Token;

/* ─────────────────────────────────────────
   AST NODE TYPES
───────────────────────────────────────── */
typedef enum {
    NODE_PROGRAM,
    NODE_VAR_DECL,      /* int x = expr */
    NODE_ASSIGN,        /* x = expr     */
    NODE_IF,            /* if/else      */
    NODE_WHILE,         /* while        */
    NODE_BLOCK,         /* { stmts }    */
    NODE_BINOP,         /* a + b        */
    NODE_UNOP,          /* -a / !a      */
    NODE_IDENT,         /* variable ref */
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_CHAR_LIT,
    NODE_RETURN
} NodeType;

typedef enum { DTYPE_INT, DTYPE_FLOAT, DTYPE_CHAR, DTYPE_UNKNOWN } DataType;

typedef struct ASTNode {
    NodeType  type;
    DataType  dtype;        /* resolved type after semantic pass */
    char      op[8];        /* operator for BINOP / UNOP        */
    char      name[MAX_IDENT_LEN]; /* identifier / literal value */
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *cond;   /* condition for IF / WHILE          */
    struct ASTNode *body;   /* then-branch / loop body           */
    struct ASTNode *els;    /* else-branch                       */
    struct ASTNode **stmts; /* BLOCK / PROGRAM children          */
    int       nstmts;
    int       line;
} ASTNode;

/* ─────────────────────────────────────────
   SYMBOL TABLE
───────────────────────────────────────── */
typedef struct {
    char     name[MAX_IDENT_LEN];
    DataType dtype;
    int      scope;
    union { int ival; float fval; char cval; } value;
    int      initialized;
} Symbol;

typedef struct {
    Symbol entries[MAX_SYMBOLS];
    int    count;
    int    current_scope;
} SymbolTable;

/* ─────────────────────────────────────────
   IR
───────────────────────────────────────── */
typedef enum {
    IR_ASSIGN,   /* t1 = t2           */
    IR_BINOP,    /* t1 = t2 op t3     */
    IR_LABEL,    /* L1:               */
    IR_JUMP,     /* goto L1           */
    IR_CJUMP,    /* if t1 op t2 goto L*/
    IR_RETURN,   /* return t1         */
    IR_NOP
} IROpType;

typedef struct {
    IROpType type;
    char     result[MAX_IDENT_LEN];
    char     arg1[MAX_IDENT_LEN];
    char     arg2[MAX_IDENT_LEN];
    char     op[8];
    char     label[MAX_IDENT_LEN];
} IRInstr;

/* ─────────────────────────────────────────
   GLOBAL STATE  (defined in main.c)
───────────────────────────────────────── */
extern Token       tokens[MAX_TOKENS];
extern int         token_count;
extern ASTNode    *ast_root;
extern SymbolTable sym_table;
extern IRInstr     ir_code[MAX_IR];
extern int         ir_count;
extern char        error_msgs[MAX_ERRORS][256];
extern int         error_count;

/* ─────────────────────────────────────────
   FUNCTION PROTOTYPES
───────────────────────────────────────── */

/* lexer.c */
void lexical_analysis(const char *source);
const char *token_type_str(TokenType t);

/* parser.c */
ASTNode *parse(void);
void     free_ast(ASTNode *node);

/* symbol_table.c */
void    sym_init(void);
void    sym_enter_scope(void);
void    sym_exit_scope(void);
Symbol *sym_declare(const char *name, DataType dtype, int line);
Symbol *sym_lookup(const char *name);
Symbol *sym_lookup_current_scope(const char *name);
void    sym_print(void);

/* semantic.c */
void semantic_analysis(ASTNode *root);

/* ir.c */
void generate_ir(ASTNode *root);
void print_ir(void);

/* optimizer.c */
void optimize_ir(void);

/* interpreter.c */
void execute_ir(void);

/* error */
void report_error(int line, const char *fmt, ...);
void print_errors(void);

#endif /* COMPILER_H */
