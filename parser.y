%{
  #include <stdio.h>
  #include <stdlib.h>
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

%token DIR HELP PRINT QUIT CLEAR CLS IDENTIFIER FOR WHILE SUM SUBTRACT POINTERCELL
%token ARGUMENT NUMBER STRING EXIT OPENBRACKET CLOSEBRACKET COMMA

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

whilestmt:
         WHILE NUMBER STRING{ 
		          whileStatement($2, $3);
			      }
         ;

%%
