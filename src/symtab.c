#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "global.h"

/*
 * Each scope stores its variables in a small chained hash table, so lookups are
 * O(1) on average instead of the O(n) linear scan the old linked list used —
 * the interpreter's hottest operation. The public API is unchanged.
 */

typedef struct Symbol {
    char           *name;
    Value           value;
    struct Symbol  *next;   /* next symbol in the same bucket */
} Symbol;

typedef struct Scope {
    Symbol       **buckets;
    int            nbuckets;
    int            count;
    struct Scope  *parent;
} Scope;

static Scope  global  = { NULL, 0, 0, NULL };
static Scope *current = &global;

static void oom(const char *name) {
    fprintf(stderr, "savo: out of memory while defining '%s'\n", name);
    exit(1);
}

/* FNV-1a over the name. */
static unsigned long hash_name(const char *s) {
    unsigned long h = 2166136261UL;
    while (*s) { h ^= (unsigned char) *s++; h *= 16777619UL; }
    return h;
}

static void scope_init(Scope *scope) {
    scope->nbuckets = 16;
    scope->count = 0;
    scope->buckets = calloc((size_t) scope->nbuckets, sizeof(Symbol *));
    if (scope->buckets == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
}

static void scope_grow(Scope *scope) {
    int old_n = scope->nbuckets, new_n = old_n * 2, i;
    Symbol **nb = calloc((size_t) new_n, sizeof(Symbol *));
    if (nb == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    for (i = 0; i < old_n; i++) {
        Symbol *s = scope->buckets[i];
        while (s != NULL) {
            Symbol *next = s->next;
            unsigned long b = hash_name(s->name) % (unsigned long) new_n;
            s->next = nb[b];
            nb[b] = s;
            s = next;
        }
    }
    free(scope->buckets);
    scope->buckets = nb;
    scope->nbuckets = new_n;
}

static Symbol *find_local(Scope *scope, const char *name) {
    Symbol *s;
    unsigned long b;
    if (scope->buckets == NULL) return NULL;
    b = hash_name(name) % (unsigned long) scope->nbuckets;
    for (s = scope->buckets[b]; s != NULL; s = s->next)
        if (strcmp(s->name, name) == 0) return s;
    return NULL;
}

void symtab_set(const char *name, Value v) {
    Symbol *s = find_local(current, name);
    unsigned long b;
    if (s != NULL) {
        value_free(s->value);
        s->value = value_copy(v);
        return;
    }
    if (current->buckets == NULL) scope_init(current);
    if (current->count + 1 > current->nbuckets * 3 / 4) scope_grow(current);

    s = malloc(sizeof(Symbol));
    if (s == NULL) oom(name);
    s->name = strdup(name);
    if (s->name == NULL) oom(name);
    s->value = value_copy(v);
    b = hash_name(name) % (unsigned long) current->nbuckets;
    s->next = current->buckets[b];
    current->buckets[b] = s;
    current->count++;
}

Value symtab_get(const char *name) {
    Scope *scope;
    for (scope = current; scope != NULL; scope = scope->parent) {
        Symbol *s = find_local(scope, name);
        if (s != NULL) return value_copy(s->value);
    }
    savo_warn("undefined variable '%s' (using nil)", name);
    return value_nil();
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
    scope->buckets = NULL;   /* allocated lazily on first insert */
    scope->nbuckets = 0;
    scope->count = 0;
    scope->parent = current;
    current = scope;
}

static void free_symbols(Scope *scope) {
    int i;
    for (i = 0; i < scope->nbuckets; i++) {
        Symbol *s = scope->buckets[i];
        while (s != NULL) {
            Symbol *next = s->next;
            value_free(s->value);
            free(s->name);
            free(s);
            s = next;
        }
    }
    free(scope->buckets);
    scope->buckets = NULL;
    scope->nbuckets = 0;
    scope->count = 0;
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
