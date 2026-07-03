#ifndef SYMTAB_H
#define SYMTAB_H

#include "value.h"

/*
 * Symbol table for Savo variables, organised as a stack of scopes.
 *
 * The bottom scope is global. A function call pushes a fresh scope for its
 * parameters and locals and pops it on return. Lookups search from the current
 * scope outward to the global one, so functions can read globals; assignments
 * always write to the current scope, so a local never clobbers a global.
 *
 * Variables hold a Value and are addressed by name (e.g. "@x"). Names and
 * values are copied internally, so callers keep ownership of what they pass in
 * and own what they get back.
 */

/* Create or update `name` in the current scope (stores a copy of `v`). */
void symtab_set(const char *name, Value v);

/* A copy of the value bound to `name`; warns and returns 0 if undefined. */
Value symtab_get(const char *name);

/* 1 if `name` is visible from the current scope, 0 otherwise. */
int symtab_has(const char *name);

/* Enter / leave a local scope (used around a function body). */
void symtab_push_scope(void);
void symtab_pop_scope(void);

/* Release every scope's memory (clean shutdown). */
void symtab_free(void);

#endif
