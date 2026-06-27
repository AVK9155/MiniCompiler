#include <math.h>
#include "compiler.h"

/* ── constant folding helpers ── */
static int is_number(const char *s) {
    if (!s || !*s) return 0;
    const char *p = s;
    if (*p == '-') p++;
    int dots = 0;
    for (; *p; p++) {
        if (*p == '.') { dots++; if (dots > 1) return 0; }
        else if (!(*p >= '0' && *p <= '9')) return 0;
    }
    return 1;
}

static double to_num(const char *s) { return atof(s); }
static int    is_int_val(double v)  { return v == (int)v; }

static void fold_binop(IRInstr *ins) {
    if (!is_number(ins->arg1) || !is_number(ins->arg2)) return;

    double a = to_num(ins->arg1);
    double b = to_num(ins->arg2);
    double r = 0;
    int    valid = 1;

    if      (strcmp(ins->op, "+") == 0) r = a + b;
    else if (strcmp(ins->op, "-") == 0) r = a - b;
    else if (strcmp(ins->op, "*") == 0) r = a * b;
    else if (strcmp(ins->op, "/") == 0) {
        if (b == 0) { valid = 0; } else r = a / b;
    }
    else if (strcmp(ins->op, "%") == 0) {
        if (b == 0) { valid = 0; } else r = fmod(a, b);
    }
    else { valid = 0; }

    if (valid) {
        ins->type = IR_ASSIGN;
        if (is_int_val(r))
            snprintf(ins->arg1, MAX_IDENT_LEN, "%d", (int)r);
        else
            snprintf(ins->arg1, MAX_IDENT_LEN, "%.6g", r);
        ins->arg2[0] = '\0';
        ins->op[0]   = '\0';
    }
}

/* ── copy propagation: track what each temp/var holds ── */
typedef struct { char name[MAX_IDENT_LEN]; char val[MAX_IDENT_LEN]; } CPEntry;
static CPEntry cp_table[MAX_IR];
static int     cp_count = 0;

static void cp_set(const char *name, const char *val) {
    for (int i = 0; i < cp_count; i++) {
        if (strcmp(cp_table[i].name, name) == 0) {
            strncpy(cp_table[i].val, val, MAX_IDENT_LEN-1);
            return;
        }
    }
    if (cp_count < MAX_IR) {
        strncpy(cp_table[cp_count].name, name, MAX_IDENT_LEN-1);
        strncpy(cp_table[cp_count].val,  val,  MAX_IDENT_LEN-1);
        cp_count++;
    }
}

static const char *cp_get(const char *name) {
    for (int i = 0; i < cp_count; i++)
        if (strcmp(cp_table[i].name, name) == 0)
            return cp_table[i].val;
    return name; /* no substitution */
}

static void cp_invalidate(const char *name) {
    for (int i = 0; i < cp_count; i++)
        if (strcmp(cp_table[i].name, name) == 0) {
            cp_table[i].val[0] = '\0';
            return;
        }
}

static void substitute(char *field) {
    const char *sub = cp_get(field);
    if (sub && sub[0]) strncpy(field, sub, MAX_IDENT_LEN-1);
}

/* ── dead code: track which temps are used ── */
static int is_used_after(int idx, const char *name) {
    for (int i = idx + 1; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i];
        if (strcmp(ins->arg1,   name) == 0) return 1;
        if (strcmp(ins->arg2,   name) == 0) return 1;
        if (strcmp(ins->result, name) == 0) return 0; /* reassigned = dead */
    }
    return 0;
}

static int is_temp(const char *name) {
    return name[0] == 't' && name[1] >= '0' && name[1] <= '9';
}

/* check if a variable is reassigned anywhere in IR (i.e. mutable) */
static int is_mutable(const char *name) {
    int count = 0;
    for (int i = 0; i < ir_count; i++) {
        if ((ir_code[i].type == IR_ASSIGN || ir_code[i].type == IR_BINOP) &&
             strcmp(ir_code[i].result, name) == 0)
            count++;
    }
    return count > 1; /* assigned more than once = mutable */
}

/* ── main optimizer pass ── */
void optimize_ir(void) {
    cp_count = 0;

    /* Pass 1: constant folding + copy propagation (only for immutable vars) */
    for (int i = 0; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i];

        if (ins->type == IR_ASSIGN || ins->type == IR_BINOP) {
            if (!is_mutable(ins->arg1)) substitute(ins->arg1);
            if (!is_mutable(ins->arg2)) substitute(ins->arg2);
        }
        if (ins->type == IR_CJUMP) {
            if (!is_mutable(ins->arg1)) substitute(ins->arg1);
            if (!is_mutable(ins->arg2)) substitute(ins->arg2);
        }

        if (ins->type == IR_BINOP) {
            fold_binop(ins);
        }

        if (ins->type == IR_ASSIGN && !is_mutable(ins->result)) {
            cp_set(ins->result, ins->arg1);
        } else if (ins->type == IR_BINOP || ins->type == IR_CJUMP) {
            cp_invalidate(ins->result);
        }
    }

    /* Pass 2: dead temp elimination */
    IRInstr optimized[MAX_IR];
    int     opt_count = 0;

    for (int i = 0; i < ir_count; i++) {
        IRInstr *ins = &ir_code[i];
        int dead = 0;

        if ((ins->type == IR_ASSIGN || ins->type == IR_BINOP) &&
             is_temp(ins->result) &&
            !is_used_after(i, ins->result)) {
            dead = 1;
        }

        if (!dead) optimized[opt_count++] = *ins;
    }

    memcpy(ir_code, optimized, sizeof(IRInstr) * opt_count);
    ir_count = opt_count;
}
