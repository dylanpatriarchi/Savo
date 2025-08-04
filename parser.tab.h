/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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
     NOTEQUAL = 273,
     VAR = 274,
     SQRT = 275,
     POW = 276,
     MAX = 277,
     MIN = 278,
     PLUS = 279,
     MINUS = 280,
     MULTIPLY = 281,
     DIVIDE = 282,
     ARGUMENT = 283,
     NUMBER = 284,
     STRING = 285,
     EXIT = 286,
     OPENBRACKET = 287,
     CLOSEBRACKET = 288,
     COMMA = 289,
     NEGATION = 290
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
#define NOTEQUAL 273
#define VAR 274
#define SQRT 275
#define POW 276
#define MAX 277
#define MIN 278
#define PLUS 279
#define MINUS 280
#define MULTIPLY 281
#define DIVIDE 282
#define ARGUMENT 283
#define NUMBER 284
#define STRING 285
#define EXIT 286
#define OPENBRACKET 287
#define CLOSEBRACKET 288
#define COMMA 289
#define NEGATION 290




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 14 "parser.y"
{
   char * string;
   int    number;
   float  fNumber;
   void * pVoid;
}
/* Line 1529 of yacc.c.  */
#line 126 "parser.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

