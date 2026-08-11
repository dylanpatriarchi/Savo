#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "parser.h"
#include "ast.h"

extern int had_error;   /* defined in main.c */

typedef struct {
    Lexer  *lx;
    Token   cur;
    jmp_buf recover;
} Parser;

/* ------------------------------ token flow ------------------------------ */

static void p_advance(Parser *P) {
    free(P->cur.text);
    P->cur = lexer_next(P->lx);
}

static int at(Parser *P, TokKind k) { return P->cur.kind == k; }

static void perror_at(Parser *P, const char *msg) {
    fprintf(stderr, "savo: line %d:%d: %s\n", P->cur.line, P->cur.col, msg);
    had_error = 1;
    longjmp(P->recover, 1);
}

static void expect(Parser *P, TokKind k, const char *what) {
    if (!at(P, k)) {
        char buf[128];
        snprintf(buf, sizeof buf, "expected %s but found %s", what, token_name(P->cur.kind));
        perror_at(P, buf);
    }
    p_advance(P);
}

/* Take ownership of the current token's string and advance. */
static char *take_text(Parser *P) {
    char *s = P->cur.text;
    P->cur.text = NULL;
    p_advance(P);
    return s;
}

/* ------------------------------ expressions ----------------------------- */

static Expr *parse_expr(Parser *P);
static Expr *parse_atom(Parser *P);

/* Map a builtin-function keyword token to its (fn, arity). Returns 1 if the
 * current token is such a function, filling *fn and *arity (1, 2 or 3). */
static int builtin_of(TokKind k, Builtin *fn, int *arity) {
    switch (k) {
        case TK_SQRT:    *fn = FN_SQRT;    *arity = 1; return 1;
        case TK_ABS:     *fn = FN_ABS;     *arity = 1; return 1;
        case TK_FLOOR:   *fn = FN_FLOOR;   *arity = 1; return 1;
        case TK_CEIL:    *fn = FN_CEIL;    *arity = 1; return 1;
        case TK_ROUND:   *fn = FN_ROUND;   *arity = 1; return 1;
        case TK_LOG:     *fn = FN_LOG;     *arity = 1; return 1;
        case TK_LOG10:   *fn = FN_LOG10;   *arity = 1; return 1;
        case TK_LEN:     *fn = FN_LEN;     *arity = 1; return 1;
        case TK_UPPER:   *fn = FN_UPPER;   *arity = 1; return 1;
        case TK_LOWER:   *fn = FN_LOWER;   *arity = 1; return 1;
        case TK_TOSTR:   *fn = FN_STR;     *arity = 1; return 1;
        case TK_TONUM:   *fn = FN_NUM;     *arity = 1; return 1;
        case TK_TRIM:    *fn = FN_TRIM;    *arity = 1; return 1;
        case TK_POP:     *fn = FN_POP;     *arity = 1; return 1;
        case TK_KEYS:    *fn = FN_KEYS;    *arity = 1; return 1;
        case TK_POW:     *fn = FN_POW;     *arity = 2; return 1;
        case TK_MAX:     *fn = FN_MAX;     *arity = 2; return 1;
        case TK_MIN:     *fn = FN_MIN;     *arity = 2; return 1;
        case TK_RANDOM:  *fn = FN_RANDOM;  *arity = 2; return 1;
        case TK_INDEXOF: *fn = FN_INDEXOF; *arity = 2; return 1;
        case TK_SPLIT:   *fn = FN_SPLIT;   *arity = 2; return 1;
        case TK_JOIN:    *fn = FN_JOIN;    *arity = 2; return 1;
        case TK_CONTAINS:*fn = FN_CONTAINS;*arity = 2; return 1;
        case TK_SUBSTR:  *fn = FN_SUBSTR;  *arity = 3; return 1;
        case TK_REPLACE: *fn = FN_REPLACE; *arity = 3; return 1;
        default: return 0;
    }
}

/* comma-separated expressions until `close`, possibly empty */
static Arg *parse_args(Parser *P, TokKind close) {
    Arg *list;
    if (at(P, close)) return NULL;
    list = arg_add(NULL, parse_expr(P));
    while (at(P, TK_COMMA)) { p_advance(P); list = arg_add(list, parse_expr(P)); }
    return list;
}

/* key: value pairs until `}`, possibly empty */
static Pair *parse_pairs(Parser *P) {
    Pair *list = NULL;
    if (at(P, TK_RBRACE)) return NULL;
    for (;;) {
        char *key;
        Expr *val;
        if (at(P, TK_IDENT) || at(P, TK_STRING)) key = take_text(P);
        else { perror_at(P, "expected an object key"); return NULL; }
        expect(P, TK_COLON, "':'");
        val = parse_expr(P);
        list = pair_add(list, key, val);
        if (at(P, TK_COMMA)) { p_advance(P); continue; }
        break;
    }
    return list;
}

/* subscripts and field access, left to right */
static Expr *parse_postfix(Parser *P, Expr *base) {
    for (;;) {
        if (at(P, TK_LBRACKET)) {
            Expr *idx;
            p_advance(P);
            idx = parse_expr(P);
            expect(P, TK_RBRACKET, "']'");
            base = expr_index(base, idx);
        } else if (at(P, TK_DOT)) {
            char *key;
            p_advance(P);
            if (!at(P, TK_IDENT)) perror_at(P, "expected a field name after '.'");
            key = take_text(P);
            base = expr_index(base, expr_str(key));
        } else {
            return base;
        }
    }
}

static Expr *parse_primary(Parser *P) {
    Builtin fn;
    int arity;

    if (at(P, TK_NUMBER)) { Expr *e = expr_num(P->cur.num); p_advance(P); return e; }
    if (at(P, TK_STRING)) { return expr_str(take_text(P)); }

    /* savoinput([prompt]): the only builtin with an optional argument. */
    if (at(P, TK_INPUT)) {
        Expr *prompt;
        p_advance(P);
        expect(P, TK_LPAREN, "'('");
        prompt = at(P, TK_RPAREN) ? expr_str(strdup("")) : parse_expr(P);
        expect(P, TK_RPAREN, "')'");
        return expr_call(FN_INPUT, prompt, NULL, NULL);
    }

    if (builtin_of(P->cur.kind, &fn, &arity)) {
        Expr *a, *b = NULL, *c = NULL;
        p_advance(P);
        expect(P, TK_LPAREN, "'('");
        a = parse_expr(P);
        if (arity >= 2) { expect(P, TK_COMMA, "','"); b = parse_expr(P); }
        if (arity >= 3) { expect(P, TK_COMMA, "','"); c = parse_expr(P); }
        expect(P, TK_RPAREN, "')'");
        return expr_call(fn, a, b, c);
    }

    if (at(P, TK_IDENT)) {
        char *name = take_text(P);
        if (at(P, TK_LPAREN)) {          /* user function call */
            Arg *args;
            p_advance(P);
            args = parse_args(P, TK_RPAREN);
            expect(P, TK_RPAREN, "')'");
            return expr_calluser(name, args);
        }
        return expr_var(name);
    }

    if (at(P, TK_LPAREN)) {
        Expr *e;
        p_advance(P);
        e = parse_expr(P);
        expect(P, TK_RPAREN, "')'");
        return e;
    }
    if (at(P, TK_LBRACKET)) {
        Arg *items;
        p_advance(P);
        items = parse_args(P, TK_RBRACKET);
        expect(P, TK_RBRACKET, "']'");
        return expr_array(items);
    }
    if (at(P, TK_LBRACE)) {
        Pair *pairs;
        p_advance(P);
        pairs = parse_pairs(P);
        expect(P, TK_RBRACE, "'}'");
        return expr_object(pairs);
    }

    perror_at(P, "expected a value");
    return NULL;   /* unreachable: perror_at longjmps */
}

/* unary '-'/'!' bind to the following atom, tighter than any binary operator */
static Expr *parse_atom(Parser *P) {
    if (at(P, TK_MINUS)) { p_advance(P); return expr_neg(parse_atom(P)); }
    if (at(P, TK_NOT))   { p_advance(P); return expr_not(parse_atom(P)); }
    return parse_postfix(P, parse_primary(P));
}

/* binary precedence, loosest to tightest (0 = not a binary operator):
 *   ||  <  &&  <  comparisons  <  +,-  <  *,/,% */
static int binprec(TokKind k) {
    switch (k) {
        case TK_OR:  return 1;
        case TK_AND: return 2;
        case TK_EQ: case TK_NE: case TK_LT: case TK_GT: case TK_LE: case TK_GE: return 3;
        case TK_PLUS: case TK_MINUS: return 4;
        case TK_STAR: case TK_SLASH: case TK_PERCENT: return 5;
        default: return 0;
    }
}

static BinOp bin_of(TokKind k) {
    switch (k) {
        case TK_OR: return OP_OR;  case TK_AND: return OP_AND;
        case TK_PLUS: return OP_ADD; case TK_MINUS: return OP_SUB;
        case TK_STAR: return OP_MUL; case TK_SLASH: return OP_DIV;
        case TK_PERCENT: return OP_MOD;
        case TK_EQ: return OP_EQ;  case TK_NE: return OP_NE;
        case TK_LT: return OP_LT;  case TK_GT: return OP_GT;
        case TK_LE: return OP_LE;  case TK_GE: return OP_GE;
        default: return OP_ADD;
    }
}

static Expr *parse_expr_bp(Parser *P, int min_bp) {
    Expr *left = parse_atom(P);
    int bp;
    while ((bp = binprec(P->cur.kind)) >= min_bp && bp > 0) {
        BinOp op = bin_of(P->cur.kind);
        Expr *right;
        p_advance(P);
        right = parse_expr_bp(P, bp + 1);   /* left-associative */
        left = expr_bin(op, left, right);
    }
    return left;
}

static Expr *parse_expr(Parser *P) { return parse_expr_bp(P, 1); }

/* a bare loop count: a number or a variable */
static Expr *parse_count(Parser *P) {
    if (at(P, TK_NUMBER)) { Expr *e = expr_num(P->cur.num); p_advance(P); return e; }
    if (at(P, TK_IDENT))  { return expr_var(take_text(P)); }
    perror_at(P, "expected a loop count (number or variable)");
    return NULL;
}

static char *take_string(Parser *P) {
    if (!at(P, TK_STRING)) perror_at(P, "expected a string");
    return take_text(P);
}

/* ------------------------------ statements ------------------------------ */

static Stmt *parse_statement(Parser *P, int allow_def);
static Stmt *parse_block(Parser *P);

/* Parse what follows a then-block: an optional savoelif chain, an optional
 * savoelse, and the closing savoend. savoelif desugars to a nested if placed in
 * the else branch, and only the innermost tail consumes savoend. */
static Stmt *parse_if_tail(Parser *P) {
    if (at(P, TK_ELIF)) {
        Expr *cond;
        Stmt *thenb, *elseb;
        p_advance(P);
        expect(P, TK_LPAREN, "'('");
        cond = parse_expr(P);
        expect(P, TK_RPAREN, "')'");
        expect(P, TK_NEWLINE, "end of line");
        thenb = parse_block(P);
        elseb = parse_if_tail(P);
        return stmt_if(cond, thenb, elseb);
    }
    if (at(P, TK_ELSE)) {
        Stmt *elseb;
        p_advance(P);
        expect(P, TK_NEWLINE, "end of line");
        elseb = parse_block(P);
        expect(P, TK_END, "savoend");
        return elseb;
    }
    expect(P, TK_END, "savoend");
    return NULL;
}

static Stmt *parse_if(Parser *P) {
    Expr *cond;
    Stmt *thenb, *elseb;
    p_advance(P);                        /* IF */
    expect(P, TK_LPAREN, "'('");
    cond = parse_expr(P);
    expect(P, TK_RPAREN, "')'");
    expect(P, TK_NEWLINE, "end of line");
    thenb = parse_block(P);
    elseb = parse_if_tail(P);
    return stmt_if(cond, thenb, elseb);
}

static Stmt *parse_while(Parser *P) {
    p_advance(P);                        /* WHILE */
    if (at(P, TK_LPAREN)) {
        Expr *cond;
        Stmt *body;
        p_advance(P);
        cond = parse_expr(P);
        expect(P, TK_RPAREN, "')'");
        expect(P, TK_NEWLINE, "end of line");
        body = parse_block(P);
        expect(P, TK_END, "savoend");
        return stmt_while(cond, body);
    }
    { Expr *count = parse_count(P); char *s = take_string(P); return stmt_repeat(count, s); }
}

static Stmt *parse_for(Parser *P) {
    p_advance(P);                        /* FOR */
    if (at(P, TK_LPAREN)) {
        Expr *a, *b, *step;
        char *s;
        p_advance(P);
        a = parse_atom(P);    expect(P, TK_COMMA, "','");
        b = parse_atom(P);    expect(P, TK_COMMA, "','");
        step = parse_atom(P); expect(P, TK_RPAREN, "')'");
        s = take_string(P);
        if (at(P, TK_PLUS)) { p_advance(P); return stmt_forrange(a, b, step, s, FOR_PLUS, parse_atom(P)); }
        if (at(P, TK_STAR)) { p_advance(P); return stmt_forrange(a, b, step, s, FOR_MUL, parse_atom(P)); }
        return stmt_forrange(a, b, step, s, FOR_NONE, NULL);
    }
    { Expr *count = parse_count(P); char *s = take_string(P); return stmt_repeat(count, s); }
}

static Stmt *parse_foreach(Parser *P) {
    char *var;
    Expr *coll;
    Stmt *body;
    p_advance(P);                        /* FOREACH */
    if (!at(P, TK_IDENT)) perror_at(P, "expected a loop variable after savoforeach");
    var = take_text(P);
    coll = parse_expr(P);
    expect(P, TK_NEWLINE, "end of line");
    body = parse_block(P);
    expect(P, TK_END, "savoend");
    return stmt_foreach(var, coll, body);
}

static Stmt *parse_set(Parser *P) {
    char *name;
    p_advance(P);                        /* SET */
    if (!at(P, TK_IDENT)) perror_at(P, "expected a variable name after savoset");
    name = take_text(P);
    if (at(P, TK_LBRACKET)) {
        Expr *idx, *val;
        p_advance(P);
        idx = parse_expr(P);
        expect(P, TK_RBRACKET, "']'");
        expect(P, TK_ASSIGN, "'='");
        val = parse_expr(P);
        return stmt_setindex(name, idx, val);
    }
    if (at(P, TK_DOT)) {
        char *key;
        Expr *val;
        p_advance(P);
        if (!at(P, TK_IDENT)) perror_at(P, "expected a field name after '.'");
        key = take_text(P);
        expect(P, TK_ASSIGN, "'='");
        val = parse_expr(P);
        return stmt_setindex(name, expr_str(key), val);
    }
    perror_at(P, "expected '[' or '.' after savoset");
    return NULL;
}

static Stmt *parse_funcdef(Parser *P) {
    char *name;
    Param *params = NULL;
    Stmt *body;
    p_advance(P);                        /* DEF */
    if (!at(P, TK_IDENT)) perror_at(P, "expected a function name after savodef");
    name = take_text(P);
    expect(P, TK_LPAREN, "'('");
    if (!at(P, TK_RPAREN)) {
        if (!at(P, TK_IDENT)) perror_at(P, "expected a parameter name");
        params = param_add(NULL, take_text(P));
        while (at(P, TK_COMMA)) {
            p_advance(P);
            if (!at(P, TK_IDENT)) perror_at(P, "expected a parameter name");
            params = param_add(params, take_text(P));
        }
    }
    expect(P, TK_RPAREN, "')'");
    expect(P, TK_NEWLINE, "end of line");
    body = parse_block(P);
    expect(P, TK_END, "savoend");
    return stmt_funcdef(name, params, body);
}

static Stmt *parse_statement(Parser *P, int allow_def) {
    switch (P->cur.kind) {
        case TK_PRINT:   p_advance(P); return stmt_print_expr(parse_expr(P));
        case TK_VAR: {
            char *name;
            p_advance(P);
            if (!at(P, TK_IDENT)) perror_at(P, "expected a variable name after savovar");
            name = take_text(P);
            if (at(P, TK_ASSIGN)) { p_advance(P); return stmt_assign(name, parse_expr(P), 0); }
            return stmt_assign(name, parse_expr(P), 1);
        }
        case TK_SUM:            { Expr *a; p_advance(P); a = parse_atom(P); return stmt_arith(OP_ADD, a, parse_atom(P)); }
        case TK_SUBTRACT:       { Expr *a; p_advance(P); a = parse_atom(P); return stmt_arith(OP_SUB, a, parse_atom(P)); }
        case TK_MULTIPLICATION: { Expr *a; p_advance(P); a = parse_atom(P); return stmt_arith(OP_MUL, a, parse_atom(P)); }
        case TK_DIVISION:       { Expr *a; p_advance(P); a = parse_atom(P); return stmt_arith(OP_DIV, a, parse_atom(P)); }
        case TK_MOD:            { Expr *a; p_advance(P); a = parse_atom(P); return stmt_arith(OP_MOD, a, parse_atom(P)); }
        case TK_SQRT:  p_advance(P); return stmt_math1(FN_SQRT, parse_atom(P));
        case TK_ABS:   p_advance(P); return stmt_math1(FN_ABS, parse_atom(P));
        case TK_FLOOR: p_advance(P); return stmt_math1(FN_FLOOR, parse_atom(P));
        case TK_CEIL:  p_advance(P); return stmt_math1(FN_CEIL, parse_atom(P));
        case TK_ROUND: p_advance(P); return stmt_math1(FN_ROUND, parse_atom(P));
        case TK_LOG:   p_advance(P); return stmt_math1(FN_LOG, parse_atom(P));
        case TK_LOG10: p_advance(P); return stmt_math1(FN_LOG10, parse_atom(P));
        case TK_POW: { Expr *a; p_advance(P); a = parse_atom(P); return stmt_math2(FN_POW, a, parse_atom(P)); }
        case TK_MAX: { Expr *a; p_advance(P); a = parse_atom(P); return stmt_math2(FN_MAX, a, parse_atom(P)); }
        case TK_MIN: { Expr *a; p_advance(P); a = parse_atom(P); return stmt_math2(FN_MIN, a, parse_atom(P)); }
        case TK_RANDOM: { Expr *a; p_advance(P); a = parse_atom(P); return stmt_random(a, parse_atom(P)); }
        case TK_IF:     return parse_if(P);
        case TK_WHILE:  return parse_while(P);
        case TK_FOR:    return parse_for(P);
        case TK_FOREACH: return parse_foreach(P);
        case TK_RETURN:
            p_advance(P);
            if (at(P, TK_NEWLINE) || at(P, TK_EOF)) return stmt_return(NULL);
            return stmt_return(parse_expr(P));
        case TK_PUSH: {
            char *name;
            p_advance(P);
            if (!at(P, TK_IDENT)) perror_at(P, "expected a variable name after savopush");
            name = take_text(P);
            return stmt_push(name, parse_expr(P));
        }
        case TK_BREAK:    p_advance(P); return stmt_simple(S_BREAK);
        case TK_CONTINUE: p_advance(P); return stmt_simple(S_CONTINUE);
        case TK_SET:    return parse_set(P);
        case TK_DEF:
            if (!allow_def) perror_at(P, "functions can only be defined at the top level");
            return parse_funcdef(P);
        case TK_DIR: case TK_LS: {
            char *arg = lexer_read_line_arg(P->lx);
            Stmt *s = stmt_dir(arg);
            p_advance(P);                /* now positioned at the newline */
            return s;
        }
        case TK_CLS:     p_advance(P); return stmt_simple(S_CLS);
        case TK_CLEAR:   p_advance(P); return stmt_simple(S_CLEAR);
        case TK_HELP:    p_advance(P); return stmt_simple(S_HELP);
        case TK_QUIT: case TK_EXIT: p_advance(P); return stmt_simple(S_QUIT);
        case TK_POINTER: p_advance(P); return stmt_pointer(take_string(P));
        default:
            perror_at(P, "unexpected token at the start of a statement");
            return NULL;
    }
}

/* newline-separated statements until savoend / savoelse / EOF */
static Stmt *parse_block(Parser *P) {
    Stmt *blk = stmt_block_new();
    for (;;) {
        while (at(P, TK_NEWLINE)) p_advance(P);
        if (at(P, TK_END) || at(P, TK_ELIF) || at(P, TK_ELSE) || at(P, TK_EOF)) break;
        stmt_block_add(blk, parse_statement(P, 0));
        if (at(P, TK_NEWLINE)) p_advance(P);
        else if (at(P, TK_END) || at(P, TK_ELIF) || at(P, TK_ELSE) || at(P, TK_EOF)) break;
        else expect(P, TK_NEWLINE, "end of line");
    }
    return blk;
}

/* ------------------------------ top level ------------------------------- */

void parser_run(Lexer *lx) {
    Parser P;
    P.lx = lx;
    P.cur = lexer_next(lx);

    for (;;) {
        if (setjmp(P.recover)) {
            while (!at(&P, TK_NEWLINE) && !at(&P, TK_EOF)) p_advance(&P);
        }
        if (at(&P, TK_EOF)) break;
        if (at(&P, TK_NEWLINE)) { p_advance(&P); continue; }

        {
            Stmt *s = parse_statement(&P, 1);

            /* The current token is already the line terminator, so checking its
             * kind validates it without reading ahead. Reject trailing garbage
             * before executing anything. */
            if (!at(&P, TK_NEWLINE) && !at(&P, TK_EOF)) {
                if (s != NULL) free_stmt(s);
                expect(&P, TK_NEWLINE, "end of line");   /* reports and recovers */
            }

            /* Execute before consuming the newline: advancing past it reads the
             * next line, which would block the REPL and stop savoquit reaching
             * exit. */
            if (s != NULL) {
                if (s->kind == S_FUNCDEF) func_define(s);   /* retained by the table */
                else { exec_stmt(s); free_stmt(s); }
            }

            if (at(&P, TK_NEWLINE)) p_advance(&P);
        }
    }
    free(P.cur.text);
}
