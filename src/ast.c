#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ast.h"
#include "value.h"
#include "global.h"
#include "symtab.h"

extern int had_error;   /* defined in the lexer; set when a runtime error occurs */

static void runtime_error(const char *msg) {
    fprintf(stderr, "savo: %s\n", msg);
    had_error = 1;
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (p == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    return p;
}

/* Return handling: set by S_RETURN, checked by block/loop execution. */
static int   g_returning = 0;
static Value g_return_value = { VAL_NUM, { 0 } };

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

/* ============================ expressions ============================ */

static Expr *new_expr(ExprKind kind) {
    Expr *e = xmalloc(sizeof(Expr));
    e->kind = kind;
    return e;
}

Expr *expr_num(double v)             { Expr *e = new_expr(E_NUM); e->as.num = v; return e; }
Expr *expr_str(char *owned)          { Expr *e = new_expr(E_STR); e->as.str = owned; return e; }
Expr *expr_var(char *name)           { Expr *e = new_expr(E_VAR); e->as.var = name; return e; }
Expr *expr_neg(Expr *u)              { Expr *e = new_expr(E_NEG); e->as.unary = u; return e; }
Expr *expr_not(Expr *u)              { Expr *e = new_expr(E_NOT); e->as.unary = u; return e; }

Expr *expr_bin(BinOp op, Expr *l, Expr *r) {
    Expr *e = new_expr(E_BIN);
    e->as.bin.op = op; e->as.bin.l = l; e->as.bin.r = r;
    return e;
}

Expr *expr_call(Builtin fn, Expr *a, Expr *b) {
    Expr *e = new_expr(E_CALL);
    e->as.call.fn = fn; e->as.call.a = a; e->as.call.b = b;
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

Expr *expr_calluser(char *name, Arg *args) {
    Expr *e = new_expr(E_CALLUSER);
    e->as.ucall.name = name;
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

/* forward declarations */
void exec_stmt(const Stmt *s);
static double eval_num(const Expr *e);

static Value call_user(const Expr *e) {
    Stmt *fn = func_lookup(e->as.ucall.name);
    int i, saved_r;
    Value saved_v, rv, *values;

    if (fn == NULL) { runtime_error("call to undefined function"); return value_num(0); }
    if (e->as.ucall.argc != fn->nparams) {
        runtime_error("wrong number of arguments in function call");
        return value_num(0);
    }

    /* Evaluate arguments in the caller's scope, before creating the new one. */
    values = e->as.ucall.argc ? xmalloc(sizeof(Value) * e->as.ucall.argc) : NULL;
    for (i = 0; i < e->as.ucall.argc; i++)
        values[i] = eval_expr(e->as.ucall.argv[i]);

    symtab_push_scope();
    for (i = 0; i < fn->nparams; i++) {
        symtab_set(fn->params[i], values[i]);
        value_free(values[i]);
    }
    free(values);

    saved_r = g_returning; saved_v = g_return_value;
    g_returning = 0; g_return_value = value_num(0);
    exec_stmt(fn->body);
    rv = g_return_value;                 /* transfer ownership out */
    g_returning = saved_r; g_return_value = saved_v;

    symtab_pop_scope();
    return rv;
}

static Value call_builtin(Builtin fn, Value a, Value b) {
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
        case FN_RANDOM: {
            int lo = (int) value_to_number(a), hi = (int) value_to_number(b), t;
            if (hi < lo) { t = lo; lo = hi; hi = t; }
            return value_num(lo + rand() % (hi - lo + 1));
        }
        case FN_LEN:   { if (a.type == VAL_ARR) return value_num(array_length(a)); { char *s = value_to_string(a); double n = (double) strlen(s); free(s); return value_num(n); } }
        case FN_STR:   return value_str(value_to_string(a));
        case FN_NUM:   return value_num(value_to_number(a));
        case FN_UPPER: { char *s = value_to_string(a), *p; for (p = s; *p; p++) *p = (char) toupper((unsigned char) *p); return value_str(s); }
        case FN_LOWER: { char *s = value_to_string(a), *p; for (p = s; *p; p++) *p = (char) tolower((unsigned char) *p); return value_str(s); }
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
        return value_num(c ? 1 : 0);
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
        case E_STR: return value_str_copy(e->as.str);
        case E_VAR: return symtab_get(e->as.var);
        case E_NEG: { Value v = eval_expr(e->as.unary); Value r = value_num(-value_to_number(v)); value_free(v); return r; }
        case E_NOT: { Value v = eval_expr(e->as.unary); Value r = value_num(value_truthy(v) ? 0 : 1); value_free(v); return r; }
        case E_BIN: {
            Value l = eval_expr(e->as.bin.l), r = eval_expr(e->as.bin.r);
            Value res = apply_binop(e->as.bin.op, l, r);
            value_free(l); value_free(r);
            return res;
        }
        case E_CALL: {
            Value a = eval_expr(e->as.call.a);
            Value b = e->as.call.b ? eval_expr(e->as.call.b) : value_num(0);
            Value res = call_builtin(e->as.call.fn, a, b);
            value_free(a); value_free(b);
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
            int i = (int) eval_num(e->as.index.idx);
            Value res;
            if (base.type == VAL_ARR) res = array_get(base, i);
            else if (base.type == VAL_STR) {   /* string indexing -> 1-char string */
                int len = (int) strlen(base.as.str);
                if (i < 0 || i >= len) { runtime_error("string index out of range"); res = value_str_copy(""); }
                else { char ch[2]; ch[0] = base.as.str[i]; ch[1] = 0; res = value_str_copy(ch); }
            } else { runtime_error("indexing a non-array value"); res = value_num(0); }
            value_free(base);
            return res;
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
        case E_CALL:   free_expr(e->as.call.a); free_expr(e->as.call.b); break;
        case E_CALLUSER: {
            int i;
            for (i = 0; i < e->as.ucall.argc; i++) free_expr(e->as.ucall.argv[i]);
            free(e->as.ucall.argv);
            free(e->as.ucall.name);
            break;
        }
        case E_ARRAY: {
            int i;
            for (i = 0; i < e->as.list.count; i++) free_expr(e->as.list.items[i]);
            free(e->as.list.items);
            break;
        }
        case E_INDEX:  free_expr(e->as.index.base); free_expr(e->as.index.idx); break;
        case E_NUM:    break;
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
Stmt *stmt_return(Expr *e) { Stmt *s = new_stmt(S_RETURN); s->a = e; return s; }
Stmt *stmt_push(char *name, Expr *e) { Stmt *s = new_stmt(S_PUSH); s->str = name; s->a = e; return s; }
Stmt *stmt_setindex(char *name, Expr *idx, Expr *e) { Stmt *s = new_stmt(S_SETINDEX); s->str = name; s->a = idx; s->b = e; return s; }
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
        case S_RANDOM: {
            int lo = (int) eval_num(s->a), hi = (int) eval_num(s->b), t;
            if (hi < lo) { t = lo; lo = hi; hi = t; }
            printf("%d\n", lo + rand() % (hi - lo + 1));
            break;
        }
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
            for (p = s->body; p != NULL && !g_returning; p = p->next) exec_stmt(p);
            break;
        }
        case S_IF:
            if (eval_truthy(s->a)) exec_stmt(s->body);
            else if (s->body2) exec_stmt(s->body2);
            break;
        case S_WHILE:
            while (!g_returning && eval_truthy(s->a)) exec_stmt(s->body);
            break;
        case S_FUNCDEF:
            func_define((Stmt *) s);   /* retained by the function table */
            break;
        case S_RETURN:
            value_free(g_return_value);
            g_return_value = s->a ? eval_expr(s->a) : value_num(0);
            g_returning = 1;
            break;
        case S_PUSH: {
            Value arr = symtab_get(s->str);   /* shares the array by reference */
            if (arr.type != VAL_ARR) runtime_error("savopush on a non-array variable");
            else { Value el = eval_expr(s->a); array_push(arr, el); value_free(el); }
            value_free(arr);
            break;
        }
        case S_SETINDEX: {
            Value arr = symtab_get(s->str);
            if (arr.type != VAL_ARR) runtime_error("savoset index on a non-array variable");
            else { int i = (int) eval_num(s->a); Value el = eval_expr(s->b); array_set(arr, i, el); value_free(el); }
            value_free(arr);
            break;
        }
        case S_DIR:
            if (interactive()) {
                if (s->str) { char cmd[600]; snprintf(cmd, sizeof cmd, "ls %s", s->str); system(cmd); }
                else system("ls");
            }
            break;
        case S_CLS:
            if (interactive()) { system("cls"); printf("%s", consoleMex); }
            break;
        case S_CLEAR:
            if (interactive()) { system("clear"); printf("%s", consoleMex); }
            break;
        case S_HELP:
            print_help();
            break;
        case S_QUIT:
            exit(had_error);
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
