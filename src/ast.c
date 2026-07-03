#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ast.h"
#include "global.h"
#include "symtab.h"

extern int had_error;   /* defined in the lexer; set when a runtime error occurs */

static void runtime_error(const char *msg) {
    fprintf(stderr, "savo: %s\n", msg);
    had_error = 1;
}

/* ============================ expressions ============================ */

static Expr *new_expr(ExprKind kind) {
    Expr *e = malloc(sizeof(Expr));
    if (e == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    e->kind = kind;
    return e;
}

Expr *expr_num(float v)              { Expr *e = new_expr(E_NUM); e->as.num = v; return e; }
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

Expr *expr_strcmp(int ne, char *a, char *b) {
    Expr *e = new_expr(E_STRCMP);
    e->as.strcmp.ne = ne; e->as.strcmp.a = a; e->as.strcmp.b = b;
    return e;
}

static float call_builtin(Builtin fn, float a, float b) {
    switch (fn) {
        case FN_SQRT:  if (a < 0)  { runtime_error("sqrt of a negative value"); return 0; } return sqrtf(a);
        case FN_ABS:   return fabsf(a);
        case FN_FLOOR: return floorf(a);
        case FN_CEIL:  return ceilf(a);
        case FN_ROUND: return roundf(a);
        case FN_LOG:   if (a <= 0) { runtime_error("log of a non-positive value"); return 0; } return logf(a);
        case FN_LOG10: if (a <= 0) { runtime_error("log10 of a non-positive value"); return 0; } return log10f(a);
        case FN_POW:   return powf(a, b);
        case FN_MAX:   return a > b ? a : b;
        case FN_MIN:   return a < b ? a : b;
        case FN_RANDOM: {
            int lo = (int) a, hi = (int) b, t;
            if (hi < lo) { t = lo; lo = hi; hi = t; }
            return (float) (lo + rand() % (hi - lo + 1));
        }
    }
    return 0;
}

float eval_expr(const Expr *e) {
    if (e == NULL) return 0;
    switch (e->kind) {
        case E_NUM: return e->as.num;
        case E_VAR: return symtab_get(e->as.var);
        case E_NEG: return -eval_expr(e->as.unary);
        case E_NOT: return eval_expr(e->as.unary) == 0 ? 1.0f : 0.0f;
        case E_STRCMP: {
            int cmp = strcmp(e->as.strcmp.a, e->as.strcmp.b);
            int truth = e->as.strcmp.ne ? (cmp != 0) : (cmp == 0);
            return truth ? 1.0f : 0.0f;
        }
        case E_CALL: {
            float a = eval_expr(e->as.call.a);
            float b = e->as.call.b ? eval_expr(e->as.call.b) : 0;
            return call_builtin(e->as.call.fn, a, b);
        }
        case E_BIN: {
            float l = eval_expr(e->as.bin.l);
            float r = eval_expr(e->as.bin.r);
            switch (e->as.bin.op) {
                case OP_ADD: return l + r;
                case OP_SUB: return l - r;
                case OP_MUL: return l * r;
                case OP_DIV: if (r == 0) { runtime_error("division by zero"); return 0; } return l / r;
                case OP_MOD: if (r == 0) { runtime_error("modulo by zero"); return 0; } return fmodf(l, r);
                case OP_EQ:  return l == r ? 1.0f : 0.0f;
                case OP_NE:  return l != r ? 1.0f : 0.0f;
                case OP_LT:  return l <  r ? 1.0f : 0.0f;
                case OP_GT:  return l >  r ? 1.0f : 0.0f;
                case OP_LE:  return l <= r ? 1.0f : 0.0f;
                case OP_GE:  return l >= r ? 1.0f : 0.0f;
            }
        }
    }
    return 0;
}

void free_expr(Expr *e) {
    if (e == NULL) return;
    switch (e->kind) {
        case E_VAR:    free(e->as.var); break;
        case E_NEG:
        case E_NOT:    free_expr(e->as.unary); break;
        case E_BIN:    free_expr(e->as.bin.l); free_expr(e->as.bin.r); break;
        case E_CALL:   free_expr(e->as.call.a); free_expr(e->as.call.b); break;
        case E_STRCMP: free(e->as.strcmp.a); free(e->as.strcmp.b); break;
        case E_NUM:    break;
    }
    free(e);
}

/* ============================ statements ============================ */

static Stmt *new_stmt(StmtKind kind) {
    Stmt *s = calloc(1, sizeof(Stmt));
    if (s == NULL) { fprintf(stderr, "savo: out of memory\n"); exit(1); }
    s->kind = kind;
    return s;
}

Stmt *stmt_print_str(char *str, Expr *trailing) {
    Stmt *s = new_stmt(S_PRINT_STR); s->str = str; s->a = trailing; return s;
}
Stmt *stmt_print_expr(Expr *e)      { Stmt *s = new_stmt(S_PRINT_EXPR); s->a = e; return s; }
Stmt *stmt_assign(char *name, Expr *e) { Stmt *s = new_stmt(S_ASSIGN); s->str = name; s->a = e; return s; }
Stmt *stmt_arith(BinOp op, Expr *a, Expr *b) { Stmt *s = new_stmt(S_ARITH); s->op = op; s->a = a; s->b = b; return s; }
Stmt *stmt_math1(Builtin fn, Expr *a) { Stmt *s = new_stmt(S_MATH1); s->fn = fn; s->a = a; return s; }
Stmt *stmt_math2(Builtin fn, Expr *a, Expr *b) { Stmt *s = new_stmt(S_MATH2); s->fn = fn; s->a = a; s->b = b; return s; }
Stmt *stmt_random(Expr *a, Expr *b) { Stmt *s = new_stmt(S_RANDOM); s->a = a; s->b = b; return s; }
Stmt *stmt_ifprint(Expr *cond)      { Stmt *s = new_stmt(S_IFPRINT); s->a = cond; return s; }
Stmt *stmt_repeat(Expr *count, char *str) { Stmt *s = new_stmt(S_REPEAT); s->a = count; s->str = str; return s; }
Stmt *stmt_dir(char *arg)           { Stmt *s = new_stmt(S_DIR); s->str = arg; return s; }
Stmt *stmt_simple(StmtKind kind)    { return new_stmt(kind); }
Stmt *stmt_pointer(char *str)       { Stmt *s = new_stmt(S_POINTER); s->str = str; return s; }

Stmt *stmt_forrange(Expr *a, Expr *b, Expr *step, char *str, ForMode mode, Expr *k) {
    Stmt *s = new_stmt(S_FORRANGE);
    s->a = a; s->b = b; s->c = step; s->str = str; s->mode = mode; s->d = k;
    return s;
}

static int interactive(void) { return strlen(prompt) > 0; }

static void print_help(void) {
    printf("\n");
    printf("savoprint\t<\"string\"> | <value> | <\"string\"> + <value>\n");
    printf("savovar\t\t<@name> [=] <expr>\t\tdefine or update a variable\n");
    printf("savosum/subtract/moltiplication/divide/mod  <a> <b>\n");
    printf("savosqrt/abs/floor/ceil/round/log/log10  <value>\n");
    printf("savopow/max/min  <a> <b>\t\t\tbinary math\n");
    printf("savorandom\t<min> <max>\t\t\trandom integer\n");
    printf("savoif\t\t(<expr>)\t\t\tprint true/false\n");
    printf("savofor\t\t<n> <\"s\"> | (a,b,s) <\"s\"> [+|* k]\n");
    printf("savowhile\t<n> <\"s\">\t\t\trepeat a string\n");
    printf("savodir/savols\t[arg]\t\t\t\tlist files\n");
    printf("savocls/savoclear\t\t\t\tclear screen\n");
    printf("savopointercell\t<\"string\">\t\tprint a memory address\n");
    printf("savoquit | savoexit\t\t\texit\n");
    printf("\nExpressions support + - * / %% , comparisons, ! and parentheses,\n");
    printf("plus function calls like savosqrt(x) and savopow(a, b).\n\n");
}

void exec_stmt(const Stmt *s) {
    if (s == NULL) return;
    switch (s->kind) {
        case S_PRINT_STR:
            printf("%s", s->str);
            if (s->a) printf("%.2f", eval_expr(s->a));
            if (interactive()) printf("\n");
            break;
        case S_PRINT_EXPR:
            printf("%.2f", eval_expr(s->a));
            if (interactive()) printf("\n");
            break;
        case S_ASSIGN: {
            float v = eval_expr(s->a);
            symtab_set(s->str, v);
            printf("Variabile %s = %.2f\n", s->str, v);
            break;
        }
        case S_ARITH: {
            float a = eval_expr(s->a), b = eval_expr(s->b);
            switch (s->op) {
                case OP_ADD: printf("%.2f\n", a + b); break;
                case OP_SUB: printf("%.2f\n", a - b); break;
                case OP_MUL: printf("%.2f\n", a * b); break;
                case OP_DIV: if (b == 0) runtime_error("division by zero"); else printf("%.2f\n", a / b); break;
                case OP_MOD: if (b == 0) runtime_error("modulo by zero"); else printf("%.2f\n", fmodf(a, b)); break;
                default: break;
            }
            break;
        }
        case S_MATH1: {
            float v = eval_expr(s->a);
            switch (s->fn) {
                case FN_SQRT:  if (v < 0) runtime_error("sqrt of a negative value"); else printf("√%.2f = %.2f\n", v, sqrtf(v)); break;
                case FN_ABS:   printf("%.2f\n", fabsf(v)); break;
                case FN_FLOOR: printf("%.2f\n", floorf(v)); break;
                case FN_CEIL:  printf("%.2f\n", ceilf(v)); break;
                case FN_ROUND: printf("%.2f\n", roundf(v)); break;
                case FN_LOG:   if (v <= 0) runtime_error("log of a non-positive value"); else printf("%.4f\n", logf(v)); break;
                case FN_LOG10: if (v <= 0) runtime_error("log10 of a non-positive value"); else printf("%.4f\n", log10f(v)); break;
                default: break;
            }
            break;
        }
        case S_MATH2: {
            float a = eval_expr(s->a), b = eval_expr(s->b);
            switch (s->fn) {
                case FN_POW: printf("%.2f^%.2f = %.2f\n", a, b, powf(a, b)); break;
                case FN_MAX: printf("max(%.2f, %.2f) = %.2f\n", a, b, a > b ? a : b); break;
                case FN_MIN: printf("min(%.2f, %.2f) = %.2f\n", a, b, a < b ? a : b); break;
                default: break;
            }
            break;
        }
        case S_RANDOM: {
            int lo = (int) eval_expr(s->a), hi = (int) eval_expr(s->b), t;
            if (hi < lo) { t = lo; lo = hi; hi = t; }
            printf("%d\n", lo + rand() % (hi - lo + 1));
            break;
        }
        case S_IFPRINT:
            printf("%s\n", eval_expr(s->a) != 0 ? "true" : "false");
            break;
        case S_REPEAT: {
            int i, n = (int) eval_expr(s->a);
            for (i = 0; i < n; i++) printf("%s\n", s->str);
            break;
        }
        case S_FORRANGE: {
            int i, from = (int) eval_expr(s->a), to = (int) eval_expr(s->b), step = (int) eval_expr(s->c);
            if (step == 0) { runtime_error("for step must not be zero"); break; }
            for (i = from; step > 0 ? i < to : i > to; i += step) {
                if (s->mode == FOR_NONE)      printf("%s\n", s->str);
                else if (s->mode == FOR_PLUS) printf("%s%.0f\n", s->str, i + eval_expr(s->d));
                else                          printf("%s%.0f\n", s->str, i * eval_expr(s->d));
            }
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
    if (s == NULL) return;
    free(s->str);
    free(s->str2);
    free_expr(s->a);
    free_expr(s->b);
    free_expr(s->c);
    free_expr(s->d);
    free(s);
}
