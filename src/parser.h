#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/*
 * Hand-written recursive-descent parser (Pratt precedence climbing for
 * expressions). Replaces the former Bison grammar. It drives execution
 * directly: each top-level statement is parsed, executed and freed in turn,
 * function definitions are retained, and a syntax error is reported with a
 * line:column and recovered from at the next line so the REPL keeps going.
 */
void parser_run(Lexer *lx);

#endif
