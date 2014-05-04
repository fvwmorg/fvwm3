/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "script.y"

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */
#include "config.h"
#include "types.h"
#include "libs/FGettext.h"

#define MAX_VARS 5120
extern int numligne;
ScriptProp *scriptprop;
int nbobj=-1;			/* Nombre d'objets */
int HasPosition,HasType=0;
TabObj *tabobj;		/* Tableau d'objets, limite=1000 */
int TabIdObj[1001]; 	/* Tableau d'indice des objets */
Bloc **TabIObj;		/* TabIObj[Obj][Case] -> bloc attache au case */
Bloc *PileBloc[10];	/* Au maximum 10 imbrications de boucle conditionnelle */
int TopPileB=0;		/* Sommet de la pile des blocs */
CaseObj *TabCObj;	/* Struct pour enregistrer les valeurs des cases et leur nb */
int CurrCase;
int i;
char **TabNVar;		/* Tableau des noms de variables */
char **TabVVar;		/* Tableau des valeurs de variables */
int NbVar;
long BuffArg[6][20];	/* Les arguments s'ajoute par couche pour chaque fonction imbriquee */
int NbArg[6];		/* Tableau: nb d'args pour chaque couche */
int SPileArg;		/* Taille de la pile d'arguments */
long l;
extern char* ScriptName;

/* Initialisation globale */
void InitVarGlob(void)
{
 scriptprop=xcalloc(1, sizeof(ScriptProp));
 scriptprop->x=-1;
 scriptprop->y=-1;
 scriptprop->colorset = -1;
 scriptprop->initbloc=NULL;

 tabobj=xcalloc(1, sizeof(TabObj));
 for (i=0;i<1001;i++)
  TabIdObj[i]=-1;
 TabNVar=NULL;
 TabVVar=NULL;
 NbVar=-1;

 SPileArg=-1;
 scriptprop->usegettext = False;
 scriptprop->periodictasks=NULL;
 scriptprop->quitfunc=NULL;
}

/* Initialisation pour un objet */
void InitObjTabCase(int HasMainLoop)
{
 if (nbobj==0)
 {
  TabIObj=xcalloc(1, sizeof(long));
  TabCObj=xcalloc(1, sizeof(CaseObj));
 }
 else
 {
  TabIObj=(Bloc**)realloc(TabIObj,sizeof(long)*(nbobj+1));
  TabCObj=(CaseObj*)realloc(TabCObj,sizeof(CaseObj)*(nbobj+1));
 }

 if (!HasMainLoop)
  TabIObj[nbobj]=NULL;
 CurrCase=-1;
 TabCObj[nbobj].NbCase=-1;
}

/* Ajout d'un case dans la table TabCase */
/* Initialisation d'un case of: agrandissement de la table */
void InitCase(int cond)
{
 CurrCase++;

 /* On enregistre la condition du case */
 TabCObj[nbobj].NbCase++;
 if (TabCObj[nbobj].NbCase==0)
  TabCObj[nbobj].LstCase=xcalloc(1, sizeof(int));
 else
  TabCObj[nbobj].LstCase=(int*)realloc(TabCObj[nbobj].LstCase,sizeof(int)*(CurrCase+1));
 TabCObj[nbobj].LstCase[CurrCase]=cond;

 if (CurrCase==0)
  TabIObj[nbobj]=xcalloc(1, sizeof(Bloc));
 else
  TabIObj[nbobj]=(Bloc*)realloc(TabIObj[nbobj],sizeof(Bloc)*(CurrCase+1));

 TabIObj[nbobj][CurrCase].NbInstr=-1;
 TabIObj[nbobj][CurrCase].TabInstr=NULL;

 /* Ce case correspond au bloc courant d'instruction: on l'empile */
 PileBloc[0]=&TabIObj[nbobj][CurrCase];
 TopPileB=0;
}

/* Enleve un niveau d'args dans la pile BuffArg */
void RmLevelBufArg(void)
{
  SPileArg--;
}

/* Fonction de concatenation des n derniers etage de la pile */
/* Retourne les elts trie et depile et la taille */
long *Depile(int NbLevelArg, int *s)
{
 long *Temp;
 int j;
 int i;
 int size;

 if (NbLevelArg>0)
 {
  Temp=xcalloc(1, sizeof(long));
  size=0;
  for (i=SPileArg-NbLevelArg+1;i<=SPileArg;i++)
  {
   size=NbArg[i]+size+1;
   Temp=(long*)realloc (Temp,sizeof(long)*size);
   for (j=0;j<=NbArg[i];j++)
   {
    Temp[j+size-NbArg[i]-1]=BuffArg[i][j];
   }
  }
  *s=size;
  for (i=0;i<NbLevelArg;i++)	/* On depile les couches d'arguments */
   RmLevelBufArg();
  return Temp;
 }
 else
 {
  *s=0;
  return NULL;
 }
}

/* Ajout d'une commande */
void AddCom(int Type, int NbLevelArg)
{
 int CurrInstr;


 PileBloc[TopPileB]->NbInstr++;
 CurrInstr=PileBloc[TopPileB]->NbInstr;

 if (CurrInstr==0)
  PileBloc[TopPileB]->TabInstr=xcalloc(1, sizeof(Instr) * (CurrInstr + 1));
 else
  PileBloc[TopPileB]->TabInstr=(Instr*)realloc(PileBloc[TopPileB]->TabInstr,
				sizeof(Instr)*(CurrInstr+1));
 /* Rangement des instructions dans le bloc */
 PileBloc[TopPileB]->TabInstr[CurrInstr].Type=Type;
 /* On enleve la derniere couche d'argument et on la range dans la commande */

 PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg=Depile(NbLevelArg,
		&PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg);
}

/* Initialisation du buffer contenant les arguments de la commande courante */
/* Ajout d'une couche d'argument dans la pile*/
void AddLevelBufArg(void)
{
 /* Agrandissment de la pile */
 SPileArg++;
 NbArg[SPileArg]=-1;
}

/* Ajout d'un arg dans la couche arg qui est au sommet de la pile TabArg */
void AddBufArg(long *TabLong,int NbLong)
{
 int i;

 for (i=0;i<NbLong;i++)
 {
  BuffArg[SPileArg][i+NbArg[SPileArg]+1]=TabLong[i];
 }
 NbArg[SPileArg]=NbArg[SPileArg]+NbLong;
}

/* Recheche d'un nom de var dans TabVar, s'il n'existe pas il le cree */
/* Retourne un Id */
void AddVar(char *Name)		/* ajout de variable a la fin de la derniere commande pointee */
{
 int i;

 /* Comparaison avec les variables deja existante */
 for (i=0;i<=NbVar;i++)
  if (strcmp(TabNVar[i],Name)==0)
  {
   l=(long)i;
   AddBufArg(&l,1);
   return ;
  }

 if (NbVar>MAX_VARS-2)
 {
  fprintf(stderr,
    "[%s] Line %d: too many variables (>5120)\n",ScriptName,numligne);
  exit(1);
 }

 /* La variable n'a pas ete trouvee: creation */
 NbVar++;

 if (NbVar==0)
 {
  TabNVar=xcalloc(1, sizeof(long));
  TabVVar=xcalloc(1, sizeof(long));
 }
 else
 {
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=xstrdup(Name);
 TabVVar[NbVar]=xcalloc(1, sizeof(char));
 TabVVar[NbVar][0]='\0';


 /* Ajout de la variable dans le buffer Arg */
 l=(long)NbVar;
 AddBufArg(&l,1);
 return ;
}

/* Ajout d'une constante str comme argument */
void AddConstStr(char *Name)
{
 /* On cree une nouvelle variable et on range la constante dedans */
 NbVar++;
 if (NbVar==0)
 {
  TabVVar=xcalloc(1, sizeof(long));
  TabNVar=xcalloc(1, sizeof(long));
 }
 else
 {
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=xcalloc(1, sizeof(char));
 TabNVar[NbVar][0]='\0';
 TabVVar[NbVar]=xstrdup(Name);

 /* Ajout de l'id de la constante dans la liste courante des arguments */
 l=(long)NbVar;
 AddBufArg(&l,1);
}

/* Ajout d'une constante numerique comme argument */
void AddConstNum(long num)
{

 /* On ne cree pas de nouvelle variable */
 /* On code la valeur numerique afin de le ranger sous forme d'id */
 l=num+200000;
 /* Ajout de la constante dans la liste courante des arguments */
 AddBufArg(&l,1);
}

/* Ajout d'une fonction comme argument */
/* Enleve les args de func de la pile, */
/* le concate, et les range dans la pile */
void AddFunct(int code,int NbLevelArg)
{
 int size;
 long *l;
 int i;

 /* Methode: depiler BuffArg et completer le niveau inferieur de BuffArg */
 l=Depile(NbLevelArg, &size);

 size++;
 if (size==1)
  l=xcalloc(1, sizeof(long));
 else
 {
  l=(long*)realloc(l,sizeof(long)*(size));
  for (i=size-2;i>-1;i--)	/* Deplacement des args */
  {
   l[i+1]=l[i];
  }
 }
 l[0]=(long)code-150000;

 AddBufArg(l,size);
}

/* Ajout d'une instruction de test pour executer un ou plusieurs blocs */
/* enregistre l'instruction et le champs de ces blocs = NULL */
void AddComBloc(int TypeCond, int NbLevelArg, int NbBloc)
{
 int i;
 int OldNA;
 int CurrInstr;

 /* Ajout de l'instruction de teste comme d'une commande */
 AddCom(TypeCond, NbLevelArg);

 /* On initialise ensuite les deux champs reserve à bloc1 et bloc2 */
 CurrInstr=PileBloc[TopPileB]->NbInstr;
 /* Attention NbArg peur changer si on utilise en arg une fonction */
 OldNA=PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg;

 PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg=(long*)realloc(
		PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg,sizeof(long)*(OldNA+NbBloc));
 for (i=0;i<NbBloc;i++)
 {
  PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg[OldNA+i]=0;
 }
 PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg=OldNA+NbBloc;
}

/* Creer un nouveau bloc, et l'empile: il devient le bloc courant */
void EmpilerBloc(void)
{
 Bloc *TmpBloc;

 TmpBloc=xcalloc(1, sizeof(Bloc));
 TmpBloc->NbInstr=-1;
 TmpBloc->TabInstr=NULL;
 TopPileB++;
 PileBloc[TopPileB]=TmpBloc;

}

/* Depile le bloc d'initialisation du script et le range a sa place speciale */
void DepilerBloc(int IdBloc)
{
 Bloc *Bloc1;
 Instr *IfInstr;

 Bloc1=PileBloc[TopPileB];
 TopPileB--;
 IfInstr=&PileBloc[TopPileB]->TabInstr[PileBloc[TopPileB]->NbInstr];
 IfInstr->TabArg[IfInstr->NbArg-IdBloc]=(long)Bloc1;
}

/* Gestion des erreurs de syntaxes */
int yyerror(char *errmsg)
{
 fprintf(stderr,"[%s] Line %d: %s\n",ScriptName,numligne,errmsg);
 return 0;
}




/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 368 "script.y"
{  char *str;
          int number;
       }
/* Line 187 of yacc.c.  */
#line 678 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 691 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1277

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  108
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  67
/* YYNRULES -- Number of rules.  */
#define YYNRULES  217
/* YYNRULES -- Number of states.  */
#define YYNSTATES  474

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   362

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,    10,    11,    12,    16,    19,    23,    27,
      31,    36,    41,    45,    49,    53,    57,    61,    64,    65,
      71,    72,    78,    79,    85,    86,    94,    96,    97,   101,
     106,   111,   115,   119,   123,   127,   131,   135,   139,   143,
     147,   151,   155,   159,   162,   166,   167,   170,   173,   176,
     179,   182,   185,   186,   188,   194,   195,   196,   201,   206,
     208,   210,   212,   216,   217,   221,   225,   229,   233,   237,
     241,   245,   249,   254,   258,   262,   266,   270,   274,   278,
     282,   286,   290,   294,   298,   302,   306,   310,   314,   318,
     322,   326,   329,   332,   335,   338,   341,   344,   347,   350,
     354,   357,   360,   363,   366,   369,   372,   375,   378,   381,
     384,   387,   390,   393,   396,   399,   402,   405,   408,   411,
     414,   419,   424,   429,   436,   443,   448,   453,   458,   463,
     468,   473,   479,   484,   485,   488,   493,   498,   509,   514,
     519,   523,   527,   535,   536,   540,   541,   545,   547,   551,
     553,   563,   571,   573,   575,   577,   579,   581,   583,   584,
     587,   590,   595,   599,   602,   606,   610,   614,   619,   622,
     624,   627,   631,   633,   636,   639,   642,   645,   648,   651,
     654,   656,   661,   665,   667,   670,   671,   674,   677,   680,
     683,   686,   689,   695,   697,   699,   701,   703,   705,   707,
     712,   714,   716,   718,   720,   725,   727,   729,   734,   736,
     738,   743,   745,   747,   749,   751,   753,   755
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     109,     0,    -1,   110,   111,   112,   113,   114,   115,    -1,
      -1,    -1,   111,    12,     4,    -1,   111,    12,    -1,   111,
       8,     4,    -1,   111,     9,     4,    -1,   111,    33,     3,
      -1,   111,    11,     7,     7,    -1,   111,    10,     7,     7,
      -1,   111,    14,     4,    -1,   111,    13,     4,    -1,   111,
      15,     4,    -1,   111,    16,     4,    -1,   111,    17,     7,
      -1,   111,     6,    -1,    -1,    19,   155,    47,   126,    23,
      -1,    -1,    20,   155,    47,   126,    23,    -1,    -1,    21,
     155,    47,   126,    23,    -1,    -1,   115,    18,   116,    24,
     117,   119,   120,    -1,     7,    -1,    -1,   117,    25,     3,
      -1,   117,    26,     7,     7,    -1,   117,    27,     7,     7,
      -1,   117,    28,     7,    -1,   117,    29,     7,    -1,   117,
      30,     7,    -1,   117,    31,     4,    -1,   117,    37,     4,
      -1,   117,    32,     4,    -1,   117,    33,     3,    -1,   117,
      14,     4,    -1,   117,    13,     4,    -1,   117,    15,     4,
      -1,   117,    16,     4,    -1,   117,    17,     7,    -1,   117,
       6,    -1,   117,    34,   118,    -1,    -1,   118,    38,    -1,
     118,    40,    -1,   118,    39,    -1,   118,    41,    -1,   118,
      42,    -1,   118,    43,    -1,    -1,    23,    -1,    22,   121,
      44,   122,    23,    -1,    -1,    -1,   122,   123,    48,   125,
      -1,   122,   124,    48,   125,    -1,    45,    -1,    46,    -1,
       7,    -1,    47,   126,    23,    -1,    -1,   126,    49,   128,
      -1,   126,    35,   145,    -1,   126,    36,   147,    -1,   126,
      50,   129,    -1,   126,    51,   130,    -1,   126,    66,   131,
      -1,   126,    67,   132,    -1,   126,    68,   133,    -1,   126,
      56,   166,   172,    -1,   126,    57,   170,    -1,   126,    27,
     134,    -1,   126,    26,   135,    -1,   126,    31,   137,    -1,
     126,    37,   149,    -1,   126,    33,   136,    -1,   126,    52,
     138,    -1,   126,    53,   139,    -1,   126,    54,   140,    -1,
     126,    55,   141,    -1,   126,    83,   142,    -1,   126,    84,
     143,    -1,   126,    77,   144,    -1,   126,    80,   146,    -1,
     126,    93,   150,    -1,   126,    96,   151,    -1,   126,    99,
     152,    -1,   126,    58,   148,    -1,    49,   128,    -1,    35,
     145,    -1,    36,   147,    -1,    50,   129,    -1,    51,   130,
      -1,    66,   131,    -1,    67,   132,    -1,    68,   133,    -1,
      56,   166,   172,    -1,    57,   170,    -1,    27,   134,    -1,
      26,   135,    -1,    31,   137,    -1,    37,   149,    -1,    33,
     136,    -1,    52,   138,    -1,    53,   139,    -1,    54,   140,
      -1,    55,   141,    -1,    83,   142,    -1,    84,   143,    -1,
      77,   144,    -1,    80,   146,    -1,    96,   151,    -1,    99,
     152,    -1,    58,   148,    -1,   166,   168,    -1,   166,   170,
      -1,   166,   170,    -1,   166,   170,   166,   170,    -1,   166,
     170,   166,   170,    -1,   166,   170,   166,   170,    -1,   166,
     170,   166,   170,   166,   170,    -1,   166,   170,   166,   170,
     166,   170,    -1,   166,   170,   166,   171,    -1,   166,   170,
     166,   172,    -1,   166,   170,   166,   168,    -1,   166,   170,
     166,   172,    -1,   166,   170,   166,   172,    -1,   166,   170,
     166,   170,    -1,   166,   173,    82,   166,   168,    -1,   166,
     170,   166,   170,    -1,    -1,   166,   170,    -1,   166,   170,
     166,   168,    -1,   166,   171,   166,   168,    -1,   166,   171,
     166,   171,   166,   170,   166,   170,   166,   168,    -1,   166,
     170,   166,   172,    -1,   153,   155,   156,   154,    -1,   158,
     155,   157,    -1,   159,   155,   157,    -1,   166,   169,   166,
     174,   166,   169,    94,    -1,    -1,    95,   155,   157,    -1,
      -1,    47,   126,    23,    -1,   127,    -1,    47,   126,    23,
      -1,   127,    -1,   166,   173,    82,   166,   169,    97,   166,
     169,    98,    -1,   166,   169,   166,   174,   166,   169,    98,
      -1,     5,    -1,     3,    -1,     4,    -1,     7,    -1,    45,
      -1,    46,    -1,    -1,    59,   170,    -1,    72,   170,    -1,
      73,   172,   170,   170,    -1,    75,   170,   170,    -1,    76,
     172,    -1,    69,   170,   170,    -1,    71,   170,   170,    -1,
      70,   170,   170,    -1,    74,   172,   170,   170,    -1,    78,
     172,    -1,    79,    -1,    81,   170,    -1,    85,   170,   170,
      -1,    86,    -1,    87,   170,    -1,    62,   170,    -1,    63,
     170,    -1,    64,   170,    -1,    65,   170,    -1,    60,   170,
      -1,    61,   170,    -1,    88,    -1,    89,   172,   172,   170,
      -1,    90,   172,   170,    -1,    91,    -1,    92,   172,    -1,
      -1,   164,   168,    -1,   165,   168,    -1,   160,   168,    -1,
     162,   168,    -1,   161,   168,    -1,   163,   168,    -1,   100,
     166,   167,   101,   168,    -1,   160,    -1,   164,    -1,   165,
      -1,   162,    -1,   161,    -1,   163,    -1,   100,   166,   167,
     101,    -1,   164,    -1,   165,    -1,   163,    -1,   160,    -1,
     100,   166,   167,   101,    -1,   160,    -1,   161,    -1,   100,
     166,   167,   101,    -1,   160,    -1,   162,    -1,   100,   166,
     167,   101,    -1,   160,    -1,   105,    -1,   103,    -1,   102,
      -1,   104,    -1,   106,    -1,   107,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   393,   393,   396,   400,   401,   406,   412,   417,   422,
     426,   432,   438,   444,   450,   456,   462,   467,   473,   474,
     479,   480,   485,   486,   495,   496,   499,   515,   516,   520,
     524,   529,   532,   535,   538,   541,   544,   547,   550,   554,
     558,   562,   566,   570,   573,   575,   576,   579,   582,   585,
     588,   591,   597,   608,   609,   612,   614,   615,   616,   619,
     620,   623,   626,   631,   632,   633,   634,   635,   636,   637,
     638,   639,   640,   641,   642,   643,   644,   645,   646,   647,
     648,   649,   650,   651,   652,   653,   654,   655,   656,   657,
     658,   662,   663,   664,   665,   666,   667,   668,   669,   670,
     671,   672,   673,   674,   675,   676,   677,   678,   679,   680,
     681,   682,   683,   684,   685,   686,   687,   690,   692,   694,
     696,   698,   700,   702,   704,   706,   708,   710,   712,   714,
     716,   718,   720,   722,   724,   726,   728,   730,   732,   734,
     736,   738,   742,   744,   745,   747,   749,   750,   753,   754,
     758,   762,   767,   769,   771,   773,   775,   777,   779,   781,
     782,   783,   784,   785,   786,   787,   788,   789,   790,   791,
     792,   793,   794,   795,   796,   797,   798,   799,   800,   801,
     802,   803,   804,   805,   806,   811,   812,   813,   814,   815,
     816,   817,   818,   822,   823,   824,   825,   826,   827,   828,
     832,   833,   834,   835,   836,   840,   841,   842,   846,   847,
     848,   852,   856,   857,   858,   859,   860,   861
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "STR", "GSTR", "VAR", "FONT", "NUMBER",
  "WINDOWTITLE", "WINDOWLOCALETITLE", "WINDOWSIZE", "WINDOWPOSITION",
  "USEGETTEXT", "FORECOLOR", "BACKCOLOR", "SHADCOLOR", "LICOLOR",
  "COLORSET", "OBJECT", "INIT", "PERIODICTASK", "QUITFUNC", "MAIN", "END",
  "PROP", "TYPE", "SIZE", "POSITION", "VALUE", "VALUEMIN", "VALUEMAX",
  "TITLE", "SWALLOWEXEC", "ICON", "FLAGS", "WARP", "WRITETOFILE",
  "LOCALETITLE", "HIDDEN", "NOFOCUS", "NORELIEFSTRING", "CENTER", "LEFT",
  "RIGHT", "CASE", "SINGLECLIC", "DOUBLECLIC", "BEG", "POINT", "EXEC",
  "HIDE", "SHOW", "CHFONT", "CHFORECOLOR", "CHBACKCOLOR", "CHCOLORSET",
  "CHWINDOWTITLE", "CHWINDOWTITLEFARG", "KEY", "GETVALUE", "GETMINVALUE",
  "GETMAXVALUE", "GETFORE", "GETBACK", "GETHILIGHT", "GETSHADOW",
  "CHVALUE", "CHVALUEMAX", "CHVALUEMIN", "ADD", "DIV", "MULT", "GETTITLE",
  "GETOUTPUT", "STRCOPY", "NUMTOHEX", "HEXTONUM", "QUIT", "LAUNCHSCRIPT",
  "GETSCRIPTFATHER", "SENDTOSCRIPT", "RECEIVFROMSCRIPT", "GET", "SET",
  "SENDSIGN", "REMAINDEROFDIV", "GETTIME", "GETSCRIPTARG", "GETPID",
  "SENDMSGANDGET", "PARSE", "LASTSTRING", "GETTEXT", "IF", "THEN", "ELSE",
  "FOR", "TO", "DO", "WHILE", "BEGF", "ENDF", "EQUAL", "INFEQ", "SUPEQ",
  "INF", "SUP", "DIFF", "$accept", "script", "initvar", "head", "initbloc",
  "periodictask", "quitfunc", "object", "id", "init", "flags", "verify",
  "mainloop", "addtabcase", "case", "clic", "number", "bloc", "instr",
  "oneinstr", "exec", "hide", "show", "chvalue", "chvaluemax",
  "chvaluemin", "position", "size", "icon", "title", "font", "chforecolor",
  "chbackcolor", "chcolorset", "set", "sendsign", "quit", "warp",
  "sendtoscript", "writetofile", "key", "localetitle", "ifthenelse",
  "loop", "while", "headif", "else", "creerbloc", "bloc1", "bloc2",
  "headloop", "headwhile", "var", "str", "gstr", "num", "singleclic2",
  "doubleclic2", "addlbuff", "function", "args", "arg", "numarg", "strarg",
  "gstrarg", "vararg", "compare", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   108,   109,   110,   111,   111,   111,   111,   111,   111,
     111,   111,   111,   111,   111,   111,   111,   111,   112,   112,
     113,   113,   114,   114,   115,   115,   116,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   118,   118,   118,   118,   118,
     118,   118,   119,   120,   120,   121,   122,   122,   122,   123,
     123,   124,   125,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   126,   126,
     126,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   127,   127,   127,
     127,   127,   127,   127,   127,   127,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   154,   155,   156,   156,   157,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   168,   168,   168,   168,   168,
     168,   168,   168,   169,   169,   169,   169,   169,   169,   169,
     170,   170,   170,   170,   170,   171,   171,   171,   172,   172,
     172,   173,   174,   174,   174,   174,   174,   174
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     6,     0,     0,     3,     2,     3,     3,     3,
       4,     4,     3,     3,     3,     3,     3,     2,     0,     5,
       0,     5,     0,     5,     0,     7,     1,     0,     3,     4,
       4,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     3,     0,     2,     2,     2,     2,
       2,     2,     0,     1,     5,     0,     0,     4,     4,     1,
       1,     1,     3,     0,     3,     3,     3,     3,     3,     3,
       3,     3,     4,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     2,     2,     2,     2,     3,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     4,     4,     6,     6,     4,     4,     4,     4,     4,
       4,     5,     4,     0,     2,     4,     4,    10,     4,     4,
       3,     3,     7,     0,     3,     0,     3,     1,     3,     1,
       9,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       2,     4,     3,     2,     3,     3,     3,     4,     2,     1,
       2,     3,     1,     2,     2,     2,     2,     2,     2,     2,
       1,     4,     3,     1,     2,     0,     2,     2,     2,     2,
       2,     2,     5,     1,     1,     1,     1,     1,     1,     4,
       1,     1,     1,     1,     4,     1,     1,     4,     1,     1,
       4,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     4,     1,    18,    17,     0,     0,     0,     0,
       6,     0,     0,     0,     0,     0,   145,     0,    20,     7,
       8,     0,     0,     5,    13,    12,    14,    15,    16,     0,
       9,   145,    22,    11,    10,    63,     0,   145,    24,     0,
      63,     0,     2,    19,   158,   158,   158,   158,   158,   158,
     158,   158,   158,   158,   158,   158,   158,   158,   158,     0,
     158,   158,   158,   158,   133,   158,   158,   158,   158,   158,
     158,     0,    63,     0,    75,     0,    74,     0,    76,     0,
      78,     0,    65,     0,    66,     0,    77,     0,    64,   185,
      67,     0,    68,     0,    79,     0,    80,     0,    81,     0,
      82,     0,     0,   152,   155,   156,   157,   158,   203,   202,
     200,   201,    73,    90,     0,    69,     0,    70,     0,    71,
       0,    85,    86,     0,    83,     0,    84,     0,    87,   145,
       0,    88,   145,     0,    89,   145,     0,    21,     0,    26,
       0,   158,   158,   158,   158,   134,   153,   158,   205,   206,
     158,   158,   154,   158,   185,   185,   185,   185,   185,   185,
     117,   118,   119,   158,   158,   158,   158,   158,   208,   209,
      72,     0,   158,   158,   158,   158,   158,   211,     0,   158,
       0,   158,   193,   197,   196,   198,   194,   195,   158,     0,
       0,     0,   158,    23,    27,     0,     0,     0,     0,     0,
     185,     0,     0,   188,   190,   189,   191,   186,   187,   185,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     169,     0,     0,   172,     0,   180,     0,     0,   183,     0,
       0,     0,     0,     0,     0,   185,   158,     0,   158,   158,
     158,   158,   158,   158,   158,    63,   158,   158,   158,   158,
     158,   158,   158,   158,     0,   158,   158,   158,   158,   133,
     158,   158,   158,   158,   158,   147,   143,     0,     0,    63,
     149,   140,   158,   141,     0,    52,   158,   158,   126,   125,
       0,   136,   138,     0,   127,   128,   129,   130,     0,   159,
     178,   179,   174,   175,   176,   177,     0,     0,     0,   160,
       0,     0,     0,   163,   168,   170,     0,   173,     0,     0,
     184,   204,   158,   120,   121,   122,   135,   185,   132,   102,
     101,   103,   105,    92,    93,   104,     0,    91,    94,    95,
     106,   107,   108,   109,     0,   100,   116,    96,    97,    98,
     112,   113,   110,   111,   114,   115,   145,   139,     0,   214,
     213,   215,   212,   216,   217,   158,     0,     0,   158,    43,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    45,     0,     0,     0,     0,   207,
     185,   210,   164,   166,   165,     0,     0,   162,   171,     0,
     182,     0,   131,   146,    99,     0,   199,     0,   148,     0,
       0,    39,    38,    40,    41,    42,    28,     0,     0,    31,
      32,    33,    34,    36,    37,    44,    35,    55,    53,    25,
     124,   123,   192,   161,   167,   181,   158,   144,     0,   158,
       0,    29,    30,    46,    48,    47,    49,    50,    51,     0,
       0,   142,     0,   151,    56,   158,     0,     0,   185,   150,
      61,    54,    59,    60,     0,     0,   137,     0,     0,    63,
      57,    58,     0,    62
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,     4,    18,    32,    38,    42,   140,   285,
     425,   386,   429,   449,   457,   464,   465,   470,    39,   280,
      88,    90,    92,   115,   117,   119,    76,    74,    80,    78,
      94,    96,    98,   100,   124,   126,   121,    82,   122,    84,
     113,    86,   128,   131,   134,   129,   357,    29,   276,   281,
     132,   135,   108,   155,   169,   109,   110,   111,    75,   240,
     160,   188,   112,   150,   170,   178,   365
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -307
static const yytype_int16 yypact[] =
{
    -307,     8,  -307,  -307,   584,  -307,    -2,    10,    -1,     9,
      13,    14,    15,    19,    21,    22,  -307,    31,    35,  -307,
    -307,    28,    57,  -307,  -307,  -307,  -307,  -307,  -307,    18,
    -307,  -307,    58,  -307,  -307,  -307,    36,  -307,  -307,   759,
    -307,    39,    64,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,    26,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,   814,  -307,    80,  -307,    26,  -307,    26,  -307,    26,
    -307,    26,  -307,    26,  -307,     2,  -307,    26,  -307,    17,
    -307,    26,  -307,    26,  -307,    26,  -307,    26,  -307,    26,
    -307,    26,     6,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,     2,  -307,    26,  -307,    26,  -307,
      26,  -307,  -307,    26,  -307,    85,  -307,    26,  -307,  -307,
      23,  -307,  -307,    85,  -307,  -307,    23,  -307,   869,  -307,
      67,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,    17,    17,    17,    17,    17,    17,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  1185,  -307,  -307,  -307,  -307,  -307,  -307,    32,  -307,
    1089,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  1144,
      33,  1144,  -307,  -307,  -307,    26,    26,     6,     2,  1185,
      17,     6,  1185,  -307,  -307,  -307,  -307,  -307,  -307,    17,
       6,     6,    26,  1185,    26,    26,    26,    26,    26,    26,
      26,    26,    26,    26,    26,     6,     6,    26,     6,     6,
    -307,    26,    26,  -307,    26,  -307,     6,     6,  -307,     6,
      -6,     2,    26,    26,    26,    17,  -307,    26,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,    26,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,    -3,  1185,   -29,  -307,
    -307,  -307,  -307,  -307,   -29,   277,  -307,  -307,  -307,  -307,
      -5,  -307,  -307,    -4,  -307,  -307,  -307,  -307,     4,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,    26,    26,    26,  -307,
      26,    26,    26,  -307,  -307,  -307,    26,  -307,     6,    26,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,    17,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,   924,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,     6,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,    12,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,   979,    23,  -307,  -307,
      95,    96,    99,   106,   109,   115,   114,   117,   118,   120,
     122,   126,   127,   133,  -307,   135,   -10,    26,    26,  -307,
      17,  -307,  -307,  -307,  -307,    26,    26,  -307,  -307,    26,
    -307,    26,  -307,  -307,  -307,  1144,  -307,    23,  -307,    40,
      23,  -307,  -307,  -307,  -307,  -307,  -307,   136,   145,  -307,
    -307,  -307,  -307,  -307,  -307,   108,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,    46,  -307,
      55,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,   116,
      26,  -307,    23,  -307,  -307,  -307,    61,    43,    17,  -307,
    -307,  -307,  -307,  -307,   113,   121,  -307,   124,   124,  -307,
    -307,  -307,  1034,  -307
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -306,   -40,   -12,
     -83,   -82,   -84,   -90,   -86,   -91,   -64,   -60,   -62,   -59,
     -67,   -65,   -68,   -63,   -74,   -69,   -61,   -47,   -58,   -46,
     -56,   -48,  -307,   -53,   -57,  -307,  -307,   -28,  -307,  -187,
    -307,  -307,   247,   102,   240,   296,   370,   377,    -9,  -132,
     360,  -121,   103,  -113,  -117,    77,   -73
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint16 yytable[] =
{
      71,   172,    19,    36,   283,   146,    21,   103,     3,    41,
     152,   103,   427,   428,    20,   192,    22,    23,    24,    25,
     146,   152,   103,    26,   104,    27,   146,   152,   103,    28,
     104,   103,   138,   104,    30,    33,    77,    79,    81,    83,
      85,    87,    89,    91,    93,    95,    97,    99,   101,   102,
     460,   114,   116,   118,   120,    31,   123,   125,   127,   130,
     133,   136,   105,   106,    34,    35,   461,   290,   105,   106,
     293,   105,   106,   359,   360,   361,   362,   363,   364,    37,
     288,   298,    73,    40,   292,   289,    72,   139,   462,   463,
     103,   194,   356,   295,   296,   321,   389,   390,   171,   411,
     412,   180,   147,   413,   189,   391,   167,   191,   310,   311,
     414,   313,   314,   406,   246,   282,   415,   153,   416,   318,
     319,   417,   320,   181,   418,   419,   107,   420,   322,   421,
     422,   423,   195,   196,   197,   198,   424,   439,   199,   426,
     451,   200,   201,   441,   202,   358,   443,   444,   445,   446,
     447,   448,   442,   453,   209,   210,   211,   212,   213,   459,
     454,   467,   471,   241,   242,   243,   244,   245,   275,   468,
     247,   469,   277,   337,   339,   338,   347,   349,   141,   278,
     142,   348,   143,   284,   144,   330,   145,   149,   329,   332,
     151,   331,   340,   342,   161,   341,   162,   352,   163,   343,
     164,   399,   165,   353,   166,   333,   335,   334,   350,   346,
     190,   368,   351,     0,     0,   336,   149,   355,   437,   173,
     354,   174,     0,   175,     0,     0,   176,   404,     0,     0,
     179,     0,   183,     0,     0,     0,     0,   327,   183,   366,
      77,    79,    81,    83,    85,    87,   409,    89,    91,    93,
      95,    97,    99,   101,   344,     0,   114,   116,   118,   120,
       0,   123,   125,   127,   133,   136,     0,     0,     0,     0,
       0,     0,     0,   367,     0,     0,     0,   387,   388,     0,
       0,     0,     0,   369,     0,     0,   438,     0,     0,   440,
     370,   371,   372,   373,   374,     0,     0,     0,   286,   287,
     149,     0,   375,   376,   377,   378,   379,   380,   381,   382,
     383,   384,     0,   401,   385,   297,     0,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   405,   156,
     312,   456,   148,     0,   315,   316,   154,   317,     0,     0,
       0,     0,     0,   149,     0,   323,   324,   325,     0,   168,
     328,     0,     0,     0,     0,     0,   407,     0,     0,   410,
       0,   148,     0,     0,     0,     0,     0,   345,     0,     0,
     184,     0,   177,     0,     0,     0,   184,   182,     0,     0,
     177,     0,     0,   182,     0,   157,     0,     0,     0,     0,
       0,     0,     0,     0,   156,   156,   156,   156,   156,   156,
       0,   154,   154,   154,   154,   154,   154,     0,     0,   392,
     393,   394,     0,   395,   396,   397,     0,     0,     0,   398,
       0,     0,   400,     0,     0,     0,   185,   450,     0,   472,
     452,     0,   185,     0,     0,     0,     0,     0,     0,     0,
     156,     0,     0,     0,   168,   148,   458,   154,   168,   156,
     157,   157,   157,   157,   157,   157,   154,   168,   168,   158,
       0,     0,     0,     0,     0,     0,   159,     0,     0,   183,
       0,     0,   168,   168,     0,   168,   168,     0,     0,     0,
       0,     0,     0,   168,   168,   156,   168,     0,   148,     0,
     430,   431,   154,     0,     0,     0,   157,     0,   433,   434,
     186,     0,   435,     0,   436,   157,   186,   187,     0,   183,
       0,     0,   183,   187,   203,   204,   205,   206,   207,   208,
       0,     0,     0,     0,   158,   158,   158,   158,   158,   158,
       0,   159,   159,   159,   159,   159,   159,     0,     0,     0,
       0,   157,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   455,   183,     0,     0,     0,     0,     0,
     291,     0,     0,     0,     0,   168,     0,   156,     0,   294,
     158,     0,     0,     0,   154,     0,     0,   159,     0,   158,
       0,     0,     0,     0,     0,     0,   159,     0,     0,     0,
       5,   168,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,     0,    16,     0,   326,     0,   184,     0,     0,
       0,     0,     0,     0,   182,   158,     0,    17,     0,     0,
       0,     0,   159,   157,     0,     0,     0,     0,     0,     0,
     156,     0,     0,     0,     0,     0,     0,   154,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   184,     0,     0,
     184,     0,     0,     0,   182,     0,     0,   182,     0,     0,
       0,     0,     0,   185,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   157,   402,     0,     0,
       0,     0,   184,     0,     0,     0,     0,   158,   156,   182,
       0,     0,     0,   185,   159,   154,   185,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
       0,     0,     0,     0,   187,     0,     0,     0,   185,     0,
     432,     0,     0,     0,   157,     0,     0,     0,     0,     0,
     158,     0,     0,     0,     0,     0,     0,   159,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   186,     0,     0,
     186,     0,    43,     0,   187,    44,    45,   187,     0,     0,
      46,     0,    47,     0,    48,    49,    50,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,   466,     0,
       0,     0,   186,     0,     0,    61,    62,    63,   158,   187,
       0,     0,     0,     0,     0,   159,    64,   137,     0,    65,
      44,    45,    66,    67,     0,    46,     0,    47,     0,    48,
      49,    50,    68,     0,     0,    69,     0,     0,    70,     0,
       0,     0,     0,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,     0,     0,     0,     0,     0,     0,     0,
      61,    62,    63,     0,     0,     0,     0,     0,     0,     0,
       0,    64,   193,     0,    65,    44,    45,    66,    67,     0,
      46,     0,    47,     0,    48,    49,    50,    68,     0,     0,
      69,     0,     0,    70,     0,     0,     0,     0,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,     0,     0,
       0,     0,     0,     0,     0,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,    64,   403,     0,    65,
      44,    45,    66,    67,     0,    46,     0,    47,     0,    48,
      49,    50,    68,     0,     0,    69,     0,     0,    70,     0,
       0,     0,     0,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,     0,     0,     0,     0,     0,     0,     0,
      61,    62,    63,     0,     0,     0,     0,     0,     0,     0,
       0,    64,   408,     0,    65,    44,    45,    66,    67,     0,
      46,     0,    47,     0,    48,    49,    50,    68,     0,     0,
      69,     0,     0,    70,     0,     0,     0,     0,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,     0,     0,
       0,     0,     0,     0,     0,    61,    62,    63,     0,     0,
       0,     0,     0,     0,     0,     0,    64,   473,     0,    65,
      44,    45,    66,    67,     0,    46,     0,    47,     0,    48,
      49,    50,    68,     0,     0,    69,     0,     0,    70,     0,
       0,     0,     0,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,     0,     0,     0,     0,     0,     0,     0,
      61,    62,    63,     0,     0,     0,     0,     0,     0,     0,
       0,    64,     0,     0,    65,   248,   249,    66,    67,     0,
     250,     0,   251,     0,   252,   253,   254,    68,     0,     0,
      69,     0,     0,    70,     0,     0,   255,     0,   256,   257,
     258,   259,   260,   261,   262,   263,   264,   265,     0,     0,
       0,     0,     0,     0,     0,   266,   267,   268,     0,     0,
       0,     0,     0,     0,     0,     0,   269,     0,     0,   270,
     248,   249,   271,   272,     0,   250,     0,   251,     0,   252,
     253,   254,     0,     0,     0,   273,     0,     0,   274,     0,
       0,   279,     0,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,     0,     0,     0,     0,     0,     0,     0,
     266,   267,   268,     0,     0,     0,     0,     0,     0,     0,
       0,   269,     0,     0,   270,     0,     0,   271,   272,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     273,     0,     0,   274,   214,   215,   216,   217,   218,   219,
     220,     0,     0,     0,   221,   222,   223,   224,   225,   226,
     227,   228,     0,   229,   230,     0,   231,     0,     0,     0,
     232,   233,   234,   235,   236,   237,   238,   239
};

static const yytype_int16 yycheck[] =
{
      40,   114,     4,    31,   191,     3,     7,     5,     0,    37,
       4,     5,    22,    23,     4,   136,     7,     4,     4,     4,
       3,     4,     5,     4,     7,     4,     3,     4,     5,     7,
       7,     5,    72,     7,     3,     7,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
       7,    60,    61,    62,    63,    20,    65,    66,    67,    68,
      69,    70,    45,    46,     7,    47,    23,   199,    45,    46,
     202,    45,    46,   102,   103,   104,   105,   106,   107,    21,
     197,   213,    18,    47,   201,   198,    47,     7,    45,    46,
       5,    24,    95,   210,   211,   101,   101,   101,   107,     4,
       4,   129,   100,     4,   132,   101,   100,   135,   225,   226,
       4,   228,   229,   101,    82,    82,     7,   100,     3,   236,
     237,     7,   239,   100,     7,     7,   100,     7,   241,     7,
       4,     4,   141,   142,   143,   144,     3,    97,   147,     4,
      94,   150,   151,     7,   153,   277,    38,    39,    40,    41,
      42,    43,     7,    98,   163,   164,   165,   166,   167,    98,
      44,    48,   468,   172,   173,   174,   175,   176,   180,    48,
     179,    47,   181,   256,   258,   257,   266,   268,    75,   188,
      77,   267,    79,   192,    81,   249,    83,    85,   248,   251,
      87,   250,   259,   261,    91,   260,    93,   271,    95,   262,
      97,   318,    99,   272,   101,   252,   254,   253,   269,   265,
     133,   284,   270,    -1,    -1,   255,   114,   274,   405,   116,
     273,   118,    -1,   120,    -1,    -1,   123,   344,    -1,    -1,
     127,    -1,   130,    -1,    -1,    -1,    -1,   246,   136,   279,
     249,   250,   251,   252,   253,   254,   367,   256,   257,   258,
     259,   260,   261,   262,   263,    -1,   265,   266,   267,   268,
      -1,   270,   271,   272,   273,   274,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   282,    -1,    -1,    -1,   286,   287,    -1,
      -1,    -1,    -1,     6,    -1,    -1,   407,    -1,    -1,   410,
      13,    14,    15,    16,    17,    -1,    -1,    -1,   195,   196,
     198,    -1,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    -1,   322,    37,   212,    -1,   214,   215,   216,
     217,   218,   219,   220,   221,   222,   223,   224,   356,    89,
     227,   452,    85,    -1,   231,   232,    89,   234,    -1,    -1,
      -1,    -1,    -1,   241,    -1,   242,   243,   244,    -1,   102,
     247,    -1,    -1,    -1,    -1,    -1,   365,    -1,    -1,   368,
      -1,   114,    -1,    -1,    -1,    -1,    -1,   264,    -1,    -1,
     130,    -1,   125,    -1,    -1,    -1,   136,   130,    -1,    -1,
     133,    -1,    -1,   136,    -1,    89,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   154,   155,   156,   157,   158,   159,
      -1,   154,   155,   156,   157,   158,   159,    -1,    -1,   306,
     307,   308,    -1,   310,   311,   312,    -1,    -1,    -1,   316,
      -1,    -1,   319,    -1,    -1,    -1,   130,   436,    -1,   469,
     439,    -1,   136,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     200,    -1,    -1,    -1,   197,   198,   455,   200,   201,   209,
     154,   155,   156,   157,   158,   159,   209,   210,   211,    89,
      -1,    -1,    -1,    -1,    -1,    -1,    89,    -1,    -1,   367,
      -1,    -1,   225,   226,    -1,   228,   229,    -1,    -1,    -1,
      -1,    -1,    -1,   236,   237,   245,   239,    -1,   241,    -1,
     387,   388,   245,    -1,    -1,    -1,   200,    -1,   395,   396,
     130,    -1,   399,    -1,   401,   209,   136,   130,    -1,   407,
      -1,    -1,   410,   136,   154,   155,   156,   157,   158,   159,
      -1,    -1,    -1,    -1,   154,   155,   156,   157,   158,   159,
      -1,   154,   155,   156,   157,   158,   159,    -1,    -1,    -1,
      -1,   245,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   450,   452,    -1,    -1,    -1,    -1,    -1,
     200,    -1,    -1,    -1,    -1,   318,    -1,   327,    -1,   209,
     200,    -1,    -1,    -1,   327,    -1,    -1,   200,    -1,   209,
      -1,    -1,    -1,    -1,    -1,    -1,   209,    -1,    -1,    -1,
       6,   344,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    19,    -1,   245,    -1,   367,    -1,    -1,
      -1,    -1,    -1,    -1,   367,   245,    -1,    33,    -1,    -1,
      -1,    -1,   245,   327,    -1,    -1,    -1,    -1,    -1,    -1,
     390,    -1,    -1,    -1,    -1,    -1,    -1,   390,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,    -1,    -1,
     410,    -1,    -1,    -1,   407,    -1,    -1,   410,    -1,    -1,
      -1,    -1,    -1,   367,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   390,   327,    -1,    -1,
      -1,    -1,   452,    -1,    -1,    -1,    -1,   327,   458,   452,
      -1,    -1,    -1,   407,   327,   458,   410,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   367,    -1,    -1,
      -1,    -1,    -1,    -1,   367,    -1,    -1,    -1,   452,    -1,
     390,    -1,    -1,    -1,   458,    -1,    -1,    -1,    -1,    -1,
     390,    -1,    -1,    -1,    -1,    -1,    -1,   390,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,    -1,    -1,
     410,    -1,    23,    -1,   407,    26,    27,   410,    -1,    -1,
      31,    -1,    33,    -1,    35,    36,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,   458,    -1,
      -1,    -1,   452,    -1,    -1,    66,    67,    68,   458,   452,
      -1,    -1,    -1,    -1,    -1,   458,    77,    23,    -1,    80,
      26,    27,    83,    84,    -1,    31,    -1,    33,    -1,    35,
      36,    37,    93,    -1,    -1,    96,    -1,    -1,    99,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    23,    -1,    80,    26,    27,    83,    84,    -1,
      31,    -1,    33,    -1,    35,    36,    37,    93,    -1,    -1,
      96,    -1,    -1,    99,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    23,    -1,    80,
      26,    27,    83,    84,    -1,    31,    -1,    33,    -1,    35,
      36,    37,    93,    -1,    -1,    96,    -1,    -1,    99,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    23,    -1,    80,    26,    27,    83,    84,    -1,
      31,    -1,    33,    -1,    35,    36,    37,    93,    -1,    -1,
      96,    -1,    -1,    99,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    23,    -1,    80,
      26,    27,    83,    84,    -1,    31,    -1,    33,    -1,    35,
      36,    37,    93,    -1,    -1,    96,    -1,    -1,    99,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    -1,    -1,    80,    26,    27,    83,    84,    -1,
      31,    -1,    33,    -1,    35,    36,    37,    93,    -1,    -1,
      96,    -1,    -1,    99,    -1,    -1,    47,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    77,    -1,    -1,    80,
      26,    27,    83,    84,    -1,    31,    -1,    33,    -1,    35,
      36,    37,    -1,    -1,    -1,    96,    -1,    -1,    99,    -1,
      -1,    47,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    77,    -1,    -1,    80,    -1,    -1,    83,    84,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      96,    -1,    -1,    99,    59,    60,    61,    62,    63,    64,
      65,    -1,    -1,    -1,    69,    70,    71,    72,    73,    74,
      75,    76,    -1,    78,    79,    -1,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    89,    90,    91,    92
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,   109,   110,     0,   111,     6,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    19,    33,   112,     4,
       4,     7,     7,     4,     4,     4,     4,     4,     7,   155,
       3,    20,   113,     7,     7,    47,   155,    21,   114,   126,
      47,   155,   115,    23,    26,    27,    31,    33,    35,    36,
      37,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    66,    67,    68,    77,    80,    83,    84,    93,    96,
      99,   126,    47,    18,   135,   166,   134,   166,   137,   166,
     136,   166,   145,   166,   147,   166,   149,   166,   128,   166,
     129,   166,   130,   166,   138,   166,   139,   166,   140,   166,
     141,   166,   166,     5,     7,    45,    46,   100,   160,   163,
     164,   165,   170,   148,   166,   131,   166,   132,   166,   133,
     166,   144,   146,   166,   142,   166,   143,   166,   150,   153,
     166,   151,   158,   166,   152,   159,   166,    23,   126,     7,
     116,   170,   170,   170,   170,   170,     3,   100,   160,   161,
     171,   170,     4,   100,   160,   161,   162,   163,   164,   165,
     168,   170,   170,   170,   170,   170,   170,   100,   160,   162,
     172,   166,   171,   170,   170,   170,   170,   160,   173,   170,
     155,   100,   160,   161,   162,   163,   164,   165,   169,   155,
     173,   155,   169,    23,    24,   166,   166,   166,   166,   166,
     166,   166,   166,   168,   168,   168,   168,   168,   168,   166,
     166,   166,   166,   166,    59,    60,    61,    62,    63,    64,
      65,    69,    70,    71,    72,    73,    74,    75,    76,    78,
      79,    81,    85,    86,    87,    88,    89,    90,    91,    92,
     167,   166,   166,   166,   166,   166,    82,   166,    26,    27,
      31,    33,    35,    36,    37,    47,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    66,    67,    68,    77,
      80,    83,    84,    96,    99,   127,   156,   166,   166,    47,
     127,   157,    82,   157,   166,   117,   170,   170,   172,   171,
     167,   168,   172,   167,   168,   172,   172,   170,   167,   170,
     170,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     172,   172,   170,   172,   172,   170,   170,   170,   172,   172,
     172,   101,   171,   170,   170,   170,   168,   166,   170,   135,
     134,   137,   136,   145,   147,   149,   126,   128,   129,   130,
     138,   139,   140,   141,   166,   170,   148,   131,   132,   133,
     144,   146,   142,   143,   151,   152,    95,   154,   167,   102,
     103,   104,   105,   106,   107,   174,   126,   166,   174,     6,
      13,    14,    15,    16,    17,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    37,   119,   166,   166,   101,
     101,   101,   170,   170,   170,   170,   170,   170,   170,   172,
     170,   166,   168,    23,   172,   155,   101,   166,    23,   169,
     166,     4,     4,     4,     4,     7,     3,     7,     7,     7,
       7,     7,     4,     4,     3,   118,     4,    22,    23,   120,
     170,   170,   168,   170,   170,   170,   170,   157,   169,    97,
     169,     7,     7,    38,    39,    40,    41,    42,    43,   121,
     166,    94,   166,    98,    44,   170,   169,   122,   166,    98,
       7,    23,    45,    46,   123,   124,   168,    48,    48,    47,
     125,   125,   126,    23
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 396 "script.y"
    { InitVarGlob(); }
    break;

  case 5:
#line 402 "script.y"
    {
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
	FGettextSetLocalePath((yyvsp[(3) - (3)].str));
}
    break;

  case 6:
#line 407 "script.y"
    {
	fprintf(stderr,"UseGettext!\n");
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
}
    break;

  case 7:
#line 413 "script.y"
    {
	/* Titre de la fenetre */
	scriptprop->titlewin=(yyvsp[(3) - (3)].str);
}
    break;

  case 8:
#line 418 "script.y"
    {
	/* Titre de la fenetre */
	scriptprop->titlewin=(char *)FGettext((yyvsp[(3) - (3)].str));
}
    break;

  case 9:
#line 423 "script.y"
    {
	scriptprop->icon=(yyvsp[(3) - (3)].str);
}
    break;

  case 10:
#line 427 "script.y"
    {
	/* Position et taille de la fenetre */
	scriptprop->x=(yyvsp[(3) - (4)].number);
	scriptprop->y=(yyvsp[(4) - (4)].number);
}
    break;

  case 11:
#line 433 "script.y"
    {
	/* Position et taille de la fenetre */
	scriptprop->width=(yyvsp[(3) - (4)].number);
	scriptprop->height=(yyvsp[(4) - (4)].number);
}
    break;

  case 12:
#line 439 "script.y"
    {
	/* Couleur de fond */
	scriptprop->backcolor=(yyvsp[(3) - (3)].str);
	scriptprop->colorset = -1;
}
    break;

  case 13:
#line 445 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->forecolor=(yyvsp[(3) - (3)].str);
	scriptprop->colorset = -1;
}
    break;

  case 14:
#line 451 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->shadcolor=(yyvsp[(3) - (3)].str);
	scriptprop->colorset = -1;
}
    break;

  case 15:
#line 457 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->hilicolor=(yyvsp[(3) - (3)].str);
	scriptprop->colorset = -1;
}
    break;

  case 16:
#line 463 "script.y"
    {
	scriptprop->colorset = (yyvsp[(3) - (3)].number);
	AllocColorset((yyvsp[(3) - (3)].number));
}
    break;

  case 17:
#line 468 "script.y"
    {
	scriptprop->font=(yyvsp[(2) - (2)].str);
}
    break;

  case 19:
#line 474 "script.y"
    {
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 21:
#line 480 "script.y"
    {
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 23:
#line 486 "script.y"
    {
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 26:
#line 499 "script.y"
    { nbobj++;
				  if (nbobj>1000)
				  { yyerror("Too many items\n");
				    exit(1);}
				  if (((yyvsp[(1) - (1)].number)<1)||((yyvsp[(1) - (1)].number)>1000))
				  { yyerror("Choose item id between 1 and 1000\n");
				    exit(1);}
				  if (TabIdObj[(yyvsp[(1) - (1)].number)]!=-1)
				  { i=(yyvsp[(1) - (1)].number); fprintf(stderr,"Line %d: item id %d already used:\n",numligne,(yyvsp[(1) - (1)].number));
				    exit(1);}
			          TabIdObj[(yyvsp[(1) - (1)].number)]=nbobj;
				  (*tabobj)[nbobj].id=(yyvsp[(1) - (1)].number);
				  (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 28:
#line 516 "script.y"
    {
				 (*tabobj)[nbobj].type=(yyvsp[(3) - (3)].str);
				 HasType=1;
				}
    break;

  case 29:
#line 520 "script.y"
    {
				 (*tabobj)[nbobj].width=(yyvsp[(3) - (4)].number);
				 (*tabobj)[nbobj].height=(yyvsp[(4) - (4)].number);
				}
    break;

  case 30:
#line 524 "script.y"
    {
				 (*tabobj)[nbobj].x=(yyvsp[(3) - (4)].number);
				 (*tabobj)[nbobj].y=(yyvsp[(4) - (4)].number);
				 HasPosition=1;
				}
    break;

  case 31:
#line 529 "script.y"
    {
				 (*tabobj)[nbobj].value=(yyvsp[(3) - (3)].number);
				}
    break;

  case 32:
#line 532 "script.y"
    {
				 (*tabobj)[nbobj].value2=(yyvsp[(3) - (3)].number);
				}
    break;

  case 33:
#line 535 "script.y"
    {
				 (*tabobj)[nbobj].value3=(yyvsp[(3) - (3)].number);
				}
    break;

  case 34:
#line 538 "script.y"
    {
				 (*tabobj)[nbobj].title= (yyvsp[(3) - (3)].str);
				}
    break;

  case 35:
#line 541 "script.y"
    {
				 (*tabobj)[nbobj].title= FGettextCopy((yyvsp[(3) - (3)].str));
				}
    break;

  case 36:
#line 544 "script.y"
    {
				 (*tabobj)[nbobj].swallow=(yyvsp[(3) - (3)].str);
				}
    break;

  case 37:
#line 547 "script.y"
    {
				 (*tabobj)[nbobj].icon=(yyvsp[(3) - (3)].str);
				}
    break;

  case 38:
#line 550 "script.y"
    {
				 (*tabobj)[nbobj].backcolor=(yyvsp[(3) - (3)].str);
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 39:
#line 554 "script.y"
    {
				 (*tabobj)[nbobj].forecolor=(yyvsp[(3) - (3)].str);
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 40:
#line 558 "script.y"
    {
				 (*tabobj)[nbobj].shadcolor=(yyvsp[(3) - (3)].str);
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 41:
#line 562 "script.y"
    {
				 (*tabobj)[nbobj].hilicolor=(yyvsp[(3) - (3)].str);
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 42:
#line 566 "script.y"
    {
				 (*tabobj)[nbobj].colorset = (yyvsp[(3) - (3)].number);
				 AllocColorset((yyvsp[(3) - (3)].number));
				}
    break;

  case 43:
#line 570 "script.y"
    {
				 (*tabobj)[nbobj].font=(yyvsp[(2) - (2)].str);
				}
    break;

  case 46:
#line 576 "script.y"
    {
				 (*tabobj)[nbobj].flags[0]=True;
				}
    break;

  case 47:
#line 579 "script.y"
    {
				 (*tabobj)[nbobj].flags[1]=True;
				}
    break;

  case 48:
#line 582 "script.y"
    {
				 (*tabobj)[nbobj].flags[2]=True;
				}
    break;

  case 49:
#line 585 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				}
    break;

  case 50:
#line 588 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				}
    break;

  case 51:
#line 591 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
				}
    break;

  case 52:
#line 597 "script.y"
    {
				  if (!HasPosition)
				   { yyerror("No position for object");
				     exit(1);}
				  if (!HasType)
				   { yyerror("No type for object");
				     exit(1);}
				  HasPosition=0;
				  HasType=0;
				 }
    break;

  case 53:
#line 608 "script.y"
    { InitObjTabCase(0); }
    break;

  case 55:
#line 612 "script.y"
    { InitObjTabCase(1); }
    break;

  case 59:
#line 619 "script.y"
    { InitCase(-1); }
    break;

  case 60:
#line 620 "script.y"
    { InitCase(-2); }
    break;

  case 61:
#line 623 "script.y"
    { InitCase((yyvsp[(1) - (1)].number)); }
    break;

  case 72:
#line 640 "script.y"
    {AddCom(27,1);}
    break;

  case 73:
#line 641 "script.y"
    {AddCom(28,1);}
    break;

  case 99:
#line 670 "script.y"
    {AddCom(27,1);}
    break;

  case 100:
#line 671 "script.y"
    {AddCom(28,1);}
    break;

  case 117:
#line 690 "script.y"
    { AddCom(1,1); }
    break;

  case 118:
#line 692 "script.y"
    { AddCom(2,1);}
    break;

  case 119:
#line 694 "script.y"
    { AddCom(3,1);}
    break;

  case 120:
#line 696 "script.y"
    { AddCom(4,2);}
    break;

  case 121:
#line 698 "script.y"
    { AddCom(21,2);}
    break;

  case 122:
#line 700 "script.y"
    { AddCom(22,2);}
    break;

  case 123:
#line 702 "script.y"
    { AddCom(5,3);}
    break;

  case 124:
#line 704 "script.y"
    { AddCom(6,3);}
    break;

  case 125:
#line 706 "script.y"
    { AddCom(7,2);}
    break;

  case 126:
#line 708 "script.y"
    { AddCom(8,2);}
    break;

  case 127:
#line 710 "script.y"
    { AddCom(9,2);}
    break;

  case 128:
#line 712 "script.y"
    { AddCom(10,2);}
    break;

  case 129:
#line 714 "script.y"
    { AddCom(19,2);}
    break;

  case 130:
#line 716 "script.y"
    { AddCom(24,2);}
    break;

  case 131:
#line 718 "script.y"
    { AddCom(11,2);}
    break;

  case 132:
#line 720 "script.y"
    { AddCom(12,2);}
    break;

  case 133:
#line 722 "script.y"
    { AddCom(13,0);}
    break;

  case 134:
#line 724 "script.y"
    { AddCom(17,1);}
    break;

  case 135:
#line 726 "script.y"
    { AddCom(23,2);}
    break;

  case 136:
#line 728 "script.y"
    { AddCom(18,2);}
    break;

  case 137:
#line 730 "script.y"
    { AddCom(25,5);}
    break;

  case 138:
#line 732 "script.y"
    { AddCom(26,2);}
    break;

  case 142:
#line 742 "script.y"
    { AddComBloc(14,3,2); }
    break;

  case 145:
#line 747 "script.y"
    { EmpilerBloc(); }
    break;

  case 146:
#line 749 "script.y"
    { DepilerBloc(2); }
    break;

  case 147:
#line 750 "script.y"
    { DepilerBloc(2); }
    break;

  case 148:
#line 753 "script.y"
    { DepilerBloc(1); }
    break;

  case 149:
#line 754 "script.y"
    { DepilerBloc(1); }
    break;

  case 150:
#line 758 "script.y"
    { AddComBloc(15,3,1); }
    break;

  case 151:
#line 762 "script.y"
    { AddComBloc(16,3,1); }
    break;

  case 152:
#line 767 "script.y"
    { AddVar((yyvsp[(1) - (1)].str)); }
    break;

  case 153:
#line 769 "script.y"
    { AddConstStr((yyvsp[(1) - (1)].str)); }
    break;

  case 154:
#line 771 "script.y"
    { AddConstStr((yyvsp[(1) - (1)].str)); }
    break;

  case 155:
#line 773 "script.y"
    { AddConstNum((yyvsp[(1) - (1)].number)); }
    break;

  case 156:
#line 775 "script.y"
    { AddConstNum(-1); }
    break;

  case 157:
#line 777 "script.y"
    { AddConstNum(-2); }
    break;

  case 158:
#line 779 "script.y"
    { AddLevelBufArg(); }
    break;

  case 159:
#line 781 "script.y"
    { AddFunct(1,1); }
    break;

  case 160:
#line 782 "script.y"
    { AddFunct(2,1); }
    break;

  case 161:
#line 783 "script.y"
    { AddFunct(3,1); }
    break;

  case 162:
#line 784 "script.y"
    { AddFunct(4,1); }
    break;

  case 163:
#line 785 "script.y"
    { AddFunct(5,1); }
    break;

  case 164:
#line 786 "script.y"
    { AddFunct(6,1); }
    break;

  case 165:
#line 787 "script.y"
    { AddFunct(7,1); }
    break;

  case 166:
#line 788 "script.y"
    { AddFunct(8,1); }
    break;

  case 167:
#line 789 "script.y"
    { AddFunct(9,1); }
    break;

  case 168:
#line 790 "script.y"
    { AddFunct(10,1); }
    break;

  case 169:
#line 791 "script.y"
    { AddFunct(11,1); }
    break;

  case 170:
#line 792 "script.y"
    { AddFunct(12,1); }
    break;

  case 171:
#line 793 "script.y"
    { AddFunct(13,1); }
    break;

  case 172:
#line 794 "script.y"
    { AddFunct(14,1); }
    break;

  case 173:
#line 795 "script.y"
    { AddFunct(15,1); }
    break;

  case 174:
#line 796 "script.y"
    { AddFunct(16,1); }
    break;

  case 175:
#line 797 "script.y"
    { AddFunct(17,1); }
    break;

  case 176:
#line 798 "script.y"
    { AddFunct(18,1); }
    break;

  case 177:
#line 799 "script.y"
    { AddFunct(19,1); }
    break;

  case 178:
#line 800 "script.y"
    { AddFunct(20,1); }
    break;

  case 179:
#line 801 "script.y"
    { AddFunct(21,1); }
    break;

  case 180:
#line 802 "script.y"
    { AddFunct(22,1); }
    break;

  case 181:
#line 803 "script.y"
    { AddFunct(23,1); }
    break;

  case 182:
#line 804 "script.y"
    { AddFunct(24,1); }
    break;

  case 183:
#line 805 "script.y"
    { AddFunct(25,1); }
    break;

  case 184:
#line 806 "script.y"
    { AddFunct(26,1); }
    break;

  case 185:
#line 811 "script.y"
    { }
    break;

  case 212:
#line 856 "script.y"
    { l=1-250000; AddBufArg(&l,1); }
    break;

  case 213:
#line 857 "script.y"
    { l=2-250000; AddBufArg(&l,1); }
    break;

  case 214:
#line 858 "script.y"
    { l=3-250000; AddBufArg(&l,1); }
    break;

  case 215:
#line 859 "script.y"
    { l=4-250000; AddBufArg(&l,1); }
    break;

  case 216:
#line 860 "script.y"
    { l=5-250000; AddBufArg(&l,1); }
    break;

  case 217:
#line 861 "script.y"
    { l=6-250000; AddBufArg(&l,1); }
    break;


/* Line 1267 of yacc.c.  */
#line 3210 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 864 "script.y"


