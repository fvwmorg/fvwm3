/* -*-c-*- */
/* A Bison parser, made from script.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	STR	257
# define	GSTR	258
# define	VAR	259
# define	FONT	260
# define	NUMBER	261
# define	WINDOWTITLE	262
# define	WINDOWLOCALETITLE	263
# define	WINDOWSIZE	264
# define	WINDOWPOSITION	265
# define	USEGETTEXT	266
# define	FORECOLOR	267
# define	BACKCOLOR	268
# define	SHADCOLOR	269
# define	LICOLOR	270
# define	COLORSET	271
# define	OBJECT	272
# define	INIT	273
# define	PERIODICTASK	274
# define	QUITFUNC	275
# define	MAIN	276
# define	END	277
# define	PROP	278
# define	TYPE	279
# define	SIZE	280
# define	POSITION	281
# define	VALUE	282
# define	VALUEMIN	283
# define	VALUEMAX	284
# define	TITLE	285
# define	SWALLOWEXEC	286
# define	ICON	287
# define	FLAGS	288
# define	WARP	289
# define	WRITETOFILE	290
# define	LOCALETITLE	291
# define	HIDDEN	292
# define	NOFOCUS	293
# define	NORELIEFSTRING	294
# define	CENTER	295
# define	LEFT	296
# define	RIGHT	297
# define	CASE	298
# define	SINGLECLIC	299
# define	DOUBLECLIC	300
# define	BEG	301
# define	POINT	302
# define	EXEC	303
# define	HIDE	304
# define	SHOW	305
# define	CHFONT	306
# define	CHFORECOLOR	307
# define	CHBACKCOLOR	308
# define	CHCOLORSET	309
# define	KEY	310
# define	GETVALUE	311
# define	GETMINVALUE	312
# define	GETMAXVALUE	313
# define	GETFORE	314
# define	GETBACK	315
# define	GETHILIGHT	316
# define	GETSHADOW	317
# define	CHVALUE	318
# define	CHVALUEMAX	319
# define	CHVALUEMIN	320
# define	ADD	321
# define	DIV	322
# define	MULT	323
# define	GETTITLE	324
# define	GETOUTPUT	325
# define	STRCOPY	326
# define	NUMTOHEX	327
# define	HEXTONUM	328
# define	QUIT	329
# define	LAUNCHSCRIPT	330
# define	GETSCRIPTFATHER	331
# define	SENDTOSCRIPT	332
# define	RECEIVFROMSCRIPT	333
# define	GET	334
# define	SET	335
# define	SENDSIGN	336
# define	REMAINDEROFDIV	337
# define	GETTIME	338
# define	GETSCRIPTARG	339
# define	GETPID	340
# define	SENDMSGANDGET	341
# define	PARSE	342
# define	LASTSTRING	343
# define	GETTEXT	344
# define	IF	345
# define	THEN	346
# define	ELSE	347
# define	FOR	348
# define	TO	349
# define	DO	350
# define	WHILE	351
# define	BEGF	352
# define	ENDF	353
# define	EQUAL	354
# define	INFEQ	355
# define	SUPEQ	356
# define	INF	357
# define	SUP	358
# define	DIFF	359

#line 1 "script.y"

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



#line 353 "script.y"
#ifndef YYSTYPE
typedef union {  char *str;
          int number;
       } yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		464
#define	YYFLAG		-32768
#define	YYNTBASE	106

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 359 ? yytranslate[x] : 172)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
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
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     7,     8,     9,    13,    16,    20,    24,    28,
      33,    38,    42,    46,    50,    54,    58,    61,    62,    68,
      69,    75,    76,    82,    83,    91,    93,    94,    98,   103,
     108,   112,   116,   120,   124,   128,   132,   136,   140,   144,
     148,   152,   156,   159,   163,   164,   167,   170,   173,   176,
     179,   182,   183,   185,   191,   192,   193,   198,   203,   205,
     207,   209,   213,   214,   218,   222,   226,   230,   234,   238,
     242,   246,   250,   254,   258,   262,   266,   270,   274,   278,
     282,   286,   290,   294,   298,   302,   306,   310,   314,   317,
     320,   323,   326,   329,   332,   335,   338,   341,   344,   347,
     350,   353,   356,   359,   362,   365,   368,   371,   374,   377,
     380,   383,   386,   389,   392,   395,   400,   405,   410,   417,
     424,   429,   434,   439,   444,   449,   454,   460,   465,   466,
     469,   474,   479,   490,   495,   500,   504,   508,   516,   517,
     521,   522,   526,   528,   532,   534,   544,   552,   554,   556,
     558,   560,   562,   564,   565,   568,   571,   576,   580,   583,
     587,   591,   595,   600,   603,   605,   608,   612,   614,   617,
     620,   623,   626,   629,   632,   635,   637,   642,   646,   648,
     651,   652,   655,   658,   661,   664,   667,   670,   676,   678,
     680,   682,   684,   686,   688,   693,   695,   697,   699,   701,
     706,   708,   710,   715,   717,   719,   724,   726,   728,   730,
     732,   734,   736
};
static const short yyrhs[] =
{
     107,   108,   109,   110,   111,   112,     0,     0,     0,   108,
      12,     4,     0,   108,    12,     0,   108,     8,     4,     0,
     108,     9,     4,     0,   108,    33,     3,     0,   108,    11,
       7,     7,     0,   108,    10,     7,     7,     0,   108,    14,
       4,     0,   108,    13,     4,     0,   108,    15,     4,     0,
     108,    16,     4,     0,   108,    17,     7,     0,   108,     6,
       0,     0,    19,   152,    47,   123,    23,     0,     0,    20,
     152,    47,   123,    23,     0,     0,    21,   152,    47,   123,
      23,     0,     0,   112,    18,   113,    24,   114,   116,   117,
       0,     7,     0,     0,   114,    25,     3,     0,   114,    26,
       7,     7,     0,   114,    27,     7,     7,     0,   114,    28,
       7,     0,   114,    29,     7,     0,   114,    30,     7,     0,
     114,    31,     4,     0,   114,    37,     4,     0,   114,    32,
       4,     0,   114,    33,     3,     0,   114,    14,     4,     0,
     114,    13,     4,     0,   114,    15,     4,     0,   114,    16,
       4,     0,   114,    17,     7,     0,   114,     6,     0,   114,
      34,   115,     0,     0,   115,    38,     0,   115,    40,     0,
     115,    39,     0,   115,    41,     0,   115,    42,     0,   115,
      43,     0,     0,    23,     0,    22,   118,    44,   119,    23,
       0,     0,     0,   119,   120,    48,   122,     0,   119,   121,
      48,   122,     0,    45,     0,    46,     0,     7,     0,    47,
     123,    23,     0,     0,   123,    49,   125,     0,   123,    35,
     142,     0,   123,    36,   144,     0,   123,    50,   126,     0,
     123,    51,   127,     0,   123,    64,   128,     0,   123,    65,
     129,     0,   123,    66,   130,     0,   123,    27,   131,     0,
     123,    26,   132,     0,   123,    31,   134,     0,   123,    37,
     146,     0,   123,    33,   133,     0,   123,    52,   135,     0,
     123,    53,   136,     0,   123,    54,   137,     0,   123,    55,
     138,     0,   123,    81,   139,     0,   123,    82,   140,     0,
     123,    75,   141,     0,   123,    78,   143,     0,   123,    91,
     147,     0,   123,    94,   148,     0,   123,    97,   149,     0,
     123,    56,   145,     0,    49,   125,     0,    35,   142,     0,
      36,   144,     0,    50,   126,     0,    51,   127,     0,    64,
     128,     0,    65,   129,     0,    66,   130,     0,    27,   131,
       0,    26,   132,     0,    31,   134,     0,    37,   146,     0,
      33,   133,     0,    52,   135,     0,    53,   136,     0,    54,
     137,     0,    55,   138,     0,    81,   139,     0,    82,   140,
       0,    75,   141,     0,    78,   143,     0,    94,   148,     0,
      97,   149,     0,    56,   145,     0,   163,   165,     0,   163,
     167,     0,   163,   167,     0,   163,   167,   163,   167,     0,
     163,   167,   163,   167,     0,   163,   167,   163,   167,     0,
     163,   167,   163,   167,   163,   167,     0,   163,   167,   163,
     167,   163,   167,     0,   163,   167,   163,   168,     0,   163,
     167,   163,   169,     0,   163,   167,   163,   165,     0,   163,
     167,   163,   169,     0,   163,   167,   163,   169,     0,   163,
     167,   163,   167,     0,   163,   170,    80,   163,   165,     0,
     163,   167,   163,   167,     0,     0,   163,   167,     0,   163,
     167,   163,   165,     0,   163,   168,   163,   165,     0,   163,
     168,   163,   168,   163,   167,   163,   167,   163,   165,     0,
     163,   167,   163,   169,     0,   150,   152,   153,   151,     0,
     155,   152,   154,     0,   156,   152,   154,     0,   163,   166,
     163,   171,   163,   166,    92,     0,     0,    93,   152,   154,
       0,     0,    47,   123,    23,     0,   124,     0,    47,   123,
      23,     0,   124,     0,   163,   170,    80,   163,   166,    95,
     163,   166,    96,     0,   163,   166,   163,   171,   163,   166,
      96,     0,     5,     0,     3,     0,     4,     0,     7,     0,
      45,     0,    46,     0,     0,    57,   167,     0,    70,   167,
       0,    71,   169,   167,   167,     0,    73,   167,   167,     0,
      74,   169,     0,    67,   167,   167,     0,    69,   167,   167,
       0,    68,   167,   167,     0,    72,   169,   167,   167,     0,
      76,   169,     0,    77,     0,    79,   167,     0,    83,   167,
     167,     0,    84,     0,    85,   167,     0,    60,   167,     0,
      61,   167,     0,    62,   167,     0,    63,   167,     0,    58,
     167,     0,    59,   167,     0,    86,     0,    87,   169,   169,
     167,     0,    88,   169,   167,     0,    89,     0,    90,   169,
       0,     0,   161,   165,     0,   162,   165,     0,   157,   165,
       0,   159,   165,     0,   158,   165,     0,   160,   165,     0,
      98,   163,   164,    99,   165,     0,   157,     0,   161,     0,
     162,     0,   159,     0,   158,     0,   160,     0,    98,   163,
     164,    99,     0,   161,     0,   162,     0,   160,     0,   157,
       0,    98,   163,   164,    99,     0,   157,     0,   158,     0,
      98,   163,   164,    99,     0,   157,     0,   159,     0,    98,
     163,   164,    99,     0,   157,     0,   103,     0,   101,     0,
     100,     0,   102,     0,   104,     0,   105,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short yyrline[] =
{
       0,   378,   381,   385,   386,   391,   397,   402,   407,   411,
     417,   423,   429,   435,   441,   447,   452,   458,   459,   464,
     465,   470,   471,   480,   481,   484,   500,   501,   505,   509,
     514,   517,   520,   523,   526,   529,   532,   535,   539,   543,
     547,   551,   555,   558,   560,   561,   564,   567,   570,   573,
     576,   582,   593,   594,   597,   599,   600,   601,   604,   605,
     608,   611,   616,   617,   618,   619,   620,   621,   622,   623,
     624,   625,   626,   627,   628,   629,   630,   631,   632,   633,
     634,   635,   636,   637,   638,   639,   640,   641,   645,   646,
     647,   648,   649,   650,   651,   652,   653,   654,   655,   656,
     657,   658,   659,   660,   661,   662,   663,   664,   665,   666,
     667,   668,   671,   673,   675,   677,   679,   681,   683,   685,
     687,   689,   691,   693,   695,   697,   699,   701,   703,   705,
     707,   709,   711,   713,   715,   717,   719,   723,   725,   726,
     728,   730,   731,   734,   735,   739,   743,   748,   750,   752,
     754,   756,   758,   760,   762,   763,   764,   765,   766,   767,
     768,   769,   770,   771,   772,   773,   774,   775,   776,   777,
     778,   779,   780,   781,   782,   783,   784,   785,   786,   787,
     792,   793,   794,   795,   796,   797,   798,   799,   803,   804,
     805,   806,   807,   808,   809,   813,   814,   815,   816,   817,
     821,   822,   823,   827,   828,   829,   833,   837,   838,   839,
     840,   841,   842
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const yytname[] =
{
  "$", "error", "$undefined.", "STR", "GSTR", "VAR", "FONT", "NUMBER",
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
  "WHILE", "BEGF", "ENDF", "EQUAL", "INFEQ", "SUPEQ", "INF", "SUP",
  "DIFF", "script", "initvar", "head", "initbloc", "periodictask",
  "quitfunc", "object", "id", "init", "flags", "verify", "mainloop",
  "addtabcase", "case", "clic", "number", "bloc", "instr", "oneinstr",
  "exec", "hide", "show", "chvalue", "chvaluemax", "chvaluemin",
  "position", "size", "icon", "title", "font", "chforecolor",
  "chbackcolor", "chcolorset", "set", "sendsign", "quit", "warp",
  "sendtoscript", "writetofile", "key", "localetitle", "ifthenelse",
  "loop", "while", "headif", "else", "creerbloc", "bloc1", "bloc2",
  "headloop", "headwhile", "var", "str", "gstr", "num", "singleclic2",
  "doubleclic2", "addlbuff", "function", "args", "arg", "numarg",
  "strarg", "gstrarg", "vararg", "compare", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short yyr1[] =
{
       0,   106,   107,   108,   108,   108,   108,   108,   108,   108,
     108,   108,   108,   108,   108,   108,   108,   109,   109,   110,
     110,   111,   111,   112,   112,   113,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   115,   115,   115,   115,   115,   115,
     115,   116,   117,   117,   118,   119,   119,   119,   120,   120,
     121,   122,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   139,   140,   141,   142,
     143,   144,   145,   146,   147,   148,   149,   150,   151,   151,
     152,   153,   153,   154,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     165,   165,   165,   165,   165,   165,   165,   165,   166,   166,
     166,   166,   166,   166,   166,   167,   167,   167,   167,   167,
     168,   168,   168,   169,   169,   169,   170,   171,   171,   171,
     171,   171,   171
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short yyr2[] =
{
       0,     6,     0,     0,     3,     2,     3,     3,     3,     4,
       4,     3,     3,     3,     3,     3,     2,     0,     5,     0,
       5,     0,     5,     0,     7,     1,     0,     3,     4,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     0,     2,     2,     2,     2,     2,
       2,     0,     1,     5,     0,     0,     4,     4,     1,     1,
       1,     3,     0,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     4,     4,     4,     6,     6,
       4,     4,     4,     4,     4,     4,     5,     4,     0,     2,
       4,     4,    10,     4,     4,     3,     3,     7,     0,     3,
       0,     3,     1,     3,     1,     9,     7,     1,     1,     1,
       1,     1,     1,     0,     2,     2,     4,     3,     2,     3,
       3,     3,     4,     2,     1,     2,     3,     1,     2,     2,
       2,     2,     2,     2,     2,     1,     4,     3,     1,     2,
       0,     2,     2,     2,     2,     2,     2,     5,     1,     1,
       1,     1,     1,     1,     4,     1,     1,     1,     1,     4,
       1,     1,     4,     1,     1,     4,     1,     1,     1,     1,
       1,     1,     1
};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short yydefact[] =
{
       2,     3,    17,    16,     0,     0,     0,     0,     5,     0,
       0,     0,     0,     0,   140,     0,    19,     6,     7,     0,
       0,     4,    12,    11,    13,    14,    15,     0,     8,   140,
      21,    10,     9,    62,     0,   140,    23,     0,    62,     0,
       1,    18,   153,   153,   153,   153,   153,   153,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   153,   153,   153,
     128,   153,   153,   153,   153,   153,   153,     0,    62,     0,
      72,     0,    71,     0,    73,     0,    75,     0,    64,     0,
      65,     0,    74,     0,    63,   180,    66,     0,    67,     0,
      76,     0,    77,     0,    78,     0,    79,     0,    87,     0,
      68,     0,    69,     0,    70,     0,    82,    83,     0,    80,
       0,    81,     0,    84,   140,     0,    85,   140,     0,    86,
     140,     0,    20,     0,    25,     0,   147,   150,   151,   152,
     153,   198,   197,   195,   196,   153,   153,   153,   153,   129,
     148,   153,   200,   201,   153,   153,   149,   153,   180,   180,
     180,   180,   180,   180,   112,   113,   114,   153,   153,   153,
     153,   153,   153,   153,   153,   153,   206,     0,   153,     0,
     153,   188,   192,   191,   193,   189,   190,   153,     0,     0,
       0,   153,    22,    26,     0,     0,     0,     0,     0,     0,
     180,     0,     0,   183,   185,   184,   186,   181,   182,   180,
       0,     0,     0,     0,     0,     0,     0,   180,   153,     0,
     153,   153,   153,   153,   153,   153,   153,    62,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   153,   153,   128,
     153,   153,   153,   153,   153,   142,   138,     0,     0,    62,
     144,   135,   153,   136,     0,    51,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   164,     0,     0,   167,     0,   175,     0,     0,
     178,     0,     0,   153,   153,   153,   203,   204,   121,   120,
       0,   131,   133,     0,   122,   123,   124,   125,   153,   115,
     116,   117,   130,   180,   127,    97,    96,    98,   100,    89,
      90,    99,     0,    88,    91,    92,   101,   102,   103,   104,
     111,    93,    94,    95,   107,   108,   105,   106,   109,   110,
     140,   134,     0,   209,   208,   210,   207,   211,   212,   153,
       0,     0,   153,    42,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    44,     0,
       0,   154,   173,   174,   169,   170,   171,   172,     0,     0,
       0,   155,     0,     0,     0,   158,   163,   165,     0,   168,
       0,     0,   179,   199,     0,     0,     0,   202,   180,     0,
     126,   141,     0,   194,     0,   143,     0,     0,    38,    37,
      39,    40,    41,    27,     0,     0,    30,    31,    32,    33,
      35,    36,    43,    34,    54,    52,    24,   159,   161,   160,
       0,     0,   157,   166,     0,   177,   119,   118,     0,   187,
     153,   139,     0,   153,     0,    28,    29,    45,    47,    46,
      48,    49,    50,     0,   156,   162,   176,   205,     0,   137,
       0,   146,    55,   153,     0,     0,   180,   145,    60,    53,
      58,    59,     0,     0,   132,     0,     0,    62,    56,    57,
       0,    61,     0,     0,     0
};

static const short yydefgoto[] =
{
     462,     1,     2,    16,    30,    36,    40,   125,   245,   402,
     350,   406,   433,   445,   452,   453,   458,    37,   240,    84,
      86,    88,   100,   102,   104,    72,    70,    76,    74,    90,
      92,    94,    96,   109,   111,   106,    78,   107,    80,    98,
      82,   113,   116,   119,   114,   321,    27,   236,   241,   117,
     120,   131,   149,   150,   132,   133,   134,    71,   272,   154,
     177,   135,   144,   278,   167,   329
};

static const short yypact[] =
{
  -32768,-32768,   567,-32768,     1,     6,    -4,     5,    47,    54,
      56,    58,    60,    59,-32768,    79,    66,-32768,-32768,    80,
      81,-32768,-32768,-32768,-32768,-32768,-32768,    52,-32768,-32768,
      82,-32768,-32768,-32768,    57,-32768,-32768,   610,-32768,    62,
      83,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,   704,-32768,   101,
  -32768,    73,-32768,    73,-32768,    73,-32768,    73,-32768,    73,
  -32768,    -1,-32768,    73,-32768,    24,-32768,    73,-32768,    73,
  -32768,    73,-32768,    73,-32768,    73,-32768,    73,-32768,    -1,
  -32768,    73,-32768,    73,-32768,    73,-32768,-32768,    73,-32768,
     105,-32768,    73,-32768,-32768,    69,-32768,-32768,   105,-32768,
  -32768,    69,-32768,   757,-32768,    87,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    24,    24,
      24,    24,    24,    24,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,    32,-32768,   969,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,  1005,    33,
    1005,-32768,-32768,-32768,  1046,    73,    73,     4,    -1,  1046,
      24,     4,  1046,-32768,-32768,-32768,-32768,-32768,-32768,    24,
       4,     4,    73,    -1,    73,    73,    73,    24,-32768,    73,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,    23,  1046,   -85,-32768,
  -32768,-32768,-32768,-32768,   -85,   683,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,     4,     4,    73,
       4,     4,-32768,    73,    73,-32768,    73,-32768,     4,     4,
  -32768,     4,    18,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
      21,-32768,-32768,    26,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,    24,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,   810,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,    31,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     863,    69,-32768,-32768,   119,   127,   129,   130,   132,   134,
     133,   136,   137,   138,   139,   143,   153,   157,-32768,   158,
      10,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    73,    73,
      73,-32768,    73,    73,    73,-32768,-32768,-32768,    73,-32768,
       4,    73,-32768,-32768,    73,    73,  1046,-32768,    24,    73,
  -32768,-32768,  1005,-32768,    69,-32768,    46,    69,-32768,-32768,
  -32768,-32768,-32768,-32768,   156,   159,-32768,-32768,-32768,-32768,
  -32768,-32768,   -17,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
      73,    73,-32768,-32768,    73,-32768,-32768,-32768,    70,-32768,
  -32768,-32768,    78,-32768,    84,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,   121,-32768,-32768,-32768,-32768,    73,-32768,
      69,-32768,-32768,-32768,    85,    61,    24,-32768,-32768,-32768,
  -32768,-32768,   128,   135,-32768,   131,   131,-32768,-32768,-32768,
     916,-32768,   175,   177,-32768
};

static const short yypgoto[] =
{
  -32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  -32768,-32768,-32768,-32768,-32768,-32768,  -274,   -38,    15,   -33,
     -32,   -34,   -37,   -39,   -28,   -20,   -15,     7,   -16,     8,
      27,     3,    29,    -3,    -5,    16,    34,     0,    17,    30,
      40,-32768,    25,    28,-32768,-32768,   -22,-32768,  -174,-32768,
  -32768,   178,    43,   204,   165,   185,   303,    -9,  -178,   292,
     -27,   -12,   -98,   268,   142,    13
};


#define	YYLAST		1136


static const short yytable[] =
{
      67,   161,   140,    19,   126,    17,   243,    34,   146,   126,
      18,   280,    20,    39,   283,   323,   324,   325,   326,   327,
     328,   427,   428,   429,   430,   431,   432,   140,   146,   126,
     123,   127,   404,   405,    73,    75,    77,    79,    81,    83,
      85,    87,    89,    91,    93,    95,    97,    99,   101,   103,
     105,    21,   108,   110,   112,   115,   118,   121,    22,   322,
      23,   136,    24,   137,    25,   138,    26,   139,   448,   128,
     129,   145,   140,   146,   126,   155,   127,   156,   126,   157,
     127,   158,    28,   159,   449,   160,    29,    31,    32,   162,
     279,   163,   169,   164,   181,   178,   165,   141,   180,    33,
     168,    69,   275,    35,    38,   288,   450,   451,   124,    68,
     126,   183,   208,   242,   128,   129,   320,   373,   128,   129,
     377,   184,   147,   388,   143,   378,   185,   186,   187,   188,
     383,   389,   189,   390,   391,   190,   191,   393,   192,   392,
     394,   423,   143,   395,   396,   397,   398,   399,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   400,   172,   209,
     401,   237,   403,   425,   172,   442,   426,   170,   238,   437,
     439,   130,   244,   273,   274,   463,   455,   464,   457,   302,
     441,   447,   459,   456,   235,   303,   305,   304,   312,   311,
     287,   296,   289,   290,   291,   295,   297,   294,   418,   293,
     313,   330,    73,    75,    77,    79,    81,    83,   421,    85,
      87,    89,    91,    93,    95,    97,    99,   101,   103,   105,
     298,   108,   110,   112,   118,   121,   308,   317,   316,   306,
     315,   143,   300,   331,   351,   352,   353,   354,   355,   356,
     357,   358,   359,   360,   361,   314,   143,   364,   299,   307,
     151,   367,   368,   309,   369,   310,   301,   332,   318,   142,
     179,     0,   319,   148,   374,   375,   376,     0,     0,     0,
     152,     0,     0,     0,     0,     0,     0,   142,     0,   379,
     174,     0,     0,     0,     0,     0,   174,     0,   166,     0,
       0,     0,     0,   171,     0,     0,   166,     0,   382,   171,
     175,     0,     0,     0,   386,     0,   175,     0,     0,     0,
       0,     0,     0,   151,   151,   151,   151,   151,   151,   173,
     384,     0,     0,   387,     0,   173,   148,   148,   148,   148,
     148,   148,     0,   152,   152,   152,   152,   152,   152,     0,
       0,     0,     0,     0,     0,     0,   407,   408,   409,     0,
     410,   411,   412,     0,     0,   151,   413,   422,     0,   415,
     424,     0,   416,   417,   151,   276,   142,   420,   148,   276,
       0,     0,   151,     0,   172,   152,     0,   148,   276,   276,
       0,   142,     0,     0,   152,   148,     0,     0,   153,     0,
       0,   277,   152,     0,     0,   277,     0,     0,   434,   435,
       0,     0,   436,     0,   277,   277,     0,     0,     0,     0,
       0,   438,     0,   444,   440,     0,     0,     0,   176,   460,
       0,     0,     0,     0,   176,     0,   443,   172,     0,     0,
     172,     0,     0,     0,   446,   276,   276,     0,   276,   276,
     193,   194,   195,   196,   197,   198,   276,   276,     0,   276,
       0,   153,   153,   153,   153,   153,   153,     0,   151,   282,
       0,   277,   277,     0,   277,   277,     0,     0,   285,   286,
       0,   148,   277,   277,     0,   277,     0,     0,   152,     0,
       0,     0,   281,   172,     0,     0,     0,     0,     0,     0,
       0,   284,     0,   153,     0,     0,   174,     0,     0,   292,
       0,     0,   153,     0,     0,     0,     0,     0,     0,   171,
     153,     0,     0,     0,     0,     0,   175,     0,     0,     0,
       0,     0,     0,     0,     0,   362,   363,     0,   365,   366,
       0,     0,     0,     0,     0,   173,   370,   371,     0,   372,
       0,     0,     0,   151,     0,     0,     0,     0,   276,   174,
       0,     0,   174,     0,     0,     0,   148,     0,     0,     0,
       0,     0,   171,   152,     0,   171,     0,     0,     0,   175,
       0,     0,   175,     3,   277,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,   380,    14,     0,   173,     0,
       0,   173,     0,     0,     0,     0,   153,     0,     0,     0,
      15,     0,     0,     0,     0,   174,     0,     0,     0,     0,
       0,   151,     0,     0,     0,     0,     0,     0,   171,     0,
       0,     0,     0,     0,   148,   175,     0,     0,     0,     0,
       0,   152,     0,    41,   176,     0,    42,    43,   414,     0,
       0,    44,     0,    45,   173,    46,    47,    48,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,     0,     0,     0,
     419,     0,     0,     0,    57,    58,    59,     0,     0,     0,
       0,   153,     0,     0,     0,    60,     0,   176,    61,   333,
     176,    62,    63,     0,     0,     0,   334,   335,   336,   337,
     338,    64,     0,     0,    65,     0,     0,    66,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,     0,     0,
     349,     0,     0,     0,     0,     0,     0,   122,     0,     0,
      42,    43,     0,     0,     0,    44,     0,    45,   454,    46,
      47,    48,     0,   176,     0,     0,     0,     0,     0,   153,
       0,     0,     0,    49,    50,    51,    52,    53,    54,    55,
      56,     0,     0,     0,     0,     0,     0,     0,    57,    58,
      59,     0,     0,     0,     0,     0,     0,     0,     0,    60,
     182,     0,    61,    42,    43,    62,    63,     0,    44,     0,
      45,     0,    46,    47,    48,    64,     0,     0,    65,     0,
       0,    66,     0,     0,     0,     0,    49,    50,    51,    52,
      53,    54,    55,    56,     0,     0,     0,     0,     0,     0,
       0,    57,    58,    59,     0,     0,     0,     0,     0,     0,
       0,     0,    60,   381,     0,    61,    42,    43,    62,    63,
       0,    44,     0,    45,     0,    46,    47,    48,    64,     0,
       0,    65,     0,     0,    66,     0,     0,     0,     0,    49,
      50,    51,    52,    53,    54,    55,    56,     0,     0,     0,
       0,     0,     0,     0,    57,    58,    59,     0,     0,     0,
       0,     0,     0,     0,     0,    60,   385,     0,    61,    42,
      43,    62,    63,     0,    44,     0,    45,     0,    46,    47,
      48,    64,     0,     0,    65,     0,     0,    66,     0,     0,
       0,     0,    49,    50,    51,    52,    53,    54,    55,    56,
       0,     0,     0,     0,     0,     0,     0,    57,    58,    59,
       0,     0,     0,     0,     0,     0,     0,     0,    60,   461,
       0,    61,    42,    43,    62,    63,     0,    44,     0,    45,
       0,    46,    47,    48,    64,     0,     0,    65,     0,     0,
      66,     0,     0,     0,     0,    49,    50,    51,    52,    53,
      54,    55,    56,     0,     0,     0,     0,     0,     0,     0,
      57,    58,    59,     0,     0,     0,     0,     0,     0,     0,
       0,    60,     0,     0,    61,   210,   211,    62,    63,     0,
     212,     0,   213,     0,   214,   215,   216,    64,     0,     0,
      65,     0,     0,    66,     0,     0,   217,     0,   218,   219,
     220,   221,   222,   223,   224,   225,     0,     0,     0,     0,
       0,   210,   211,   226,   227,   228,   212,     0,   213,     0,
     214,   215,   216,     0,   229,     0,     0,   230,     0,     0,
     231,   232,   239,     0,   218,   219,   220,   221,   222,   223,
     224,   225,     0,   233,     0,     0,   234,     0,     0,   226,
     227,   228,     0,     0,     0,     0,     0,     0,     0,     0,
     229,     0,     0,   230,     0,     0,   231,   232,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   233,
       0,     0,   234,   246,   247,   248,   249,   250,   251,   252,
       0,     0,     0,   253,   254,   255,   256,   257,   258,   259,
     260,     0,   261,   262,     0,   263,     0,     0,     0,   264,
     265,   266,   267,   268,   269,   270,   271
};

static const short yycheck[] =
{
      38,    99,     3,     7,     5,     4,   180,    29,     4,     5,
       4,   189,     7,    35,   192,   100,   101,   102,   103,   104,
     105,    38,    39,    40,    41,    42,    43,     3,     4,     5,
      68,     7,    22,    23,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,     4,    61,    62,    63,    64,    65,    66,     4,   237,
       4,    73,     4,    75,     4,    77,     7,    79,     7,    45,
      46,    83,     3,     4,     5,    87,     7,    89,     5,    91,
       7,    93,     3,    95,    23,    97,    20,     7,     7,   101,
     188,   103,   114,   105,   121,   117,   108,    98,   120,    47,
     112,    18,    98,    21,    47,   203,    45,    46,     7,    47,
       5,    24,    80,    80,    45,    46,    93,    99,    45,    46,
      99,   130,    98,     4,    81,    99,   135,   136,   137,   138,
      99,     4,   141,     4,     4,   144,   145,     3,   147,     7,
       7,    95,    99,     7,     7,     7,     7,     4,   157,   158,
     159,   160,   161,   162,   163,   164,   165,     4,   115,   168,
       3,   170,     4,     7,   121,    44,     7,    98,   177,    99,
      92,    98,   181,   185,   186,     0,    48,     0,    47,   217,
      96,    96,   456,    48,   169,   218,   220,   219,   227,   226,
     202,   211,   204,   205,   206,   210,   212,   209,   376,   208,
     228,   239,   211,   212,   213,   214,   215,   216,   382,   218,
     219,   220,   221,   222,   223,   224,   225,   226,   227,   228,
     213,   230,   231,   232,   233,   234,   223,   232,   231,   221,
     230,   188,   215,   242,   246,   247,   248,   249,   250,   251,
     252,   253,   254,   255,   256,   229,   203,   259,   214,   222,
      85,   263,   264,   224,   266,   225,   216,   244,   233,    81,
     118,    -1,   234,    85,   273,   274,   275,    -1,    -1,    -1,
      85,    -1,    -1,    -1,    -1,    -1,    -1,    99,    -1,   288,
     115,    -1,    -1,    -1,    -1,    -1,   121,    -1,   110,    -1,
      -1,    -1,    -1,   115,    -1,    -1,   118,    -1,   320,   121,
     115,    -1,    -1,    -1,   331,    -1,   121,    -1,    -1,    -1,
      -1,    -1,    -1,   148,   149,   150,   151,   152,   153,   115,
     329,    -1,    -1,   332,    -1,   121,   148,   149,   150,   151,
     152,   153,    -1,   148,   149,   150,   151,   152,   153,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   358,   359,   360,    -1,
     362,   363,   364,    -1,    -1,   190,   368,   384,    -1,   371,
     387,    -1,   374,   375,   199,   187,   188,   379,   190,   191,
      -1,    -1,   207,    -1,   331,   190,    -1,   199,   200,   201,
      -1,   203,    -1,    -1,   199,   207,    -1,    -1,    85,    -1,
      -1,   187,   207,    -1,    -1,   191,    -1,    -1,   410,   411,
      -1,    -1,   414,    -1,   200,   201,    -1,    -1,    -1,    -1,
      -1,   420,    -1,   440,   423,    -1,    -1,    -1,   115,   457,
      -1,    -1,    -1,    -1,   121,    -1,   438,   384,    -1,    -1,
     387,    -1,    -1,    -1,   443,   257,   258,    -1,   260,   261,
     148,   149,   150,   151,   152,   153,   268,   269,    -1,   271,
      -1,   148,   149,   150,   151,   152,   153,    -1,   293,   191,
      -1,   257,   258,    -1,   260,   261,    -1,    -1,   200,   201,
      -1,   293,   268,   269,    -1,   271,    -1,    -1,   293,    -1,
      -1,    -1,   190,   440,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   199,    -1,   190,    -1,    -1,   331,    -1,    -1,   207,
      -1,    -1,   199,    -1,    -1,    -1,    -1,    -1,    -1,   331,
     207,    -1,    -1,    -1,    -1,    -1,   331,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   257,   258,    -1,   260,   261,
      -1,    -1,    -1,    -1,    -1,   331,   268,   269,    -1,   271,
      -1,    -1,    -1,   378,    -1,    -1,    -1,    -1,   370,   384,
      -1,    -1,   387,    -1,    -1,    -1,   378,    -1,    -1,    -1,
      -1,    -1,   384,   378,    -1,   387,    -1,    -1,    -1,   384,
      -1,    -1,   387,     6,   370,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,   293,    19,    -1,   384,    -1,
      -1,   387,    -1,    -1,    -1,    -1,   293,    -1,    -1,    -1,
      33,    -1,    -1,    -1,    -1,   440,    -1,    -1,    -1,    -1,
      -1,   446,    -1,    -1,    -1,    -1,    -1,    -1,   440,    -1,
      -1,    -1,    -1,    -1,   446,   440,    -1,    -1,    -1,    -1,
      -1,   446,    -1,    23,   331,    -1,    26,    27,   370,    -1,
      -1,    31,    -1,    33,   440,    35,    36,    37,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    54,    55,    56,    -1,    -1,    -1,
     378,    -1,    -1,    -1,    64,    65,    66,    -1,    -1,    -1,
      -1,   378,    -1,    -1,    -1,    75,    -1,   384,    78,     6,
     387,    81,    82,    -1,    -1,    -1,    13,    14,    15,    16,
      17,    91,    -1,    -1,    94,    -1,    -1,    97,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    -1,    -1,
      37,    -1,    -1,    -1,    -1,    -1,    -1,    23,    -1,    -1,
      26,    27,    -1,    -1,    -1,    31,    -1,    33,   446,    35,
      36,    37,    -1,   440,    -1,    -1,    -1,    -1,    -1,   446,
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
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

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

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if ! defined (yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
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
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || defined (YYERROR_VERBOSE) */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
# if YYLSP_NEEDED
  YYLTYPE yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
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
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;	\
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif


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
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
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
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");			\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)       	\
   Current.last_line   = Rhs[N].last_line;	\
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, &yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval, &yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		yylex (&yylval, YYLEX_PARAM)
#  else
#   define YYLEX		yylex (&yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX			yylex ()
#endif /* !YYPURE */


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
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
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

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

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
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int yyparse (void *);
# else
int yyparse (void);
# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES			\
/* The lookahead symbol.  */				\
int yychar;						\
							\
/* The semantic value of the lookahead symbol. */	\
YYSTYPE yylval;						\
							\
/* Number of parse errors so far.  */			\
int yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
						\
/* Location data for the lookahead symbol.  */	\
YYLTYPE yylloc;
#else
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
yyparse (YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif  /* !YYPURE */

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yychar1 = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
# define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  YYSIZE_T yystacksize = YYINITDEPTH;


  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
#if YYLSP_NEEDED
  YYLTYPE yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
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
#if YYLSP_NEEDED
  yylsp = yyls;
#endif
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

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
	YYLTYPE *yyls1 = yyls;
	/* This used to be a conditional around just the two extra args,
	   but that might be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
# else
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);
# endif
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
# if YYLSP_NEEDED
	YYSTACK_RELOCATE (yyls);
# endif
# undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
#if YYLSP_NEEDED
      yylsp = yyls + yysize - 1;
#endif

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long) yystacksize));

      if (yyssp >= yyss + yystacksize - 1)
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
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yychar1 = YYTRANSLATE (yychar);

#if YYDEBUG
     /* We have to keep this `#if YYDEBUG', since we use variables
	which are defined only if `YYDEBUG' is set.  */
      if (yydebug)
	{
	  YYFPRINTF (stderr, "Next token is %d (%s",
		     yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise
	     meaning of a token, for further debugging info.  */
# ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
# endif
	  YYFPRINTF (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %d (%s), ",
	      yychar, yytname[yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  yyloc = yylsp[1-yylen];
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (yydebug)
    {
      int yyi;

      YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
		 yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (yyi = yyprhs[yyn]; yyrhs[yyi] > 0; yyi++)
	YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
      YYFPRINTF (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif

  switch (yyn) {

case 2:
#line 381 "script.y"
{ InitVarGlob(); }
    break;
case 4:
#line 387 "script.y"
{
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
	FGettextSetLocalePath(yyvsp[0].str);
}
    break;
case 5:
#line 392 "script.y"
{
	fprintf(stderr,"UseGettext!\n");
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
}
    break;
case 6:
#line 398 "script.y"
{
	/* Titre de la fenetre */
	scriptprop->titlewin=yyvsp[0].str;
}
    break;
case 7:
#line 403 "script.y"
{
	/* Titre de la fenetre */
	scriptprop->titlewin=(char *)FGettext(yyvsp[0].str);
}
    break;
case 8:
#line 408 "script.y"
{
	scriptprop->icon=yyvsp[0].str;
}
    break;
case 9:
#line 412 "script.y"
{
	/* Position et taille de la fenetre */
	scriptprop->x=yyvsp[-1].number;
	scriptprop->y=yyvsp[0].number;
}
    break;
case 10:
#line 418 "script.y"
{
	/* Position et taille de la fenetre */
	scriptprop->width=yyvsp[-1].number;
	scriptprop->height=yyvsp[0].number;
}
    break;
case 11:
#line 424 "script.y"
{
	/* Couleur de fond */
	scriptprop->backcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;
case 12:
#line 430 "script.y"
{
	/* Couleur des lignes */
	scriptprop->forecolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;
case 13:
#line 436 "script.y"
{
	/* Couleur des lignes */
	scriptprop->shadcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;
case 14:
#line 442 "script.y"
{
	/* Couleur des lignes */
	scriptprop->hilicolor=yyvsp[0].str;
	scriptprop->colorset = -1;
}
    break;
case 15:
#line 448 "script.y"
{
	scriptprop->colorset = yyvsp[0].number;
	AllocColorset(yyvsp[0].number);
}
    break;
case 16:
#line 453 "script.y"
{
	scriptprop->font=yyvsp[0].str;
}
    break;
case 18:
#line 459 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;
case 20:
#line 465 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;
case 22:
#line 471 "script.y"
{
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--;
				}
    break;
case 25:
#line 484 "script.y"
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
case 27:
#line 501 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				}
    break;
case 28:
#line 505 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				}
    break;
case 29:
#line 509 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				}
    break;
case 30:
#line 514 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				}
    break;
case 31:
#line 517 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				}
    break;
case 32:
#line 520 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				}
    break;
case 33:
#line 523 "script.y"
{
				 (*tabobj)[nbobj].title= yyvsp[0].str;
				}
    break;
case 34:
#line 526 "script.y"
{
				 (*tabobj)[nbobj].title= FGettextCopy(yyvsp[0].str);
				}
    break;
case 35:
#line 529 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				}
    break;
case 36:
#line 532 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				}
    break;
case 37:
#line 535 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;
case 38:
#line 539 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;
case 39:
#line 543 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;
case 40:
#line 547 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
    break;
case 41:
#line 551 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				}
    break;
case 42:
#line 555 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				}
    break;
case 45:
#line 561 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				}
    break;
case 46:
#line 564 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				}
    break;
case 47:
#line 567 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				}
    break;
case 48:
#line 570 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				}
    break;
case 49:
#line 573 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				}
    break;
case 50:
#line 576 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
				}
    break;
case 51:
#line 582 "script.y"
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
case 52:
#line 593 "script.y"
{ InitObjTabCase(0); }
    break;
case 54:
#line 597 "script.y"
{ InitObjTabCase(1); }
    break;
case 58:
#line 604 "script.y"
{ InitCase(-1); }
    break;
case 59:
#line 605 "script.y"
{ InitCase(-2); }
    break;
case 60:
#line 608 "script.y"
{ InitCase(yyvsp[0].number); }
    break;
case 112:
#line 671 "script.y"
{ AddCom(1,1); }
    break;
case 113:
#line 673 "script.y"
{ AddCom(2,1);}
    break;
case 114:
#line 675 "script.y"
{ AddCom(3,1);}
    break;
case 115:
#line 677 "script.y"
{ AddCom(4,2);}
    break;
case 116:
#line 679 "script.y"
{ AddCom(21,2);}
    break;
case 117:
#line 681 "script.y"
{ AddCom(22,2);}
    break;
case 118:
#line 683 "script.y"
{ AddCom(5,3);}
    break;
case 119:
#line 685 "script.y"
{ AddCom(6,3);}
    break;
case 120:
#line 687 "script.y"
{ AddCom(7,2);}
    break;
case 121:
#line 689 "script.y"
{ AddCom(8,2);}
    break;
case 122:
#line 691 "script.y"
{ AddCom(9,2);}
    break;
case 123:
#line 693 "script.y"
{ AddCom(10,2);}
    break;
case 124:
#line 695 "script.y"
{ AddCom(19,2);}
    break;
case 125:
#line 697 "script.y"
{ AddCom(24,2);}
    break;
case 126:
#line 699 "script.y"
{ AddCom(11,2);}
    break;
case 127:
#line 701 "script.y"
{ AddCom(12,2);}
    break;
case 128:
#line 703 "script.y"
{ AddCom(13,0);}
    break;
case 129:
#line 705 "script.y"
{ AddCom(17,1);}
    break;
case 130:
#line 707 "script.y"
{ AddCom(23,2);}
    break;
case 131:
#line 709 "script.y"
{ AddCom(18,2);}
    break;
case 132:
#line 711 "script.y"
{ AddCom(25,5);}
    break;
case 133:
#line 713 "script.y"
{ AddCom(26,2);}
    break;
case 137:
#line 723 "script.y"
{ AddComBloc(14,3,2); }
    break;
case 140:
#line 728 "script.y"
{ EmpilerBloc(); }
    break;
case 141:
#line 730 "script.y"
{ DepilerBloc(2); }
    break;
case 142:
#line 731 "script.y"
{ DepilerBloc(2); }
    break;
case 143:
#line 734 "script.y"
{ DepilerBloc(1); }
    break;
case 144:
#line 735 "script.y"
{ DepilerBloc(1); }
    break;
case 145:
#line 739 "script.y"
{ AddComBloc(15,3,1); }
    break;
case 146:
#line 743 "script.y"
{ AddComBloc(16,3,1); }
    break;
case 147:
#line 748 "script.y"
{ AddVar(yyvsp[0].str); }
    break;
case 148:
#line 750 "script.y"
{ AddConstStr(yyvsp[0].str); }
    break;
case 149:
#line 752 "script.y"
{ AddConstStr(yyvsp[0].str); }
    break;
case 150:
#line 754 "script.y"
{ AddConstNum(yyvsp[0].number); }
    break;
case 151:
#line 756 "script.y"
{ AddConstNum(-1); }
    break;
case 152:
#line 758 "script.y"
{ AddConstNum(-2); }
    break;
case 153:
#line 760 "script.y"
{ AddLevelBufArg(); }
    break;
case 154:
#line 762 "script.y"
{ AddFunct(1,1); }
    break;
case 155:
#line 763 "script.y"
{ AddFunct(2,1); }
    break;
case 156:
#line 764 "script.y"
{ AddFunct(3,1); }
    break;
case 157:
#line 765 "script.y"
{ AddFunct(4,1); }
    break;
case 158:
#line 766 "script.y"
{ AddFunct(5,1); }
    break;
case 159:
#line 767 "script.y"
{ AddFunct(6,1); }
    break;
case 160:
#line 768 "script.y"
{ AddFunct(7,1); }
    break;
case 161:
#line 769 "script.y"
{ AddFunct(8,1); }
    break;
case 162:
#line 770 "script.y"
{ AddFunct(9,1); }
    break;
case 163:
#line 771 "script.y"
{ AddFunct(10,1); }
    break;
case 164:
#line 772 "script.y"
{ AddFunct(11,1); }
    break;
case 165:
#line 773 "script.y"
{ AddFunct(12,1); }
    break;
case 166:
#line 774 "script.y"
{ AddFunct(13,1); }
    break;
case 167:
#line 775 "script.y"
{ AddFunct(14,1); }
    break;
case 168:
#line 776 "script.y"
{ AddFunct(15,1); }
    break;
case 169:
#line 777 "script.y"
{ AddFunct(16,1); }
    break;
case 170:
#line 778 "script.y"
{ AddFunct(17,1); }
    break;
case 171:
#line 779 "script.y"
{ AddFunct(18,1); }
    break;
case 172:
#line 780 "script.y"
{ AddFunct(19,1); }
    break;
case 173:
#line 781 "script.y"
{ AddFunct(20,1); }
    break;
case 174:
#line 782 "script.y"
{ AddFunct(21,1); }
    break;
case 175:
#line 783 "script.y"
{ AddFunct(22,1); }
    break;
case 176:
#line 784 "script.y"
{ AddFunct(23,1); }
    break;
case 177:
#line 785 "script.y"
{ AddFunct(24,1); }
    break;
case 178:
#line 786 "script.y"
{ AddFunct(25,1); }
    break;
case 179:
#line 787 "script.y"
{ AddFunct(26,1); }
    break;
case 180:
#line 792 "script.y"
{ }
    break;
case 207:
#line 837 "script.y"
{ l=1-250000; AddBufArg(&l,1); }
    break;
case 208:
#line 838 "script.y"
{ l=2-250000; AddBufArg(&l,1); }
    break;
case 209:
#line 839 "script.y"
{ l=3-250000; AddBufArg(&l,1); }
    break;
case 210:
#line 840 "script.y"
{ l=4-250000; AddBufArg(&l,1); }
    break;
case 211:
#line 841 "script.y"
{ l=5-250000; AddBufArg(&l,1); }
    break;
case 212:
#line 842 "script.y"
{ l=6-250000; AddBufArg(&l,1); }
    break;
}

#line 705 "/usr/share/bison/bison.simple"


  yyvsp -= yylen;
  yyssp -= yylen;
#if YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;
#if YYLSP_NEEDED
  *++yylsp = yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("parse error, unexpected ") + 1;
	  yysize += yystrlen (yytname[YYTRANSLATE (yychar)]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "parse error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[YYTRANSLATE (yychar)]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exhausted");
	}
      else
#endif /* defined (YYERROR_VERBOSE) */
	yyerror ("parse error");
    }
  goto yyerrlab1;


/*--------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
yyerrlab1:
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;
      YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
		  yychar, yytname[yychar1]));
      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;


/*-------------------------------------------------------------------.
| yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
yyerrdefault:
#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */

  /* If its default is to accept any token, ok.  Otherwise pop it.  */
  yyn = yydefact[yystate];
  if (yyn)
    goto yydefault;
#endif


/*---------------------------------------------------------------.
| yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
yyerrpop:
  if (yyssp == yyss)
    YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#if YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG
  if (yydebug)
    {
      short *yyssp1 = yyss - 1;
      YYFPRINTF (stderr, "Error: state stack now");
      while (yyssp1 != yyssp)
	YYFPRINTF (stderr, " %d", *++yyssp1);
      YYFPRINTF (stderr, "\n");
    }
#endif

/*--------------.
| yyerrhandle.  |
`--------------*/
yyerrhandle:
  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;
#if YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

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

/*---------------------------------------------.
| yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}
#line 845 "script.y"

