#include "compiler.h"

static int temp_count  = 0;
static int label_count = 0;

static void new_temp(char *out) {
    snprintf(out, MAX_IDENT_LEN, "t%d", temp_count++);
}

static void new_label(char *out) {
    snprintf(out, MAX_IDENT_LEN, "L%d", label_count++);
}

static void emit(IROpType op, const char *res, const char *a1,
                 const char *a2, const char *opr, const char *lbl) {
    if (ir_count >= MAX_IR) return;
    IRInstr *i = &ir_code[ir_count++];
    i->type = op;
    if (res) strncpy(i->result, res, MAX_IDENT_LEN-1);
    if (a1)  strncpy(i->arg1,   a1,  MAX_IDENT_LEN-1);
    if (a2)  strncpy(i->arg2,   a2,  MAX_IDENT_LEN-1);
    if (opr) strncpy(i->op,     opr, 7);
    if (lbl) strncpy(i->label,  lbl, MAX_IDENT_LEN-1);
}

/* generate expr, store result name in 'out' */
static void gen_expr(ASTNode *n, char *out) {
    if (!n) { strcpy(out, "?"); return; }

    switch (n->type) {
        case NODE_INT_LIT:
        case NODE_FLOAT_LIT:
        case NODE_CHAR_LIT:
            strncpy(out, n->name, MAX_IDENT_LEN-1);
            return;

        case NODE_IDENT:
            strncpy(out, n->name, MAX_IDENT_LEN-1);
            return;

        case NODE_UNOP: {
            char a[MAX_IDENT_LEN];
            gen_expr(n->left, a);
            new_temp(out);
            /* emit: out = op a */
            char op_str[8];
            snprintf(op_str, 8, "%s", n->op);
            emit(IR_BINOP, out, "0", a, strcmp(n->op,"-")==0?"-":"!", NULL);
            return;
        }

        case NODE_BINOP: {
            char a[MAX_IDENT_LEN], b[MAX_IDENT_LEN];
            gen_expr(n->left,  a);
            gen_expr(n->right, b);
            new_temp(out);
            emit(IR_BINOP, out, a, b, n->op, NULL);
            return;
        }

        default:
            strcpy(out, "?");
    }
}

static void gen_stmt(ASTNode *n);

static void gen_stmt(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_VAR_DECL: {
            if (n->right) {
                char val[MAX_IDENT_LEN];
                gen_expr(n->right, val);
                emit(IR_ASSIGN, n->name, val, NULL, NULL, NULL);
            } else {
                /* initialize to 0 */
                emit(IR_ASSIGN, n->name, "0", NULL, NULL, NULL);
            }
            break;
        }

        case NODE_ASSIGN: {
            char val[MAX_IDENT_LEN];
            gen_expr(n->right, val);
            emit(IR_ASSIGN, n->name, val, NULL, NULL, NULL);
            break;
        }

        case NODE_IF: {
            char cond_l[MAX_IDENT_LEN], cond_r[MAX_IDENT_LEN];
            char l_true[MAX_IDENT_LEN], l_false[MAX_IDENT_LEN], l_end[MAX_IDENT_LEN];
            new_label(l_true);
            new_label(l_false);
            new_label(l_end);

            /* evaluate condition */
            if (n->cond && n->cond->type == NODE_BINOP) {
                gen_expr(n->cond->left,  cond_l);
                gen_expr(n->cond->right, cond_r);
                emit(IR_CJUMP, NULL, cond_l, cond_r, n->cond->op, l_true);
            } else {
                char cv[MAX_IDENT_LEN];
                gen_expr(n->cond, cv);
                emit(IR_CJUMP, NULL, cv, "0", "!=", l_true);
            }
            emit(IR_JUMP,  NULL, NULL, NULL, NULL, l_false);
            emit(IR_LABEL, NULL, NULL, NULL, NULL, l_true);
            gen_stmt(n->body);
            if (n->els) {
                emit(IR_JUMP,  NULL, NULL, NULL, NULL, l_end);
                emit(IR_LABEL, NULL, NULL, NULL, NULL, l_false);
                gen_stmt(n->els);
                emit(IR_LABEL, NULL, NULL, NULL, NULL, l_end);
            } else {
                emit(IR_LABEL, NULL, NULL, NULL, NULL, l_false);
            }
            break;
        }

        case NODE_WHILE: {
            char l_start[MAX_IDENT_LEN], l_body[MAX_IDENT_LEN], l_end[MAX_IDENT_LEN];
            char cond_l[MAX_IDENT_LEN], cond_r[MAX_IDENT_LEN];
            new_label(l_start);
            new_label(l_body);
            new_label(l_end);

            emit(IR_LABEL, NULL, NULL, NULL, NULL, l_start);
            if (n->cond && n->cond->type == NODE_BINOP) {
                gen_expr(n->cond->left,  cond_l);
                gen_expr(n->cond->right, cond_r);
                emit(IR_CJUMP, NULL, cond_l, cond_r, n->cond->op, l_body);
            } else {
                char cv[MAX_IDENT_LEN];
                gen_expr(n->cond, cv);
                emit(IR_CJUMP, NULL, cv, "0", "!=", l_body);
            }
            emit(IR_JUMP,  NULL, NULL, NULL, NULL, l_end);
            emit(IR_LABEL, NULL, NULL, NULL, NULL, l_body);
            gen_stmt(n->body);
            emit(IR_JUMP,  NULL, NULL, NULL, NULL, l_start);
            emit(IR_LABEL, NULL, NULL, NULL, NULL, l_end);
            break;
        }

        case NODE_BLOCK:
            for (int i = 0; i < n->nstmts; i++)
                gen_stmt(n->stmts[i]);
            break;

        case NODE_RETURN: {
            char rv[MAX_IDENT_LEN] = "0";
            if (n->right) gen_expr(n->right, rv);
            emit(IR_RETURN, NULL, rv, NULL, NULL, NULL);
            break;
        }

        default: break;
    }
}

void generate_ir(ASTNode *root) {
    temp_count  = 0;
    label_count = 0;
    ir_count    = 0;
    if (!root) return;
    for (int i = 0; i < root->nstmts; i++)
        gen_stmt(root->stmts[i]);
}

void print_ir(void) {
    printf("\n=== INTERMEDIATE REPRESENTATION (Three-Address Code) ===\n");
    for (int i = 0; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i];
        switch (ins->type) {
            case IR_ASSIGN:
                printf("  %s = %s\n", ins->result, ins->arg1);
                break;
            case IR_BINOP:
                printf("  %s = %s %s %s\n", ins->result, ins->arg1, ins->op, ins->arg2);
                break;
            case IR_LABEL:
                printf("%s:\n", ins->label);
                break;
            case IR_JUMP:
                printf("  goto %s\n", ins->label);
                break;
            case IR_CJUMP:
                printf("  if %s %s %s goto %s\n",
                       ins->arg1, ins->op, ins->arg2, ins->label);
                break;
            case IR_RETURN:
                printf("  return %s\n", ins->arg1);
                break;
            default:
                printf("  nop\n");
        }
    }
    printf("========================================================\n");
}
