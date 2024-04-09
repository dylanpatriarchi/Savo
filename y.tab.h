
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     DIR = 258,
     HELP = 259,
     PRINT = 260,
     QUIT = 261,
     CLEAR = 262,
     CLS = 263,
     IDENTIFIER = 264,
     FOR = 265,
     WHILE = 266,
     SUM = 267,
     SUBTRACT = 268,
     POINTERCELL = 269,
     MOLTIPLICATION = 270,
     EQUAL = 271,
     IF = 272,
     ARGUMENT = 273,
     NUMBER = 274,
     STRING = 275,
     EXIT = 276,
     OPENBRACKET = 277,
     CLOSEBRACKET = 278,
     COMMA = 279,
     NEGATION = 280
   };
#endif
/* Tokens.  */
#define DIR 258
#define HELP 259
#define PRINT 260
#define QUIT 261
#define CLEAR 262
#define CLS 263
#define IDENTIFIER 264
#define FOR 265
#define WHILE 266
#define SUM 267
#define SUBTRACT 268
#define POINTERCELL 269
#define MOLTIPLICATION 270
#define EQUAL 271
#define IF 272
#define ARGUMENT 273
#define NUMBER 274
#define STRING 275
#define EXIT 276
#define OPENBRACKET 277
#define CLOSEBRACKET 278
#define COMMA 279
#define NEGATION 280




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 13 "parser.y"

   char * string;
   int    number;
   float  fNumber;
   void * pVoid;



/* Line 1676 of yacc.c  */
#line 111 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


