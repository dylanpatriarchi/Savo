%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <string.h>
  #include "global.h"
  #include "symtab.h"
  #include "statements/printstmt.h"
  #include "statements/forstmt.h"
  #include "statements/whilestmt.h"
  int yylex(void);
  int yyerror(const char *s);
%}

%union {
   char * string;
   int    number;
   float  fNumber;
   void * pVoid;
};

%token DIR HELP PRINT QUIT CLEAR CLS IDENTIFIER FOR WHILE SUM SUBTRACT POINTERCELL MOLTIPLICATION EQUAL IF NOTEQUAL
%token VAR SQRT POW MAX MIN PLUS MINUS MULTIPLY DIVIDE
%token ARGUMENT NUMBER STRING EXIT OPENBRACKET CLOSEBRACKET COMMA NEGATION
%token LS DIVISION MOD ABS RANDOM LT GT LE GE

%type <string>  STRING
%type <string>  ARGUMENT
%type <string>  IDENTIFIER
%type <fNumber> NUMBER
%type <fNumber> value

%%

program :  commands
         | commands program;

commands:
           printstmt
         | dirstmt
         | lsstmt
         | helpstmt
         | quitstmt
         | clsstmt
         | clearstmt
         | forstmt
         | whilestmt
         | addstmt
         | subtractstmt
         | pointerstmt
         | moltiplicationstmt
         | divisionstmt
         | modstmt
         | absstmt
         | randomstmt
         | ifstmt
         | varstmt
         | sqrtstmt
         | powstmt
         | maxstmt
         | minstmt
         ;

/* A numeric operand: either a literal number or a previously defined variable. */
value:
      NUMBER              { $$ = $1; }
    | IDENTIFIER          { $$ = symtab_get($1); free($1); }
    ;

ifstmt:
      IF OPENBRACKET value EQUAL value CLOSEBRACKET {
          printf("%s\n", ($3 == $5) ? "true" : "false");
      }
    | IF OPENBRACKET value NOTEQUAL value CLOSEBRACKET {
          printf("%s\n", ($3 != $5) ? "true" : "false");
      }
    | IF OPENBRACKET value LT value CLOSEBRACKET {
          printf("%s\n", ($3 < $5) ? "true" : "false");
      }
    | IF OPENBRACKET value GT value CLOSEBRACKET {
          printf("%s\n", ($3 > $5) ? "true" : "false");
      }
    | IF OPENBRACKET value LE value CLOSEBRACKET {
          printf("%s\n", ($3 <= $5) ? "true" : "false");
      }
    | IF OPENBRACKET value GE value CLOSEBRACKET {
          printf("%s\n", ($3 >= $5) ? "true" : "false");
      }
    | IF OPENBRACKET value CLOSEBRACKET {
          printf("%s\n", ($3 != 0) ? "true" : "false");
      }
    | IF OPENBRACKET NEGATION value CLOSEBRACKET {
          printf("%s\n", ($4 == 0) ? "true" : "false");
      }
    | IF OPENBRACKET STRING EQUAL STRING CLOSEBRACKET {
          printf("%s\n", (strcmp($3, $5) == 0) ? "true" : "false");
          free($3); free($5);
      }
    | IF OPENBRACKET STRING NOTEQUAL STRING CLOSEBRACKET {
          printf("%s\n", (strcmp($3, $5) != 0) ? "true" : "false");
          free($3); free($5);
      }
    ;

moltiplicationstmt:
    MOLTIPLICATION value value {
        printf("%.2f\n", $2 * $3);
    }
    ;

divisionstmt:
    DIVISION value value {
        if ($3 == 0) {
            fprintf(stderr, "savo: division by zero\n");
        } else {
            printf("%.2f\n", $2 / $3);
        }
    }
    ;

modstmt:
    MOD value value {
        if ($3 == 0) {
            fprintf(stderr, "savo: modulo by zero\n");
        } else {
            printf("%.2f\n", fmodf($2, $3));
        }
    }
    ;

absstmt:
    ABS value {
        printf("%.2f\n", fabsf($2));
    }
    ;

randomstmt:
    RANDOM value value {
        int lo = (int) $2;
        int hi = (int) $3;
        if (hi < lo) { int t = lo; lo = hi; hi = t; }
        printf("%d\n", lo + rand() % (hi - lo + 1));
    }
    ;

pointerstmt:
    POINTERCELL STRING {
        printf("%s: %p\n", $2, (void *) $2);
        free($2);
    }
    ;

addstmt:
    SUM value value {
        printf("%.2f\n", $2 + $3);
    }
    ;

subtractstmt:
    SUBTRACT value value {
        printf("%.2f\n", $2 - $3);
    }
    ;

printstmt:
      PRINT STRING {
          printStatement($2);
          if (strlen(prompt) > 0) printf("\n");
          free($2);
      }
    | PRINT STRING PLUS value {
          printf("%s%.2f", $2, $4);
          if (strlen(prompt) > 0) printf("\n");
          free($2);
      }
    ;

dirstmt:
      DIR ARGUMENT {
          if (strlen(prompt) > 0) {
              char cmd[600];
              snprintf(cmd, sizeof cmd, "ls %s", $2);
              system(cmd);
          }
          free($2);
      }
    | DIR { if (strlen(prompt) > 0) system("ls"); }
    ;

lsstmt:
      LS ARGUMENT {
          if (strlen(prompt) > 0) {
              char cmd[600];
              snprintf(cmd, sizeof cmd, "ls %s", $2);
              system(cmd);
          }
          free($2);
      }
    | LS { if (strlen(prompt) > 0) system("ls"); }
    ;

helpstmt:
    HELP {
        printf("\n");
        printf("savocls\t\t\t\t\tclear screen (windows)\n");
        printf("savoclear\t\t\t\tclear screen (linux/macOS)\n");
        printf("savodir\t\t<arguments>\t\tlist files (alias of ls)\n");
        printf("savols\t\t<arguments>\t\tlist files in directory\n");
        printf("savoprint\t<\"string\">\t\tprint a string literal\n");
        printf("savoprint\t<\"string\"> + <value>\tprint a string followed by a value\n");
        printf("savovar\t\t<@name> <value>\t\tdefine or update a variable\n");
        printf("savofor\t\t<count> <\"string\">\trepeat a string count times\n");
        printf("savofor\t\t(a, b, s) <\"string\">\tC-style loop from a to b step s\n");
        printf("savowhile\t<count> <\"string\">\trepeat a string count times\n");
        printf("savosum\t\t<value> <value>\t\tadd two values\n");
        printf("savosubtract\t<value> <value>\t\tsubtract two values\n");
        printf("savomoltiplication <value> <value>\tmultiply two values\n");
        printf("savodivide\t<value> <value>\t\tdivide two values\n");
        printf("savomod\t\t<value> <value>\t\tmodulo of two values\n");
        printf("savosqrt\t<value>\t\t\tsquare root\n");
        printf("savopow\t\t<base> <exp>\t\tpower function\n");
        printf("savoabs\t\t<value>\t\t\tabsolute value\n");
        printf("savomax\t\t<value> <value>\t\tmaximum of two values\n");
        printf("savomin\t\t<value> <value>\t\tminimum of two values\n");
        printf("savorandom\t<min> <max>\t\trandom integer in [min, max]\n");
        printf("savoif\t\t(<value> <op> <value>)\tcompare with == != < > <= >=\n");
        printf("savopointercell\t<\"string\">\t\tprint the string's memory address\n");
        printf("savoquit | savoexit\t\t\texit this program\n");
        printf("\n");
    }
    ;

quitstmt:
      QUIT { exit(0); }
    | EXIT { exit(0); }
    ;

clsstmt:
    CLS {
        if (strlen(prompt) > 0) {
            system("cls");
            printf("%s", consoleMex);
        }
    }
    ;

clearstmt:
    CLEAR {
        if (strlen(prompt) > 0) {
            system("clear");
            printf("%s", consoleMex);
        }
    }
    ;

forstmt:
      FOR value STRING {
          forStatement((int) $2, $3);
          free($3);
      }
    | FOR OPENBRACKET value COMMA value COMMA value CLOSEBRACKET STRING {
          int i;
          for (i = (int) $3; i < (int) $5; i += (int) $7) {
              printf("%s\n", $9);
          }
          free($9);
      }
    | FOR OPENBRACKET value COMMA value COMMA value CLOSEBRACKET STRING PLUS value {
          int i;
          for (i = (int) $3; i < (int) $5; i += (int) $7) {
              printf("%s%.0f\n", $9, i + $11);
          }
          free($9);
      }
    | FOR OPENBRACKET value COMMA value COMMA value CLOSEBRACKET STRING MULTIPLY value {
          int i;
          for (i = (int) $3; i < (int) $5; i += (int) $7) {
              printf("%s%.0f\n", $9, i * $11);
          }
          free($9);
      }
    ;

whilestmt:
    WHILE value STRING {
        whileStatement((int) $2, $3);
        free($3);
    }
    ;

varstmt:
    VAR IDENTIFIER value {
        symtab_set($2, $3);
        printf("Variabile %s = %.2f\n", $2, $3);
        free($2);
    }
    ;

sqrtstmt:
    SQRT value {
        printf("%s%.2f = %.2f\n", "√", $2, sqrtf($2));
    }
    ;

powstmt:
    POW value value {
        printf("%.2f^%.2f = %.2f\n", $2, $3, powf($2, $3));
    }
    ;

maxstmt:
    MAX value value {
        printf("max(%.2f, %.2f) = %.2f\n", $2, $3, ($2 > $3) ? $2 : $3);
    }
    ;

minstmt:
    MIN value value {
        printf("min(%.2f, %.2f) = %.2f\n", $2, $3, ($2 < $3) ? $2 : $3);
    }
    ;

%%
