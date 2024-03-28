%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "global.h"
	#include "statements/printstmt.h"
	#include "statements/forstmt.h"
	#include "statements/whilestmt.h"
	#include "y.tab.h"
	int yyerror(const char *errormsg);
%}

Number        ([0-9]+)|([0-9]+\.[0-9]+)|(\.[0-9]+)
Argument      (([-*./|a-zA-Z])|([-*./|_a-zA-Z][-*./|_a-zA-Z0-9]+))
String        \"(([^\"]*)|([\!]*))\"
Identifier    @?(([a-zA-Z])|([_a-zA-Z][_a-zA-Z0-9]+))

A [aA]
B [bB]
C [cC]
D [dD]
E [eE]
F [fF]
G [gG]
H [hH]
I [iI]
J [jJ]
K [kK]
L [lL]
M [mM]
N [nN]
O [oO]
P [pP]
Q [qQ]
R [rR]
S [sS]
T [tT]
U [uU]
V [vV]
W [wW]
X [xX]
Y [yY]
Z [zZ]
NQUOTE [^']

%%
{S}{A}{V}{O}{P}{R}{I}{N}{T}       { return PRINT; }
{S}{A}{V}{O}{C}{L}{E}{A}{R}       { return CLEAR; }
{S}{A}{V}{O}{H}{E}{L}{P}	      { return HELP; }
{S}{A}{V}{O}{Q}{U}{I}{T}	      { return QUIT; }
{S}{A}{V}{O}{E}{X}{I}{T}	      { return EXIT; }
{S}{A}{V}{O}{D}{I}{R}             { return DIR; }
{S}{A}{V}{O}{C}{L}{S}             { return CLS; }
{S}{A}{V}{O}{F}{O}{R}             { return FOR; }
{S}{A}{V}{O}{W}{H}{I}{L}{E}       { return WHILE; }
{S}{A}{V}{O}{S}{U}{M}             { return SUM; }
{S}{A}{V}{O}{S}{U}{B}{T}{R}{A}{C}{T}   { return SUBTRACT; }
{S}{A}{V}{O}{P}{O}{I}{N}{T}{E}{R}{C}{E}{L}{L}   { return POINTERCELL; }

"("                   { return OPENBRACKET;}
")"                   { return CLOSEBRACKET;}
","                   { return COMMA;}
[\n]                  { if(strlen(prompt) > 0) printf(prompt); }
[ \t\v\f]+            {}
{String}              { yylval.string = strdup( yytext + 1 ); yylval.string[ yyleng - 2 ] = 0; return STRING; }
{Number}              { yylval.fNumber = atof( yytext ); return NUMBER; }
{Argument}            { yylval.string = strdup( yytext); return ARGUMENT; }
{Identifier}          { yylval.string = strdup( yytext); return IDENTIFIER; }

%%
int main(int argc, char **argv) {
    ++argv;
    yyin = stdin;
    if ( --argc > 0 ) {
        prompt = "";
        yyin = fopen(argv[0], "r");
    } else {
        printf("%s", consoleMex);
        printf(prompt);
    }
    yyparse();
    return 0;
}

int yywrap(void) {
   return 0;
}

int yyerror(const char *errormsg) {
   fprintf(stderr, "%s\n", errormsg);
   exit(1);
}
