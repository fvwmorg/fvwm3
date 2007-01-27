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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

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




/* Copy the first part of user declarations.  */
#line 1 "script.y"

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
 scriptprop=(ScriptProp*) safecalloc(1,sizeof(ScriptProp));
 scriptprop->x=-1;
 scriptprop->y=-1;
 scriptprop->colorset = -1;
 scriptprop->initbloc=NULL;

 tabobj=(TabObj*) safecalloc(1,sizeof(TabObj));
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
  TabIObj=(Bloc**)safecalloc(1,sizeof(long));
  TabCObj=(CaseObj*)safecalloc(1,sizeof(CaseObj));
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
  TabCObj[nbobj].LstCase=(int*)safecalloc(1,sizeof(int));
 else
  TabCObj[nbobj].LstCase=(int*)realloc(TabCObj[nbobj].LstCase,sizeof(int)*(CurrCase+1));
 TabCObj[nbobj].LstCase[CurrCase]=cond;

 if (CurrCase==0)
  TabIObj[nbobj]=(Bloc*)safecalloc(1,sizeof(Bloc));
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
  Temp=(long*)safecalloc(1,sizeof(long));
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
  PileBloc[TopPileB]->TabInstr=(Instr*)safecalloc(1,sizeof(Instr)*(CurrInstr+1));
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
  TabNVar=(char**)safecalloc(1,sizeof(long));
  TabVVar=(char**)safecalloc(1,sizeof(long));
 }
 else
 {
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=(char*)safestrdup(Name);
 TabVVar[NbVar]=(char*)safecalloc(1,sizeof(char));
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
  TabVVar=(char**)safecalloc(1,sizeof(long));
  TabNVar=(char**)safecalloc(1,sizeof(long));
 }
 else
 {
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=(char*)safecalloc(1,sizeof(char));
 TabNVar[NbVar][0]='\0';
 TabVVar[NbVar]=(char*)safestrdup(Name);

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
  l=(long*)safecalloc(1,sizeof(long));
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

 TmpBloc=(Bloc*)safecalloc(1,sizeof(Bloc));
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

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 354 "script.y"
typedef union YYSTYPE {  char *str;
          int number;
       } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 640 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 652 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
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
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1136

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  106
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  67
/* YYNRULES -- Number of rules. */
#define YYNRULES  213
/* YYNRULES -- Number of states. */
#define YYNSTATES  464

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   360

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
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
     105
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,    10,    11,    12,    16,    19,    23,    27,
      31,    36,    41,    45,    49,    53,    57,    61,    64,    65,
      71,    72,    78,    79,    85,    86,    94,    96,    97,   101,
     106,   111,   115,   119,   123,   127,   131,   135,   139,   143,
     147,   151,   155,   159,   162,   166,   167,   170,   173,   176,
     179,   182,   185,   186,   188,   194,   195,   196,   201,   206,
     208,   210,   212,   216,   217,   221,   225,   229,   233,   237,
     241,   245,   249,   253,   257,   261,   265,   269,   273,   277,
     281,   285,   289,   293,   297,   301,   305,   309,   313,   317,
     320,   323,   326,   329,   332,   335,   338,   341,   344,   347,
     350,   353,   356,   359,   362,   365,   368,   371,   374,   377,
     380,   383,   386,   389,   392,   395,   398,   403,   408,   413,
     420,   427,   432,   437,   442,   447,   452,   457,   463,   468,
     469,   472,   477,   482,   493,   498,   503,   507,   511,   519,
     520,   524,   525,   529,   531,   535,   537,   547,   555,   557,
     559,   561,   563,   565,   567,   568,   571,   574,   579,   583,
     586,   590,   594,   598,   603,   606,   608,   611,   615,   617,
     620,   623,   626,   629,   632,   635,   638,   640,   645,   649,
     651,   654,   655,   658,   661,   664,   667,   670,   673,   679,
     681,   683,   685,   687,   689,   691,   696,   698,   700,   702,
     704,   709,   711,   713,   718,   720,   722,   727,   729,   731,
     733,   735,   737,   739
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     107,     0,    -1,   108,   109,   110,   111,   112,   113,    -1,
      -1,    -1,   109,    12,     4,    -1,   109,    12,    -1,   109,
       8,     4,    -1,   109,     9,     4,    -1,   109,    33,     3,
      -1,   109,    11,     7,     7,    -1,   109,    10,     7,     7,
      -1,   109,    14,     4,    -1,   109,    13,     4,    -1,   109,
      15,     4,    -1,   109,    16,     4,    -1,   109,    17,     7,
      -1,   109,     6,    -1,    -1,    19,   153,    47,   124,    23,
      -1,    -1,    20,   153,    47,   124,    23,    -1,    -1,    21,
     153,    47,   124,    23,    -1,    -1,   113,    18,   114,    24,
     115,   117,   118,    -1,     7,    -1,    -1,   115,    25,     3,
      -1,   115,    26,     7,     7,    -1,   115,    27,     7,     7,
      -1,   115,    28,     7,    -1,   115,    29,     7,    -1,   115,
      30,     7,    -1,   115,    31,     4,    -1,   115,    37,     4,
      -1,   115,    32,     4,    -1,   115,    33,     3,    -1,   115,
      14,     4,    -1,   115,    13,     4,    -1,   115,    15,     4,
      -1,   115,    16,     4,    -1,   115,    17,     7,    -1,   115,
       6,    -1,   115,    34,   116,    -1,    -1,   116,    38,    -1,
     116,    40,    -1,   116,    39,    -1,   116,    41,    -1,   116,
      42,    -1,   116,    43,    -1,    -1,    23,    -1,    22,   119,
      44,   120,    23,    -1,    -1,    -1,   120,   121,    48,   123,
      -1,   120,   122,    48,   123,    -1,    45,    -1,    46,    -1,
       7,    -1,    47,   124,    23,    -1,    -1,   124,    49,   126,
      -1,   124,    35,   143,    -1,   124,    36,   145,    -1,   124,
      50,   127,    -1,   124,    51,   128,    -1,   124,    64,   129,
      -1,   124,    65,   130,    -1,   124,    66,   131,    -1,   124,
      27,   132,    -1,   124,    26,   133,    -1,   124,    31,   135,
      -1,   124,    37,   147,    -1,   124,    33,   134,    -1,   124,
      52,   136,    -1,   124,    53,   137,    -1,   124,    54,   138,
      -1,   124,    55,   139,    -1,   124,    81,   140,    -1,   124,
      82,   141,    -1,   124,    75,   142,    -1,   124,    78,   144,
      -1,   124,    91,   148,    -1,   124,    94,   149,    -1,   124,
      97,   150,    -1,   124,    56,   146,    -1,    49,   126,    -1,
      35,   143,    -1,    36,   145,    -1,    50,   127,    -1,    51,
     128,    -1,    64,   129,    -1,    65,   130,    -1,    66,   131,
      -1,    27,   132,    -1,    26,   133,    -1,    31,   135,    -1,
      37,   147,    -1,    33,   134,    -1,    52,   136,    -1,    53,
     137,    -1,    54,   138,    -1,    55,   139,    -1,    81,   140,
      -1,    82,   141,    -1,    75,   142,    -1,    78,   144,    -1,
      94,   149,    -1,    97,   150,    -1,    56,   146,    -1,   164,
     166,    -1,   164,   168,    -1,   164,   168,    -1,   164,   168,
     164,   168,    -1,   164,   168,   164,   168,    -1,   164,   168,
     164,   168,    -1,   164,   168,   164,   168,   164,   168,    -1,
     164,   168,   164,   168,   164,   168,    -1,   164,   168,   164,
     169,    -1,   164,   168,   164,   170,    -1,   164,   168,   164,
     166,    -1,   164,   168,   164,   170,    -1,   164,   168,   164,
     170,    -1,   164,   168,   164,   168,    -1,   164,   171,    80,
     164,   166,    -1,   164,   168,   164,   168,    -1,    -1,   164,
     168,    -1,   164,   168,   164,   166,    -1,   164,   169,   164,
     166,    -1,   164,   169,   164,   169,   164,   168,   164,   168,
     164,   166,    -1,   164,   168,   164,   170,    -1,   151,   153,
     154,   152,    -1,   156,   153,   155,    -1,   157,   153,   155,
      -1,   164,   167,   164,   172,   164,   167,    92,    -1,    -1,
      93,   153,   155,    -1,    -1,    47,   124,    23,    -1,   125,
      -1,    47,   124,    23,    -1,   125,    -1,   164,   171,    80,
     164,   167,    95,   164,   167,    96,    -1,   164,   167,   164,
     172,   164,   167,    96,    -1,     5,    -1,     3,    -1,     4,
      -1,     7,    -1,    45,    -1,    46,    -1,    -1,    57,   168,
      -1,    70,   168,    -1,    71,   170,   168,   168,    -1,    73,
     168,   168,    -1,    74,   170,    -1,    67,   168,   168,    -1,
      69,   168,   168,    -1,    68,   168,   168,    -1,    72,   170,
     168,   168,    -1,    76,   170,    -1,    77,    -1,    79,   168,
      -1,    83,   168,   168,    -1,    84,    -1,    85,   168,    -1,
      60,   168,    -1,    61,   168,    -1,    62,   168,    -1,    63,
     168,    -1,    58,   168,    -1,    59,   168,    -1,    86,    -1,
      87,   170,   170,   168,    -1,    88,   170,   168,    -1,    89,
      -1,    90,   170,    -1,    -1,   162,   166,    -1,   163,   166,
      -1,   158,   166,    -1,   160,   166,    -1,   159,   166,    -1,
     161,   166,    -1,    98,   164,   165,    99,   166,    -1,   158,
      -1,   162,    -1,   163,    -1,   160,    -1,   159,    -1,   161,
      -1,    98,   164,   165,    99,    -1,   162,    -1,   163,    -1,
     161,    -1,   158,    -1,    98,   164,   165,    99,    -1,   158,
      -1,   159,    -1,    98,   164,   165,    99,    -1,   158,    -1,
     160,    -1,    98,   164,   165,    99,    -1,   158,    -1,   103,
      -1,   101,    -1,   100,    -1,   102,    -1,   104,    -1,   105,
      -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   379,   379,   382,   386,   387,   392,   398,   403,   408,
     412,   418,   424,   430,   436,   442,   448,   453,   459,   460,
     465,   466,   471,   472,   481,   482,   485,   501,   502,   506,
     510,   515,   518,   521,   524,   527,   530,   533,   536,   540,
     544,   548,   552,   556,   559,   561,   562,   565,   568,   571,
     574,   577,   583,   594,   595,   598,   600,   601,   602,   605,
     606,   609,   612,   617,   618,   619,   620,   621,   622,   623,
     624,   625,   626,   627,   628,   629,   630,   631,   632,   633,
     634,   635,   636,   637,   638,   639,   640,   641,   642,   646,
     647,   648,   649,   650,   651,   652,   653,   654,   655,   656,
     657,   658,   659,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   669,   672,   674,   676,   678,   680,   682,   684,
     686,   688,   690,   692,   694,   696,   698,   700,   702,   704,
     706,   708,   710,   712,   714,   716,   718,   720,   724,   726,
     727,   729,   731,   732,   735,   736,   740,   744,   749,   751,
     753,   755,   757,   759,   761,   763,   764,   765,   766,   767,
     768,   769,   770,   771,   772,   773,   774,   775,   776,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,   787,
     788,   793,   794,   795,   796,   797,   798,   799,   800,   804,
     805,   806,   807,   808,   809,   810,   814,   815,   816,   817,
     818,   822,   823,   824,   828,   829,   830,   834,   838,   839,
     840,   841,   842,   843
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
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
  "KEY", "GETVALUE", "GETMINVALUE", "GETMAXVALUE", "GETFORE", "GETBACK",
  "GETHILIGHT", "GETSHADOW", "CHVALUE", "CHVALUEMAX", "CHVALUEMIN", "ADD",
  "DIV", "MULT", "GETTITLE", "GETOUTPUT", "STRCOPY", "NUMTOHEX",
  "HEXTONUM", "QUIT", "LAUNCHSCRIPT", "GETSCRIPTFATHER", "SENDTOSCRIPT",
  "RECEIVFROMSCRIPT", "GET", "SET", "SENDSIGN", "REMAINDEROFDIV",
  "GETTIME", "GETSCRIPTARG", "GETPID", "SENDMSGANDGET", "PARSE",
  "LASTSTRING", "GETTEXT", "IF", "THEN", "ELSE", "FOR", "TO", "DO",
  "WHILE", "BEGF", "ENDF", "EQUAL", "INFEQ", "SUPEQ", "INF", "SUP", "DIFF",
  "$accept", "script", "initvar", "head", "initbloc", "periodictask",
  "quitfunc", "object", "id", "init", "flags", "verify", "mainloop",
  "addtabcase", "case", "clic", "number", "bloc", "instr", "oneinstr",
  "exec", "hide", "show", "chvalue", "chvaluemax", "chvaluemin",
  "position", "size", "icon", "title", "font", "chforecolor",
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
static const unsigned short int yytoknum[] =
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
     355,   356,   357,   358,   359,   360
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   106,   107,   108,   109,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   110,   110,
     111,   111,   112,   112,   113,   113,   114,   115,   115,   115,
     115,   115,   115,   115,   115,   115,   115,   115,   115,   115,
     115,   115,   115,   115,   115,   116,   116,   116,   116,   116,
     116,   116,   117,   118,   118,   119,   120,   120,   120,   121,
     121,   122,   123,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   125,
     125,   125,   125,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   125,   125,   125,
     125,   125,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     152,   153,   154,   154,   155,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   166,   166,   166,   166,   166,   166,   166,   166,   167,
     167,   167,   167,   167,   167,   167,   168,   168,   168,   168,
     168,   169,   169,   169,   170,   170,   170,   171,   172,   172,
     172,   172,   172,   172
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     6,     0,     0,     3,     2,     3,     3,     3,
       4,     4,     3,     3,     3,     3,     3,     2,     0,     5,
       0,     5,     0,     5,     0,     7,     1,     0,     3,     4,
       4,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     2,     3,     0,     2,     2,     2,     2,
       2,     2,     0,     1,     5,     0,     0,     4,     4,     1,
       1,     1,     3,     0,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     4,     4,     4,     6,
       6,     4,     4,     4,     4,     4,     4,     5,     4,     0,
       2,     4,     4,    10,     4,     4,     3,     3,     7,     0,
       3,     0,     3,     1,     3,     1,     9,     7,     1,     1,
       1,     1,     1,     1,     0,     2,     2,     4,     3,     2,
       3,     3,     3,     4,     2,     1,     2,     3,     1,     2,
       2,     2,     2,     2,     2,     2,     1,     4,     3,     1,
       2,     0,     2,     2,     2,     2,     2,     2,     5,     1,
       1,     1,     1,     1,     1,     4,     1,     1,     1,     1,
       4,     1,     1,     4,     1,     1,     4,     1,     1,     1,
       1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       3,     0,     4,     1,    18,    17,     0,     0,     0,     0,
       6,     0,     0,     0,     0,     0,   141,     0,    20,     7,
       8,     0,     0,     5,    13,    12,    14,    15,    16,     0,
       9,   141,    22,    11,    10,    63,     0,   141,    24,     0,
      63,     0,     2,    19,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   129,   154,   154,   154,   154,   154,   154,     0,
      63,     0,    73,     0,    72,     0,    74,     0,    76,     0,
      65,     0,    66,     0,    75,     0,    64,   181,    67,     0,
      68,     0,    77,     0,    78,     0,    79,     0,    80,     0,
      88,     0,    69,     0,    70,     0,    71,     0,    83,    84,
       0,    81,     0,    82,     0,    85,   141,     0,    86,   141,
       0,    87,   141,     0,    21,     0,    26,     0,   148,   151,
     152,   153,   154,   199,   198,   196,   197,   154,   154,   154,
     154,   130,   149,   154,   201,   202,   154,   154,   150,   154,
     181,   181,   181,   181,   181,   181,   113,   114,   115,   154,
     154,   154,   154,   154,   154,   154,   154,   154,   207,     0,
     154,     0,   154,   189,   193,   192,   194,   190,   191,   154,
       0,     0,     0,   154,    23,    27,     0,     0,     0,     0,
       0,     0,   181,     0,     0,   184,   186,   185,   187,   182,
     183,   181,     0,     0,     0,     0,     0,     0,     0,   181,
     154,     0,   154,   154,   154,   154,   154,   154,   154,    63,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   129,   154,   154,   154,   154,   154,   143,   139,     0,
       0,    63,   145,   136,   154,   137,     0,    52,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   165,     0,     0,   168,     0,   176,
       0,     0,   179,     0,     0,   154,   154,   154,   204,   205,
     122,   121,     0,   132,   134,     0,   123,   124,   125,   126,
     154,   116,   117,   118,   131,   181,   128,    98,    97,    99,
     101,    90,    91,   100,     0,    89,    92,    93,   102,   103,
     104,   105,   112,    94,    95,    96,   108,   109,   106,   107,
     110,   111,   141,   135,     0,   210,   209,   211,   208,   212,
     213,   154,     0,     0,   154,    43,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,     0,     0,   155,   174,   175,   170,   171,   172,   173,
       0,     0,     0,   156,     0,     0,     0,   159,   164,   166,
       0,   169,     0,     0,   180,   200,     0,     0,     0,   203,
     181,     0,   127,   142,     0,   195,     0,   144,     0,     0,
      39,    38,    40,    41,    42,    28,     0,     0,    31,    32,
      33,    34,    36,    37,    44,    35,    55,    53,    25,   160,
     162,   161,     0,     0,   158,   167,     0,   178,   120,   119,
       0,   188,   154,   140,     0,   154,     0,    29,    30,    46,
      48,    47,    49,    50,    51,     0,   157,   163,   177,   206,
       0,   138,     0,   147,    56,   154,     0,     0,   181,   146,
      61,    54,    59,    60,     0,     0,   133,     0,     0,    63,
      57,    58,     0,    62
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,     4,    18,    32,    38,    42,   127,   247,
     404,   352,   408,   435,   447,   454,   455,   460,    39,   242,
      86,    88,    90,   102,   104,   106,    74,    72,    78,    76,
      92,    94,    96,    98,   111,   113,   108,    80,   109,    82,
     100,    84,   115,   118,   121,   116,   323,    29,   238,   243,
     119,   122,   133,   151,   152,   134,   135,   136,    73,   274,
     156,   179,   137,   146,   280,   169,   331
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -282
static const short int yypact[] =
{
    -282,     3,  -282,  -282,   567,  -282,     1,     6,     5,    44,
      54,    56,    58,    60,    62,    75,  -282,    83,    67,  -282,
    -282,    81,    92,  -282,  -282,  -282,  -282,  -282,  -282,    57,
    -282,  -282,    80,  -282,  -282,  -282,    63,  -282,  -282,   610,
    -282,    64,    85,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,   704,
    -282,   101,  -282,    73,  -282,    73,  -282,    73,  -282,    73,
    -282,    73,  -282,    -1,  -282,    73,  -282,    24,  -282,    73,
    -282,    73,  -282,    73,  -282,    73,  -282,    73,  -282,    73,
    -282,    -1,  -282,    73,  -282,    73,  -282,    73,  -282,  -282,
      73,  -282,   104,  -282,    73,  -282,  -282,    69,  -282,  -282,
     104,  -282,  -282,    69,  -282,   757,  -282,    88,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
      24,    24,    24,    24,    24,    24,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,    33,
    -282,   969,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    1005,    36,  1005,  -282,  -282,  -282,  1046,    73,    73,     4,
      -1,  1046,    24,     4,  1046,  -282,  -282,  -282,  -282,  -282,
    -282,    24,     4,     4,    73,    -1,    73,    73,    73,    24,
    -282,    73,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,    27,  1046,
     -85,  -282,  -282,  -282,  -282,  -282,   -85,   683,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,     4,
       4,    73,     4,     4,  -282,    73,    73,  -282,    73,  -282,
       4,     4,  -282,     4,    18,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,    26,  -282,  -282,    31,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,    24,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,   810,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,    32,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,   863,    69,  -282,  -282,   119,   129,   130,   133,
     132,   137,   134,   136,   138,   139,   140,   153,   156,   141,
    -282,   158,    10,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
      73,    73,    73,  -282,    73,    73,    73,  -282,  -282,  -282,
      73,  -282,     4,    73,  -282,  -282,    73,    73,  1046,  -282,
      24,    73,  -282,  -282,  1005,  -282,    69,  -282,    68,    69,
    -282,  -282,  -282,  -282,  -282,  -282,   159,   162,  -282,  -282,
    -282,  -282,  -282,  -282,   -17,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,    73,    73,  -282,  -282,    73,  -282,  -282,  -282,
      66,  -282,  -282,  -282,    78,  -282,    79,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,   142,  -282,  -282,  -282,  -282,
      73,  -282,    69,  -282,  -282,  -282,    82,    61,    24,  -282,
    -282,  -282,  -282,  -282,   128,   135,  -282,   144,   144,  -282,
    -282,  -282,   916,  -282
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -281,   -40,     9,
     -39,   -37,   -35,   -46,   -44,   -42,   -18,   -23,   -19,    12,
      -3,     8,   -25,     2,    -6,    -5,    14,    37,    -2,    38,
      21,    39,  -282,    23,    13,  -282,  -282,   -24,  -282,  -176,
    -282,  -282,   176,    41,   202,   163,   183,   301,   -11,  -180,
     290,   -29,   -14,  -100,   266,   147,    15
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned short int yytable[] =
{
      69,   163,   142,     3,   128,    19,   245,    36,   148,   128,
      20,   282,    21,    41,   285,   325,   326,   327,   328,   329,
     330,   429,   430,   431,   432,   433,   434,   142,   148,   128,
     125,   129,   406,   407,    75,    77,    79,    81,    83,    85,
      87,    89,    91,    93,    95,    97,    99,   101,   103,   105,
     107,    22,   110,   112,   114,   117,   120,   123,    23,   324,
      24,   138,    25,   139,    26,   140,    27,   141,   450,   130,
     131,   147,   142,   148,   128,   157,   129,   158,   128,   159,
     129,   160,    28,   161,   451,   162,    30,    31,    33,   164,
     281,   165,   171,   166,   183,   180,   167,   143,   182,    34,
     170,    37,   277,    71,    35,   290,   452,   453,   126,   128,
      40,    70,   185,   210,   130,   131,   244,   375,   130,   131,
     322,   186,   149,   390,   145,   379,   187,   188,   189,   190,
     380,   385,   191,   391,   392,   192,   193,   393,   194,   394,
     395,   396,   145,   397,   403,   398,   399,   400,   201,   202,
     203,   204,   205,   206,   207,   208,   209,   401,   174,   211,
     402,   239,   405,   425,   174,   439,   427,   172,   240,   428,
     441,   132,   246,   275,   276,   443,   457,   461,   449,   304,
     237,   305,   313,   458,   306,   314,   444,   307,   315,   297,
     289,   459,   291,   292,   293,   298,   300,   296,   420,   295,
     310,   332,    75,    77,    79,    81,    83,    85,   423,    87,
      89,    91,    93,    95,    97,    99,   101,   103,   105,   107,
     308,   110,   112,   114,   120,   123,   299,   318,   311,   319,
     317,   145,   309,   333,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   316,   145,   366,   312,   321,
     153,   369,   370,   301,   371,   302,     0,   303,   320,   144,
       0,   334,     0,   150,   376,   377,   378,   181,     0,     0,
     154,     0,     0,     0,     0,     0,     0,   144,     0,   381,
     176,     0,     0,     0,     0,     0,   176,     0,   168,     0,
       0,     0,     0,   173,     0,     0,   168,     0,   384,   173,
     177,     0,     0,     0,   388,     0,   177,     0,     0,     0,
       0,     0,     0,   153,   153,   153,   153,   153,   153,   175,
     386,     0,     0,   389,     0,   175,   150,   150,   150,   150,
     150,   150,     0,   154,   154,   154,   154,   154,   154,     0,
       0,     0,     0,     0,     0,     0,   409,   410,   411,     0,
     412,   413,   414,     0,     0,   153,   415,   424,     0,   417,
     426,     0,   418,   419,   153,   278,   144,   422,   150,   278,
       0,     0,   153,     0,   174,   154,     0,   150,   278,   278,
       0,   144,     0,     0,   154,   150,     0,     0,   155,     0,
       0,   279,   154,     0,     0,   279,     0,     0,   436,   437,
       0,     0,   438,     0,   279,   279,     0,     0,     0,     0,
       0,   440,     0,   446,   442,     0,     0,     0,   178,   462,
       0,     0,     0,     0,   178,     0,   445,   174,     0,     0,
     174,     0,     0,     0,   448,   278,   278,     0,   278,   278,
     195,   196,   197,   198,   199,   200,   278,   278,     0,   278,
       0,   155,   155,   155,   155,   155,   155,     0,   153,   284,
       0,   279,   279,     0,   279,   279,     0,     0,   287,   288,
       0,   150,   279,   279,     0,   279,     0,     0,   154,     0,
       0,     0,   283,   174,     0,     0,     0,     0,     0,     0,
       0,   286,     0,   155,     0,     0,   176,     0,     0,   294,
       0,     0,   155,     0,     0,     0,     0,     0,     0,   173,
     155,     0,     0,     0,     0,     0,   177,     0,     0,     0,
       0,     0,     0,     0,     0,   364,   365,     0,   367,   368,
       0,     0,     0,     0,     0,   175,   372,   373,     0,   374,
       0,     0,     0,   153,     0,     0,     0,     0,   278,   176,
       0,     0,   176,     0,     0,     0,   150,     0,     0,     0,
       0,     0,   173,   154,     0,   173,     0,     0,     0,   177,
       0,     0,   177,     5,   279,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,   382,    16,     0,   175,     0,
       0,   175,     0,     0,     0,     0,   155,     0,     0,     0,
      17,     0,     0,     0,     0,   176,     0,     0,     0,     0,
       0,   153,     0,     0,     0,     0,     0,     0,   173,     0,
       0,     0,     0,     0,   150,   177,     0,     0,     0,     0,
       0,   154,     0,    43,   178,     0,    44,    45,   416,     0,
       0,    46,     0,    47,   175,    48,    49,    50,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    51,
      52,    53,    54,    55,    56,    57,    58,     0,     0,     0,
     421,     0,     0,     0,    59,    60,    61,     0,     0,     0,
       0,   155,     0,     0,     0,    62,     0,   178,    63,   335,
     178,    64,    65,     0,     0,     0,   336,   337,   338,   339,
     340,    66,     0,     0,    67,     0,     0,    68,   341,   342,
     343,   344,   345,   346,   347,   348,   349,   350,     0,     0,
     351,     0,     0,     0,     0,     0,     0,   124,     0,     0,
      44,    45,     0,     0,     0,    46,     0,    47,   456,    48,
      49,    50,     0,   178,     0,     0,     0,     0,     0,   155,
       0,     0,     0,    51,    52,    53,    54,    55,    56,    57,
      58,     0,     0,     0,     0,     0,     0,     0,    59,    60,
      61,     0,     0,     0,     0,     0,     0,     0,     0,    62,
     184,     0,    63,    44,    45,    64,    65,     0,    46,     0,
      47,     0,    48,    49,    50,    66,     0,     0,    67,     0,
       0,    68,     0,     0,     0,     0,    51,    52,    53,    54,
      55,    56,    57,    58,     0,     0,     0,     0,     0,     0,
       0,    59,    60,    61,     0,     0,     0,     0,     0,     0,
       0,     0,    62,   383,     0,    63,    44,    45,    64,    65,
       0,    46,     0,    47,     0,    48,    49,    50,    66,     0,
       0,    67,     0,     0,    68,     0,     0,     0,     0,    51,
      52,    53,    54,    55,    56,    57,    58,     0,     0,     0,
       0,     0,     0,     0,    59,    60,    61,     0,     0,     0,
       0,     0,     0,     0,     0,    62,   387,     0,    63,    44,
      45,    64,    65,     0,    46,     0,    47,     0,    48,    49,
      50,    66,     0,     0,    67,     0,     0,    68,     0,     0,
       0,     0,    51,    52,    53,    54,    55,    56,    57,    58,
       0,     0,     0,     0,     0,     0,     0,    59,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,    62,   463,
       0,    63,    44,    45,    64,    65,     0,    46,     0,    47,
       0,    48,    49,    50,    66,     0,     0,    67,     0,     0,
      68,     0,     0,     0,     0,    51,    52,    53,    54,    55,
      56,    57,    58,     0,     0,     0,     0,     0,     0,     0,
      59,    60,    61,     0,     0,     0,     0,     0,     0,     0,
       0,    62,     0,     0,    63,   212,   213,    64,    65,     0,
     214,     0,   215,     0,   216,   217,   218,    66,     0,     0,
      67,     0,     0,    68,     0,     0,   219,     0,   220,   221,
     222,   223,   224,   225,   226,   227,     0,     0,     0,     0,
       0,   212,   213,   228,   229,   230,   214,     0,   215,     0,
     216,   217,   218,     0,   231,     0,     0,   232,     0,     0,
     233,   234,   241,     0,   220,   221,   222,   223,   224,   225,
     226,   227,     0,   235,     0,     0,   236,     0,     0,   228,
     229,   230,     0,     0,     0,     0,     0,     0,     0,     0,
     231,     0,     0,   232,     0,     0,   233,   234,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   235,
       0,     0,   236,   248,   249,   250,   251,   252,   253,   254,
       0,     0,     0,   255,   256,   257,   258,   259,   260,   261,
     262,     0,   263,   264,     0,   265,     0,     0,     0,   266,
     267,   268,   269,   270,   271,   272,   273
};

static const short int yycheck[] =
{
      40,   101,     3,     0,     5,     4,   182,    31,     4,     5,
       4,   191,     7,    37,   194,   100,   101,   102,   103,   104,
     105,    38,    39,    40,    41,    42,    43,     3,     4,     5,
      70,     7,    22,    23,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,     7,    63,    64,    65,    66,    67,    68,     4,   239,
       4,    75,     4,    77,     4,    79,     4,    81,     7,    45,
      46,    85,     3,     4,     5,    89,     7,    91,     5,    93,
       7,    95,     7,    97,    23,    99,     3,    20,     7,   103,
     190,   105,   116,   107,   123,   119,   110,    98,   122,     7,
     114,    21,    98,    18,    47,   205,    45,    46,     7,     5,
      47,    47,    24,    80,    45,    46,    80,    99,    45,    46,
      93,   132,    98,     4,    83,    99,   137,   138,   139,   140,
      99,    99,   143,     4,     4,   146,   147,     4,   149,     7,
       3,     7,   101,     7,     3,     7,     7,     7,   159,   160,
     161,   162,   163,   164,   165,   166,   167,     4,   117,   170,
       4,   172,     4,    95,   123,    99,     7,    98,   179,     7,
      92,    98,   183,   187,   188,    96,    48,   458,    96,   219,
     171,   220,   228,    48,   221,   229,    44,   222,   230,   212,
     204,    47,   206,   207,   208,   213,   215,   211,   378,   210,
     225,   241,   213,   214,   215,   216,   217,   218,   384,   220,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     223,   232,   233,   234,   235,   236,   214,   233,   226,   234,
     232,   190,   224,   244,   248,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   231,   205,   261,   227,   236,
      87,   265,   266,   216,   268,   217,    -1,   218,   235,    83,
      -1,   246,    -1,    87,   275,   276,   277,   120,    -1,    -1,
      87,    -1,    -1,    -1,    -1,    -1,    -1,   101,    -1,   290,
     117,    -1,    -1,    -1,    -1,    -1,   123,    -1,   112,    -1,
      -1,    -1,    -1,   117,    -1,    -1,   120,    -1,   322,   123,
     117,    -1,    -1,    -1,   333,    -1,   123,    -1,    -1,    -1,
      -1,    -1,    -1,   150,   151,   152,   153,   154,   155,   117,
     331,    -1,    -1,   334,    -1,   123,   150,   151,   152,   153,
     154,   155,    -1,   150,   151,   152,   153,   154,   155,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   360,   361,   362,    -1,
     364,   365,   366,    -1,    -1,   192,   370,   386,    -1,   373,
     389,    -1,   376,   377,   201,   189,   190,   381,   192,   193,
      -1,    -1,   209,    -1,   333,   192,    -1,   201,   202,   203,
      -1,   205,    -1,    -1,   201,   209,    -1,    -1,    87,    -1,
      -1,   189,   209,    -1,    -1,   193,    -1,    -1,   412,   413,
      -1,    -1,   416,    -1,   202,   203,    -1,    -1,    -1,    -1,
      -1,   422,    -1,   442,   425,    -1,    -1,    -1,   117,   459,
      -1,    -1,    -1,    -1,   123,    -1,   440,   386,    -1,    -1,
     389,    -1,    -1,    -1,   445,   259,   260,    -1,   262,   263,
     150,   151,   152,   153,   154,   155,   270,   271,    -1,   273,
      -1,   150,   151,   152,   153,   154,   155,    -1,   295,   193,
      -1,   259,   260,    -1,   262,   263,    -1,    -1,   202,   203,
      -1,   295,   270,   271,    -1,   273,    -1,    -1,   295,    -1,
      -1,    -1,   192,   442,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   201,    -1,   192,    -1,    -1,   333,    -1,    -1,   209,
      -1,    -1,   201,    -1,    -1,    -1,    -1,    -1,    -1,   333,
     209,    -1,    -1,    -1,    -1,    -1,   333,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   259,   260,    -1,   262,   263,
      -1,    -1,    -1,    -1,    -1,   333,   270,   271,    -1,   273,
      -1,    -1,    -1,   380,    -1,    -1,    -1,    -1,   372,   386,
      -1,    -1,   389,    -1,    -1,    -1,   380,    -1,    -1,    -1,
      -1,    -1,   386,   380,    -1,   389,    -1,    -1,    -1,   386,
      -1,    -1,   389,     6,   372,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,   295,    19,    -1,   386,    -1,
      -1,   389,    -1,    -1,    -1,    -1,   295,    -1,    -1,    -1,
      33,    -1,    -1,    -1,    -1,   442,    -1,    -1,    -1,    -1,
      -1,   448,    -1,    -1,    -1,    -1,    -1,    -1,   442,    -1,
      -1,    -1,    -1,    -1,   448,   442,    -1,    -1,    -1,    -1,
      -1,   448,    -1,    23,   333,    -1,    26,    27,   372,    -1,
      -1,    31,    -1,    33,   442,    35,    36,    37,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    -1,    -1,    -1,
     380,    -1,    -1,    -1,    64,    65,    66,    -1,    -1,    -1,
      -1,   380,    -1,    -1,    -1,    75,    -1,   386,    78,     6,
     389,    81,    82,    -1,    -1,    -1,    13,    14,    15,    16,
      17,    91,    -1,    -1,    94,    -1,    -1,    97,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,
      26,    27,    -1,    -1,    -1,    31,    -1,    33,   448,    35,
      36,    37,    -1,   442,    -1,    -1,    -1,    -1,    -1,   448,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    54,    55,
      56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    65,
      66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,
      23,    -1,    78,    26,    27,    81,    82,    -1,    31,    -1,
      33,    -1,    35,    36,    37,    91,    -1,    -1,    94,    -1,
      -1,    97,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    54,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    64,    65,    66,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    75,    23,    -1,    78,    26,    27,    81,    82,
      -1,    31,    -1,    33,    -1,    35,    36,    37,    91,    -1,
      -1,    94,    -1,    -1,    97,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    64,    65,    66,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    75,    23,    -1,    78,    26,
      27,    81,    82,    -1,    31,    -1,    33,    -1,    35,    36,
      37,    91,    -1,    -1,    94,    -1,    -1,    97,    -1,    -1,
      -1,    -1,    49,    50,    51,    52,    53,    54,    55,    56,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    64,    65,    66,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    75,    23,
      -1,    78,    26,    27,    81,    82,    -1,    31,    -1,    33,
      -1,    35,    36,    37,    91,    -1,    -1,    94,    -1,    -1,
      97,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      54,    55,    56,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      64,    65,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    75,    -1,    -1,    78,    26,    27,    81,    82,    -1,
      31,    -1,    33,    -1,    35,    36,    37,    91,    -1,    -1,
      94,    -1,    -1,    97,    -1,    -1,    47,    -1,    49,    50,
      51,    52,    53,    54,    55,    56,    -1,    -1,    -1,    -1,
      -1,    26,    27,    64,    65,    66,    31,    -1,    33,    -1,
      35,    36,    37,    -1,    75,    -1,    -1,    78,    -1,    -1,
      81,    82,    47,    -1,    49,    50,    51,    52,    53,    54,
      55,    56,    -1,    94,    -1,    -1,    97,    -1,    -1,    64,
      65,    66,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      75,    -1,    -1,    78,    -1,    -1,    81,    82,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    94,
      -1,    -1,    97,    57,    58,    59,    60,    61,    62,    63,
      -1,    -1,    -1,    67,    68,    69,    70,    71,    72,    73,
      74,    -1,    76,    77,    -1,    79,    -1,    -1,    -1,    83,
      84,    85,    86,    87,    88,    89,    90
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,   107,   108,     0,   109,     6,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    19,    33,   110,     4,
       4,     7,     7,     4,     4,     4,     4,     4,     7,   153,
       3,    20,   111,     7,     7,    47,   153,    21,   112,   124,
      47,   153,   113,    23,    26,    27,    31,    33,    35,    36,
      37,    49,    50,    51,    52,    53,    54,    55,    56,    64,
      65,    66,    75,    78,    81,    82,    91,    94,    97,   124,
      47,    18,   133,   164,   132,   164,   135,   164,   134,   164,
     143,   164,   145,   164,   147,   164,   126,   164,   127,   164,
     128,   164,   136,   164,   137,   164,   138,   164,   139,   164,
     146,   164,   129,   164,   130,   164,   131,   164,   142,   144,
     164,   140,   164,   141,   164,   148,   151,   164,   149,   156,
     164,   150,   157,   164,    23,   124,     7,   114,     5,     7,
      45,    46,    98,   158,   161,   162,   163,   168,   168,   168,
     168,   168,     3,    98,   158,   159,   169,   168,     4,    98,
     158,   159,   160,   161,   162,   163,   166,   168,   168,   168,
     168,   168,   168,   169,   168,   168,   168,   168,   158,   171,
     168,   153,    98,   158,   159,   160,   161,   162,   163,   167,
     153,   171,   153,   167,    23,    24,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   166,   166,   166,   166,   166,
     166,   164,   164,   164,   164,   164,   164,   164,   164,   164,
      80,   164,    26,    27,    31,    33,    35,    36,    37,    47,
      49,    50,    51,    52,    53,    54,    55,    56,    64,    65,
      66,    75,    78,    81,    82,    94,    97,   125,   154,   164,
     164,    47,   125,   155,    80,   155,   164,   115,    57,    58,
      59,    60,    61,    62,    63,    67,    68,    69,    70,    71,
      72,    73,    74,    76,    77,    79,    83,    84,    85,    86,
      87,    88,    89,    90,   165,   168,   168,    98,   158,   160,
     170,   169,   165,   166,   170,   165,   166,   170,   170,   168,
     169,   168,   168,   168,   166,   164,   168,   133,   132,   135,
     134,   143,   145,   147,   124,   126,   127,   128,   136,   137,
     138,   139,   146,   129,   130,   131,   142,   144,   140,   141,
     149,   150,    93,   152,   165,   100,   101,   102,   103,   104,
     105,   172,   124,   164,   172,     6,    13,    14,    15,    16,
      17,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    37,   117,   168,   168,   168,   168,   168,   168,   168,
     168,   168,   168,   168,   170,   170,   168,   170,   170,   168,
     168,   168,   170,   170,   170,    99,   164,   164,   164,    99,
      99,   164,   166,    23,   153,    99,   164,    23,   167,   164,
       4,     4,     4,     4,     7,     3,     7,     7,     7,     7,
       7,     4,     4,     3,   116,     4,    22,    23,   118,   168,
     168,   168,   168,   168,   168,   168,   170,   168,   168,   168,
     165,   166,   168,   155,   167,    95,   167,     7,     7,    38,
      39,    40,    41,    42,    43,   119,   168,   168,   168,    99,
     164,    92,   164,    96,    44,   168,   167,   120,   164,    96,
       7,    23,    45,    46,   121,   122,   166,    48,    48,    47,
     123,   123,   124,    23
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

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
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
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
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
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
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

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
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
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

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
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
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
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

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
#line 382 "script.y"
    { InitVarGlob(); }
    break;

  case 5:
#line 388 "script.y"
    {
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
	FGettextSetLocalePath(yyvsp[0].str);
}
    break;

  case 6:
#line 393 "script.y"
    {
	fprintf(stderr,"UseGettext!\n");
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
}
    break;

  case 7:
#line 399 "script.y"
    {
	/* Titre de la fenetre */
	scriptprop->titlewin=yyvsp[0].str;
}
    break;

  case 8:
#line 404 "script.y"
    {
	/* Titre de la fenetre */
	scriptprop->titlewin=(char *)FGettext(yyvsp[0].str);
}
    break;

  case 9:
#line 409 "script.y"
    {
	scriptprop->icon=yyvsp[0].str;
}
    break;

  case 10:
#line 413 "script.y"
    {
	/* Position et taille de la fenetre */
	scriptprop->x=yyvsp[-1].number;
	scriptprop->y=yyvsp[0].number;
}
    break;

  case 11:
#line 419 "script.y"
    {
	/* Position et taille de la fenetre */
	scriptprop->width=yyvsp[-1].number;
	scriptprop->height=yyvsp[0].number;
}
    break;

  case 12:
#line 425 "script.y"
    {
	/* Couleur de fond */
	scriptprop->backcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;

  case 13:
#line 431 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->forecolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;

  case 14:
#line 437 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->shadcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;

  case 15:
#line 443 "script.y"
    {
	/* Couleur des lignes */
	scriptprop->hilicolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;

  case 16:
#line 449 "script.y"
    {
	scriptprop->colorset = yyvsp[0].number;
	AllocColorset(yyvsp[0].number);
}
    break;

  case 17:
#line 454 "script.y"
    {
	scriptprop->font=yyvsp[0].str;
}
    break;

  case 19:
#line 460 "script.y"
    {
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 21:
#line 466 "script.y"
    {
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 23:
#line 472 "script.y"
    {
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;

  case 26:
#line 485 "script.y"
    { nbobj++;
				  if (nbobj>1000)
				  { yyerror("Too many items\n");
				    exit(1);}
				  if ((yyvsp[0].number<1)||(yyvsp[0].number>1000))
				  { yyerror("Choose item id between 1 and 1000\n");
				    exit(1);}
				  if (TabIdObj[yyvsp[0].number]!=-1)
				  { i=yyvsp[0].number; fprintf(stderr,"Line %d: item id %d already used:\n",numligne,yyvsp[0].number);
				    exit(1);}
			          TabIdObj[yyvsp[0].number]=nbobj;
				  (*tabobj)[nbobj].id=yyvsp[0].number;
				  (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 28:
#line 502 "script.y"
    {
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				}
    break;

  case 29:
#line 506 "script.y"
    {
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				}
    break;

  case 30:
#line 510 "script.y"
    {
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				}
    break;

  case 31:
#line 515 "script.y"
    {
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				}
    break;

  case 32:
#line 518 "script.y"
    {
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				}
    break;

  case 33:
#line 521 "script.y"
    {
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				}
    break;

  case 34:
#line 524 "script.y"
    {
				 (*tabobj)[nbobj].title= yyvsp[0].str;
				}
    break;

  case 35:
#line 527 "script.y"
    {
				 (*tabobj)[nbobj].title= FGettextCopy(yyvsp[0].str);
				}
    break;

  case 36:
#line 530 "script.y"
    {
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				}
    break;

  case 37:
#line 533 "script.y"
    {
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				}
    break;

  case 38:
#line 536 "script.y"
    {
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 39:
#line 540 "script.y"
    {
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 40:
#line 544 "script.y"
    {
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 41:
#line 548 "script.y"
    {
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;

  case 42:
#line 552 "script.y"
    {
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				}
    break;

  case 43:
#line 556 "script.y"
    {
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				}
    break;

  case 46:
#line 562 "script.y"
    {
				 (*tabobj)[nbobj].flags[0]=True;
				}
    break;

  case 47:
#line 565 "script.y"
    {
				 (*tabobj)[nbobj].flags[1]=True;
				}
    break;

  case 48:
#line 568 "script.y"
    {
				 (*tabobj)[nbobj].flags[2]=True;
				}
    break;

  case 49:
#line 571 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				}
    break;

  case 50:
#line 574 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				}
    break;

  case 51:
#line 577 "script.y"
    {
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
				}
    break;

  case 52:
#line 583 "script.y"
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
#line 594 "script.y"
    { InitObjTabCase(0); }
    break;

  case 55:
#line 598 "script.y"
    { InitObjTabCase(1); }
    break;

  case 59:
#line 605 "script.y"
    { InitCase(-1); }
    break;

  case 60:
#line 606 "script.y"
    { InitCase(-2); }
    break;

  case 61:
#line 609 "script.y"
    { InitCase(yyvsp[0].number); }
    break;

  case 113:
#line 672 "script.y"
    { AddCom(1,1); }
    break;

  case 114:
#line 674 "script.y"
    { AddCom(2,1);}
    break;

  case 115:
#line 676 "script.y"
    { AddCom(3,1);}
    break;

  case 116:
#line 678 "script.y"
    { AddCom(4,2);}
    break;

  case 117:
#line 680 "script.y"
    { AddCom(21,2);}
    break;

  case 118:
#line 682 "script.y"
    { AddCom(22,2);}
    break;

  case 119:
#line 684 "script.y"
    { AddCom(5,3);}
    break;

  case 120:
#line 686 "script.y"
    { AddCom(6,3);}
    break;

  case 121:
#line 688 "script.y"
    { AddCom(7,2);}
    break;

  case 122:
#line 690 "script.y"
    { AddCom(8,2);}
    break;

  case 123:
#line 692 "script.y"
    { AddCom(9,2);}
    break;

  case 124:
#line 694 "script.y"
    { AddCom(10,2);}
    break;

  case 125:
#line 696 "script.y"
    { AddCom(19,2);}
    break;

  case 126:
#line 698 "script.y"
    { AddCom(24,2);}
    break;

  case 127:
#line 700 "script.y"
    { AddCom(11,2);}
    break;

  case 128:
#line 702 "script.y"
    { AddCom(12,2);}
    break;

  case 129:
#line 704 "script.y"
    { AddCom(13,0);}
    break;

  case 130:
#line 706 "script.y"
    { AddCom(17,1);}
    break;

  case 131:
#line 708 "script.y"
    { AddCom(23,2);}
    break;

  case 132:
#line 710 "script.y"
    { AddCom(18,2);}
    break;

  case 133:
#line 712 "script.y"
    { AddCom(25,5);}
    break;

  case 134:
#line 714 "script.y"
    { AddCom(26,2);}
    break;

  case 138:
#line 724 "script.y"
    { AddComBloc(14,3,2); }
    break;

  case 141:
#line 729 "script.y"
    { EmpilerBloc(); }
    break;

  case 142:
#line 731 "script.y"
    { DepilerBloc(2); }
    break;

  case 143:
#line 732 "script.y"
    { DepilerBloc(2); }
    break;

  case 144:
#line 735 "script.y"
    { DepilerBloc(1); }
    break;

  case 145:
#line 736 "script.y"
    { DepilerBloc(1); }
    break;

  case 146:
#line 740 "script.y"
    { AddComBloc(15,3,1); }
    break;

  case 147:
#line 744 "script.y"
    { AddComBloc(16,3,1); }
    break;

  case 148:
#line 749 "script.y"
    { AddVar(yyvsp[0].str); }
    break;

  case 149:
#line 751 "script.y"
    { AddConstStr(yyvsp[0].str); }
    break;

  case 150:
#line 753 "script.y"
    { AddConstStr(yyvsp[0].str); }
    break;

  case 151:
#line 755 "script.y"
    { AddConstNum(yyvsp[0].number); }
    break;

  case 152:
#line 757 "script.y"
    { AddConstNum(-1); }
    break;

  case 153:
#line 759 "script.y"
    { AddConstNum(-2); }
    break;

  case 154:
#line 761 "script.y"
    { AddLevelBufArg(); }
    break;

  case 155:
#line 763 "script.y"
    { AddFunct(1,1); }
    break;

  case 156:
#line 764 "script.y"
    { AddFunct(2,1); }
    break;

  case 157:
#line 765 "script.y"
    { AddFunct(3,1); }
    break;

  case 158:
#line 766 "script.y"
    { AddFunct(4,1); }
    break;

  case 159:
#line 767 "script.y"
    { AddFunct(5,1); }
    break;

  case 160:
#line 768 "script.y"
    { AddFunct(6,1); }
    break;

  case 161:
#line 769 "script.y"
    { AddFunct(7,1); }
    break;

  case 162:
#line 770 "script.y"
    { AddFunct(8,1); }
    break;

  case 163:
#line 771 "script.y"
    { AddFunct(9,1); }
    break;

  case 164:
#line 772 "script.y"
    { AddFunct(10,1); }
    break;

  case 165:
#line 773 "script.y"
    { AddFunct(11,1); }
    break;

  case 166:
#line 774 "script.y"
    { AddFunct(12,1); }
    break;

  case 167:
#line 775 "script.y"
    { AddFunct(13,1); }
    break;

  case 168:
#line 776 "script.y"
    { AddFunct(14,1); }
    break;

  case 169:
#line 777 "script.y"
    { AddFunct(15,1); }
    break;

  case 170:
#line 778 "script.y"
    { AddFunct(16,1); }
    break;

  case 171:
#line 779 "script.y"
    { AddFunct(17,1); }
    break;

  case 172:
#line 780 "script.y"
    { AddFunct(18,1); }
    break;

  case 173:
#line 781 "script.y"
    { AddFunct(19,1); }
    break;

  case 174:
#line 782 "script.y"
    { AddFunct(20,1); }
    break;

  case 175:
#line 783 "script.y"
    { AddFunct(21,1); }
    break;

  case 176:
#line 784 "script.y"
    { AddFunct(22,1); }
    break;

  case 177:
#line 785 "script.y"
    { AddFunct(23,1); }
    break;

  case 178:
#line 786 "script.y"
    { AddFunct(24,1); }
    break;

  case 179:
#line 787 "script.y"
    { AddFunct(25,1); }
    break;

  case 180:
#line 788 "script.y"
    { AddFunct(26,1); }
    break;

  case 181:
#line 793 "script.y"
    { }
    break;

  case 208:
#line 838 "script.y"
    { l=1-250000; AddBufArg(&l,1); }
    break;

  case 209:
#line 839 "script.y"
    { l=2-250000; AddBufArg(&l,1); }
    break;

  case 210:
#line 840 "script.y"
    { l=3-250000; AddBufArg(&l,1); }
    break;

  case 211:
#line 841 "script.y"
    { l=4-250000; AddBufArg(&l,1); }
    break;

  case 212:
#line 842 "script.y"
    { l=5-250000; AddBufArg(&l,1); }
    break;

  case 213:
#line 843 "script.y"
    { l=6-250000; AddBufArg(&l,1); }
    break;


    }

/* Line 1010 of yacc.c.  */
#line 2809 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


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
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 846 "script.y"


