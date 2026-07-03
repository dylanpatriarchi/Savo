#ifndef AST_H
#define AST_H

#include "value.h"

/*
 * Abstract syntax tree for Savo.
 *
 * The parser builds a tree of Stmt/Expr nodes instead of executing inline.
 * Each top-level statement is executed (exec_stmt) and freed (free_stmt) as
 * soon as it is fully parsed, which keeps the REPL responsive while still
 * allowing compound statements (blocks, loops, functions) to be captured and
 * run as a unit. Expressions evaluate to a dynamically-typed Value.
 */

/* ---------- expressions ---------- */

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE
} BinOp;

/* Built-in functions usable both as commands and inside expressions. */
typedef enum {
    FN_SQRT, FN_ABS, FN_FLOOR, FN_CEIL, FN_ROUND, FN_LOG, FN_LOG10, /* num, unary  */
    FN_POW, FN_MAX, FN_MIN, FN_RANDOM,                              /* num, binary */
    FN_LEN, FN_UPPER, FN_LOWER, FN_STR, FN_NUM                      /* string ops  */
} Builtin;

typedef enum {
    E_NUM, E_STR, E_VAR, E_BIN, E_NEG, E_NOT, E_CALL, E_CALLUSER,
    E_ARRAY, E_INDEX, E_OBJECT
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        double num;                                   /* E_NUM  */
        char  *str;                                   /* E_STR (owned literal) */
        char  *var;                                   /* E_VAR  */
        struct { BinOp op; struct Expr *l, *r; } bin; /* E_BIN  */
        struct Expr *unary;                           /* E_NEG / E_NOT */
        struct { Builtin fn; struct Expr *a, *b; } call; /* E_CALL (b may be NULL) */
        struct { char *name; struct Expr **argv; int argc; } ucall; /* E_CALLUSER */
        struct { struct Expr **items; int count; } list; /* E_ARRAY */
        struct { struct Expr *base, *idx; } index;    /* E_INDEX (array/string/object) */
        struct { char **keys; struct Expr **vals; int count; } object; /* E_OBJECT */
    } as;
} Expr;

/* Builders for comma-separated parameter / argument / key-value lists. */
typedef struct Param { char *name; struct Param *next; } Param;
typedef struct Arg   { Expr *e;    struct Arg   *next; } Arg;
typedef struct Pair  { char *key;  Expr *val; struct Pair *next; } Pair;
Param *param_add(Param *list, char *name);   /* append name, return head */
Arg   *arg_add(Arg *list, Expr *e);          /* append e, return head */
Pair  *pair_add(Pair *list, char *key, Expr *val); /* append key:val, return head */

Expr *expr_num(double v);
Expr *expr_str(char *owned);           /* takes ownership of the literal */
Expr *expr_var(char *name);            /* takes ownership of name */
Expr *expr_bin(BinOp op, Expr *l, Expr *r);
Expr *expr_neg(Expr *e);
Expr *expr_not(Expr *e);
Expr *expr_call(Builtin fn, Expr *a, Expr *b);
Expr *expr_calluser(char *name, Arg *args);    /* user function call */
Expr *expr_array(Arg *items);                  /* array literal [ ... ] */
Expr *expr_index(Expr *base, Expr *idx);       /* subscript base[idx] / base.key */
Expr *expr_object(Pair *pairs);                /* object literal { k: v, ... } */

Value eval_expr(const Expr *e);        /* caller owns the returned Value */
void  free_expr(Expr *e);

/* ---------- statements ---------- */

typedef enum {
    S_PRINT_EXPR,  /* print the value of an expression                     */
    S_ASSIGN,      /* savovar: define/update a variable (flag = echo)      */
    S_ARITH,       /* savosum/subtract/moltiplication/divide/mod           */
    S_MATH1,       /* unary math command (sqrt/abs/floor/ceil/round/log..) */
    S_MATH2,       /* binary math command (pow/max/min)                    */
    S_RANDOM,      /* savorandom                                           */
    S_REPEAT,      /* savofor N "s" and savowhile N "s"                    */
    S_FORRANGE,    /* savofor (a,b,s) "str" [+|* k]                        */
    S_BLOCK,       /* a sequence of statements (body of if/while)          */
    S_IF,          /* savoif (cond) ... [savoelse ...] savoend             */
    S_WHILE,       /* savowhile (cond) ... savoend                         */
    S_FUNCDEF,     /* savodef name(params) ... savoend                     */
    S_RETURN,      /* savoreturn [expr]                                    */
    S_PUSH,        /* savopush @a <expr>                                   */
    S_SETINDEX,    /* savoset @a[i] = <expr>                               */
    S_DIR,         /* savodir / savols                                     */
    S_CLS, S_CLEAR, S_HELP, S_QUIT, S_POINTER
} StmtKind;

/* trailing-operation mode for S_FORRANGE */
typedef enum { FOR_NONE, FOR_PLUS, FOR_MUL } ForMode;

typedef struct Stmt {
    StmtKind kind;
    /* generic slots reused per kind; see constructors for meaning */
    char        *str;
    char        *str2;
    Expr        *a;
    Expr        *b;
    Expr        *c;
    Expr        *d;
    BinOp        op;
    Builtin      fn;
    ForMode      mode;
    int          flag;    /* S_ASSIGN: echo the assignment */
    struct Stmt *body;    /* S_IF/S_WHILE body, S_BLOCK head, S_FUNCDEF body */
    struct Stmt *body2;   /* S_IF else-branch */
    struct Stmt *next;    /* next statement in a block */
    char       **params;  /* S_FUNCDEF parameter names */
    int          nparams; /* S_FUNCDEF parameter count */
} Stmt;

Stmt *stmt_print_expr(Expr *e);
Stmt *stmt_assign(char *name, Expr *e, int echo);
Stmt *stmt_block_new(void);
void  stmt_block_add(Stmt *block, Stmt *s);   /* appends s to block */
Stmt *stmt_if(Expr *cond, Stmt *thenb, Stmt *elseb /*nullable*/);
Stmt *stmt_while(Expr *cond, Stmt *body);
Stmt *stmt_funcdef(char *name, Param *params, Stmt *body);
Stmt *stmt_return(Expr *e /*nullable*/);
Stmt *stmt_push(char *name, Expr *e);           /* savopush @a <expr> */
Stmt *stmt_setindex(char *name, Expr *idx, Expr *e); /* savoset @a[i] = <expr> */

/* Register a function definition (the interpreter keeps ownership). */
void  func_define(Stmt *def);
Stmt *stmt_arith(BinOp op, Expr *a, Expr *b);
Stmt *stmt_math1(Builtin fn, Expr *a);
Stmt *stmt_math2(Builtin fn, Expr *a, Expr *b);
Stmt *stmt_random(Expr *a, Expr *b);
Stmt *stmt_repeat(Expr *count, char *s);
Stmt *stmt_forrange(Expr *a, Expr *b, Expr *step, char *s, ForMode mode, Expr *k);
Stmt *stmt_dir(char *arg /*nullable*/);
Stmt *stmt_simple(StmtKind kind);      /* S_CLS / S_CLEAR / S_HELP / S_QUIT */
Stmt *stmt_pointer(char *s);

void exec_stmt(const Stmt *s);
void free_stmt(Stmt *s);

#endif
