/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see: <http://www.gnu.org/licenses/>
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
     CHWINDOWTITLE = 311,
     CHWINDOWTITLEFARG = 312,
     KEY = 313,
     GETVALUE = 314,
     GETMINVALUE = 315,
     GETMAXVALUE = 316,
     GETFORE = 317,
     GETBACK = 318,
     GETHILIGHT = 319,
     GETSHADOW = 320,
     CHVALUE = 321,
     CHVALUEMAX = 322,
     CHVALUEMIN = 323,
     ADD = 324,
     DIV = 325,
     MULT = 326,
     GETTITLE = 327,
     GETOUTPUT = 328,
     STRCOPY = 329,
     NUMTOHEX = 330,
     HEXTONUM = 331,
     QUIT = 332,
     LAUNCHSCRIPT = 333,
     GETSCRIPTFATHER = 334,
     SENDTOSCRIPT = 335,
     RECEIVFROMSCRIPT = 336,
     GET = 337,
     SET = 338,
     SENDSIGN = 339,
     REMAINDEROFDIV = 340,
     GETTIME = 341,
     GETSCRIPTARG = 342,
     GETPID = 343,
     SENDMSGANDGET = 344,
     PARSE = 345,
     LASTSTRING = 346,
     GETTEXT = 347,
     IF = 348,
     THEN = 349,
     ELSE = 350,
     FOR = 351,
     TO = 352,
     DO = 353,
     WHILE = 354,
     BEGF = 355,
     ENDF = 356,
     EQUAL = 357,
     INFEQ = 358,
     SUPEQ = 359,
     INF = 360,
     SUP = 361,
     DIFF = 362
   };
#endif
/* Tokens.  */
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
#define CHWINDOWTITLE 311
#define CHWINDOWTITLEFARG 312
#define KEY 313
#define GETVALUE 314
#define GETMINVALUE 315
#define GETMAXVALUE 316
#define GETFORE 317
#define GETBACK 318
#define GETHILIGHT 319
#define GETSHADOW 320
#define CHVALUE 321
#define CHVALUEMAX 322
#define CHVALUEMIN 323
#define ADD 324
#define DIV 325
#define MULT 326
#define GETTITLE 327
#define GETOUTPUT 328
#define STRCOPY 329
#define NUMTOHEX 330
#define HEXTONUM 331
#define QUIT 332
#define LAUNCHSCRIPT 333
#define GETSCRIPTFATHER 334
#define SENDTOSCRIPT 335
#define RECEIVFROMSCRIPT 336
#define GET 337
#define SET 338
#define SENDSIGN 339
#define REMAINDEROFDIV 340
#define GETTIME 341
#define GETSCRIPTARG 342
#define GETPID 343
#define SENDMSGANDGET 344
#define PARSE 345
#define LASTSTRING 346
#define GETTEXT 347
#define IF 348
#define THEN 349
#define ELSE 350
#define FOR 351
#define TO 352
#define DO 353
#define WHILE 354
#define BEGF 355
#define ENDF 356
#define EQUAL 357
#define INFEQ 358
#define SUPEQ 359
#define INF 360
#define SUP 361
#define DIFF 362




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 368 "script.y"
{  char *str;
          int number;
       }
/* Line 1489 of yacc.c.  */
#line 267 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

