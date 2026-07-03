#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

static char *xstrdup(const char *s) {
    char *p = strdup(s ? s : "");
    if (p == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    return p;
}

Value value_num(double n) {
    Value v; v.type = VAL_NUM; v.as.num = n; return v;
}

Value value_str(char *owned) {
    Value v; v.type = VAL_STR; v.as.str = owned ? owned : xstrdup(""); return v;
}

Value value_str_copy(const char *s) {
    return value_str(xstrdup(s));
}

Value value_array(void) {
    Array *a = malloc(sizeof(Array));
    Value v;
    if (a == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    a->rc = 1; a->items = NULL; a->count = 0; a->cap = 0;
    v.type = VAL_ARR; v.as.arr = a;
    return v;
}

Value value_object(void) {
    Map *m = malloc(sizeof(Map));
    Value v;
    if (m == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    m->rc = 1; m->head = NULL; m->count = 0;
    v.type = VAL_OBJ; v.as.obj = m;
    return v;
}

Value value_copy(Value v) {
    if (v.type == VAL_STR) return value_str(xstrdup(v.as.str));
    if (v.type == VAL_ARR) { v.as.arr->rc++; return v; }   /* share by reference */
    if (v.type == VAL_OBJ) { v.as.obj->rc++; return v; }
    return v;
}

void value_free(Value v) {
    if (v.type == VAL_STR) { free(v.as.str); return; }
    if (v.type == VAL_ARR) {
        Array *a = v.as.arr;
        int i;
        if (--a->rc > 0) return;
        for (i = 0; i < a->count; i++) value_free(a->items[i]);
        free(a->items);
        free(a);
        return;
    }
    if (v.type == VAL_OBJ) {
        Map *m = v.as.obj;
        MapEntry *e;
        if (--m->rc > 0) return;
        e = m->head;
        while (e) {
            MapEntry *next = e->next;
            value_free(*e->val);
            free(e->val);
            free(e->key);
            free(e);
            e = next;
        }
        free(m);
    }
}

static MapEntry *map_find(Map *m, const char *key) {
    MapEntry *e;
    for (e = m->head; e; e = e->next)
        if (strcmp(e->key, key) == 0) return e;
    return NULL;
}

void object_set(Value v, const char *key, Value elem) {
    Map *m = v.as.obj;
    MapEntry *e = map_find(m, key);
    if (e != NULL) { value_free(*e->val); *e->val = value_copy(elem); return; }
    e = malloc(sizeof(MapEntry));
    if (e == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    e->key = xstrdup(key);
    e->val = malloc(sizeof(Value));
    if (e->val == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    *e->val = value_copy(elem);
    e->next = m->head;
    m->head = e;
    m->count++;
}

Value object_get(Value v, const char *key) {
    MapEntry *e = map_find(v.as.obj, key);
    if (e == NULL) {
        fprintf(stderr, "savo: object has no key '%s' (using 0)\n", key);
        return value_num(0);
    }
    return value_copy(*e->val);
}

int object_length(Value v) { return v.as.obj->count; }

void array_push(Value v, Value elem) {
    Array *a = v.as.arr;
    if (a->count == a->cap) {
        a->cap = a->cap ? a->cap * 2 : 4;
        a->items = realloc(a->items, sizeof(Value) * a->cap);
        if (a->items == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    }
    a->items[a->count++] = value_copy(elem);
}

int array_length(Value v) { return v.as.arr->count; }

Value array_get(Value v, int i) {
    Array *a = v.as.arr;
    if (i < 0 || i >= a->count) {
        fprintf(stderr, "savo: array index %d out of range (using 0)\n", i);
        return value_num(0);
    }
    return value_copy(a->items[i]);
}

void array_set(Value v, int i, Value elem) {
    Array *a = v.as.arr;
    if (i < 0) { fprintf(stderr, "savo: array index %d out of range\n", i); return; }
    while (a->count <= i) array_push(v, value_num(0));   /* grow with zeros */
    value_free(a->items[i]);
    a->items[i] = value_copy(elem);
}

double value_to_number(Value v) {
    if (v.type == VAL_NUM) return v.as.num;
    if (v.type == VAL_ARR) return v.as.arr->count;   /* arrays coerce to length */
    if (v.type == VAL_OBJ) return v.as.obj->count;   /* objects coerce to size  */
    return atof(v.as.str);
}

/* growable string buffer helper */
static void sb_append(char **buf, size_t *len, size_t *cap, const char *s) {
    size_t n = strlen(s);
    if (*len + n + 1 > *cap) {
        *cap = (*len + n + 1) * 2;
        *buf = realloc(*buf, *cap);
        if (*buf == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    }
    memcpy(*buf + *len, s, n + 1);
    *len += n;
}

/* Render a value; array elements quote their strings so nesting stays readable. */
static void repr_into(char **buf, size_t *len, size_t *cap, Value v, int quote_str) {
    char tmp[64];
    switch (v.type) {
        case VAL_NUM:
            snprintf(tmp, sizeof tmp, "%.2f", v.as.num);
            sb_append(buf, len, cap, tmp);
            break;
        case VAL_STR:
            if (quote_str) sb_append(buf, len, cap, "\"");
            sb_append(buf, len, cap, v.as.str);
            if (quote_str) sb_append(buf, len, cap, "\"");
            break;
        case VAL_ARR: {
            int i;
            sb_append(buf, len, cap, "[");
            for (i = 0; i < v.as.arr->count; i++) {
                if (i) sb_append(buf, len, cap, ", ");
                repr_into(buf, len, cap, v.as.arr->items[i], 1);
            }
            sb_append(buf, len, cap, "]");
            break;
        }
        case VAL_OBJ: {
            MapEntry *e;
            int first = 1;
            sb_append(buf, len, cap, "{");
            for (e = v.as.obj->head; e; e = e->next) {
                if (!first) sb_append(buf, len, cap, ", ");
                first = 0;
                sb_append(buf, len, cap, e->key);
                sb_append(buf, len, cap, ": ");
                repr_into(buf, len, cap, *e->val, 1);
            }
            sb_append(buf, len, cap, "}");
            break;
        }
    }
}

/* Numbers use Savo's usual two-decimal format so existing output is unchanged. */
char *value_to_string(Value v) {
    if (v.type == VAL_STR) return xstrdup(v.as.str);
    {
        char  *buf = xstrdup("");
        size_t len = 0, cap = 1;
        repr_into(&buf, &len, &cap, v, 0);
        return buf;
    }
}

int value_truthy(Value v) {
    if (v.type == VAL_NUM) return v.as.num != 0;
    if (v.type == VAL_ARR) return v.as.arr->count != 0;
    if (v.type == VAL_OBJ) return v.as.obj->count != 0;
    return v.as.str[0] != '\0';
}

int value_is_str(Value v) { return v.type == VAL_STR; }

void value_print(Value v) {
    if (v.type == VAL_STR) { printf("%s", v.as.str); return; }
    {
        char *s = value_to_string(v);
        printf("%s", s);
        free(s);
    }
}
