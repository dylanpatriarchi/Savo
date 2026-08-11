#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "global.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symtab.h"
#include "lineedit.h"

static void usage(const char *prog) {
    printf("Savo %s — a tiny scripting language\n\n", SAVO_VERSION);
    printf("Usage:\n");
    printf("  %s [file]        run a .savo script\n", prog);
    printf("  %s -e \"code\"     run code passed on the command line\n", prog);
    printf("  %s -              read a script from stdin\n", prog);
    printf("  %s                start the interactive REPL\n\n", prog);
    printf("Options:\n");
    printf("  -e, --eval CODE   evaluate CODE and exit\n");
    printf("  -h, --help        show this help and exit\n");
    printf("  -v, --version     show the version and exit\n");
}

/* Read an entire stream into a fresh NUL-terminated buffer. */
static char *slurp(FILE *fp) {
    size_t cap = 4096, len = 0;
    char  *buf = malloc(cap);
    size_t n;
    if (buf == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    while ((n = fread(buf + len, 1, cap - len, fp)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (buf == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
        }
    }
    buf[len] = '\0';
    return buf;
}

int main(int argc, char **argv) {
    const char *prog = argv[0];
    const char *filename = NULL;
    const char *evalcode = NULL;
    Lexer lx;
    int i;

    srand((unsigned int) time(NULL));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(prog);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Savo %s\n", SAVO_VERSION);
            return 0;
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--eval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "savo: option '%s' requires an argument\n", argv[i]);
                return 2;
            }
            evalcode = argv[++i];
        } else if (argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "savo: unknown option '%s' (try --help)\n", argv[i]);
            return 2;
        } else {
            filename = argv[i];
        }
    }

    if (evalcode != NULL) {
        char *code = malloc(strlen(evalcode) + 1);
        if (code == NULL) { fprintf(stderr, "savo: out of memory\n"); return 1; }
        strcpy(code, evalcode);
        prompt = "";
        lexer_init_buffer(&lx, code);
    } else if (filename != NULL && strcmp(filename, "-") != 0) {
        FILE *fp = fopen(filename, "r");
        char *code;
        if (fp == NULL) { fprintf(stderr, "savo: cannot open file '%s'\n", filename); return 1; }
        code = slurp(fp);
        fclose(fp);
        prompt = "";
        lexer_init_buffer(&lx, code);
    } else if (filename != NULL) {          /* explicit "-": stdin as a script */
        prompt = "";
        lexer_init_buffer(&lx, slurp(stdin));
    } else if (isatty(fileno(stdin))) {     /* interactive REPL */
        prompt = ">>> ";
        printf("%s", consoleMex);
        printf("SavoLanguage Console %s\n", SAVO_VERSION);
        printf("digit savohelp for some tips.\n\n");
        lexer_init_stream(&lx, stdin, 1);
    } else {                                /* piped script, no banner */
        prompt = "";
        lexer_init_buffer(&lx, slurp(stdin));
    }

    parser_run(&lx);
    lexer_free(&lx);
    symtab_free();
    func_free_all();
    savo_history_free();
    return had_error;
}
