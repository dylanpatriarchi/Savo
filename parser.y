%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include "global.h"
  #include <string.h>
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

%type <string>  STRING
%type <string>  ARGUMENT
%type <string>  IDENTIFIER
%type <fNumber> NUMBER

%%

program :  commands 
         | commands program;

commands:  
           printstmt 
         | dirstmt
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
       | ifstmt
       | varstmt
       | sqrtstmt
       | powstmt
       | maxstmt
       | minstmt
         ;

ifstmt:
    IF OPENBRACKET NUMBER EQUAL NUMBER CLOSEBRACKET {
        if ($3 == $5) {
            printf("true\n");
        }else{
            printf("false\n");
        }
    } ;
    | IF OPENBRACKET NUMBER CLOSEBRACKET {
            if($3){
                printf("true\n");
            }else{
                printf("false\n");
            }
        };
    | IF OPENBRACKET NEGATION NUMBER CLOSEBRACKET {
            if(!$4){
                printf("true\n");
            }else{
                printf("false\n");
            }
        };
    | IF OPENBRACKET STRING EQUAL STRING CLOSEBRACKET{
            if(strcmp($3, $5) == 0){
                printf("true\n");
            }else{
                printf("false\n");
            }
        };
    | IF OPENBRACKET NUMBER NOTEQUAL NUMBER CLOSEBRACKET {
        if ($3 != $5) {
            printf("true\n");
        }else{
            printf("false\n");
        }
    } ;
    | IF OPENBRACKET STRING NOTEQUAL STRING CLOSEBRACKET{
            if(strcmp($3, $5) != 0){
                printf("true\n");
            }else{
                printf("false\n");
            }
        };
    

moltiplicationstmt:
    MOLTIPLICATION NUMBER NUMBER{
        float result = $2 * $3;
        printf("%.2f\n", result);
    }
    ;

pointerstmt:
    POINTERCELL STRING {
        printf("%s: %p\n", $2, &($2));
    }
    ;


addstmt:
    SUM NUMBER NUMBER {
        float result = $2 + $3;
        printf("%.2f\n", result);
    }
    ;

subtractstmt:
    SUBTRACT NUMBER NUMBER {
        float result = $2 - $3;
        printf("%.2f\n", result);
    }
    ;

printstmt:
         PRINT STRING { 
		                    printStatement($2);    
						        if(strlen(prompt) > 0) printf("\n");
                      }
         | PRINT STRING PLUS NUMBER {
             printf("%s%.2f", $2, $4);
             if(strlen(prompt) > 0) printf("\n");
         }
         | PRINT STRING MULTIPLY NUMBER {
             printf("%s%.2f", $2, $4);
             if(strlen(prompt) > 0) printf("\n");
         }
         ;

dirstmt:
           DIR ARGUMENT {  
		                     if(strlen(prompt) > 0) {
		                        char *dest;
		                        char *src;
						            strcpy(dest, "dir ");
						            strcpy(src, $2);
						            strcat(dest, src);
		                        system(dest); 
						         }
					         }
		   | DIR { if(strlen(prompt) > 0) system("dir *.*"); }
         ;

helpstmt:
         HELP {
		           printf("\n");
                 printf("savocls\t\t\t\tclear screen (windows)\n");
                 printf("savoclear\t\t\t\tclear screen (linux)\n");
                 printf("savodelete\t<\"file name\">\t\tdelete file in argument (windows)\n");
                 printf("savodir\t<arguments>\t\tlist files in directory (windows)\n");
                 printf("savoEndPage\t<\"file name\">\t\tclose file HTML converted\n");
                 printf("savoLs\t<arguments>\t\tlist files in directory (linux)\n");
                 printf("savoPage\t<\"file\", \"Title\">\tcreate file HTML to convert\n");
                 printf("savoPrint\t<string literal>\tprint to stdout/file string literal in argument\n");
                 printf("savofor\t<open bracket><index><comma><index><comma><index><close bracket><string literal> or <index><string literal>\t\n");
                 printf("savoPointerCell\t<string literal>\tprint memory address\n");
                 printf("savoSubtract\t<number literal><number literal>\tSubtract Numbers\n");
                 printf("savoAdd\t<number literal><number literal>\tAdd Numbers\n");
                 printf("savoMoltiplication\t<number literal><number literal>\tMoltiplications of Numbers\n");
                 printf("savoIf\t<condition>\t\n");
                 printf("savoVar\t<variable><number>\t\tDefine a variable\n");
                 printf("savoSqrt\t<number>\t\t\tSquare root\n");
                 printf("savoPow\t<base><exponent>\t\tPower function\n");
                 printf("savoMax\t<num1><num2>\t\t\tMaximum of two numbers\n");
                 printf("savoMin\t<num1><num2>\t\t\tMinimum of two numbers\n");
                 printf("savoquit | Exit\t\t\tExit this program\n");
                 printf("savorm\t<\"file name\">\t\tdelete file in argument (linux)\n");
		           printf("\n");
              }
         ;

quitstmt:
         QUIT { exit(0); }
		 | EXIT { exit(0); }
         ;
		
clsstmt:
         CLS { 
		          if(strlen(prompt) > 0) {
			          system("cls"); 
	                printf("%s", consoleMex);
			       }	  
		       }
         ;

clearstmt:
         CLEAR { 
		            if(strlen(prompt) > 0) {
				         system("clear"); 
	                  printf("%s", consoleMex);
				      } 
			      }
         ;

forstmt:
         FOR NUMBER STRING{ 
		          forStatement($2, $3);
			      }
         ;
         | FOR OPENBRACKET NUMBER COMMA NUMBER COMMA NUMBER CLOSEBRACKET STRING{
            int i = 0;
            for(i = $3; i < $5; i = i + $7){
                printf("%s\n", $9);
            }
         }
         | FOR OPENBRACKET NUMBER COMMA NUMBER COMMA NUMBER CLOSEBRACKET STRING PLUS NUMBER{
            int i = 0;
            for(i = $3; i < $5; i = i + $7){
                printf("%s%.0f\n", $9, i + $11);
            }
         }
         | FOR OPENBRACKET NUMBER COMMA NUMBER COMMA NUMBER CLOSEBRACKET STRING MULTIPLY NUMBER{
            int i = 0;
            for(i = $3; i < $5; i = i + $7){
                printf("%s%.0f\n", $9, i * $11);
            }
         }
         ;

whilestmt:
         WHILE NUMBER STRING{ 
		          whileStatement($2, $3);
			      }
         ;

varstmt:
    VAR IDENTIFIER NUMBER {
        printf("Variabile %s = %.2f\n", $2, $3);
    }
    ;

sqrtstmt:
    SQRT NUMBER {
        float result = sqrt($2);
        printf("√%.2f = %.2f\n", $2, result);
    }
    ;

powstmt:
    POW NUMBER NUMBER {
        float result = pow($2, $3);
        printf("%.2f^%.2f = %.2f\n", $2, $3, result);
    }
    ;

maxstmt:
    MAX NUMBER NUMBER {
        float result = ($2 > $3) ? $2 : $3;
        printf("max(%.2f, %.2f) = %.2f\n", $2, $3, result);
    }
    ;

minstmt:
    MIN NUMBER NUMBER {
        float result = ($2 < $3) ? $2 : $3;
        printf("min(%.2f, %.2f) = %.2f\n", $2, $3, result);
    }
    ;

%%
