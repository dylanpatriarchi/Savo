#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/*
 * Hand-written lexer for Savo.
 *
 * Replaces the former Flex scanner. It is reentrant (all state lives in a
 * Lexer struct, no globals) and streams input a line at a time, which lets the
 * REPL read interactively while a file or -e string is lexed straight through.
 * Tokens carry a line and column so the parser can report precise errors.
 */

typedef enum {
    TK_EOF, TK_NEWLINE,

    /* literals */
    TK_NUMBER, TK_STRING, TK_IDENT,

    /* command / function keywords */
    TK_PRINT, TK_VAR,
    TK_SUM, TK_SUBTRACT, TK_MULTIPLICATION, TK_DIVISION, TK_MOD,
    TK_SQRT, TK_POW, TK_ABS, TK_RANDOM, TK_FLOOR, TK_CEIL, TK_ROUND,
    TK_LOG, TK_LOG10, TK_MAX, TK_MIN,
    TK_LEN, TK_UPPER, TK_LOWER, TK_TOSTR, TK_TONUM,
    TK_IF, TK_ELSE, TK_END, TK_WHILE, TK_FOR, TK_DEF, TK_RETURN,
    TK_PUSH, TK_SET,
    TK_DIR, TK_LS, TK_CLS, TK_CLEAR, TK_HELP, TK_QUIT, TK_EXIT, TK_POINTER,

    /* operators and punctuation */
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
    TK_EQ, TK_NE, TK_LT, TK_GT, TK_LE, TK_GE, TK_ASSIGN, TK_NOT,
    TK_AND, TK_OR,
    TK_LPAREN, TK_RPAREN, TK_LBRACKET, TK_RBRACKET, TK_LBRACE, TK_RBRACE,
    TK_COLON, TK_DOT, TK_COMMA
} TokKind;

typedef struct {
    TokKind kind;
    char   *text;   /* owned: identifier name / unescaped string; NULL otherwise */
    double  num;    /* TK_NUMBER value */
    int     line;
    int     col;
} Token;

typedef struct {
    FILE   *fp;          /* interactive stream, or NULL when lexing a fixed buffer */
    char   *buf;         /* current input (whole program, or one REPL line)        */
    size_t  pos, len, cap;
    int     interactive; /* reprint the prompt when refilling from fp              */
    int     line, col;
    int     started;     /* whether any line has been read yet (interactive)       */
    int     eof_nl_done; /* emitted the synthetic trailing newline                 */
} Lexer;

/* Initialise a lexer over an owned, NUL-terminated buffer (fp == NULL: batch). */
void  lexer_init_buffer(Lexer *lx, char *owned_buf);
/* Initialise a lexer that reads lines from a stream (used by the REPL). */
void  lexer_init_stream(Lexer *lx, FILE *fp, int interactive);
void  lexer_free(Lexer *lx);

/* Produce the next token. The caller owns tok->text and must free it. */
Token lexer_next(Lexer *lx);

/* Consume the rest of the current line as a raw argument (for savodir/savols),
 * skipping leading blanks and a trailing comment. Returns an owned string, or
 * NULL when the line is empty. Leaves the lexer positioned at the newline. */
char *lexer_read_line_arg(Lexer *lx);

/* Human-readable token name, for error messages. */
const char *token_name(TokKind k);

#endif
