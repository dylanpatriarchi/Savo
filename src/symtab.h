#ifndef SYMTAB_H
#define SYMTAB_H

/*
 * Minimal symbol table for Savo variables.
 *
 * Variables hold a single floating-point value and are addressed by name
 * (e.g. "@x"). Names are copied internally, so callers keep ownership of the
 * strings they pass in.
 */

/* Create or update the variable `name` with `value`. */
void symtab_set(const char *name, float value);

/*
 * Return the value bound to `name`. If the variable is undefined a warning is
 * printed to stderr and 0 is returned, so scripts keep running.
 */
float symtab_get(const char *name);

/* Return 1 if `name` is defined, 0 otherwise. */
int symtab_has(const char *name);

/* Release all memory held by the table (mostly useful for a clean shutdown). */
void symtab_free(void);

#endif
