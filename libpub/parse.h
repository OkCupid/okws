
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
     T_2L_BRACE = 258,
     T_2R_BRACE = 259,
     T_P3_EQEQ = 260,
     T_P3_NEQ = 261,
     T_P3_GTEQ = 262,
     T_P3_LTEQ = 263,
     T_P3_OR = 264,
     T_P3_AND = 265,
     T_P3_IF = 266,
     T_P3_FALSE = 267,
     T_P3_BEGIN_EXPR = 268,
     T_P3_INCLUDE = 269,
     T_P3_LOAD = 270,
     T_P3_LOCALS = 271,
     T_P3_UNIVERSALS = 272,
     T_P3_GLOBALS = 273,
     T_P3_PIPE = 274,
     T_P3_OPEN = 275,
     T_P3_CLOSE = 276,
     T_P3_PRINT = 277,
     T_P3_FOR = 278,
     T_P3_TRUE = 279,
     T_P3_ELIF = 280,
     T_P3_ELSE = 281,
     T_P3_EMPTY = 282,
     T_P3_NULL = 283,
     T_P3_CASE = 284,
     T_P3_SWITCH = 285,
     T_P3_DEF = 286,
     T_P3_LAMBDA = 287,
     T_P3_BREAK = 288,
     T_P3_RETURN = 289,
     T_P3_CONTINUE = 290,
     T_P3_HTML_HEREDOC = 291,
     T_P3_PUB_HEREDOC = 292,
     T_P3_IDENTIFIER = 293,
     T_P3_INT = 294,
     T_P3_UINT = 295,
     T_P3_CHAR = 296,
     T_P3_FLOAT = 297,
     T_P3_STRING = 298,
     T_P3_REGEX = 299,
     T_P3_HTML = 300,
     T_P3_HTML_CH = 301,
     T_P3_BEGIN_PRE = 302,
     T_P3_END_PRE = 303
   };
#endif
/* Tokens.  */
#define T_2L_BRACE 258
#define T_2R_BRACE 259
#define T_P3_EQEQ 260
#define T_P3_NEQ 261
#define T_P3_GTEQ 262
#define T_P3_LTEQ 263
#define T_P3_OR 264
#define T_P3_AND 265
#define T_P3_IF 266
#define T_P3_FALSE 267
#define T_P3_BEGIN_EXPR 268
#define T_P3_INCLUDE 269
#define T_P3_LOAD 270
#define T_P3_LOCALS 271
#define T_P3_UNIVERSALS 272
#define T_P3_GLOBALS 273
#define T_P3_PIPE 274
#define T_P3_OPEN 275
#define T_P3_CLOSE 276
#define T_P3_PRINT 277
#define T_P3_FOR 278
#define T_P3_TRUE 279
#define T_P3_ELIF 280
#define T_P3_ELSE 281
#define T_P3_EMPTY 282
#define T_P3_NULL 283
#define T_P3_CASE 284
#define T_P3_SWITCH 285
#define T_P3_DEF 286
#define T_P3_LAMBDA 287
#define T_P3_BREAK 288
#define T_P3_RETURN 289
#define T_P3_CONTINUE 290
#define T_P3_HTML_HEREDOC 291
#define T_P3_PUB_HEREDOC 292
#define T_P3_IDENTIFIER 293
#define T_P3_INT 294
#define T_P3_UINT 295
#define T_P3_CHAR 296
#define T_P3_FLOAT 297
#define T_P3_STRING 298
#define T_P3_REGEX 299
#define T_P3_HTML 300
#define T_P3_HTML_CH 301
#define T_P3_BEGIN_PRE 302
#define T_P3_END_PRE 303




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


