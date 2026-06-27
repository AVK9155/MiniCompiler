#include "compiler.h"

void sym_init(void) {
    sym_table.count         = 0;
    sym_table.current_scope = 0;
}

void sym_enter_scope(void) {
    sym_table.current_scope++;
}

void sym_exit_scope(void) {
    /* remove all symbols belonging to current scope */
    int scope = sym_table.current_scope;
    int i = 0;
    while (i < sym_table.count) {
        if (sym_table.entries[i].scope == scope) {
            /* shift left */
            for (int j = i; j < sym_table.count - 1; j++)
                sym_table.entries[j] = sym_table.entries[j + 1];
            sym_table.count--;
        } else {
            i++;
        }
    }
    if (sym_table.current_scope > 0)
        sym_table.current_scope--;
}

/* declare a new symbol in current scope; returns NULL on redeclaration */
Symbol *sym_declare(const char *name, DataType dtype, int line) {
    if (sym_lookup_current_scope(name)) {
        report_error(line, "Variable '%s' already declared in this scope", name);
        return NULL;
    }
    if (sym_table.count >= MAX_SYMBOLS) {
        report_error(line, "Symbol table full");
        return NULL;
    }
    Symbol *s = &sym_table.entries[sym_table.count++];
    strncpy(s->name, name, MAX_IDENT_LEN - 1);
    s->dtype       = dtype;
    s->scope       = sym_table.current_scope;
    s->initialized = 0;
    s->value.ival  = 0;
    return s;
}

/* lookup: walks from current scope outward */
Symbol *sym_lookup(const char *name) {
    for (int i = sym_table.count - 1; i >= 0; i--)
        if (strcmp(sym_table.entries[i].name, name) == 0)
            return &sym_table.entries[i];
    return NULL;
}

/* lookup only in current scope */
Symbol *sym_lookup_current_scope(const char *name) {
    int scope = sym_table.current_scope;
    for (int i = sym_table.count - 1; i >= 0; i--)
        if (sym_table.entries[i].scope == scope &&
            strcmp(sym_table.entries[i].name, name) == 0)
            return &sym_table.entries[i];
    return NULL;
}

void sym_print(void) {
    printf("\n=== SYMBOL TABLE ===\n");
    printf("%-16s %-8s %-6s %-10s\n", "NAME", "TYPE", "SCOPE", "INIT");
    printf("%-16s %-8s %-6s %-10s\n", "----", "----", "-----", "----");
    for (int i = 0; i < sym_table.count; i++) {
        Symbol *s = &sym_table.entries[i];
        const char *dtype = s->dtype == DTYPE_INT   ? "int"   :
                            s->dtype == DTYPE_FLOAT ? "float" :
                            s->dtype == DTYPE_CHAR  ? "char"  : "?";
        printf("%-16s %-8s %-6d %-10s\n",
               s->name, dtype, s->scope,
               s->initialized ? "yes" : "no");
    }
    printf("====================\n");
}
