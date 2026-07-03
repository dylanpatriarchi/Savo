#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/*
 * The table is a small singly-linked list. Savo scripts define a handful of
 * variables at most, so a linear scan is more than fast enough and keeps the
 * implementation trivial to read.
 */
typedef struct Symbol {
    char           *name;
    float           value;
    struct Symbol  *next;
} Symbol;

static Symbol *head = NULL;

static Symbol *find(const char *name) {
    Symbol *s;
    for (s = head; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s;
        }
    }
    return NULL;
}

void symtab_set(const char *name, float value) {
    Symbol *s = find(name);
    if (s != NULL) {
        s->value = value;
        return;
    }

    s = malloc(sizeof(Symbol));
    if (s == NULL) {
        fprintf(stderr, "savo: out of memory while defining '%s'\n", name);
        exit(1);
    }
    s->name = strdup(name);
    if (s->name == NULL) {
        fprintf(stderr, "savo: out of memory while defining '%s'\n", name);
        exit(1);
    }
    s->value = value;
    s->next = head;
    head = s;
}

float symtab_get(const char *name) {
    Symbol *s = find(name);
    if (s == NULL) {
        fprintf(stderr, "savo: undefined variable '%s' (using 0)\n", name);
        return 0.0f;
    }
    return s->value;
}

int symtab_has(const char *name) {
    return find(name) != NULL;
}

void symtab_free(void) {
    Symbol *s = head;
    while (s != NULL) {
        Symbol *next = s->next;
        free(s->name);
        free(s);
        s = next;
    }
    head = NULL;
}
