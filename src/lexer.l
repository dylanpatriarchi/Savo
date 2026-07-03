%{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <unistd.h>
	#include "global.h"
	#include "symtab.h"
	#include "ast.h"
	#include "parser.tab.h"
	int yyerror(const char *errormsg);
	int yyparse(void);

	/* Set to 1 by yyerror or a runtime error; becomes the process exit status. */
	int had_error = 0;

	/* Rewrite common escape sequences (\n \t \r \\ \") in place. */
	static void unescape(char *s) {
		char *w = s;
		char *r;
		for (r = s; *r; r++) {
			if (*r == '\\' && *(r + 1)) {
				r++;
				switch (*r) {
					case 'n':  *w++ = '\n'; break;
					case 't':  *w++ = '\t'; break;
					case 'r':  *w++ = '\r'; break;
					case '\\': *w++ = '\\'; break;
					case '"':  *w++ = '"';  break;
					default:   *w++ = '\\'; *w++ = *r; break;
				}
			} else {
				*w++ = *r;
			}
		}
		*w = 0;
	}
%}

%option yylineno
/* ARG collects the rest of the line as a shell argument, but only right after
 * savodir / savols. Keeping it in its own state stops paths and flags (which
 * may start with '-') from clashing with the minus operator elsewhere. */
%x ARG

Number        ([0-9]+)|([0-9]+\.[0-9]+)|(\.[0-9]+)
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

%%
{S}{A}{V}{O}{P}{R}{I}{N}{T}       { return PRINT; }
{S}{A}{V}{O}{C}{L}{E}{A}{R}       { return CLEAR; }
{S}{A}{V}{O}{H}{E}{L}{P}	      { return HELP; }
{S}{A}{V}{O}{Q}{U}{I}{T}	      { return QUIT; }
{S}{A}{V}{O}{E}{X}{I}{T}	      { return EXIT; }
{S}{A}{V}{O}{D}{I}{R}             { BEGIN(ARG); return DIR; }
{S}{A}{V}{O}{L}{S}                { BEGIN(ARG); return LS; }
{S}{A}{V}{O}{C}{L}{S}             { return CLS; }
{S}{A}{V}{O}{F}{O}{R}             { return FOR; }
{S}{A}{V}{O}{W}{H}{I}{L}{E}       { return WHILE; }
{S}{A}{V}{O}{S}{U}{M}             { return SUM; }
{S}{A}{V}{O}{S}{U}{B}{T}{R}{A}{C}{T}   { return SUBTRACT; }
{S}{A}{V}{O}{P}{O}{I}{N}{T}{E}{R}{C}{E}{L}{L}   { return POINTERCELL; }
{S}{A}{V}{O}{M}{O}{L}{T}{I}{P}{L}{I}{C}{A}{T}{I}{O}{N}   { return MOLTIPLICATION; }
{S}{A}{V}{O}{D}{I}{V}{I}{D}{E}    { return DIVISION; }
{S}{A}{V}{O}{M}{O}{D}             { return MOD; }
{S}{A}{V}{O}{A}{B}{S}             { return ABS; }
{S}{A}{V}{O}{R}{A}{N}{D}{O}{M}    { return RANDOM; }
{S}{A}{V}{O}{F}{L}{O}{O}{R}       { return FLOOR; }
{S}{A}{V}{O}{C}{E}{I}{L}          { return CEIL; }
{S}{A}{V}{O}{R}{O}{U}{N}{D}       { return ROUND; }
{S}{A}{V}{O}{L}{O}{G}10           { return LOG10; }
{S}{A}{V}{O}{L}{O}{G}             { return LOG; }
{S}{A}{V}{O}{I}{F}                { return IF; }
{S}{A}{V}{O}{E}{L}{S}{E}          { return SAVOELSE; }
{S}{A}{V}{O}{E}{N}{D}             { return SAVOEND; }
{S}{A}{V}{O}{D}{E}{F}             { return SAVODEF; }
{S}{A}{V}{O}{R}{E}{T}{U}{R}{N}    { return SAVORETURN; }
{S}{A}{V}{O}{V}{A}{R}             { return VAR; }
{S}{A}{V}{O}{S}{Q}{R}{T}          { return SQRT; }
{S}{A}{V}{O}{P}{O}{W}             { return POW; }
{S}{A}{V}{O}{M}{A}{X}             { return MAX; }
{S}{A}{V}{O}{M}{I}{N}             { return MIN; }
"<="                  { return LE; }
">="                  { return GE; }
"!="                  { return NOTEQUAL; }
"=="                  { return EQUAL; }
"="                   { return ASSIGN; }
"<"                   { return LT; }
">"                   { return GT; }
"("                   { return OPENBRACKET; }
"!"                   { return NEGATION; }
")"                   { return CLOSEBRACKET; }
","                   { return COMMA; }
"+"                   { return PLUS; }
"-"                   { return MINUS; }
"*"                   { return MULTIPLY; }
"/"                   { return DIVIDE; }
"%"                   { return PERCENT; }
#[^\n]*               { }
[\n]                  { return NEWLINE; }
[ \t\v\f\r]+          { }
{String}              { yylval.string = strdup( yytext + 1 ); yylval.string[ yyleng - 2 ] = 0; unescape( yylval.string ); return STRING; }
{Number}              { yylval.fNumber = atof( yytext ); return NUMBER; }
{Identifier}          { yylval.string = strdup( yytext); return IDENTIFIER; }

<ARG>[ \t]+           { }
<ARG>#[^\n]*          { }
<ARG>[^ \t\n#][^\n]*  { yylval.string = strdup( yytext ); BEGIN(INITIAL); return ARGUMENT; }
<ARG>\n               { BEGIN(INITIAL); return NEWLINE; }

<<EOF>>               { static int nl = 0; if (!nl) { nl = 1; return NEWLINE; } return 0; }

%%
static void usage(const char *prog) {
    printf("Savo %s — a tiny scripting language\n\n", SAVO_VERSION);
    printf("Usage:\n");
    printf("  %s [file]        run a .savo script\n", prog);
    printf("  %s -e \"code\"     run code passed on the command line\n", prog);
    printf("  %s -              read a script from stdin\n", prog);
    printf("  %s                start the interactive REPL\n\n", prog);
    printf("Options:\n");
    printf("  -e, --eval CODE   evaluate CODE and exit\n");
    printf("  -h, --help        show this help and exit\n");
    printf("  -v, --version     show the version and exit\n");
}

int main(int argc, char **argv) {
    const char *prog = argv[0];
    const char *filename = NULL;
    const char *evalcode = NULL;
    int i;

    srand((unsigned int) time(NULL));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(prog);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Savo %s\n", SAVO_VERSION);
            return 0;
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--eval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "savo: option '%s' requires an argument\n", argv[i]);
                return 2;
            }
            evalcode = argv[++i];
        } else if (argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "savo: unknown option '%s' (try --help)\n", argv[i]);
            return 2;
        } else {
            filename = argv[i];
        }
    }

    if (evalcode != NULL) {
        prompt = "";
        yy_scan_string(evalcode);
    } else if (filename != NULL && strcmp(filename, "-") != 0) {
        prompt = "";
        yyin = fopen(filename, "r");
        if (yyin == NULL) {
            fprintf(stderr, "savo: cannot open file '%s'\n", filename);
            return 1;
        }
    } else if (filename != NULL) {   /* explicit "-" -> stdin as a script */
        prompt = "";
        yyin = stdin;
    } else if (isatty(fileno(stdin))) {
        prompt = ">>> ";
        yyin = stdin;
        printf("%s", consoleMex);
        printf("SavoLanguage Console %s\n", SAVO_VERSION);
        printf("digit savohelp for some tips.\n\n");
        printf("%s", prompt);
    } else {
        prompt = "";
        yyin = stdin;
    }

    yyparse();
    symtab_free();
    return had_error;
}

int yywrap(void) {
   return 1;
}

int yyerror(const char *errormsg) {
   fprintf(stderr, "savo: line %d: %s\n", yylineno, errormsg);
   had_error = 1;
   return 0;
}
