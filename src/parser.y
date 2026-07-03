%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include "ast.h"
  int yylex(void);
  int yyerror(const char *s);
%}

%union {
   char  *string;
   float  fNumber;
   Expr  *expr;
   Stmt  *stmt;
   Param *plist;
   Arg   *alist;
   Pair  *pairs;
};

%token DIR HELP PRINT QUIT CLEAR CLS FOR WHILE SUM SUBTRACT POINTERCELL MOLTIPLICATION IF
%token VAR SQRT POW MAX MIN ABS RANDOM FLOOR CEIL ROUND LOG LOG10 DIVISION MOD LS
%token LEN UPPER LOWER TOSTR TONUM
%token OPENBRACKET CLOSEBRACKET COMMA EXIT NEWLINE ASSIGN SAVOEND SAVOELSE SAVODEF SAVORETURN
%token LBRACKET RBRACKET PUSH SET LBRACE RBRACE COLON DOT
%token EQUAL NOTEQUAL LT GT LE GE
%token PLUS MINUS MULTIPLY DIVIDE PERCENT NEGATION
%token <string>  STRING ARGUMENT IDENTIFIER
%token <fNumber> NUMBER

%type <expr> expr atom callexpr ifcond count
%type <stmt> stmt printstmt varstmt addstmt subtractstmt moltiplicationstmt
%type <stmt> divisionstmt modstmt sqrtstmt powstmt maxstmt minstmt absstmt
%type <stmt> floorstmt ceilstmt roundstmt logstmt log10stmt randomstmt
%type <stmt> ifstmt forstmt whilestmt dirstmt lsstmt pointerstmt
%type <stmt> helpstmt quitstmt clsstmt clearstmt block funcdefstmt returnstmt
%type <stmt> pushstmt setstmt
%type <plist> paramlist paramnames
%type <alist> arglist argvalues
%type <pairs> pairlist pairs
%type <string> objkey

%left EQUAL NOTEQUAL LT GT LE GE
%left PLUS MINUS
%left MULTIPLY DIVIDE PERCENT
%right NEGATION
%right UMINUS

/* Expected, benign shift/reduce conflicts, all resolved by shifting (what we
 * want): postfix '(' (call), '[' (subscript) and '.' (field access) each bind
 * tighter than reducing the bare atom/identifier. */
%expect 6

%%

program:
      /* empty */
    | program topline
    ;

/*
 * Statements are newline-terminated. Each top-level statement is executed and
 * freed as soon as it is parsed; 'error NEWLINE' resynchronises after a bad
 * line so one typo does not abort the session.
 */
topline:
      NEWLINE
    | stmt NEWLINE        { if ($1) { exec_stmt($1); free_stmt($1); } }
    | funcdefstmt NEWLINE { func_define($1); }   /* retained, not freed */
    | error NEWLINE       { yyerrok; }
    ;

stmt:
      printstmt | varstmt | dirstmt | lsstmt | helpstmt | quitstmt
    | clsstmt | clearstmt | forstmt | whilestmt | addstmt | subtractstmt
    | pointerstmt | moltiplicationstmt | divisionstmt | modstmt | absstmt
    | randomstmt | floorstmt | ceilstmt | roundstmt | logstmt | log10stmt
    | ifstmt | sqrtstmt | powstmt | maxstmt | minstmt | returnstmt
    | pushstmt | setstmt
    ;

/* ------------------------------ expressions ------------------------------ */

expr:
      atom                        { $$ = $1; }
    | expr PLUS expr              { $$ = expr_bin(OP_ADD, $1, $3); }
    | expr MINUS expr             { $$ = expr_bin(OP_SUB, $1, $3); }
    | expr MULTIPLY expr          { $$ = expr_bin(OP_MUL, $1, $3); }
    | expr DIVIDE expr            { $$ = expr_bin(OP_DIV, $1, $3); }
    | expr PERCENT expr           { $$ = expr_bin(OP_MOD, $1, $3); }
    | expr EQUAL expr             { $$ = expr_bin(OP_EQ, $1, $3); }
    | expr NOTEQUAL expr          { $$ = expr_bin(OP_NE, $1, $3); }
    | expr LT expr                { $$ = expr_bin(OP_LT, $1, $3); }
    | expr GT expr                { $$ = expr_bin(OP_GT, $1, $3); }
    | expr LE expr                { $$ = expr_bin(OP_LE, $1, $3); }
    | expr GE expr                { $$ = expr_bin(OP_GE, $1, $3); }
    ;

atom:
      NUMBER                          { $$ = expr_num($1); }
    | STRING                          { $$ = expr_str($1); }
    | IDENTIFIER                      { $$ = expr_var($1); }
    | OPENBRACKET expr CLOSEBRACKET   { $$ = $2; }
    | LBRACKET arglist RBRACKET       { $$ = expr_array($2); }        /* array literal */
    | LBRACE pairlist RBRACE          { $$ = expr_object($2); }       /* object literal */
    | atom LBRACKET expr RBRACKET     { $$ = expr_index($1, $3); }    /* subscript */
    | atom DOT IDENTIFIER             { $$ = expr_index($1, expr_str($3)); } /* field */
    | MINUS atom %prec UMINUS         { $$ = expr_neg($2); }
    | NEGATION atom                   { $$ = expr_not($2); }
    | callexpr                        { $$ = $1; }
    ;

/* Built-in functions usable inside expressions, e.g. savosqrt(x), savopow(a,b). */
callexpr:
      SQRT  OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_SQRT, $3, NULL); }
    | ABS   OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_ABS, $3, NULL); }
    | FLOOR OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_FLOOR, $3, NULL); }
    | CEIL  OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_CEIL, $3, NULL); }
    | ROUND OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_ROUND, $3, NULL); }
    | LOG   OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_LOG, $3, NULL); }
    | LOG10 OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_LOG10, $3, NULL); }
    | POW   OPENBRACKET expr COMMA expr CLOSEBRACKET    { $$ = expr_call(FN_POW, $3, $5); }
    | MAX   OPENBRACKET expr COMMA expr CLOSEBRACKET    { $$ = expr_call(FN_MAX, $3, $5); }
    | MIN   OPENBRACKET expr COMMA expr CLOSEBRACKET    { $$ = expr_call(FN_MIN, $3, $5); }
    | RANDOM OPENBRACKET expr COMMA expr CLOSEBRACKET   { $$ = expr_call(FN_RANDOM, $3, $5); }
    | LEN   OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_LEN, $3, NULL); }
    | UPPER OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_UPPER, $3, NULL); }
    | LOWER OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_LOWER, $3, NULL); }
    | TOSTR OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_STR, $3, NULL); }
    | TONUM OPENBRACKET expr CLOSEBRACKET               { $$ = expr_call(FN_NUM, $3, NULL); }
    | IDENTIFIER OPENBRACKET arglist CLOSEBRACKET       { $$ = expr_calluser($1, $3); }
    ;

/* comma-separated call arguments (possibly empty) */
arglist:
      /* empty */   { $$ = NULL; }
    | argvalues     { $$ = $1; }
    ;
argvalues:
      expr                   { $$ = arg_add(NULL, $1); }
    | argvalues COMMA expr   { $$ = arg_add($1, $3); }
    ;

/* object literal contents: key: value pairs (possibly empty) */
pairlist:
      /* empty */   { $$ = NULL; }
    | pairs         { $$ = $1; }
    ;
pairs:
      objkey COLON expr              { $$ = pair_add(NULL, $1, $3); }
    | pairs COMMA objkey COLON expr  { $$ = pair_add($1, $3, $5); }
    ;
objkey:
      IDENTIFIER   { $$ = $1; }
    | STRING       { $$ = $1; }
    ;

/* A loop count: a bare number or variable (no leading '(' so it never clashes
 * with the parenthesised range form of savofor). */
count:
      NUMBER      { $$ = expr_num($1); }
    | IDENTIFIER  { $$ = expr_var($1); }
    ;

/* A condition is any expression: truthy when non-zero / a non-empty string.
 * String comparisons like "a" == "b" fall out of the expression grammar. */
ifcond:
      expr                          { $$ = $1; }
    ;

/* ------------------------------ statements ------------------------------ */

printstmt:
      PRINT expr                    { $$ = stmt_print_expr($2); }
    ;

varstmt:
      VAR IDENTIFIER expr           { $$ = stmt_assign($2, $3, 1); }  /* echoes  */
    | VAR IDENTIFIER ASSIGN expr    { $$ = stmt_assign($2, $4, 0); }  /* silent  */
    ;

addstmt:            SUM atom atom            { $$ = stmt_arith(OP_ADD, $2, $3); } ;
subtractstmt:       SUBTRACT atom atom       { $$ = stmt_arith(OP_SUB, $2, $3); } ;
moltiplicationstmt: MOLTIPLICATION atom atom { $$ = stmt_arith(OP_MUL, $2, $3); } ;
divisionstmt:       DIVISION atom atom       { $$ = stmt_arith(OP_DIV, $2, $3); } ;
modstmt:            MOD atom atom            { $$ = stmt_arith(OP_MOD, $2, $3); } ;

sqrtstmt:   SQRT atom    { $$ = stmt_math1(FN_SQRT, $2); } ;
absstmt:    ABS atom     { $$ = stmt_math1(FN_ABS, $2); } ;
floorstmt:  FLOOR atom   { $$ = stmt_math1(FN_FLOOR, $2); } ;
ceilstmt:   CEIL atom    { $$ = stmt_math1(FN_CEIL, $2); } ;
roundstmt:  ROUND atom   { $$ = stmt_math1(FN_ROUND, $2); } ;
logstmt:    LOG atom     { $$ = stmt_math1(FN_LOG, $2); } ;
log10stmt:  LOG10 atom   { $$ = stmt_math1(FN_LOG10, $2); } ;

powstmt:    POW atom atom { $$ = stmt_math2(FN_POW, $2, $3); } ;
maxstmt:    MAX atom atom { $$ = stmt_math2(FN_MAX, $2, $3); } ;
minstmt:    MIN atom atom { $$ = stmt_math2(FN_MIN, $2, $3); } ;

randomstmt: RANDOM atom atom { $$ = stmt_random($2, $3); } ;

/* A block is a newline-separated sequence of statements, collected (not
 * executed) until its terminator. */
block:
      /* empty */          { $$ = stmt_block_new(); }
    | block NEWLINE        { $$ = $1; }
    | block stmt NEWLINE   { stmt_block_add($1, $2); $$ = $1; }
    ;

ifstmt:
      IF OPENBRACKET ifcond CLOSEBRACKET NEWLINE block SAVOEND {
          $$ = stmt_if($3, $6, NULL);
      }
    | IF OPENBRACKET ifcond CLOSEBRACKET NEWLINE block SAVOELSE NEWLINE block SAVOEND {
          $$ = stmt_if($3, $6, $9);
      }
    ;

forstmt:
      FOR count STRING {
          $$ = stmt_repeat($2, $3);
      }
    | FOR OPENBRACKET atom COMMA atom COMMA atom CLOSEBRACKET STRING {
          $$ = stmt_forrange($3, $5, $7, $9, FOR_NONE, NULL);
      }
    | FOR OPENBRACKET atom COMMA atom COMMA atom CLOSEBRACKET STRING PLUS atom {
          $$ = stmt_forrange($3, $5, $7, $9, FOR_PLUS, $11);
      }
    | FOR OPENBRACKET atom COMMA atom COMMA atom CLOSEBRACKET STRING MULTIPLY atom {
          $$ = stmt_forrange($3, $5, $7, $9, FOR_MUL, $11);
      }
    ;

whilestmt:
      WHILE count STRING  { $$ = stmt_repeat($2, $3); }
    | WHILE OPENBRACKET ifcond CLOSEBRACKET NEWLINE block SAVOEND {
          $$ = stmt_while($3, $6);
      }
    ;

/* user-defined functions */
funcdefstmt:
    SAVODEF IDENTIFIER OPENBRACKET paramlist CLOSEBRACKET NEWLINE block SAVOEND {
        $$ = stmt_funcdef($2, $4, $7);
    }
    ;

paramlist:
      /* empty */  { $$ = NULL; }
    | paramnames   { $$ = $1; }
    ;
paramnames:
      IDENTIFIER                  { $$ = param_add(NULL, $1); }
    | paramnames COMMA IDENTIFIER { $$ = param_add($1, $3); }
    ;

returnstmt:
      SAVORETURN expr  { $$ = stmt_return($2); }
    | SAVORETURN       { $$ = stmt_return(NULL); }
    ;

pushstmt:
    PUSH IDENTIFIER expr  { $$ = stmt_push($2, $3); }
    ;

setstmt:
      SET IDENTIFIER LBRACKET expr RBRACKET ASSIGN expr  { $$ = stmt_setindex($2, $4, $7); }
    | SET IDENTIFIER DOT IDENTIFIER ASSIGN expr          { $$ = stmt_setindex($2, expr_str($4), $6); }
    ;

dirstmt:
      DIR ARGUMENT  { $$ = stmt_dir($2); }
    | DIR           { $$ = stmt_dir(NULL); }
    ;

lsstmt:
      LS ARGUMENT   { $$ = stmt_dir($2); }
    | LS            { $$ = stmt_dir(NULL); }
    ;

pointerstmt: POINTERCELL STRING { $$ = stmt_pointer($2); } ;

helpstmt:   HELP  { $$ = stmt_simple(S_HELP); } ;
quitstmt:   QUIT  { $$ = stmt_simple(S_QUIT); }
          | EXIT  { $$ = stmt_simple(S_QUIT); } ;
clsstmt:    CLS   { $$ = stmt_simple(S_CLS); } ;
clearstmt:  CLEAR { $$ = stmt_simple(S_CLEAR); } ;

%%
