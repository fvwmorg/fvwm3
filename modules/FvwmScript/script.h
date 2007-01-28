/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

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
     STR = 258,
     GSTR = 259,
     VAR = 260,
     FONT = 261,
     NUMBER = 262,
     WINDOWTITLE = 263,
     WINDOWLOCALETITLE = 264,
     WINDOWSIZE = 265,
     WINDOWPOSITION = 266,
     USEGETTEXT = 267,
     FORECOLOR = 268,
     BACKCOLOR = 269,
     SHADCOLOR = 270,
     LICOLOR = 271,
     COLORSET = 272,
     OBJECT = 273,
     INIT = 274,
     PERIODICTASK = 275,
     QUITFUNC = 276,
     MAIN = 277,
     END = 278,
     PROP = 279,
     TYPE = 280,
     SIZE = 281,
     POSITION = 282,
     VALUE = 283,
     VALUEMIN = 284,
     VALUEMAX = 285,
     TITLE = 286,
     SWALLOWEXEC = 287,
     ICON = 288,
     FLAGS = 289,
     WARP = 290,
     WRITETOFILE = 291,
     LOCALETITLE = 292,
     HIDDEN = 293,
     NOFOCUS = 294,
     NORELIEFSTRING = 295,
     CENTER = 296,
     LEFT = 297,
     RIGHT = 298,
     CASE = 299,
     SINGLECLIC = 300,
     DOUBLECLIC = 301,
     BEG = 302,
     POINT = 303,
     EXEC = 304,
     HIDE = 305,
     SHOW = 306,
     CHFONT = 307,
     CHFORECOLOR = 308,
     CHBACKCOLOR = 309,
     CHCOLORSET = 310,
     KEY = 311,
     GETVALUE = 312,
     GETMINVALUE = 313,
     GETMAXVALUE = 314,
     GETFORE = 315,
     GETBACK = 316,
     GETHILIGHT = 317,
     GETSHADOW = 318,
     CHVALUE = 319,
     CHVALUEMAX = 320,
     CHVALUEMIN = 321,
     ADD = 322,
     DIV = 323,
     MULT = 324,
     GETTITLE = 325,
     GETOUTPUT = 326,
     STRCOPY = 327,
     NUMTOHEX = 328,
     HEXTONUM = 329,
     QUIT = 330,
     LAUNCHSCRIPT = 331,
     GETSCRIPTFATHER = 332,
     SENDTOSCRIPT = 333,
     RECEIVFROMSCRIPT = 334,
     GET = 335,
     SET = 336,
     SENDSIGN = 337,
     REMAINDEROFDIV = 338,
     GETTIME = 339,
     GETSCRIPTARG = 340,
     GETPID = 341,
     SENDMSGANDGET = 342,
     PARSE = 343,
     LASTSTRING = 344,
     GETTEXT = 345,
     IF = 346,
     THEN = 347,
     ELSE = 348,
     FOR = 349,
     TO = 350,
     DO = 351,
     WHILE = 352,
     BEGF = 353,
     ENDF = 354,
     EQUAL = 355,
     INFEQ = 356,
     SUPEQ = 357,
     INF = 358,
     SUP = 359,
     DIFF = 360
   };
#endif
#define STR 258
#define GSTR 259
#define VAR 260
#define FONT 261
#define NUMBER 262
#define WINDOWTITLE 263
#define WINDOWLOCALETITLE 264
#define WINDOWSIZE 265
#define WINDOWPOSITION 266
#define USEGETTEXT 267
#define FORECOLOR 268
#define BACKCOLOR 269
#define SHADCOLOR 270
#define LICOLOR 271
#define COLORSET 272
#define OBJECT 273
#define INIT 274
#define PERIODICTASK 275
#define QUITFUNC 276
#define MAIN 277
#define END 278
#define PROP 279
#define TYPE 280
#define SIZE 281
#define POSITION 282
#define VALUE 283
#define VALUEMIN 284
#define VALUEMAX 285
#define TITLE 286
#define SWALLOWEXEC 287
#define ICON 288
#define FLAGS 289
#define WARP 290
#define WRITETOFILE 291
#define LOCALETITLE 292
#define HIDDEN 293
#define NOFOCUS 294
#define NORELIEFSTRING 295
#define CENTER 296
#define LEFT 297
#define RIGHT 298
#define CASE 299
#define SINGLECLIC 300
#define DOUBLECLIC 301
#define BEG 302
#define POINT 303
#define EXEC 304
#define HIDE 305
#define SHOW 306
#define CHFONT 307
#define CHFORECOLOR 308
#define CHBACKCOLOR 309
#define CHCOLORSET 310
#define KEY 311
#define GETVALUE 312
#define GETMINVALUE 313
#define GETMAXVALUE 314
#define GETFORE 315
#define GETBACK 316
#define GETHILIGHT 317
#define GETSHADOW 318
#define CHVALUE 319
#define CHVALUEMAX 320
#define CHVALUEMIN 321
#define ADD 322
#define DIV 323
#define MULT 324
#define GETTITLE 325
#define GETOUTPUT 326
#define STRCOPY 327
#define NUMTOHEX 328
#define HEXTONUM 329
#define QUIT 330
#define LAUNCHSCRIPT 331
#define GETSCRIPTFATHER 332
#define SENDTOSCRIPT 333
#define RECEIVFROMSCRIPT 334
#define GET 335
#define SET 336
#define SENDSIGN 337
#define REMAINDEROFDIV 338
#define GETTIME 339
#define GETSCRIPTARG 340
#define GETPID 341
#define SENDMSGANDGET 342
#define PARSE 343
#define LASTSTRING 344
#define GETTEXT 345
#define IF 346
#define THEN 347
#define ELSE 348
#define FOR 349
#define TO 350
#define DO 351
#define WHILE 352
#define BEGF 353
#define ENDF 354
#define EQUAL 355
#define INFEQ 356
#define SUPEQ 357
#define INF 358
#define SUP 359
#define DIFF 360




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 368 "script.y"
typedef union YYSTYPE {  char *str;
          int number;
       } YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 251 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



