#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include "ast.h"
#include "value.h"
#include "global.h"
#include "symtab.h"

extern int had_error;   /* defined in main.c; set when a runtime error occurs */

static void runtime_error(const char *msg) {
    if (savo_line > 0) fprintf(stderr, "savo: line %d: %s\n", savo_line, msg);
    else fprintf(stderr, "savo: %s\n", msg);
    had_error = 1;
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (p == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    return p;
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char  *p = xmalloc(n);
    memcpy(p, s, n);
    return p;
}

/* Uniform-ish random integer in [lo, hi], bounds swapped if reversed. Computed
 * in long so a wide int range cannot overflow (hi - lo + 1) as it did before. */
static double random_in_range(double dlo, double dhi) {
    long lo = (long) dlo, hi = (long) dhi, t;
    unsigned long span;
    if (hi < lo) { t = lo; lo = hi; hi = t; }
    span = (unsigned long) (hi - lo) + 1UL;
    return (double) (lo + (long) ((unsigned long) rand() % span));
}

/* Return handling: set by S_RETURN, checked by block/loop execution. */
static int   g_returning = 0;
static Value g_return_value = { VAL_NUM, { 0 } };

/* Loop control: set by savobreak/savocontinue, consumed by the nearest loop. */
enum { LOOP_NONE, LOOP_BREAK, LOOP_CONTINUE };
static int g_loop_signal = LOOP_NONE;

/* Registered user functions (each points at a retained S_FUNCDEF node). */
static Stmt *g_functions = NULL;

static Stmt *func_lookup(const char *name) {
    Stmt *f;
    for (f = g_functions; f != NULL; f = f->next)
        if (strcmp(f->str, name) == 0) return f;
    return NULL;
}

void func_define(Stmt *def) {
    def->next = g_functions;   /* reuse next to chain the table; defs live forever */
    g_functions = def;
}

/* Release every registered function at shutdown. The table is a plain ->next
 * chain of S_FUNCDEF nodes, so a single free_stmt frees them all (and their
 * bodies). Lets the process exit with no leaked definitions. */
void func_free_all(void) {
    Stmt *f = g_functions;
    g_functions = NULL;
    free_stmt(f);
}

/* Small builders for parameter / argument lists (used by the parser). */
Param *param_add(Param *list, char *name) {
    Param *p = xmalloc(sizeof(Param)), *tail;
    p->name = name; p->next = NULL;
    if (list == NULL) return p;
    for (tail = list; tail->next; tail = tail->next) ;
    tail->next = p;
    return list;
}

Arg *arg_add(Arg *list, Expr *e) {
    Arg *a = xmalloc(sizeof(Arg)), *tail;
    a->e = e; a->next = NULL;
    if (list == NULL) return a;
    for (tail = list; tail->next; tail = tail->next) ;
    tail->next = a;
    return list;
}

Pair *pair_add(Pair *list, char *key, Expr *val) {
    Pair *p = xmalloc(sizeof(Pair)), *tail;
    p->key = key; p->val = val; p->next = NULL;
    if (list == NULL) return p;
    for (tail = list; tail->next; tail = tail->next) ;
    tail->next = p;
    return list;
}

/* ============================ expressions ============================ */

static Expr *new_expr(ExprKind kind) {
    Expr *e = xmalloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

Expr *expr_num(double v)             { Expr *e = new_expr(E_NUM); e->as.num = v; return e; }
Expr *expr_bool(int b)               { Expr *e = new_expr(E_BOOL); e->as.num = b ? 1 : 0; return e; }
Expr *expr_nil(void)                 { Expr *e = new_expr(E_NIL); return e; }
Expr *expr_str(char *owned)          { Expr *e = new_expr(E_STR); e->as.str = owned; return e; }
Expr *expr_var(char *name)           { Expr *e = new_expr(E_VAR); e->as.var = name; return e; }
Expr *expr_neg(Expr *u)              { Expr *e = new_expr(E_NEG); e->as.unary = u; return e; }
Expr *expr_not(Expr *u)              { Expr *e = new_expr(E_NOT); e->as.unary = u; return e; }

Expr *expr_bin(BinOp op, Expr *l, Expr *r) {
    Expr *e = new_expr(E_BIN);
    e->as.bin.op = op; e->as.bin.l = l; e->as.bin.r = r;
    return e;
}

Expr *expr_call(Builtin fn, Expr *a, Expr *b, Expr *c) {
    Expr *e = new_expr(E_CALL);
    e->as.call.fn = fn; e->as.call.a = a; e->as.call.b = b; e->as.call.c = c;
    return e;
}

/* Flatten an Arg list into an Expr* array, freeing the Arg cells. */
static Expr **args_to_array(Arg *args, int *out_n) {
    int n = 0, i = 0;
    Arg *a;
    Expr **items;
    for (a = args; a != NULL; a = a->next) n++;
    items = n ? xmalloc(sizeof(Expr *) * n) : NULL;
    for (a = args; a != NULL; ) {
        Arg *next = a->next;
        items[i++] = a->e;
        free(a);
        a = next;
    }
    *out_n = n;
    return items;
}

Expr *expr_calluser(Expr *callee, Arg *args) {
    Expr *e = new_expr(E_CALLUSER);
    e->as.ucall.callee = callee;
    e->as.ucall.argv = args_to_array(args, &e->as.ucall.argc);
    return e;
}

Expr *expr_array(Arg *items) {
    Expr *e = new_expr(E_ARRAY);
    e->as.list.items = args_to_array(items, &e->as.list.count);
    return e;
}

Expr *expr_index(Expr *base, Expr *idx) {
    Expr *e = new_expr(E_INDEX);
    e->as.index.base = base; e->as.index.idx = idx;
    return e;
}

Expr *expr_object(Pair *pairs) {
    Expr *e = new_expr(E_OBJECT);
    int n = 0, i = 0;
    Pair *p;
    for (p = pairs; p != NULL; p = p->next) n++;
    e->as.object.count = n;
    e->as.object.keys = n ? xmalloc(sizeof(char *) * n) : NULL;
    e->as.object.vals = n ? xmalloc(sizeof(Expr *) * n) : NULL;
    for (p = pairs; p != NULL; ) {
        Pair *next = p->next;
        e->as.object.keys[i] = p->key;
        e->as.object.vals[i] = p->val;
        i++;
        free(p);
        p = next;
    }
    return e;
}

/* forward declarations */
void exec_stmt(const Stmt *s);
static double eval_num(const Expr *e);

/* Guards the C stack against unbounded (e.g. accidentally infinite) Savo
 * recursion, which would otherwise segfault the interpreter. */
#define MAX_CALL_DEPTH 1000
static int g_call_depth = 0;

/* Invoke a function with already-evaluated arguments (borrowed; the callee
 * keeps its own copies). Shared by named calls and the higher-order builtins. */
static Value invoke_func(Stmt *fn, Value *argv, int argc) {
    int i, saved_r, saved_sig;
    Value saved_v, rv;

    if (argc != fn->nparams) {
        runtime_error("wrong number of arguments in function call");
        return value_nil();
    }
    if (g_call_depth >= MAX_CALL_DEPTH) {
        runtime_error("maximum call depth exceeded (infinite recursion?)");
        return value_nil();
    }

    symtab_push_scope();
    for (i = 0; i < fn->nparams; i++) symtab_set(fn->params[i], argv[i]);

    saved_r = g_returning; saved_v = g_return_value; saved_sig = g_loop_signal;
    g_returning = 0; g_return_value = value_num(0); g_loop_signal = LOOP_NONE;
    g_call_depth++;
    exec_stmt(fn->body);
    g_call_depth--;
    rv = g_return_value;                 /* transfer ownership out */
    g_returning = saved_r; g_return_value = saved_v;
    g_loop_signal = saved_sig;           /* break/continue never crosses a call */

    symtab_pop_scope();
    return rv;
}

static Value call_user(const Expr *e) {
    Value callee = eval_expr(e->as.ucall.callee);
    Value rv, *values;
    Stmt *fn;
    int i;

    if (callee.type != VAL_FUNC) {
        runtime_error("attempt to call a value that is not a function");
        value_free(callee);
        return value_nil();
    }
    fn = (Stmt *) callee.as.func;

    /* Evaluate arguments in the caller's scope, before creating the new one. */
    values = e->as.ucall.argc ? xmalloc(sizeof(Value) * e->as.ucall.argc) : NULL;
    for (i = 0; i < e->as.ucall.argc; i++)
        values[i] = eval_expr(e->as.ucall.argv[i]);

    rv = invoke_func(fn, values, e->as.ucall.argc);

    for (i = 0; i < e->as.ucall.argc; i++) value_free(values[i]);
    free(values);
    value_free(callee);
    return rv;
}

/* Replace every occurrence of `old` in `s` with `rep`; returns a fresh string.
 * An empty `old` is returned unchanged to avoid an infinite match. */
static char *str_replace_all(const char *s, const char *old, const char *rep) {
    size_t oldlen = strlen(old), replen = strlen(rep);
    size_t cap, len = 0;
    char *out;
    const char *p;
    if (oldlen == 0) return xstrdup(s);
    cap = strlen(s) + 1;
    out = xmalloc(cap);
    for (p = s; *p; ) {
        if (strncmp(p, old, oldlen) == 0) {
            if (len + replen + 1 > cap) { cap = (len + replen + 1) * 2; out = realloc(out, cap); if (!out) { fprintf(stderr, "savo: out of memory\n"); exit(1); } }
            memcpy(out + len, rep, replen); len += replen; p += oldlen;
        } else {
            if (len + 2 > cap) { cap = (len + 2) * 2; out = realloc(out, cap); if (!out) { fprintf(stderr, "savo: out of memory\n"); exit(1); } }
            out[len++] = *p++;
        }
    }
    out[len] = 0;
    return out;
}

/* True if the array holds a value equal to `x` (numbers by value, strings by
 * content), or the string/object cases handled by the caller. */
static int array_contains(Value arr, Value x) {
    Array *a = arr.as.arr;
    int i;
    for (i = 0; i < a->count; i++) {
        Value el = a->items[i];
        if (el.type == VAL_STR && x.type == VAL_STR) { if (strcmp(el.as.str, x.as.str) == 0) return 1; }
        else if (el.type == VAL_NUM && x.type == VAL_NUM) { if (el.as.num == x.as.num) return 1; }
    }
    return 0;
}

/* Order two values: a comparator function if given, else numbers ascending and
 * strings lexicographically. Returns <0, 0 or >0. */
static Value invoke_func(Stmt *fn, Value *argv, int argc);
static int compare_values(Value x, Value y, Stmt *cmp) {
    if (cmp != NULL) {
        Value args[2]; double d;
        args[0] = x; args[1] = y;
        { Value r = invoke_func(cmp, args, 2); d = value_to_number(r); value_free(r); }
        return d < 0 ? -1 : (d > 0 ? 1 : 0);
    }
    if (x.type == VAL_STR && y.type == VAL_STR) return strcmp(x.as.str, y.as.str);
    { double a = value_to_number(x), b = value_to_number(y);
      return a < b ? -1 : (a > b ? 1 : 0); }
}

static Value read_input_line(Value prompt) {
    char  *line = NULL;
    size_t cap = 0;
    ssize_t got;
    if (prompt.type == VAL_STR && prompt.as.str[0] != '\0') { printf("%s", prompt.as.str); fflush(stdout); }
    got = getline(&line, &cap, stdin);
    if (got < 0) { free(line); return value_str_copy(""); }
    while (got > 0 && (line[got - 1] == '\n' || line[got - 1] == '\r')) line[--got] = 0;
    return value_str(line);   /* takes ownership of the getline buffer */
}

static Value call_builtin(Builtin fn, Value a, Value b, Value c) {
    switch (fn) {
        case FN_SQRT:  { double x = value_to_number(a); if (x < 0)  { runtime_error("sqrt of a negative value"); return value_num(0); } return value_num(sqrt(x)); }
        case FN_ABS:   return value_num(fabs(value_to_number(a)));
        case FN_FLOOR: return value_num(floor(value_to_number(a)));
        case FN_CEIL:  return value_num(ceil(value_to_number(a)));
        case FN_ROUND: return value_num(round(value_to_number(a)));
        case FN_LOG:   { double x = value_to_number(a); if (x <= 0) { runtime_error("log of a non-positive value"); return value_num(0); } return value_num(log(x)); }
        case FN_LOG10: { double x = value_to_number(a); if (x <= 0) { runtime_error("log10 of a non-positive value"); return value_num(0); } return value_num(log10(x)); }
        case FN_POW:   return value_num(pow(value_to_number(a), value_to_number(b)));
        case FN_MAX:   { double x = value_to_number(a), y = value_to_number(b); return value_num(x > y ? x : y); }
        case FN_MIN:   { double x = value_to_number(a), y = value_to_number(b); return value_num(x < y ? x : y); }
        case FN_RANDOM:
            return value_num(random_in_range(value_to_number(a), value_to_number(b)));
        case FN_LEN:   { if (a.type == VAL_ARR) return value_num(array_length(a)); if (a.type == VAL_OBJ) return value_num(object_length(a)); { char *s = value_to_string(a); double n = (double) strlen(s); free(s); return value_num(n); } }
        case FN_STR:   return value_str(value_to_string(a));
        case FN_NUM:   return value_num(value_to_number(a));
        case FN_UPPER: { char *s = value_to_string(a), *p; for (p = s; *p; p++) *p = (char) toupper((unsigned char) *p); return value_str(s); }
        case FN_LOWER: { char *s = value_to_string(a), *p; for (p = s; *p; p++) *p = (char) tolower((unsigned char) *p); return value_str(s); }
        case FN_TRIM: {
            char *s = value_to_string(a), *start = s, *end, *out;
            size_t n;
            while (*start && isspace((unsigned char) *start)) start++;
            end = start + strlen(start);
            while (end > start && isspace((unsigned char) end[-1])) end--;
            n = (size_t) (end - start);
            out = xmalloc(n + 1); memcpy(out, start, n); out[n] = 0;
            free(s);
            return value_str(out);
        }
        case FN_SUBSTR: {
            char *s = value_to_string(a), *out;
            int slen = (int) strlen(s);
            int start = (int) value_to_number(b);
            int len = (int) value_to_number(c);
            if (start < 0) start = 0;
            if (start > slen) start = slen;
            if (len < 0) len = 0;
            if (start + len > slen) len = slen - start;
            out = xmalloc((size_t) len + 1); memcpy(out, s + start, (size_t) len); out[len] = 0;
            free(s);
            return value_str(out);
        }
        case FN_INDEXOF: {
            char *s = value_to_string(a), *sub = value_to_string(b), *hit = strstr(s, sub);
            double idx = hit ? (double) (hit - s) : -1.0;
            free(s); free(sub);
            return value_num(idx);
        }
        case FN_REPLACE: {
            char *s = value_to_string(a), *o = value_to_string(b), *r = value_to_string(c);
            char *out = str_replace_all(s, o, r);
            free(s); free(o); free(r);
            return value_str(out);
        }
        case FN_SPLIT: {
            char *s = value_to_string(a), *sep = value_to_string(b);
            Value arr = value_array();
            size_t seplen = strlen(sep);
            if (seplen == 0) {
                char *p, ch[2]; ch[1] = 0;
                for (p = s; *p; p++) { Value v; ch[0] = *p; v = value_str_copy(ch); array_push(arr, v); value_free(v); }
            } else {
                char *seg = s, *hit;
                while ((hit = strstr(seg, sep)) != NULL) {
                    size_t n = (size_t) (hit - seg);
                    char *piece = xmalloc(n + 1); Value v;
                    memcpy(piece, seg, n); piece[n] = 0;
                    v = value_str(piece); array_push(arr, v); value_free(v);
                    seg = hit + seplen;
                }
                { Value v = value_str_copy(seg); array_push(arr, v); value_free(v); }
            }
            free(s); free(sep);
            return arr;
        }
        case FN_JOIN: {
            char *sep, *out;
            size_t len = 0, seplen;
            int i;
            Array *ar;
            if (a.type != VAL_ARR) { runtime_error("savojoin expects an array"); return value_str_copy(""); }
            sep = value_to_string(b); seplen = strlen(sep);
            out = xstrdup(""); ar = a.as.arr;
            for (i = 0; i < ar->count; i++) {
                char  *piece = value_to_string(ar->items[i]);
                size_t plen = strlen(piece);
                out = realloc(out, len + (i ? seplen : 0) + plen + 1);
                if (out == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
                if (i) { memcpy(out + len, sep, seplen); len += seplen; }
                memcpy(out + len, piece, plen); len += plen; out[len] = 0;
                free(piece);
            }
            free(sep);
            return value_str(out);
        }
        case FN_POP:
            if (a.type != VAL_ARR) { runtime_error("savopop expects an array"); return value_num(0); }
            return array_pop(a);
        case FN_CONTAINS:
            if (a.type == VAL_ARR) return value_num(array_contains(a, b));
            if (a.type == VAL_OBJ) {
                char *k = value_to_string(b); MapEntry *e; int found = 0;
                for (e = a.as.obj->head; e; e = e->next) if (strcmp(e->key, k) == 0) { found = 1; break; }
                free(k);
                return value_num(found);
            }
            { char *s = value_to_string(a), *sub = value_to_string(b); int f = strstr(s, sub) != NULL; free(s); free(sub); return value_num(f); }
        case FN_KEYS: {
            Value arr = value_array();
            if (a.type == VAL_OBJ) {
                MapEntry *e;
                for (e = a.as.obj->head; e; e = e->next) { Value v = value_str_copy(e->key); array_push(arr, v); value_free(v); }
            } else {
                runtime_error("savokeys expects an object");
            }
            return arr;
        }
        case FN_MAP: {
            Value out; int i;
            if (a.type != VAL_ARR) { runtime_error("savomap expects an array"); return value_nil(); }
            if (b.type != VAL_FUNC) { runtime_error("savomap expects a function"); return value_nil(); }
            out = value_array();
            for (i = 0; i < a.as.arr->count; i++) {
                Value arg = value_copy(a.as.arr->items[i]);
                Value r = invoke_func((Stmt *) b.as.func, &arg, 1);
                array_push(out, r);
                value_free(r); value_free(arg);
            }
            return out;
        }
        case FN_FILTER: {
            Value out; int i;
            if (a.type != VAL_ARR) { runtime_error("savofilter expects an array"); return value_nil(); }
            if (b.type != VAL_FUNC) { runtime_error("savofilter expects a function"); return value_nil(); }
            out = value_array();
            for (i = 0; i < a.as.arr->count; i++) {
                Value arg = value_copy(a.as.arr->items[i]);
                Value r = invoke_func((Stmt *) b.as.func, &arg, 1);
                if (value_truthy(r)) array_push(out, a.as.arr->items[i]);
                value_free(r); value_free(arg);
            }
            return out;
        }
        case FN_REDUCE: {
            Value acc; int i;
            if (a.type != VAL_ARR) { runtime_error("savoreduce expects an array"); return value_nil(); }
            if (b.type != VAL_FUNC) { runtime_error("savoreduce expects a function"); return value_nil(); }
            acc = value_copy(c);   /* initial accumulator */
            for (i = 0; i < a.as.arr->count; i++) {
                Value args[2];
                Value r;
                args[0] = acc; args[1] = value_copy(a.as.arr->items[i]);
                r = invoke_func((Stmt *) b.as.func, args, 2);
                value_free(acc); value_free(args[1]);
                acc = r;
            }
            return acc;
        }
        case FN_SORT: {
            Value out; int i, j;
            Stmt *cmp = (b.type == VAL_FUNC) ? (Stmt *) b.as.func : NULL;
            if (a.type != VAL_ARR) { runtime_error("savosort expects an array"); return value_nil(); }
            out = value_array();
            for (i = 0; i < a.as.arr->count; i++) { Value v = value_copy(a.as.arr->items[i]); array_push(out, v); value_free(v); }
            /* insertion sort: arrays are small in a scripting language */
            for (i = 1; i < out.as.arr->count; i++) {
                Value key = out.as.arr->items[i];
                j = i - 1;
                while (j >= 0 && compare_values(out.as.arr->items[j], key, cmp) > 0) {
                    out.as.arr->items[j + 1] = out.as.arr->items[j];
                    j--;
                }
                out.as.arr->items[j + 1] = key;
            }
            return out;
        }
        case FN_INPUT:
            return read_input_line(a);
    }
    return value_num(0);
}

static Value apply_binop(BinOp op, Value l, Value r) {
    /* '+' concatenates when either side is a string. */
    if (op == OP_ADD && (value_is_str(l) || value_is_str(r))) {
        char *ls = value_to_string(l), *rs = value_to_string(r);
        char *res = xmalloc(strlen(ls) + strlen(rs) + 1);
        strcpy(res, ls); strcat(res, rs);
        free(ls); free(rs);
        return value_str(res);
    }

    if (op >= OP_EQ) {   /* comparisons yield 1/0 */
        int c = 0;
        if (value_is_str(l) && value_is_str(r)) {
            int cmp = strcmp(l.as.str, r.as.str);
            switch (op) {
                case OP_EQ: c = cmp == 0; break;  case OP_NE: c = cmp != 0; break;
                case OP_LT: c = cmp <  0; break;  case OP_GT: c = cmp >  0; break;
                case OP_LE: c = cmp <= 0; break;  case OP_GE: c = cmp >= 0; break;
                default: break;
            }
        } else {
            double a = value_to_number(l), b = value_to_number(r);
            switch (op) {
                case OP_EQ: c = a == b; break;  case OP_NE: c = a != b; break;
                case OP_LT: c = a <  b; break;  case OP_GT: c = a >  b; break;
                case OP_LE: c = a <= b; break;  case OP_GE: c = a >= b; break;
                default: break;
            }
        }
        return value_bool(c);
    }

    {   /* numeric arithmetic */
        double a = value_to_number(l), b = value_to_number(r);
        switch (op) {
            case OP_ADD: return value_num(a + b);
            case OP_SUB: return value_num(a - b);
            case OP_MUL: return value_num(a * b);
            case OP_DIV: if (b == 0) { runtime_error("division by zero"); return value_num(0); } return value_num(a / b);
            case OP_MOD: if (b == 0) { runtime_error("modulo by zero"); return value_num(0); } return value_num(fmod(a, b));
            default: return value_num(0);
        }
    }
}

Value eval_expr(const Expr *e) {
    if (e == NULL) return value_num(0);
    switch (e->kind) {
        case E_NUM: return value_num(e->as.num);
        case E_BOOL: return value_bool((int) e->as.num);
        case E_NIL: return value_nil();
        case E_STR: return value_str_copy(e->as.str);
        case E_VAR:
            /* a name that is not a variable but is a defined function evaluates
             * to that function value, so functions can be passed as arguments */
            if (!symtab_has(e->as.var)) {
                Stmt *fn = func_lookup(e->as.var);
                if (fn != NULL) return value_func(fn);
            }
            return symtab_get(e->as.var);
        case E_NEG: { Value v = eval_expr(e->as.unary); Value r = value_num(-value_to_number(v)); value_free(v); return r; }
        case E_NOT: { Value v = eval_expr(e->as.unary); Value r = value_bool(!value_truthy(v)); value_free(v); return r; }
        case E_BIN: {
            Value l, r, res;
            /* Logical operators short-circuit: the right side is evaluated only
             * when the left does not already decide the result. */
            if (e->as.bin.op == OP_AND || e->as.bin.op == OP_OR) {
                int lt;
                l = eval_expr(e->as.bin.l);
                lt = value_truthy(l);
                value_free(l);
                if (e->as.bin.op == OP_AND && !lt) return value_bool(0);
                if (e->as.bin.op == OP_OR  &&  lt) return value_bool(1);
                r = eval_expr(e->as.bin.r);
                res = value_bool(value_truthy(r));
                value_free(r);
                return res;
            }
            l = eval_expr(e->as.bin.l);
            r = eval_expr(e->as.bin.r);
            res = apply_binop(e->as.bin.op, l, r);
            value_free(l); value_free(r);
            return res;
        }
        case E_CALL: {
            Value a = eval_expr(e->as.call.a);
            Value b = e->as.call.b ? eval_expr(e->as.call.b) : value_num(0);
            Value c = e->as.call.c ? eval_expr(e->as.call.c) : value_num(0);
            Value res = call_builtin(e->as.call.fn, a, b, c);
            value_free(a); value_free(b); value_free(c);
            return res;
        }
        case E_CALLUSER: return call_user(e);
        case E_ARRAY: {
            Value arr = value_array();
            int i;
            for (i = 0; i < e->as.list.count; i++) {
                Value el = eval_expr(e->as.list.items[i]);
                array_push(arr, el);
                value_free(el);
            }
            return arr;
        }
        case E_INDEX: {
            Value base = eval_expr(e->as.index.base);
            Value res;
            if (base.type == VAL_OBJ) {          /* object: key is a string */
                Value k = eval_expr(e->as.index.idx);
                char *key = value_to_string(k);
                res = object_get(base, key);
                free(key); value_free(k);
            } else if (base.type == VAL_ARR) {
                res = array_get(base, (int) eval_num(e->as.index.idx));
            } else if (base.type == VAL_STR) {   /* string indexing -> 1-char string */
                int i = (int) eval_num(e->as.index.idx);
                int len = (int) strlen(base.as.str);
                if (i < 0 || i >= len) { runtime_error("string index out of range"); res = value_str_copy(""); }
                else { char ch[2]; ch[0] = base.as.str[i]; ch[1] = 0; res = value_str_copy(ch); }
            } else { runtime_error("indexing a non-collection value"); res = value_num(0); }
            value_free(base);
            return res;
        }
        case E_OBJECT: {
            Value obj = value_object();
            int i;
            for (i = 0; i < e->as.object.count; i++) {
                Value v = eval_expr(e->as.object.vals[i]);
                object_set(obj, e->as.object.keys[i], v);
                value_free(v);
            }
            return obj;
        }
    }
    return value_num(0);
}

void free_expr(Expr *e) {
    if (e == NULL) return;
    switch (e->kind) {
        case E_STR:    free(e->as.str); break;
        case E_VAR:    free(e->as.var); break;
        case E_NEG:
        case E_NOT:    free_expr(e->as.unary); break;
        case E_BIN:    free_expr(e->as.bin.l); free_expr(e->as.bin.r); break;
        case E_CALL:   free_expr(e->as.call.a); free_expr(e->as.call.b); free_expr(e->as.call.c); break;
        case E_CALLUSER: {
            int i;
            for (i = 0; i < e->as.ucall.argc; i++) free_expr(e->as.ucall.argv[i]);
            free(e->as.ucall.argv);
            free_expr(e->as.ucall.callee);
            break;
        }
        case E_ARRAY: {
            int i;
            for (i = 0; i < e->as.list.count; i++) free_expr(e->as.list.items[i]);
            free(e->as.list.items);
            break;
        }
        case E_INDEX:  free_expr(e->as.index.base); free_expr(e->as.index.idx); break;
        case E_OBJECT: {
            int i;
            for (i = 0; i < e->as.object.count; i++) { free(e->as.object.keys[i]); free_expr(e->as.object.vals[i]); }
            free(e->as.object.keys);
            free(e->as.object.vals);
            break;
        }
        case E_NUM:
        case E_BOOL:
        case E_NIL:    break;
    }
    free(e);
}

/* helper: evaluate to a number and release the value */
static double eval_num(const Expr *e) {
    Value v = eval_expr(e);
    double n = value_to_number(v);
    value_free(v);
    return n;
}

/* helper: evaluate a condition to a truth value */
static int eval_truthy(const Expr *e) {
    Value v = eval_expr(e);
    int t = value_truthy(v);
    value_free(v);
    return t;
}

/* ============================ statements ============================ */

static Stmt *new_stmt(StmtKind kind) {
    Stmt *s = calloc(1, sizeof(Stmt));
    if (s == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    s->kind = kind;
    return s;
}

Stmt *stmt_print_expr(Expr *e)      { Stmt *s = new_stmt(S_PRINT_EXPR); s->a = e; return s; }
Stmt *stmt_assign(char *name, Expr *e, int echo) { Stmt *s = new_stmt(S_ASSIGN); s->str = name; s->a = e; s->flag = echo; return s; }
Stmt *stmt_arith(BinOp op, Expr *a, Expr *b) { Stmt *s = new_stmt(S_ARITH); s->op = op; s->a = a; s->b = b; return s; }
Stmt *stmt_math1(Builtin fn, Expr *a) { Stmt *s = new_stmt(S_MATH1); s->fn = fn; s->a = a; return s; }
Stmt *stmt_math2(Builtin fn, Expr *a, Expr *b) { Stmt *s = new_stmt(S_MATH2); s->fn = fn; s->a = a; s->b = b; return s; }
Stmt *stmt_random(Expr *a, Expr *b) { Stmt *s = new_stmt(S_RANDOM); s->a = a; s->b = b; return s; }
Stmt *stmt_repeat(Expr *count, char *str) { Stmt *s = new_stmt(S_REPEAT); s->a = count; s->str = str; return s; }
Stmt *stmt_dir(char *arg)           { Stmt *s = new_stmt(S_DIR); s->str = arg; return s; }
Stmt *stmt_simple(StmtKind kind)    { return new_stmt(kind); }
Stmt *stmt_pointer(char *str)       { Stmt *s = new_stmt(S_POINTER); s->str = str; return s; }
Stmt *stmt_if(Expr *cond, Stmt *thenb, Stmt *elseb) { Stmt *s = new_stmt(S_IF); s->a = cond; s->body = thenb; s->body2 = elseb; return s; }
Stmt *stmt_while(Expr *cond, Stmt *body) { Stmt *s = new_stmt(S_WHILE); s->a = cond; s->body = body; return s; }
Stmt *stmt_foreach(char *var, Expr *coll, Stmt *body) { Stmt *s = new_stmt(S_FOREACH); s->str = var; s->a = coll; s->body = body; return s; }
Stmt *stmt_return(Expr *e) { Stmt *s = new_stmt(S_RETURN); s->a = e; return s; }
Stmt *stmt_assert(Expr *cond, char *msg) { Stmt *s = new_stmt(S_ASSERT); s->a = cond; s->str = msg; return s; }
Stmt *stmt_push(char *name, Expr *e) { Stmt *s = new_stmt(S_PUSH); s->str = name; s->a = e; return s; }
Stmt *stmt_setindex(Expr *target, Expr *value) { Stmt *s = new_stmt(S_SETINDEX); s->a = target; s->b = value; return s; }
Stmt *stmt_block_new(void) { return new_stmt(S_BLOCK); }

void stmt_block_add(Stmt *block, Stmt *s) {
    if (s == NULL) return;
    if (block->body == NULL) { block->body = s; return; }
    {
        Stmt *p = block->body;
        while (p->next) p = p->next;
        p->next = s;
    }
}

Stmt *stmt_funcdef(char *name, Param *params, Stmt *body) {
    Stmt *s = new_stmt(S_FUNCDEF);
    int n = 0, i = 0;
    Param *p;
    for (p = params; p != NULL; p = p->next) n++;
    s->str = name;
    s->nparams = n;
    s->params = n ? xmalloc(sizeof(char *) * n) : NULL;
    for (p = params; p != NULL; ) {
        Param *next = p->next;
        s->params[i++] = p->name;
        free(p);
        p = next;
    }
    s->body = body;
    return s;
}

Stmt *stmt_forrange(Expr *a, Expr *b, Expr *step, char *str, ForMode mode, Expr *k) {
    Stmt *s = new_stmt(S_FORRANGE);
    s->a = a; s->b = b; s->c = step; s->str = str; s->mode = mode; s->d = k;
    return s;
}

static int interactive(void) { return strlen(prompt) > 0; }

/* List a directory without a shell, so a savodir/savols argument can never be
 * interpreted as a command (the old system("ls %s") allowed `savols ; rm -rf ~`).
 * An argument naming a directory lists it; otherwise it is treated as a glob
 * pattern (optionally with a leading directory) matched against entry names. */
static void list_dir(const char *arg) {
    const char *dir = ".";
    const char *pat = NULL;
    char        dirbuf[1024];
    char        argbuf[1024];
    struct dirent **names;
    int n, i;

    /* The lexer hands over the raw rest of the line, so a quoted argument such
     * as savols "*.md" arrives with its quotes; strip a matched surrounding
     * pair (the shell used to do this before we dropped system()). */
    if (arg != NULL) {
        size_t alen = strlen(arg);
        if (alen >= 2 && arg[0] == '"' && arg[alen - 1] == '"') {
            size_t inner = alen - 2;
            if (inner >= sizeof argbuf) inner = sizeof argbuf - 1;
            memcpy(argbuf, arg + 1, inner);
            argbuf[inner] = '\0';
            arg = argbuf;
        }
    }

    if (arg != NULL && *arg != '\0') {
        struct stat st;
        if (stat(arg, &st) == 0 && S_ISDIR(st.st_mode)) {
            dir = arg;
        } else {
            const char *slash = strrchr(arg, '/');
            if (slash != NULL) {
                size_t plen = (size_t) (slash - arg);
                if (plen >= sizeof dirbuf) plen = sizeof dirbuf - 1;
                memcpy(dirbuf, arg, plen);
                dirbuf[plen] = '\0';
                dir = dirbuf[0] ? dirbuf : "/";
                pat = slash + 1;
            } else {
                pat = arg;
            }
        }
    }

    n = scandir(dir, &names, NULL, alphasort);
    if (n < 0) { fprintf(stderr, "savo: cannot list '%s'\n", dir); return; }
    for (i = 0; i < n; i++) {
        const char *nm = names[i]->d_name;
        int show = pat ? fnmatch(pat, nm, 0) == 0 : nm[0] != '.';
        if (show) printf("%s\n", nm);
        free(names[i]);
    }
    free(names);
}

static void print_help(void) {
    printf("\n");
    printf("savoprint\t<expr>\t\t\t\tprint a value (string or number)\n");
    printf("savovar\t\t<@name> [=] <expr>\t\tdefine or update a variable\n");
    printf("savosum/subtract/moltiplication/divide/mod  <a> <b>\n");
    printf("savosqrt/abs/floor/ceil/round/log/log10  <value>\n");
    printf("savopow/max/min  <a> <b>\t\t\tbinary math\n");
    printf("savorandom\t<min> <max>\t\t\trandom integer\n");
    printf("savolen/upper/lower/str/num(<expr>)\t\tstring functions\n");
    printf("savoif\t\t(<cond>) .. [savoelse ..] savoend\tconditional block\n");
    printf("savowhile\t(<cond>) .. savoend\t\twhile loop\n");
    printf("savofor\t\t<n> <\"s\"> | (a,b,s) <\"s\"> [+|* k]\trepeat / counted loop\n");
    printf("savodef\t\tname(@a, @b) .. savoend\t\tdefine a function\n");
    printf("savoreturn\t<expr>\t\t\t\treturn a value from a function\n");
    printf("savoquit | savoexit\t\t\texit\n");
    printf("\nExpressions: + - * / %% , comparisons, ! and parentheses; '+' concatenates\n");
    printf("when either side is a string. Functions: savosqrt(x), savopow(a,b), ...\n\n");
}

void exec_stmt(const Stmt *s) {
    if (s == NULL) return;
    if (s->line > 0) savo_line = s->line;   /* track for runtime diagnostics */
    switch (s->kind) {
        case S_PRINT_EXPR: {
            Value v = eval_expr(s->a);
            value_print(v);
            if (value_is_str(v)) { if (interactive()) printf("\n"); }
            else printf("\n");
            value_free(v);
            break;
        }
        case S_ASSIGN: {
            Value v = eval_expr(s->a);
            symtab_set(s->str, v);
            if (s->flag) { printf("Variabile %s = ", s->str); value_print(v); printf("\n"); }
            value_free(v);
            break;
        }
        case S_ARITH: {
            double a = eval_num(s->a), b = eval_num(s->b);
            switch (s->op) {
                case OP_ADD: printf("%.2f\n", a + b); break;
                case OP_SUB: printf("%.2f\n", a - b); break;
                case OP_MUL: printf("%.2f\n", a * b); break;
                case OP_DIV: if (b == 0) runtime_error("division by zero"); else printf("%.2f\n", a / b); break;
                case OP_MOD: if (b == 0) runtime_error("modulo by zero"); else printf("%.2f\n", fmod(a, b)); break;
                default: break;
            }
            break;
        }
        case S_MATH1: {
            double v = eval_num(s->a);
            switch (s->fn) {
                case FN_SQRT:  if (v < 0) runtime_error("sqrt of a negative value"); else printf("√%.2f = %.2f\n", v, sqrt(v)); break;
                case FN_ABS:   printf("%.2f\n", fabs(v)); break;
                case FN_FLOOR: printf("%.2f\n", floor(v)); break;
                case FN_CEIL:  printf("%.2f\n", ceil(v)); break;
                case FN_ROUND: printf("%.2f\n", round(v)); break;
                case FN_LOG:   if (v <= 0) runtime_error("log of a non-positive value"); else printf("%.4f\n", log(v)); break;
                case FN_LOG10: if (v <= 0) runtime_error("log10 of a non-positive value"); else printf("%.4f\n", log10(v)); break;
                default: break;
            }
            break;
        }
        case S_MATH2: {
            double a = eval_num(s->a), b = eval_num(s->b);
            switch (s->fn) {
                case FN_POW: printf("%.2f^%.2f = %.2f\n", a, b, pow(a, b)); break;
                case FN_MAX: printf("max(%.2f, %.2f) = %.2f\n", a, b, a > b ? a : b); break;
                case FN_MIN: printf("min(%.2f, %.2f) = %.2f\n", a, b, a < b ? a : b); break;
                default: break;
            }
            break;
        }
        case S_RANDOM:
            printf("%d\n", (int) random_in_range(eval_num(s->a), eval_num(s->b)));
            break;
        case S_REPEAT: {
            int i, n = (int) eval_num(s->a);
            for (i = 0; i < n; i++) printf("%s\n", s->str);
            break;
        }
        case S_FORRANGE: {
            int i, from = (int) eval_num(s->a), to = (int) eval_num(s->b), step = (int) eval_num(s->c);
            if (step == 0) { runtime_error("for step must not be zero"); break; }
            for (i = from; step > 0 ? i < to : i > to; i += step) {
                if (s->mode == FOR_NONE)      printf("%s\n", s->str);
                else if (s->mode == FOR_PLUS) printf("%s%.0f\n", s->str, i + eval_num(s->d));
                else                          printf("%s%.0f\n", s->str, i * eval_num(s->d));
            }
            break;
        }
        case S_BLOCK: {
            Stmt *p;
            for (p = s->body; p != NULL && !g_returning && !g_loop_signal; p = p->next) exec_stmt(p);
            break;
        }
        case S_IF:
            if (eval_truthy(s->a)) exec_stmt(s->body);
            else if (s->body2) exec_stmt(s->body2);
            break;
        case S_WHILE:
            while (!g_returning && eval_truthy(s->a)) {
                exec_stmt(s->body);
                if (g_loop_signal == LOOP_BREAK) { g_loop_signal = LOOP_NONE; break; }
                if (g_loop_signal == LOOP_CONTINUE) g_loop_signal = LOOP_NONE;
            }
            break;
        case S_FOREACH: {
            Value coll = eval_expr(s->a);
            if (coll.type == VAL_ARR) {
                int i;
                for (i = 0; i < coll.as.arr->count && !g_returning; i++) {
                    Value el = value_copy(coll.as.arr->items[i]);
                    symtab_set(s->str, el);
                    value_free(el);
                    exec_stmt(s->body);
                    if (g_loop_signal == LOOP_BREAK) { g_loop_signal = LOOP_NONE; break; }
                    if (g_loop_signal == LOOP_CONTINUE) g_loop_signal = LOOP_NONE;
                }
            } else if (coll.type == VAL_OBJ) {
                MapEntry *e;
                for (e = coll.as.obj->head; e != NULL && !g_returning; e = e->next) {
                    Value k = value_str_copy(e->key);
                    symtab_set(s->str, k);
                    value_free(k);
                    exec_stmt(s->body);
                    if (g_loop_signal == LOOP_BREAK) { g_loop_signal = LOOP_NONE; break; }
                    if (g_loop_signal == LOOP_CONTINUE) g_loop_signal = LOOP_NONE;
                }
            } else {
                runtime_error("savoforeach expects an array or object");
            }
            value_free(coll);
            break;
        }
        case S_FUNCDEF:
            func_define((Stmt *) s);   /* retained by the function table */
            break;
        case S_RETURN:
            value_free(g_return_value);
            g_return_value = s->a ? eval_expr(s->a) : value_num(0);
            g_returning = 1;
            break;
        case S_BREAK:
            g_loop_signal = LOOP_BREAK;
            break;
        case S_CONTINUE:
            g_loop_signal = LOOP_CONTINUE;
            break;
        case S_ASSERT: {
            Value v = eval_expr(s->a);
            int ok = value_truthy(v);
            value_free(v);
            if (!ok) {
                char buf[300];
                if (s->str) snprintf(buf, sizeof buf, "assertion failed: %s", s->str);
                else        snprintf(buf, sizeof buf, "assertion failed");
                runtime_error(buf);
            }
            break;
        }
        case S_PUSH: {
            Value arr = symtab_get(s->str);   /* shares the array by reference */
            if (arr.type != VAL_ARR) runtime_error("savopush on a non-array variable");
            else { Value el = eval_expr(s->a); array_push(arr, el); value_free(el); }
            value_free(arr);
            break;
        }
        case S_SETINDEX: {
            /* The target is an index/field chain (E_INDEX). Evaluating its base
             * yields the innermost container shared by reference, so a single
             * set at the end mutates it in place at any nesting depth. */
            const Expr *target = s->a;
            Value el = eval_expr(s->b);
            Value container = eval_expr(target->as.index.base);
            if (container.type == VAL_OBJ) {
                Value k = eval_expr(target->as.index.idx);
                char *key = value_to_string(k);
                object_set(container, key, el);
                free(key); value_free(k);
            } else if (container.type == VAL_ARR) {
                array_set(container, (int) eval_num(target->as.index.idx), el);
            } else {
                runtime_error("savoset target is not an array or object");
            }
            value_free(el);
            value_free(container);
            break;
        }
        case S_DIR:
            if (interactive()) list_dir(s->str);
            break;
        case S_CLS:
        case S_CLEAR:
            /* ANSI erase-screen + cursor-home: portable and shell-free, unlike
             * system("cls") (which does not even exist on Unix). */
            if (interactive()) { printf("\033[2J\033[H%s", consoleMex); }
            break;
        case S_HELP:
            print_help();
            break;
        case S_QUIT:
            if (savo_exit_on_quit) exit(had_error);
            savo_quit_flag = 1;   /* embedded: stop this run without exiting */
            break;
        case S_POINTER:
            printf("%s: %p\n", s->str, (void *) s->str);
            break;
    }
}

void free_stmt(Stmt *s) {
    while (s != NULL) {
        Stmt *next = s->next;   /* free this statement and its block siblings */
        free(s->str);
        free(s->str2);
        free_expr(s->a);
        free_expr(s->b);
        free_expr(s->c);
        free_expr(s->d);
        free_stmt(s->body);
        free_stmt(s->body2);
        if (s->params) {
            int i;
            for (i = 0; i < s->nparams; i++) free(s->params[i]);
            free(s->params);
        }
        free(s);
        s = next;
    }
}
