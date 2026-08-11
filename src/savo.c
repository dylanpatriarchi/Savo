#include <stdlib.h>
#include <string.h>
#include "savo.h"
#include "lexer.h"
#include "parser.h"
#include "symtab.h"
#include "ast.h"
#include "global.h"

int savo_run_string(const char *code) {
    Lexer lx;
    char *copy;
    size_t n;

    if (code == NULL) code = "";
    n = strlen(code) + 1;
    copy = malloc(n);
    if (copy == NULL) return 1;
    memcpy(copy, code, n);

    prompt = "";                /* non-interactive: no banner, no auto-newlines */
    savo_exit_on_quit = 0;      /* savoquit stops this run, never exits the host */
    savo_quit_flag = 0;
    had_error = 0;

    lexer_init_buffer(&lx, copy);
    parser_run(&lx);
    lexer_free(&lx);

    return had_error;
}

void savo_reset(void) {
    symtab_free();
    func_free_all();
}
