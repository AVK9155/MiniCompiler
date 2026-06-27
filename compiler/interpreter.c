#include <math.h>
#include "compiler.h"

typedef struct {
    char   name[MAX_IDENT_LEN];
    double value;
} EnvEntry;

static EnvEntry env[MAX_SYMBOLS * 2];
static int      env_count = 0;

static void env_set(const char *name, double val) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env[i].name, name) == 0) {
            env[i].value = val;
            return;
        }
    }
    if (env_count < MAX_SYMBOLS * 2) {
        strncpy(env[env_count].name, name, MAX_IDENT_LEN-1);
        env[env_count].value = val;
        env_count++;
    }
}

static double env_get(const char *name) {
    /* numeric literal */
    char *end;
    double v = strtod(name, &end);
    if (end != name && *end == '\0') return v;

    for (int i = 0; i < env_count; i++)
        if (strcmp(env[i].name, name) == 0)
            return env[i].value;
    return 0.0;
}

static int find_label(const char *lbl) {
    for (int i = 0; i < ir_count; i++)
        if (ir_code[i].type == IR_LABEL &&
            strcmp(ir_code[i].label, lbl) == 0)
            return i;
    return -1;
}

static int eval_cond(double a, const char *op, double b) {
    if (strcmp(op, "==") == 0) return a == b;
    if (strcmp(op, "!=") == 0) return a != b;
    if (strcmp(op, "<")  == 0) return a <  b;
    if (strcmp(op, ">")  == 0) return a >  b;
    if (strcmp(op, "<=") == 0) return a <= b;
    if (strcmp(op, ">=") == 0) return a >= b;
    return 0;
}

static double eval_binop(double a, const char *op, double b) {
    if (strcmp(op, "+") == 0) return a + b;
    if (strcmp(op, "-") == 0) return a - b;
    if (strcmp(op, "*") == 0) return a * b;
    if (strcmp(op, "/") == 0) return b != 0 ? a / b : 0;
    if (strcmp(op, "%") == 0) return b != 0 ? fmod(a, b) : 0;
    if (strcmp(op, "&&") == 0) return (a && b) ? 1 : 0;
    if (strcmp(op, "||") == 0) return (a || b) ? 1 : 0;
    return 0;
}

void execute_ir(void) {
    env_count = 0;
    printf("\n=== EXECUTION OUTPUT ===\n");

    int pc = 0;
    int steps = 0;
    int max_steps = 100000; /* prevent infinite loops */

    while (pc < ir_count && steps++ < max_steps) {
        IRInstr *ins = &ir_code[pc];

        switch (ins->type) {
            case IR_ASSIGN: {
                double val = env_get(ins->arg1);
                env_set(ins->result, val);
                pc++;
                break;
            }

            case IR_BINOP: {
                double a   = env_get(ins->arg1);
                double b   = env_get(ins->arg2);
                double res = eval_binop(a, ins->op, b);
                env_set(ins->result, res);
                pc++;
                break;
            }

            case IR_LABEL:
                pc++;
                break;

            case IR_JUMP: {
                int target = find_label(ins->label);
                if (target < 0) { fprintf(stderr, "Unknown label %s\n", ins->label); pc++; }
                else pc = target + 1;
                break;
            }

            case IR_CJUMP: {
                double a = env_get(ins->arg1);
                double b = env_get(ins->arg2);
                if (eval_cond(a, ins->op, b)) {
                    int target = find_label(ins->label);
                    if (target < 0) pc++;
                    else pc = target + 1;
                } else {
                    pc++;
                }
                break;
            }

            case IR_RETURN:
                printf("return %g\n", env_get(ins->arg1));
                goto done;

            default:
                pc++;
        }
    }

    if (steps >= max_steps)
        printf("[WARNING] Execution halted: possible infinite loop\n");

done:
    /* print all non-temp variables */
    printf("\n--- Variable State ---\n");
    for (int i = 0; i < env_count; i++) {
        if (env[i].name[0] == 't') continue; /* skip temps */
        double v = env[i].value;
        if (v == (int)v)
            printf("  %s = %d\n", env[i].name, (int)v);
        else
            printf("  %s = %.6g\n", env[i].name, v);
    }
    printf("========================\n");
}
