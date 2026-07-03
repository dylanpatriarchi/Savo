#ifndef AST_H
#define AST_H

/*
 * Abstract syntax tree for Savo.
 *
 * The parser builds a tree of Stmt/Expr nodes instead of executing inline.
 * Each top-level statement is executed (exec_stmt) and freed (free_stmt) as
 * soon as it is fully parsed, which keeps the REPL responsive while still
 * allowing compound statements (blocks, loops, functions) to be captured and
 * run as a unit.
 */

/* ---------- expressions (numeric) ---------- */

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE
} BinOp;

/* Built-in functions usable both as commands and inside expressions. */
typedef enum {
    FN_SQRT, FN_ABS, FN_FLOOR, FN_CEIL, FN_ROUND, FN_LOG, FN_LOG10, /* unary  */
    FN_POW, FN_MAX, FN_MIN, FN_RANDOM                                /* binary */
} Builtin;

typedef enum {
    E_NUM, E_VAR, E_BIN, E_NEG, E_NOT, E_CALL, E_STRCMP
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        float  num;                                   /* E_NUM  */
        char  *var;                                   /* E_VAR  */
        struct { BinOp op; struct Expr *l, *r; } bin; /* E_BIN  */
        struct Expr *unary;                           /* E_NEG / E_NOT */
        struct { Builtin fn; struct Expr *a, *b; } call; /* E_CALL (b may be NULL) */
        struct { int ne; char *a, *b; } strcmp;       /* E_STRCMP (ne = != when 1) */
    } as;
} Expr;

Expr *expr_num(float v);
Expr *expr_var(char *name);            /* takes ownership of name */
Expr *expr_bin(BinOp op, Expr *l, Expr *r);
Expr *expr_neg(Expr *e);
Expr *expr_not(Expr *e);
Expr *expr_call(Builtin fn, Expr *a, Expr *b);
Expr *expr_strcmp(int ne, char *a, char *b);   /* takes ownership of a and b */

float eval_expr(const Expr *e);
void  free_expr(Expr *e);

/* ---------- statements ---------- */

typedef enum {
    S_PRINT_STR,   /* print a string, with an optional trailing value      */
    S_PRINT_EXPR,  /* print a numeric value                                */
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
    struct Stmt *body;    /* S_IF/S_WHILE body, or S_BLOCK statement list head */
    struct Stmt *body2;   /* S_IF else-branch */
    struct Stmt *next;    /* next statement in a block */
} Stmt;

Stmt *stmt_print_str(char *s, Expr *trailing /*nullable*/);
Stmt *stmt_print_expr(Expr *e);
Stmt *stmt_assign(char *name, Expr *e, int echo);
Stmt *stmt_block_new(void);
void  stmt_block_add(Stmt *block, Stmt *s);   /* appends s to block */
Stmt *stmt_if(Expr *cond, Stmt *thenb, Stmt *elseb /*nullable*/);
Stmt *stmt_while(Expr *cond, Stmt *body);
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
