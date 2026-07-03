#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

typedef struct Symbol {
    char           *name;
    float           value;
    struct Symbol  *next;
} Symbol;

typedef struct Scope {
    Symbol        *head;
    struct Scope  *parent;
} Scope;

/* The global scope always exists at the bottom of the stack. */
static Scope  global  = { NULL, NULL };
static Scope *current = &global;

static void oom(const char *name) {
    fprintf(stderr, "savo: out of memory while defining '%s'\n", name);
    exit(1);
}

/* Find a symbol in one scope only. */
static Symbol *find_local(Scope *scope, const char *name) {
    Symbol *s;
    for (s = scope->head; s != NULL; s = s->next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

void symtab_set(const char *name, float value) {
    Symbol *s = find_local(current, name);
    if (s != NULL) { s->value = value; return; }

    s = malloc(sizeof(Symbol));
    if (s == NULL) oom(name);
    s->name = strdup(name);
    if (s->name == NULL) oom(name);
    s->value = value;
    s->next = current->head;
    current->head = s;
}

float symtab_get(const char *name) {
    Scope *scope;
    for (scope = current; scope != NULL; scope = scope->parent) {
        Symbol *s = find_local(scope, name);
        if (s != NULL) return s->value;
    }
    fprintf(stderr, "savo: undefined variable '%s' (using 0)\n", name);
    return 0.0f;
}

int symtab_has(const char *name) {
    Scope *scope;
    for (scope = current; scope != NULL; scope = scope->parent)
        if (find_local(scope, name) != NULL) return 1;
    return 0;
}

void symtab_push_scope(void) {
    Scope *scope = malloc(sizeof(Scope));
    if (scope == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    scope->head = NULL;
    scope->parent = current;
    current = scope;
}

static void free_symbols(Scope *scope) {
    Symbol *s = scope->head;
    while (s != NULL) {
        Symbol *next = s->next;
        free(s->name);
        free(s);
        s = next;
    }
    scope->head = NULL;
}

void symtab_pop_scope(void) {
    Scope *dead;
    if (current == &global) return;   /* never pop the global scope */
    dead = current;
    current = current->parent;
    free_symbols(dead);
    free(dead);
}

void symtab_free(void) {
    while (current != &global) symtab_pop_scope();
    free_symbols(&global);
}
