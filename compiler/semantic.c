#include "compiler.h"

static DataType check_expr(ASTNode *n);

static DataType check_expr(ASTNode *n) {
    if (!n) return DTYPE_UNKNOWN;

    switch (n->type) {
        case NODE_INT_LIT:   n->dtype = DTYPE_INT;   return DTYPE_INT;
        case NODE_FLOAT_LIT: n->dtype = DTYPE_FLOAT; return DTYPE_FLOAT;
        case NODE_CHAR_LIT:  n->dtype = DTYPE_CHAR;  return DTYPE_CHAR;

        case NODE_IDENT: {
            Symbol *s = sym_lookup(n->name);
            if (!s) {
                report_error(n->line, "Undeclared variable '%s'", n->name);
                n->dtype = DTYPE_UNKNOWN;
            } else {
                n->dtype = s->dtype;
                if (!s->initialized)
                    report_error(n->line, "Variable '%s' used before initialization", n->name);
            }
            return n->dtype;
        }

        case NODE_UNOP: {
            DataType dt = check_expr(n->left);
            n->dtype = dt;
            return dt;
        }

        case NODE_BINOP: {
            DataType lt = check_expr(n->left);
            DataType rt = check_expr(n->right);
            /* type promotion: int op float -> float */
            if (lt == DTYPE_FLOAT || rt == DTYPE_FLOAT)
                n->dtype = DTYPE_FLOAT;
            else if (lt == DTYPE_INT && rt == DTYPE_INT)
                n->dtype = DTYPE_INT;
            else {
                report_error(n->line, "Type mismatch in binary operation '%s'", n->op);
                n->dtype = DTYPE_UNKNOWN;
            }
            return n->dtype;
        }

        default:
            return DTYPE_UNKNOWN;
    }
}

static void check_stmt(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_VAR_DECL: {
            Symbol *s = sym_declare(n->name, n->dtype, n->line);
            if (s && n->right) {
                DataType rhs = check_expr(n->right);
                /* allow int = float and float = int with warning */
                if (rhs != DTYPE_UNKNOWN && rhs != s->dtype &&
                    !(s->dtype == DTYPE_FLOAT && rhs == DTYPE_INT)) {
                    report_error(n->line,
                        "Type mismatch: cannot assign %s to %s '%s'",
                        rhs == DTYPE_INT ? "int" : rhs == DTYPE_FLOAT ? "float" : "char",
                        s->dtype == DTYPE_INT ? "int" : s->dtype == DTYPE_FLOAT ? "float" : "char",
                        n->name);
                }
                if (s) s->initialized = 1;
            }
            break;
        }

        case NODE_ASSIGN: {
            Symbol *s = sym_lookup(n->name);
            if (!s) {
                report_error(n->line, "Assignment to undeclared variable '%s'", n->name);
            } else {
                check_expr(n->right);
                s->initialized = 1;
            }
            break;
        }

        case NODE_IF: {
            check_expr(n->cond);
            sym_enter_scope();
            check_stmt(n->body);
            sym_exit_scope();
            if (n->els) {
                sym_enter_scope();
                check_stmt(n->els);
                sym_exit_scope();
            }
            break;
        }

        case NODE_WHILE: {
            check_expr(n->cond);
            sym_enter_scope();
            check_stmt(n->body);
            sym_exit_scope();
            break;
        }

        case NODE_BLOCK: {
            for (int i = 0; i < n->nstmts; i++)
                check_stmt(n->stmts[i]);
            break;
        }

        case NODE_RETURN:
            check_expr(n->right);
            break;

        default:
            check_expr(n);
            break;
    }
}

void semantic_analysis(ASTNode *root) {
    sym_init();
    sym_enter_scope(); /* global scope */
    if (!root) return;
    for (int i = 0; i < root->nstmts; i++)
        check_stmt(root->stmts[i]);
}
