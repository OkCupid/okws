/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

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

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_ID = 258,
     T_NUM = 259,
     T_CONST = 260,
     T_STRUCT = 261,
     T_UNION = 262,
     T_ENUM = 263,
     T_TYPEDEF = 264,
     T_PROGRAM = 265,
     T_UNSIGNED = 266,
     T_INT = 267,
     T_HYPER = 268,
     T_DOUBLE = 269,
     T_QUADRUPLE = 270,
     T_VOID = 271,
     T_VERSION = 272,
     T_SWITCH = 273,
     T_CASE = 274,
     T_DEFAULT = 275,
     T_OPAQUE = 276,
     T_STRING = 277
   };
#endif
/* Tokens.  */
#define T_ID 258
#define T_NUM 259
#define T_CONST 260
#define T_STRUCT 261
#define T_UNION 262
#define T_ENUM 263
#define T_TYPEDEF 264
#define T_PROGRAM 265
#define T_UNSIGNED 266
#define T_INT 267
#define T_HYPER 268
#define T_DOUBLE 269
#define T_QUADRUPLE 270
#define T_VOID 271
#define T_VERSION 272
#define T_SWITCH 273
#define T_CASE 274
#define T_DEFAULT 275
#define T_OPAQUE 276
#define T_STRING 277




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



