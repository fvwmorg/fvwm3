
/*  A Bison parser, made from script.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	STR	257
#define	GSTR	258
#define	VAR	259
#define	NUMBER	260
#define	WINDOWTITLE	261
#define	WINDOWSIZE	262
#define	WINDOWPOSITION	263
#define	FONT	264
#define	FORECOLOR	265
#define	BACKCOLOR	266
#define	SHADCOLOR	267
#define	LICOLOR	268
#define	COLORSET	269
#define	OBJECT	270
#define	INIT	271
#define	PERIODICTASK	272
#define	MAIN	273
#define	END	274
#define	PROP	275
#define	TYPE	276
#define	SIZE	277
#define	POSITION	278
#define	VALUE	279
#define	VALUEMIN	280
#define	VALUEMAX	281
#define	TITLE	282
#define	SWALLOWEXEC	283
#define	ICON	284
#define	FLAGS	285
#define	WARP	286
#define	WRITETOFILE	287
#define	HIDDEN	288
#define	CANBESELECTED	289
#define	NORELIEFSTRING	290
#define	CASE	291
#define	SINGLECLIC	292
#define	DOUBLECLIC	293
#define	BEG	294
#define	POINT	295
#define	EXEC	296
#define	HIDE	297
#define	SHOW	298
#define	CHFORECOLOR	299
#define	CHBACKCOLOR	300
#define	CHCOLORSET	301
#define	GETVALUE	302
#define	GETMINVALUE	303
#define	GETMAXVALUE	304
#define	GETFORE	305
#define	GETBACK	306
#define	GETHILIGHT	307
#define	GETSHADOW	308
#define	CHVALUE	309
#define	CHVALUEMAX	310
#define	CHVALUEMIN	311
#define	ADD	312
#define	DIV	313
#define	MULT	314
#define	GETTITLE	315
#define	GETOUTPUT	316
#define	STRCOPY	317
#define	NUMTOHEX	318
#define	HEXTONUM	319
#define	QUIT	320
#define	LAUNCHSCRIPT	321
#define	GETSCRIPTFATHER	322
#define	SENDTOSCRIPT	323
#define	RECEIVFROMSCRIPT	324
#define	GET	325
#define	SET	326
#define	SENDSIGN	327
#define	REMAINDEROFDIV	328
#define	GETTIME	329
#define	GETSCRIPTARG	330
#define	IF	331
#define	THEN	332
#define	ELSE	333
#define	FOR	334
#define	TO	335
#define	DO	336
#define	WHILE	337
#define	BEGF	338
#define	ENDF	339
#define	EQUAL	340
#define	INFEQ	341
#define	SUPEQ	342
#define	INF	343
#define	SUP	344
#define	DIFF	345

#line 1 "script.y"

#include "types.h"

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

/* Initialisation globale */
void InitVarGlob()
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
 scriptprop->periodictasks=NULL;
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
void RmLevelBufArg()
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
  return NULL;
  *s=0;
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
void AddLevelBufArg()
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
  fprintf(stderr,"Line %d: too many variables (>512)\n",numligne);
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
void EmpilerBloc()
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
 fprintf(stderr,"Line %d: %s\n",numligne,errmsg);
 return 0;
}



#line 348 "script.y"
typedef union {  char *str;
          int number;
       } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		418
#define	YYFLAG		-32768
#define	YYNTBASE	92

#define YYTRANSLATE(x) ((unsigned)(x) <= 345 ? yytranslate[x] : 155)

static const char yytranslate[] = {     0,
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
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
    67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
    77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
    87,    88,    89,    90,    91
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     6,     7,     8,    12,    16,    21,    26,    30,    34,
    38,    42,    46,    50,    51,    57,    58,    64,    65,    73,
    75,    76,    80,    85,    90,    94,    98,   102,   106,   110,
   114,   118,   122,   126,   130,   134,   138,   142,   143,   146,
   149,   152,   153,   155,   161,   162,   163,   168,   173,   175,
   177,   179,   183,   184,   188,   192,   196,   200,   204,   208,
   212,   216,   220,   224,   228,   232,   236,   240,   244,   248,
   252,   256,   260,   264,   268,   272,   276,   279,   282,   285,
   288,   291,   294,   297,   300,   303,   306,   309,   312,   315,
   318,   321,   324,   327,   330,   333,   336,   339,   342,   345,
   348,   351,   356,   361,   366,   373,   380,   385,   390,   395,
   400,   405,   410,   416,   421,   422,   425,   430,   435,   440,
   444,   448,   456,   457,   461,   462,   466,   468,   472,   474,
   484,   492,   494,   496,   498,   500,   502,   504,   505,   508,
   511,   516,   520,   523,   527,   531,   535,   540,   543,   545,
   548,   552,   554,   557,   560,   563,   566,   569,   572,   575,
   576,   579,   582,   585,   588,   591,   594,   600,   602,   604,
   606,   608,   610,   612,   617,   619,   621,   623,   625,   630,
   632,   634,   639,   641,   643,   648,   650,   652,   654,   656,
   658,   660
};

static const short yyrhs[] = {    93,
    94,    95,    96,    97,     0,     0,     0,    94,     7,     4,
     0,    94,    30,     3,     0,    94,     9,     6,     6,     0,
    94,     8,     6,     6,     0,    94,    12,     4,     0,    94,
    11,     4,     0,    94,    13,     4,     0,    94,    14,     4,
     0,    94,    15,     6,     0,    94,    10,     3,     0,     0,
    17,   135,    40,   108,    20,     0,     0,    18,   135,    40,
   108,    20,     0,     0,    97,    16,    98,    21,    99,   101,
   102,     0,     6,     0,     0,    99,    22,     3,     0,    99,
    23,     6,     6,     0,    99,    24,     6,     6,     0,    99,
    25,     6,     0,    99,    26,     6,     0,    99,    27,     6,
     0,    99,    28,     4,     0,    99,    29,     4,     0,    99,
    30,     3,     0,    99,    12,     4,     0,    99,    11,     4,
     0,    99,    13,     4,     0,    99,    14,     4,     0,    99,
    15,     6,     0,    99,    10,     3,     0,    99,    31,   100,
     0,     0,   100,    34,     0,   100,    36,     0,   100,    35,
     0,     0,    20,     0,    19,   103,    37,   104,    20,     0,
     0,     0,   104,   105,    41,   107,     0,   104,   106,    41,
   107,     0,    38,     0,    39,     0,     6,     0,    40,   108,
    20,     0,     0,   108,    42,   110,     0,   108,    32,   127,
     0,   108,    33,   129,     0,   108,    43,   111,     0,   108,
    44,   112,     0,   108,    55,   113,     0,   108,    56,   114,
     0,   108,    57,   115,     0,   108,    24,   116,     0,   108,
    23,   117,     0,   108,    28,   119,     0,   108,    30,   118,
     0,   108,    10,   120,     0,   108,    45,   121,     0,   108,
    46,   122,     0,   108,    47,   123,     0,   108,    72,   124,
     0,   108,    73,   125,     0,   108,    66,   126,     0,   108,
    69,   128,     0,   108,    77,   130,     0,   108,    80,   131,
     0,   108,    83,   132,     0,    42,   110,     0,    32,   127,
     0,    33,   129,     0,    43,   111,     0,    44,   112,     0,
    55,   113,     0,    56,   114,     0,    57,   115,     0,    24,
   116,     0,    23,   117,     0,    28,   119,     0,    30,   118,
     0,    10,   120,     0,    45,   121,     0,    46,   122,     0,
    47,   123,     0,    72,   124,     0,    73,   125,     0,    66,
   126,     0,    69,   128,     0,    80,   131,     0,    83,   132,
     0,   146,   148,     0,   146,   150,     0,   146,   150,     0,
   146,   150,   146,   150,     0,   146,   150,   146,   150,     0,
   146,   150,   146,   150,     0,   146,   150,   146,   150,   146,
   150,     0,   146,   150,   146,   150,   146,   150,     0,   146,
   150,   146,   151,     0,   146,   150,   146,   152,     0,   146,
   150,   146,   151,     0,   146,   150,   146,   152,     0,   146,
   150,   146,   152,     0,   146,   150,   146,   150,     0,   146,
   153,    71,   146,   148,     0,   146,   150,   146,   150,     0,
     0,   146,   150,     0,   146,   150,   146,   148,     0,   146,
   151,   146,   148,     0,   133,   135,   136,   134,     0,   138,
   135,   137,     0,   139,   135,   137,     0,   146,   149,   146,
   154,   146,   149,    78,     0,     0,    79,   135,   137,     0,
     0,    40,   108,    20,     0,   109,     0,    40,   108,    20,
     0,   109,     0,   146,   153,    71,   146,   149,    81,   146,
   149,    82,     0,   146,   149,   146,   154,   146,   149,    82,
     0,     5,     0,     3,     0,     4,     0,     6,     0,    38,
     0,    39,     0,     0,    48,   150,     0,    61,   150,     0,
    62,   152,   150,   150,     0,    64,   150,   150,     0,    65,
   152,     0,    58,   150,   150,     0,    60,   150,   150,     0,
    59,   150,   150,     0,    63,   152,   150,   150,     0,    67,
   152,     0,    68,     0,    70,   150,     0,    74,   150,   150,
     0,    75,     0,    76,   150,     0,    51,   150,     0,    52,
   150,     0,    53,   150,     0,    54,   150,     0,    49,   150,
     0,    50,   150,     0,     0,   144,   148,     0,   145,   148,
     0,   140,   148,     0,   142,   148,     0,   141,   148,     0,
   143,   148,     0,    84,   146,   147,    85,   148,     0,   140,
     0,   144,     0,   145,     0,   142,     0,   141,     0,   143,
     0,    84,   146,   147,    85,     0,   144,     0,   145,     0,
   143,     0,   140,     0,    84,   146,   147,    85,     0,   140,
     0,   141,     0,    84,   146,   147,    85,     0,   140,     0,
   142,     0,    84,   146,   147,    85,     0,   140,     0,    89,
     0,    87,     0,    86,     0,    88,     0,    90,     0,    91,
     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   372,   375,   379,   380,   383,   386,   391,   396,   400,   404,
   408,   412,   416,   422,   423,   428,   429,   438,   439,   442,
   458,   459,   463,   467,   472,   475,   478,   481,   484,   487,
   490,   494,   498,   502,   506,   510,   513,   515,   516,   519,
   522,   528,   539,   540,   543,   545,   546,   547,   550,   551,
   554,   557,   562,   563,   564,   565,   566,   567,   568,   569,
   570,   571,   572,   573,   574,   575,   576,   577,   578,   579,
   580,   581,   582,   583,   584,   585,   589,   590,   591,   592,
   593,   594,   595,   596,   597,   598,   599,   600,   601,   602,
   603,   604,   605,   606,   607,   608,   609,   610,   613,   615,
   617,   619,   621,   623,   625,   627,   629,   631,   633,   635,
   637,   639,   641,   643,   645,   647,   649,   651,   653,   655,
   657,   661,   663,   664,   666,   668,   669,   672,   673,   677,
   681,   686,   688,   690,   692,   694,   696,   698,   700,   701,
   702,   703,   704,   705,   706,   707,   708,   709,   710,   711,
   712,   713,   714,   715,   716,   717,   718,   719,   720,   725,
   726,   727,   728,   729,   730,   731,   732,   737,   738,   739,
   740,   741,   742,   743,   747,   748,   749,   750,   751,   755,
   756,   757,   761,   762,   763,   767,   771,   772,   773,   774,
   775,   776
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","STR","GSTR",
"VAR","NUMBER","WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FONT","FORECOLOR",
"BACKCOLOR","SHADCOLOR","LICOLOR","COLORSET","OBJECT","INIT","PERIODICTASK",
"MAIN","END","PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX","TITLE",
"SWALLOWEXEC","ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","CANBESELECTED","NORELIEFSTRING",
"CASE","SINGLECLIC","DOUBLECLIC","BEG","POINT","EXEC","HIDE","SHOW","CHFORECOLOR",
"CHBACKCOLOR","CHCOLORSET","GETVALUE","GETMINVALUE","GETMAXVALUE","GETFORE",
"GETBACK","GETHILIGHT","GETSHADOW","CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD",
"DIV","MULT","GETTITLE","GETOUTPUT","STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT",
"GETSCRIPTFATHER","SENDTOSCRIPT","RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV",
"GETTIME","GETSCRIPTARG","IF","THEN","ELSE","FOR","TO","DO","WHILE","BEGF","ENDF",
"EQUAL","INFEQ","SUPEQ","INF","SUP","DIFF","script","initvar","head","initbloc",
"periodictask","object","id","init","flags","verify","mainloop","addtabcase",
"case","clic","number","bloc","instr","oneinstr","exec","hide","show","chvalue",
"chvaluemax","chvaluemin","position","size","icon","title","font","chforecolor",
"chbackcolor","chcolorset","set","sendsign","quit","warp","sendtoscript","writetofile",
"ifthenelse","loop","while","headif","else","creerbloc","bloc1","bloc2","headloop",
"headwhile","var","str","gstr","num","singleclic2","doubleclic2","addlbuff",
"function","args","arg","numarg","strarg","gstrarg","vararg","compare", NULL
};
#endif

static const short yyr1[] = {     0,
    92,    93,    94,    94,    94,    94,    94,    94,    94,    94,
    94,    94,    94,    95,    95,    96,    96,    97,    97,    98,
    99,    99,    99,    99,    99,    99,    99,    99,    99,    99,
    99,    99,    99,    99,    99,    99,    99,   100,   100,   100,
   100,   101,   102,   102,   103,   104,   104,   104,   105,   105,
   106,   107,   108,   108,   108,   108,   108,   108,   108,   108,
   108,   108,   108,   108,   108,   108,   108,   108,   108,   108,
   108,   108,   108,   108,   108,   108,   109,   109,   109,   109,
   109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
   109,   109,   109,   109,   109,   109,   109,   109,   110,   111,
   112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
   122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
   132,   133,   134,   134,   135,   136,   136,   137,   137,   138,
   139,   140,   141,   142,   143,   144,   145,   146,   147,   147,
   147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
   147,   147,   147,   147,   147,   147,   147,   147,   147,   148,
   148,   148,   148,   148,   148,   148,   148,   149,   149,   149,
   149,   149,   149,   149,   150,   150,   150,   150,   150,   151,
   151,   151,   152,   152,   152,   153,   154,   154,   154,   154,
   154,   154
};

static const short yyr2[] = {     0,
     5,     0,     0,     3,     3,     4,     4,     3,     3,     3,
     3,     3,     3,     0,     5,     0,     5,     0,     7,     1,
     0,     3,     4,     4,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     0,     2,     2,
     2,     0,     1,     5,     0,     0,     4,     4,     1,     1,
     1,     3,     0,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     4,     4,     4,     6,     6,     4,     4,     4,     4,
     4,     4,     5,     4,     0,     2,     4,     4,     4,     3,
     3,     7,     0,     3,     0,     3,     1,     3,     1,     9,
     7,     1,     1,     1,     1,     1,     1,     0,     2,     2,
     4,     3,     2,     3,     3,     3,     4,     2,     1,     2,
     3,     1,     2,     2,     2,     2,     2,     2,     2,     0,
     2,     2,     2,     2,     2,     2,     5,     1,     1,     1,
     1,     1,     1,     4,     1,     1,     1,     1,     4,     1,
     1,     4,     1,     1,     4,     1,     1,     1,     1,     1,
     1,     1
};

static const short yydefact[] = {     2,
     3,    14,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   125,     0,    16,     4,     0,     0,    13,     9,     8,
    10,    11,    12,     0,     5,   125,    18,     7,     6,    53,
     0,     1,     0,    53,     0,   138,    15,   138,   138,   138,
   138,   138,   138,   138,   138,   138,   138,   138,   138,   138,
   138,   138,   115,   138,   138,   138,   138,   138,   138,     0,
    20,     0,    66,     0,    63,     0,    62,     0,    64,     0,
    65,     0,    55,     0,    56,     0,    54,   160,    57,     0,
    58,     0,    67,     0,    68,     0,    69,     0,    59,     0,
    60,     0,    61,     0,    72,    73,     0,    70,     0,    71,
     0,    74,   125,     0,    75,   125,     0,    76,   125,     0,
    17,    21,   132,   135,   136,   137,   138,   178,   177,   175,
   176,   138,   138,   138,   138,   138,   116,   133,   138,   180,
   181,   138,   134,   138,   160,   160,   160,   160,   160,   160,
    99,   100,   101,   138,   138,   138,   138,   138,   138,   138,
   186,     0,   138,     0,   138,   168,   172,   171,   173,   169,
   170,   138,     0,     0,     0,   138,    42,     0,     0,     0,
     0,     0,     0,     0,   160,     0,   163,   165,   164,   166,
   161,   162,     0,     0,     0,     0,     0,     0,   160,   138,
     0,   138,   138,   138,   138,   138,   138,   138,    53,   138,
   138,   138,   138,   138,   138,   138,   138,   138,   115,   138,
   138,   138,   138,   138,   127,   123,     0,     0,    53,   129,
   120,   138,   121,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   149,     0,     0,
   152,     0,     0,   109,   138,   138,   138,   183,   184,   108,
   107,     0,   118,     0,   110,   111,   112,   102,   103,   104,
   117,   160,   114,    89,    86,    85,    87,    88,    78,    79,
     0,    77,    80,    81,    90,    91,    92,    82,    83,    84,
    95,    96,    93,    94,    97,    98,   125,   119,     0,   189,
   188,   190,   187,   191,   192,   138,     0,     0,   138,    36,
    32,    31,    33,    34,    35,    22,     0,     0,    25,    26,
    27,    28,    29,    30,    37,    45,    43,    19,   139,   158,
   159,   154,   155,   156,   157,     0,     0,     0,   140,     0,
     0,     0,   143,   148,   150,     0,   153,   179,     0,     0,
     0,   182,   160,   113,   126,     0,   174,     0,   128,     0,
     0,    23,    24,    39,    41,    40,     0,   144,   146,   145,
     0,     0,   142,   151,   106,   105,     0,   167,   124,     0,
   138,     0,    46,   141,   147,   185,   122,     0,   131,     0,
     0,    51,    44,    49,    50,     0,     0,   130,     0,     0,
    53,    47,    48,     0,    52,     0,     0,     0
};

static const short yydefgoto[] = {   416,
     1,     2,    14,    27,    32,    62,   167,   335,   241,   338,
   377,   400,   406,   407,   412,    33,   220,    77,    79,    81,
    89,    91,    93,    67,    65,    71,    69,    63,    83,    85,
    87,    98,   100,    95,    73,    96,    75,   102,   105,   108,
   103,   308,    24,   216,   221,   106,   109,   118,   136,   137,
   119,   120,   121,    64,   263,   141,   162,   122,   132,   270,
   152,   316
};

static const short yypact[] = {-32768,
-32768,   457,    -1,    31,    33,    26,    45,    47,    48,    49,
    51,-32768,    52,    40,-32768,    57,    59,-32768,-32768,-32768,
-32768,-32768,-32768,    27,-32768,-32768,-32768,-32768,-32768,-32768,
    32,    50,   556,-32768,    65,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   656,
-32768,    58,-32768,    35,-32768,    35,-32768,    35,-32768,    35,
-32768,    35,-32768,    35,-32768,     6,-32768,    56,-32768,    35,
-32768,    35,-32768,    35,-32768,    35,-32768,    35,-32768,    35,
-32768,    35,-32768,    35,-32768,-32768,    35,-32768,    73,-32768,
    35,-32768,-32768,   215,-32768,-32768,    73,-32768,-32768,   215,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,    56,    56,    56,    56,    56,    56,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,     9,-32768,   888,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   942,    10,   942,-32768,  1006,   370,     6,    35,
    35,     8,     6,   370,    56,   370,-32768,-32768,-32768,-32768,
-32768,-32768,     8,     8,    35,    35,    35,    35,    56,-32768,
    35,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,     5,   370,    68,-32768,-32768,
-32768,-32768,-32768,    68,    84,    87,    92,    93,   100,   103,
   108,   106,   121,   122,   124,   127,   102,   130,   132,-32768,
    23,    35,    35,    35,    35,    35,    35,    35,    35,    35,
    35,    35,     8,     8,    35,     8,     8,-32768,    35,    35,
-32768,    35,    54,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    67,-32768,    75,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    56,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   714,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    78,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   772,   215,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   131,   135,-32768,-32768,
-32768,-32768,-32768,-32768,    11,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    35,    35,    35,-32768,    35,
    35,    35,-32768,-32768,-32768,    35,-32768,-32768,    35,    35,
   370,-32768,    56,-32768,-32768,   942,-32768,   215,-32768,    55,
   215,-32768,-32768,-32768,-32768,-32768,   138,-32768,-32768,-32768,
    35,    35,-32768,-32768,-32768,-32768,    79,-32768,-32768,   113,
-32768,   111,-32768,-32768,-32768,-32768,-32768,   215,-32768,    30,
   114,-32768,-32768,-32768,-32768,   154,   156,-32768,   159,   159,
-32768,-32768,-32768,   830,-32768,   201,   202,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,  -206,   -34,    53,    15,     4,    20,
     0,     1,     2,    17,    43,    28,    38,    64,    34,    36,
    41,    37,    39,    66,    42,    76,    60,-32768,    44,    62,
-32768,-32768,   -20,-32768,  -163,-32768,-32768,   -22,   -66,   -40,
    90,   134,   145,   -24,  -169,   175,  -109,   313,  -165,  -108,
   140,    21
};


#define	YYLAST		1037


static const short yytable[] = {    60,
   166,   223,    15,   264,   272,    31,   274,   271,   128,   131,
   113,   133,   113,    66,    68,    70,    72,    74,    76,    78,
    80,    82,    84,    86,    88,    90,    92,    94,    18,    97,
    99,   101,   104,   107,   110,   402,    16,   157,    17,   113,
   114,   336,   337,   157,   374,   375,   376,   309,    19,   403,
    20,    21,    22,   130,    25,   135,    23,    26,   128,   133,
   113,   114,    28,   158,    29,    35,    30,   404,   405,   158,
    61,    34,   115,   116,   275,   276,   151,   113,   112,   190,
   222,   156,   154,   307,   151,   163,   320,   156,   165,   129,
   321,   267,   168,   115,   116,   322,   323,   169,   170,   171,
   172,   173,   131,   324,   174,   332,   131,   175,   325,   176,
   326,   327,   135,   135,   135,   135,   135,   135,   117,   183,
   184,   185,   186,   187,   188,   189,   328,   329,   191,   330,
   217,   269,   331,   333,   334,   391,   372,   218,   358,   134,
   373,   224,   269,   269,   350,   351,   130,   353,   354,   268,
   130,   362,   135,   310,   311,   312,   313,   314,   315,   363,
   268,   268,   367,   396,   291,   282,   135,   138,    66,    68,
    70,    72,    74,    76,   393,    78,    80,    82,    84,    86,
    88,    90,    92,    94,   317,    97,    99,   101,   107,   110,
   397,   387,   399,   159,   409,   408,   410,   318,   411,   159,
   417,   418,   389,   413,   293,   298,   215,   299,   370,   300,
   286,   139,   269,   269,   292,   269,   269,   128,   133,   113,
   114,   294,   140,   288,   138,   138,   138,   138,   138,   138,
   268,   268,   287,   268,   268,   285,   295,   160,   289,   296,
   359,   360,   361,   160,   319,   297,   164,   303,   161,     0,
   304,   157,   115,   116,   161,   284,   305,   290,   390,   135,
     0,   392,     0,     0,   138,     0,     0,     0,   139,   139,
   139,   139,   139,   139,   301,   306,     0,   158,   138,   140,
   140,   140,   140,   140,   140,   302,   366,     0,   401,     0,
     0,   368,     0,     0,   371,   156,     0,     0,   155,     0,
     0,   157,     0,     0,   157,     0,     0,     0,   139,   177,
   178,   179,   180,   181,   182,     0,     0,     0,     0,   140,
     0,     0,   139,     0,     0,     0,     0,   158,     0,     0,
   158,   157,     0,   140,     0,     0,     0,     0,     0,     0,
   135,     0,     0,     0,     0,   156,     0,     0,   156,   273,
     0,     0,     0,     0,     0,     0,     0,   158,     0,     0,
     0,     0,     0,   281,     0,     0,   398,     0,     0,     0,
     0,   138,     0,     0,     0,   156,   414,     0,   123,     0,
   124,     0,   125,     0,   126,     0,   127,     0,     0,     0,
     0,     0,   142,     0,   143,     0,   144,     0,   145,     0,
   146,     0,   147,     0,   148,     0,   149,   159,     0,   150,
     0,     0,     0,   153,     0,   139,     0,   242,   243,   244,
   245,   246,   247,   248,     0,     0,   140,   249,   250,   251,
   252,   253,   254,   255,   256,     0,   257,   258,     0,   259,
     0,     0,     0,   260,   261,   262,     0,     0,     0,     0,
     0,   160,   138,     0,     0,     0,   364,   159,     0,     0,
   159,     0,   161,     3,     4,     5,     6,     7,     8,     9,
    10,    11,     0,    12,     0,     0,     0,     0,     0,     0,
     0,     0,   265,   266,     0,     0,    13,   159,     0,     0,
     0,     0,     0,     0,     0,     0,   139,   277,   278,   279,
   280,   160,     0,   283,   160,     0,     0,   140,     0,     0,
     0,     0,   161,     0,     0,   161,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   160,     0,     0,     0,     0,     0,   388,     0,     0,
     0,     0,   161,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   339,   340,   341,   342,   343,   344,
   345,   346,   347,   348,   349,    36,     0,   352,     0,     0,
     0,   355,   356,     0,   357,    37,     0,     0,    38,    39,
     0,     0,     0,    40,     0,    41,     0,    42,    43,     0,
     0,     0,     0,     0,     0,     0,     0,    44,    45,    46,
    47,    48,    49,     0,     0,     0,     0,     0,     0,     0,
    50,    51,    52,     0,     0,     0,     0,     0,     0,     0,
     0,    53,     0,     0,    54,     0,     0,    55,    56,     0,
     0,     0,    57,     0,     0,    58,     0,     0,    59,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,   378,   379,
   380,     0,   381,   382,   383,    36,     0,     0,   384,     0,
     0,   385,   386,     0,     0,   111,     0,     0,    38,    39,
     0,     0,     0,    40,     0,    41,     0,    42,    43,     0,
     0,     0,     0,   394,   395,     0,     0,    44,    45,    46,
    47,    48,    49,     0,     0,     0,     0,     0,     0,     0,
    50,    51,    52,     0,     0,     0,     0,     0,     0,     0,
     0,    53,     0,    36,    54,     0,     0,    55,    56,     0,
     0,     0,    57,   365,     0,    58,    38,    39,    59,     0,
     0,    40,     0,    41,     0,    42,    43,     0,     0,     0,
     0,     0,     0,     0,     0,    44,    45,    46,    47,    48,
    49,     0,     0,     0,     0,     0,     0,     0,    50,    51,
    52,     0,     0,     0,     0,     0,     0,     0,     0,    53,
     0,    36,    54,     0,     0,    55,    56,     0,     0,     0,
    57,   369,     0,    58,    38,    39,    59,     0,     0,    40,
     0,    41,     0,    42,    43,     0,     0,     0,     0,     0,
     0,     0,     0,    44,    45,    46,    47,    48,    49,     0,
     0,     0,     0,     0,     0,     0,    50,    51,    52,     0,
     0,     0,     0,     0,     0,     0,     0,    53,     0,    36,
    54,     0,     0,    55,    56,     0,     0,     0,    57,   415,
     0,    58,    38,    39,    59,     0,     0,    40,     0,    41,
     0,    42,    43,     0,     0,     0,     0,     0,     0,     0,
     0,    44,    45,    46,    47,    48,    49,     0,     0,     0,
     0,     0,     0,     0,    50,    51,    52,     0,     0,     0,
     0,     0,     0,     0,     0,    53,     0,   192,    54,     0,
     0,    55,    56,     0,     0,     0,    57,     0,     0,    58,
   193,   194,    59,     0,     0,   195,     0,   196,     0,   197,
   198,     0,     0,     0,     0,     0,     0,   199,     0,   200,
   201,   202,   203,   204,   205,     0,     0,     0,     0,     0,
     0,     0,   206,   207,   208,     0,     0,     0,     0,     0,
     0,   192,     0,   209,     0,     0,   210,     0,     0,   211,
   212,     0,     0,     0,   193,   194,     0,   213,     0,   195,
   214,   196,     0,   197,   198,     0,     0,     0,     0,     0,
     0,   219,     0,   200,   201,   202,   203,   204,   205,     0,
     0,     0,     0,     0,     0,     0,   206,   207,   208,     0,
     0,     0,     0,     0,     0,     0,     0,   209,     0,     0,
   210,     0,     0,   211,   212,   225,   226,   227,   228,   229,
   230,   213,     0,     0,   214,     0,     0,   231,   232,   233,
   234,   235,   236,   237,   238,   239,   240
};

static const short yycheck[] = {    34,
   110,   165,     4,   169,   174,    26,   176,   173,     3,    76,
     5,     4,     5,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,     3,    54,
    55,    56,    57,    58,    59,     6,     6,   104,     6,     5,
     6,    19,    20,   110,    34,    35,    36,   217,     4,    20,
     4,     4,     4,    76,     3,    78,     6,    18,     3,     4,
     5,     6,     6,   104,     6,    16,    40,    38,    39,   110,
     6,    40,    38,    39,   183,   184,    99,     5,    21,    71,
    71,   104,   103,    79,   107,   106,     3,   110,   109,    84,
     4,    84,   117,    38,    39,     4,     4,   122,   123,   124,
   125,   126,   169,     4,   129,     4,   173,   132,     6,   134,
     3,     6,   135,   136,   137,   138,   139,   140,    84,   144,
   145,   146,   147,   148,   149,   150,     6,     6,   153,     6,
   155,   172,     6,     4,     3,    81,     6,   162,    85,    84,
     6,   166,   183,   184,   253,   254,   169,   256,   257,   172,
   173,    85,   175,    86,    87,    88,    89,    90,    91,    85,
   183,   184,    85,    85,   199,   190,   189,    78,   193,   194,
   195,   196,   197,   198,    37,   200,   201,   202,   203,   204,
   205,   206,   207,   208,   219,   210,   211,   212,   213,   214,
    78,   361,    82,   104,    41,    82,    41,   222,    40,   110,
     0,     0,   366,   410,   201,   206,   154,   207,   318,   208,
   194,    78,   253,   254,   200,   256,   257,     3,     4,     5,
     6,   202,    78,   196,   135,   136,   137,   138,   139,   140,
   253,   254,   195,   256,   257,   193,   203,   104,   197,   204,
   265,   266,   267,   110,   224,   205,   107,   211,   104,    -1,
   212,   318,    38,    39,   110,   192,   213,   198,   368,   282,
    -1,   371,    -1,    -1,   175,    -1,    -1,    -1,   135,   136,
   137,   138,   139,   140,   209,   214,    -1,   318,   189,   135,
   136,   137,   138,   139,   140,   210,   307,    -1,   398,    -1,
    -1,   316,    -1,    -1,   319,   318,    -1,    -1,    84,    -1,
    -1,   368,    -1,    -1,   371,    -1,    -1,    -1,   175,   135,
   136,   137,   138,   139,   140,    -1,    -1,    -1,    -1,   175,
    -1,    -1,   189,    -1,    -1,    -1,    -1,   368,    -1,    -1,
   371,   398,    -1,   189,    -1,    -1,    -1,    -1,    -1,    -1,
   363,    -1,    -1,    -1,    -1,   368,    -1,    -1,   371,   175,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,   398,    -1,    -1,
    -1,    -1,    -1,   189,    -1,    -1,   391,    -1,    -1,    -1,
    -1,   282,    -1,    -1,    -1,   398,   411,    -1,    66,    -1,
    68,    -1,    70,    -1,    72,    -1,    74,    -1,    -1,    -1,
    -1,    -1,    80,    -1,    82,    -1,    84,    -1,    86,    -1,
    88,    -1,    90,    -1,    92,    -1,    94,   318,    -1,    97,
    -1,    -1,    -1,   101,    -1,   282,    -1,    48,    49,    50,
    51,    52,    53,    54,    -1,    -1,   282,    58,    59,    60,
    61,    62,    63,    64,    65,    -1,    67,    68,    -1,    70,
    -1,    -1,    -1,    74,    75,    76,    -1,    -1,    -1,    -1,
    -1,   318,   363,    -1,    -1,    -1,   282,   368,    -1,    -1,
   371,    -1,   318,     7,     8,     9,    10,    11,    12,    13,
    14,    15,    -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,   170,   171,    -1,    -1,    30,   398,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,   363,   185,   186,   187,
   188,   368,    -1,   191,   371,    -1,    -1,   363,    -1,    -1,
    -1,    -1,   368,    -1,    -1,   371,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,   398,    -1,    -1,    -1,    -1,    -1,   363,    -1,    -1,
    -1,    -1,   398,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,   242,   243,   244,   245,   246,   247,
   248,   249,   250,   251,   252,    10,    -1,   255,    -1,    -1,
    -1,   259,   260,    -1,   262,    20,    -1,    -1,    23,    24,
    -1,    -1,    -1,    28,    -1,    30,    -1,    32,    33,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    44,
    45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    66,    -1,    -1,    69,    -1,    -1,    72,    73,    -1,
    -1,    -1,    77,    -1,    -1,    80,    -1,    -1,    83,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   346,   347,
   348,    -1,   350,   351,   352,    10,    -1,    -1,   356,    -1,
    -1,   359,   360,    -1,    -1,    20,    -1,    -1,    23,    24,
    -1,    -1,    -1,    28,    -1,    30,    -1,    32,    33,    -1,
    -1,    -1,    -1,   381,   382,    -1,    -1,    42,    43,    44,
    45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    55,    56,    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    66,    -1,    10,    69,    -1,    -1,    72,    73,    -1,
    -1,    -1,    77,    20,    -1,    80,    23,    24,    83,    -1,
    -1,    28,    -1,    30,    -1,    32,    33,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,
    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,
    57,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,
    -1,    10,    69,    -1,    -1,    72,    73,    -1,    -1,    -1,
    77,    20,    -1,    80,    23,    24,    83,    -1,    -1,    28,
    -1,    30,    -1,    32,    33,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    42,    43,    44,    45,    46,    47,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,    -1,    10,
    69,    -1,    -1,    72,    73,    -1,    -1,    -1,    77,    20,
    -1,    80,    23,    24,    83,    -1,    -1,    28,    -1,    30,
    -1,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    55,    56,    57,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    66,    -1,    10,    69,    -1,
    -1,    72,    73,    -1,    -1,    -1,    77,    -1,    -1,    80,
    23,    24,    83,    -1,    -1,    28,    -1,    30,    -1,    32,
    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,
    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    55,    56,    57,    -1,    -1,    -1,    -1,    -1,
    -1,    10,    -1,    66,    -1,    -1,    69,    -1,    -1,    72,
    73,    -1,    -1,    -1,    23,    24,    -1,    80,    -1,    28,
    83,    30,    -1,    32,    33,    -1,    -1,    -1,    -1,    -1,
    -1,    40,    -1,    42,    43,    44,    45,    46,    47,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    55,    56,    57,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    66,    -1,    -1,
    69,    -1,    -1,    72,    73,    10,    11,    12,    13,    14,
    15,    80,    -1,    -1,    83,    -1,    -1,    22,    23,    24,
    25,    26,    27,    28,    29,    30,    31
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
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
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 2:
#line 375 "script.y"
{ InitVarGlob(); ;
    break;}
case 4:
#line 380 "script.y"
{		/* Titre de la fenetre */
				 scriptprop->titlewin=yyvsp[0].str;
				;
    break;}
case 5:
#line 383 "script.y"
{
				 scriptprop->icon=yyvsp[0].str;
				;
    break;}
case 6:
#line 387 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->x=yyvsp[-1].number;
				 scriptprop->y=yyvsp[0].number;
				;
    break;}
case 7:
#line 392 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->width=yyvsp[-1].number;
				 scriptprop->height=yyvsp[0].number;
				;
    break;}
case 8:
#line 396 "script.y"
{ 		/* Couleur de fond */
				 scriptprop->backcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 9:
#line 400 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->forecolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 10:
#line 404 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 11:
#line 408 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->hilicolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 12:
#line 412 "script.y"
{
				 scriptprop->colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				;
    break;}
case 13:
#line 416 "script.y"
{
				 scriptprop->font=yyvsp[0].str;
				;
    break;}
case 15:
#line 423 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 17:
#line 429 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 20:
#line 442 "script.y"
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
				;
    break;}
case 22:
#line 459 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				;
    break;}
case 23:
#line 463 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				;
    break;}
case 24:
#line 467 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				;
    break;}
case 25:
#line 472 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				;
    break;}
case 26:
#line 475 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				;
    break;}
case 27:
#line 478 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				;
    break;}
case 28:
#line 481 "script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[0].str;
				;
    break;}
case 29:
#line 484 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				;
    break;}
case 30:
#line 487 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				;
    break;}
case 31:
#line 490 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 32:
#line 494 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 33:
#line 498 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 34:
#line 502 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 35:
#line 506 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				;
    break;}
case 36:
#line 510 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				;
    break;}
case 39:
#line 516 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				;
    break;}
case 40:
#line 519 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				;
    break;}
case 41:
#line 522 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				;
    break;}
case 42:
#line 528 "script.y"
{ 
				  if (!HasPosition)
				   { yyerror("No position for object");
				     exit(1);}
				  if (!HasType)
				   { yyerror("No type for object");
				     exit(1);}
				  HasPosition=0;
				  HasType=0;
				 ;
    break;}
case 43:
#line 539 "script.y"
{ InitObjTabCase(0); ;
    break;}
case 45:
#line 543 "script.y"
{ InitObjTabCase(1); ;
    break;}
case 49:
#line 550 "script.y"
{ InitCase(-1); ;
    break;}
case 50:
#line 551 "script.y"
{ InitCase(-2); ;
    break;}
case 51:
#line 554 "script.y"
{ InitCase(yyvsp[0].number); ;
    break;}
case 99:
#line 613 "script.y"
{ AddCom(1,1); ;
    break;}
case 100:
#line 615 "script.y"
{ AddCom(2,1);;
    break;}
case 101:
#line 617 "script.y"
{ AddCom(3,1);;
    break;}
case 102:
#line 619 "script.y"
{ AddCom(4,2);;
    break;}
case 103:
#line 621 "script.y"
{ AddCom(21,2);;
    break;}
case 104:
#line 623 "script.y"
{ AddCom(22,2);;
    break;}
case 105:
#line 625 "script.y"
{ AddCom(5,3);;
    break;}
case 106:
#line 627 "script.y"
{ AddCom(6,3);;
    break;}
case 107:
#line 629 "script.y"
{ AddCom(7,2);;
    break;}
case 108:
#line 631 "script.y"
{ AddCom(8,2);;
    break;}
case 109:
#line 633 "script.y"
{ AddCom(9,2);;
    break;}
case 110:
#line 635 "script.y"
{ AddCom(10,2);;
    break;}
case 111:
#line 637 "script.y"
{ AddCom(19,2);;
    break;}
case 112:
#line 639 "script.y"
{ AddCom(24,2);;
    break;}
case 113:
#line 641 "script.y"
{ AddCom(11,2);;
    break;}
case 114:
#line 643 "script.y"
{ AddCom(12,2);;
    break;}
case 115:
#line 645 "script.y"
{ AddCom(13,0);;
    break;}
case 116:
#line 647 "script.y"
{ AddCom(17,1);;
    break;}
case 117:
#line 649 "script.y"
{ AddCom(23,2);;
    break;}
case 118:
#line 651 "script.y"
{ AddCom(18,2);;
    break;}
case 122:
#line 661 "script.y"
{ AddComBloc(14,3,2); ;
    break;}
case 125:
#line 666 "script.y"
{ EmpilerBloc(); ;
    break;}
case 126:
#line 668 "script.y"
{ DepilerBloc(2); ;
    break;}
case 127:
#line 669 "script.y"
{ DepilerBloc(2); ;
    break;}
case 128:
#line 672 "script.y"
{ DepilerBloc(1); ;
    break;}
case 129:
#line 673 "script.y"
{ DepilerBloc(1); ;
    break;}
case 130:
#line 677 "script.y"
{ AddComBloc(15,3,1); ;
    break;}
case 131:
#line 681 "script.y"
{ AddComBloc(16,3,1); ;
    break;}
case 132:
#line 686 "script.y"
{ AddVar(yyvsp[0].str); ;
    break;}
case 133:
#line 688 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 134:
#line 690 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 135:
#line 692 "script.y"
{ AddConstNum(yyvsp[0].number); ;
    break;}
case 136:
#line 694 "script.y"
{ AddConstNum(-1); ;
    break;}
case 137:
#line 696 "script.y"
{ AddConstNum(-2); ;
    break;}
case 138:
#line 698 "script.y"
{ AddLevelBufArg(); ;
    break;}
case 139:
#line 700 "script.y"
{ AddFunct(1,1); ;
    break;}
case 140:
#line 701 "script.y"
{ AddFunct(2,1); ;
    break;}
case 141:
#line 702 "script.y"
{ AddFunct(3,1); ;
    break;}
case 142:
#line 703 "script.y"
{ AddFunct(4,1); ;
    break;}
case 143:
#line 704 "script.y"
{ AddFunct(5,1); ;
    break;}
case 144:
#line 705 "script.y"
{ AddFunct(6,1); ;
    break;}
case 145:
#line 706 "script.y"
{ AddFunct(7,1); ;
    break;}
case 146:
#line 707 "script.y"
{ AddFunct(8,1); ;
    break;}
case 147:
#line 708 "script.y"
{ AddFunct(9,1); ;
    break;}
case 148:
#line 709 "script.y"
{ AddFunct(10,1); ;
    break;}
case 149:
#line 710 "script.y"
{ AddFunct(11,1); ;
    break;}
case 150:
#line 711 "script.y"
{ AddFunct(12,1); ;
    break;}
case 151:
#line 712 "script.y"
{ AddFunct(13,1); ;
    break;}
case 152:
#line 713 "script.y"
{ AddFunct(14,1); ;
    break;}
case 153:
#line 714 "script.y"
{ AddFunct(15,1); ;
    break;}
case 154:
#line 715 "script.y"
{ AddFunct(16,1); ;
    break;}
case 155:
#line 716 "script.y"
{ AddFunct(17,1); ;
    break;}
case 156:
#line 717 "script.y"
{ AddFunct(18,1); ;
    break;}
case 157:
#line 718 "script.y"
{ AddFunct(19,1); ;
    break;}
case 158:
#line 719 "script.y"
{ AddFunct(20,1); ;
    break;}
case 159:
#line 720 "script.y"
{ AddFunct(21,1); ;
    break;}
case 160:
#line 725 "script.y"
{ ;
    break;}
case 187:
#line 771 "script.y"
{ l=1-250000; AddBufArg(&l,1); ;
    break;}
case 188:
#line 772 "script.y"
{ l=2-250000; AddBufArg(&l,1); ;
    break;}
case 189:
#line 773 "script.y"
{ l=3-250000; AddBufArg(&l,1); ;
    break;}
case 190:
#line 774 "script.y"
{ l=4-250000; AddBufArg(&l,1); ;
    break;}
case 191:
#line 775 "script.y"
{ l=5-250000; AddBufArg(&l,1); ;
    break;}
case 192:
#line 776 "script.y"
{ l=6-250000; AddBufArg(&l,1); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

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

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 779 "script.y"

