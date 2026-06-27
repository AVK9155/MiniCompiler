#include "compiler.h"

/* ── global state definitions ── */
Token       tokens[MAX_TOKENS];
int         token_count = 0;
ASTNode    *ast_root    = NULL;
SymbolTable sym_table;
IRInstr     ir_code[MAX_IR];
int         ir_count    = 0;
char        error_msgs[MAX_ERRORS][256];
int         error_count = 0;

/* ── sample program ── */
static const char *sample_source =
    "int x = 10;\n"
    "int y = 20;\n"
    "int z = x + y * 2;\n"
    "if (z > 30) {\n"
    "    int result = z - 10;\n"
    "} else {\n"
    "    int result = z + 10;\n"
    "}\n"
    "int i = 0;\n"
    "while (i < 5) {\n"
    "    i = i + 1;\n"
    "}\n";

int main(int argc, char *argv[]) {
    const char *source = sample_source;

    /* read from file if provided */
    char *file_buf = NULL;
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { fprintf(stderr, "Cannot open file: %s\n", argv[1]); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        file_buf = malloc(sz + 1);
        fread(file_buf, 1, sz, f);
        file_buf[sz] = '\0';
        fclose(f);
        source = file_buf;
    }

    printf("╔══════════════════════════════════╗\n");
    printf("║      MiniCompiler v2.0           ║\n");
    printf("╚══════════════════════════════════╝\n\n");

    /* ── Phase 1: Lexical Analysis ── */
    printf("[1/5] Lexical Analysis...\n");
    lexical_analysis(source);
    printf("      %d tokens generated.\n", token_count);

    /* print tokens */
    printf("\n=== TOKENS ===\n");
    for (int i = 0; i < token_count && tokens[i].type != TOK_EOF; i++)
        printf("  [%3d] %-12s  '%s'\n", i+1,
               token_type_str(tokens[i].type), tokens[i].value);

    if (error_count > 0) { print_errors(); goto cleanup; }

    /* ── Phase 2: Parsing ── */
    printf("\n[2/5] Parsing...\n");
    ast_root = parse();
    printf("      AST built.\n");
    if (error_count > 0) { print_errors(); goto cleanup; }

    /* ── Phase 3: Semantic Analysis ── */
    printf("\n[3/5] Semantic Analysis...\n");
    semantic_analysis(ast_root);
    sym_print();
    if (error_count > 0) { print_errors(); goto cleanup; }
    printf("      No semantic errors.\n");

    /* ── Phase 4: IR Generation ── */
    printf("\n[4/5] IR Generation...\n");
    generate_ir(ast_root);
    print_ir();

    /* ── Phase 4b: Optimization ── */
    printf("\n[4b]  Optimizing IR...\n");
    int before = ir_count;
    optimize_ir();
    printf("      %d → %d instructions (-%d eliminated).\n",
           before, ir_count, before - ir_count);
    printf("\n=== OPTIMIZED IR ===\n");
    print_ir();

    /* ── Phase 5: Execution ── */
    printf("\n[5/5] Executing...\n");
    execute_ir();

cleanup:
    if (error_count > 0) print_errors();
    free_ast(ast_root);
    free(file_buf);
    printf("\nDone.\n");
    return error_count > 0 ? 1 : 0;
}
