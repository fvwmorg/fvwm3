
/*  A Bison parser, made from script.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	STR	258
#define	GSTR	259
#define	VAR	260
#define	NUMBER	261
#define	WINDOWTITLE	262
#define	WINDOWSIZE	263
#define	WINDOWPOSITION	264
#define	FONT	265
#define	FORECOLOR	266
#define	BACKCOLOR	267
#define	SHADCOLOR	268
#define	LICOLOR	269
#define	COLORSET	270
#define	OBJECT	271
#define	INIT	272
#define	PERIODICTASK	273
#define	MAIN	274
#define	END	275
#define	PROP	276
#define	TYPE	277
#define	SIZE	278
#define	POSITION	279
#define	VALUE	280
#define	VALUEMIN	281
#define	VALUEMAX	282
#define	TITLE	283
#define	SWALLOWEXEC	284
#define	ICON	285
#define	FLAGS	286
#define	WARP	287
#define	WRITETOFILE	288
#define	HIDDEN	289
#define	CANBESELECTED	290
#define	NORELIEFSTRING	291
#define	CASE	292
#define	SINGLECLIC	293
#define	DOUBLECLIC	294
#define	BEG	295
#define	POINT	296
#define	EXEC	297
#define	HIDE	298
#define	SHOW	299
#define	CHFORECOLOR	300
#define	CHBACKCOLOR	301
#define	CHCOLORSET	302
#define	GETVALUE	303
#define	GETFORE	304
#define	GETBACK	305
#define	GETHILIGHT	306
#define	GETSHADOW	307
#define	CHVALUE	308
#define	CHVALUEMAX	309
#define	CHVALUEMIN	310
#define	ADD	311
#define	DIV	312
#define	MULT	313
#define	GETTITLE	314
#define	GETOUTPUT	315
#define	STRCOPY	316
#define	NUMTOHEX	317
#define	HEXTONUM	318
#define	QUIT	319
#define	LAUNCHSCRIPT	320
#define	GETSCRIPTFATHER	321
#define	SENDTOSCRIPT	322
#define	RECEIVFROMSCRIPT	323
#define	GET	324
#define	SET	325
#define	SENDSIGN	326
#define	REMAINDEROFDIV	327
#define	GETTIME	328
#define	GETSCRIPTARG	329
#define	IF	330
#define	THEN	331
#define	ELSE	332
#define	FOR	333
#define	TO	334
#define	DO	335
#define	WHILE	336
#define	BEGF	337
#define	ENDF	338
#define	EQUAL	339
#define	INFEQ	340
#define	SUPEQ	341
#define	INF	342
#define	SUP	343
#define	DIFF	344

#line 1 "script.y"

#include "types.h"

#define MAX_VARS 512
extern int numligne;
ScriptProp *scriptprop;
int nbobj=-1;			/* Nombre d'objets */
int HasPosition,HasType=0;
TabObj *tabobj;		/* Tableau d'objets, limite=100 */
int TabIdObj[101]; 	/* Tableau d'indice des objets */
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
 scriptprop=(ScriptProp*) calloc(1,sizeof(ScriptProp));
 scriptprop->x=-1;
 scriptprop->y=-1;
 scriptprop->colorset = -1;
 scriptprop->initbloc=NULL;

 tabobj=(TabObj*) calloc(1,sizeof(TabObj));
 for (i=0;i<101;i++)
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
  TabIObj=(Bloc**)calloc(1,sizeof(long));
  TabCObj=(CaseObj*)calloc(1,sizeof(CaseObj));
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
  TabCObj[nbobj].LstCase=(int*)calloc(1,sizeof(int));
 else
  TabCObj[nbobj].LstCase=(int*)realloc(TabCObj[nbobj].LstCase,sizeof(int)*(CurrCase+1));
 TabCObj[nbobj].LstCase[CurrCase]=cond;

 if (CurrCase==0)
  TabIObj[nbobj]=(Bloc*)calloc(1,sizeof(Bloc));
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
  Temp=(long*)calloc(1,sizeof(long));
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
  PileBloc[TopPileB]->TabInstr=(Instr*)calloc(1,sizeof(Instr)*(CurrInstr+1));
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
  TabNVar=(char**)calloc(1,sizeof(long));
  TabVVar=(char**)calloc(1,sizeof(long));
 }
 else
 {
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=(char*)strdup(Name);
 TabVVar[NbVar]=(char*)calloc(1,sizeof(char));
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
  TabVVar=(char**)calloc(1,sizeof(long));
  TabNVar=(char**)calloc(1,sizeof(long));
 }
 else
 {
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=(char*)calloc(1,sizeof(char));
 TabNVar[NbVar][0]='\0';
 TabVVar[NbVar]=(char*)strdup(Name);

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
  l=(long*)calloc(1,sizeof(long));
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

 TmpBloc=(Bloc*)calloc(1,sizeof(Bloc));
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



#define	YYFINAL		414
#define	YYFLAG		-32768
#define	YYNTBASE	90

#define YYTRANSLATE(x) ((unsigned)(x) <= 344 ? yytranslate[x] : 153)

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
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
    66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
    76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
    86,    87,    88,    89
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
   548,   552,   554,   557,   560,   563,   566,   569,   570,   573,
   576,   579,   582,   585,   588,   594,   596,   598,   600,   602,
   604,   606,   611,   613,   615,   617,   619,   624,   626,   628,
   633,   635,   637,   642,   644,   646,   648,   650,   652,   654
};

static const short yyrhs[] = {    91,
    92,    93,    94,    95,     0,     0,     0,    92,     7,     4,
     0,    92,    30,     3,     0,    92,     9,     6,     6,     0,
    92,     8,     6,     6,     0,    92,    12,     4,     0,    92,
    11,     4,     0,    92,    13,     4,     0,    92,    14,     4,
     0,    92,    15,     6,     0,    92,    10,     3,     0,     0,
    17,   133,    40,   106,    20,     0,     0,    18,   133,    40,
   106,    20,     0,     0,    95,    16,    96,    21,    97,    99,
   100,     0,     6,     0,     0,    97,    22,     3,     0,    97,
    23,     6,     6,     0,    97,    24,     6,     6,     0,    97,
    25,     6,     0,    97,    26,     6,     0,    97,    27,     6,
     0,    97,    28,     4,     0,    97,    29,     4,     0,    97,
    30,     3,     0,    97,    12,     4,     0,    97,    11,     4,
     0,    97,    13,     4,     0,    97,    14,     4,     0,    97,
    15,     6,     0,    97,    10,     3,     0,    97,    31,    98,
     0,     0,    98,    34,     0,    98,    36,     0,    98,    35,
     0,     0,    20,     0,    19,   101,    37,   102,    20,     0,
     0,     0,   102,   103,    41,   105,     0,   102,   104,    41,
   105,     0,    38,     0,    39,     0,     6,     0,    40,   106,
    20,     0,     0,   106,    42,   108,     0,   106,    32,   125,
     0,   106,    33,   127,     0,   106,    43,   109,     0,   106,
    44,   110,     0,   106,    53,   111,     0,   106,    54,   112,
     0,   106,    55,   113,     0,   106,    24,   114,     0,   106,
    23,   115,     0,   106,    28,   117,     0,   106,    30,   116,
     0,   106,    10,   118,     0,   106,    45,   119,     0,   106,
    46,   120,     0,   106,    47,   121,     0,   106,    70,   122,
     0,   106,    71,   123,     0,   106,    64,   124,     0,   106,
    67,   126,     0,   106,    75,   128,     0,   106,    78,   129,
     0,   106,    81,   130,     0,    42,   108,     0,    32,   125,
     0,    33,   127,     0,    43,   109,     0,    44,   110,     0,
    53,   111,     0,    54,   112,     0,    55,   113,     0,    24,
   114,     0,    23,   115,     0,    28,   117,     0,    30,   116,
     0,    10,   118,     0,    45,   119,     0,    46,   120,     0,
    47,   121,     0,    70,   122,     0,    71,   123,     0,    64,
   124,     0,    67,   126,     0,    78,   129,     0,    81,   130,
     0,   144,   146,     0,   144,   148,     0,   144,   148,     0,
   144,   148,   144,   148,     0,   144,   148,   144,   148,     0,
   144,   148,   144,   148,     0,   144,   148,   144,   148,   144,
   148,     0,   144,   148,   144,   148,   144,   148,     0,   144,
   148,   144,   149,     0,   144,   148,   144,   150,     0,   144,
   148,   144,   149,     0,   144,   148,   144,   150,     0,   144,
   148,   144,   150,     0,   144,   148,   144,   148,     0,   144,
   151,    69,   144,   146,     0,   144,   148,   144,   148,     0,
     0,   144,   148,     0,   144,   148,   144,   146,     0,   144,
   149,   144,   146,     0,   131,   133,   134,   132,     0,   136,
   133,   135,     0,   137,   133,   135,     0,   144,   147,   144,
   152,   144,   147,    76,     0,     0,    77,   133,   135,     0,
     0,    40,   106,    20,     0,   107,     0,    40,   106,    20,
     0,   107,     0,   144,   151,    69,   144,   147,    79,   144,
   147,    80,     0,   144,   147,   144,   152,   144,   147,    80,
     0,     5,     0,     3,     0,     4,     0,     6,     0,    38,
     0,    39,     0,     0,    48,   148,     0,    59,   148,     0,
    60,   150,   148,   148,     0,    62,   148,   148,     0,    63,
   150,     0,    56,   148,   148,     0,    58,   148,   148,     0,
    57,   148,   148,     0,    61,   150,   148,   148,     0,    65,
   150,     0,    66,     0,    68,   148,     0,    72,   148,   148,
     0,    73,     0,    74,   148,     0,    49,   148,     0,    50,
   148,     0,    51,   148,     0,    52,   148,     0,     0,   142,
   146,     0,   143,   146,     0,   138,   146,     0,   140,   146,
     0,   139,   146,     0,   141,   146,     0,    82,   144,   145,
    83,   146,     0,   138,     0,   142,     0,   143,     0,   140,
     0,   139,     0,   141,     0,    82,   144,   145,    83,     0,
   142,     0,   143,     0,   141,     0,   138,     0,    82,   144,
   145,    83,     0,   138,     0,   139,     0,    82,   144,   145,
    83,     0,   138,     0,   140,     0,    82,   144,   145,    83,
     0,   138,     0,    87,     0,    85,     0,    84,     0,    86,
     0,    88,     0,    89,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   372,   375,   379,   380,   383,   386,   391,   396,   400,   404,
   408,   412,   415,   421,   422,   427,   428,   437,   438,   441,
   457,   458,   462,   466,   471,   474,   477,   480,   483,   486,
   489,   493,   497,   501,   505,   508,   511,   513,   514,   517,
   520,   526,   537,   538,   541,   543,   544,   545,   548,   549,
   552,   555,   560,   561,   562,   563,   564,   565,   566,   567,
   568,   569,   570,   571,   572,   573,   574,   575,   576,   577,
   578,   579,   580,   581,   582,   583,   587,   588,   589,   590,
   591,   592,   593,   594,   595,   596,   597,   598,   599,   600,
   601,   602,   603,   604,   605,   606,   607,   608,   611,   613,
   615,   617,   619,   621,   623,   625,   627,   629,   631,   633,
   635,   637,   639,   641,   643,   645,   647,   649,   651,   653,
   655,   659,   661,   662,   664,   666,   667,   670,   671,   675,
   679,   684,   686,   688,   690,   692,   694,   696,   698,   699,
   700,   701,   702,   703,   704,   705,   706,   707,   708,   709,
   710,   711,   712,   713,   714,   715,   716,   721,   722,   723,
   724,   725,   726,   727,   728,   733,   734,   735,   736,   737,
   738,   739,   743,   744,   745,   746,   747,   751,   752,   753,
   757,   758,   759,   763,   767,   768,   769,   770,   771,   772
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","STR","GSTR",
"VAR","NUMBER","WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FONT","FORECOLOR",
"BACKCOLOR","SHADCOLOR","LICOLOR","COLORSET","OBJECT","INIT","PERIODICTASK",
"MAIN","END","PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX","TITLE",
"SWALLOWEXEC","ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","CANBESELECTED","NORELIEFSTRING",
"CASE","SINGLECLIC","DOUBLECLIC","BEG","POINT","EXEC","HIDE","SHOW","CHFORECOLOR",
"CHBACKCOLOR","CHCOLORSET","GETVALUE","GETFORE","GETBACK","GETHILIGHT","GETSHADOW",
"CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT","GETTITLE","GETOUTPUT",
"STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT","GETSCRIPTFATHER","SENDTOSCRIPT",
"RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV","GETTIME","GETSCRIPTARG",
"IF","THEN","ELSE","FOR","TO","DO","WHILE","BEGF","ENDF","EQUAL","INFEQ","SUPEQ",
"INF","SUP","DIFF","script","initvar","head","initbloc","periodictask","object",
"id","init","flags","verify","mainloop","addtabcase","case","clic","number",
"bloc","instr","oneinstr","exec","hide","show","chvalue","chvaluemax","chvaluemin",
"position","size","icon","title","font","chforecolor","chbackcolor","chcolorset",
"set","sendsign","quit","warp","sendtoscript","writetofile","ifthenelse","loop",
"while","headif","else","creerbloc","bloc1","bloc2","headloop","headwhile","var",
"str","gstr","num","singleclic2","doubleclic2","addlbuff","function","args",
"arg","numarg","strarg","gstrarg","vararg","compare", NULL
};
#endif

static const short yyr1[] = {     0,
    90,    91,    92,    92,    92,    92,    92,    92,    92,    92,
    92,    92,    92,    93,    93,    94,    94,    95,    95,    96,
    97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
    97,    97,    97,    97,    97,    97,    97,    98,    98,    98,
    98,    99,   100,   100,   101,   102,   102,   102,   103,   103,
   104,   105,   106,   106,   106,   106,   106,   106,   106,   106,
   106,   106,   106,   106,   106,   106,   106,   106,   106,   106,
   106,   106,   106,   106,   106,   106,   107,   107,   107,   107,
   107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
   107,   107,   107,   107,   107,   107,   107,   107,   108,   109,
   110,   111,   112,   113,   114,   115,   116,   117,   118,   119,
   120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
   130,   131,   132,   132,   133,   134,   134,   135,   135,   136,
   137,   138,   139,   140,   141,   142,   143,   144,   145,   145,
   145,   145,   145,   145,   145,   145,   145,   145,   145,   145,
   145,   145,   145,   145,   145,   145,   145,   146,   146,   146,
   146,   146,   146,   146,   146,   147,   147,   147,   147,   147,
   147,   147,   148,   148,   148,   148,   148,   149,   149,   149,
   150,   150,   150,   151,   152,   152,   152,   152,   152,   152
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
     3,     1,     2,     2,     2,     2,     2,     0,     2,     2,
     2,     2,     2,     2,     5,     1,     1,     1,     1,     1,
     1,     4,     1,     1,     1,     1,     4,     1,     1,     4,
     1,     1,     4,     1,     1,     1,     1,     1,     1,     1
};

static const short yydefact[] = {     2,
     3,    14,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   125,     0,    16,     4,     0,     0,    13,     9,     8,
    10,    11,    12,     0,     5,   125,    18,     7,     6,    53,
     0,     1,     0,    53,     0,   138,    15,   138,   138,   138,
   138,   138,   138,   138,   138,   138,   138,   138,   138,   138,
   138,   138,   115,   138,   138,   138,   138,   138,   138,     0,
    20,     0,    66,     0,    63,     0,    62,     0,    64,     0,
    65,     0,    55,     0,    56,     0,    54,   158,    57,     0,
    58,     0,    67,     0,    68,     0,    69,     0,    59,     0,
    60,     0,    61,     0,    72,    73,     0,    70,     0,    71,
     0,    74,   125,     0,    75,   125,     0,    76,   125,     0,
    17,    21,   132,   135,   136,   137,   138,   176,   175,   173,
   174,   138,   138,   138,   138,   138,   116,   133,   138,   178,
   179,   138,   134,   138,   158,   158,   158,   158,   158,   158,
    99,   100,   101,   138,   138,   138,   138,   138,   138,   138,
   184,     0,   138,     0,   138,   166,   170,   169,   171,   167,
   168,   138,     0,     0,     0,   138,    42,     0,     0,     0,
     0,     0,     0,     0,   158,     0,   161,   163,   162,   164,
   159,   160,     0,     0,     0,     0,     0,     0,   158,   138,
     0,   138,   138,   138,   138,   138,   138,   138,    53,   138,
   138,   138,   138,   138,   138,   138,   138,   138,   115,   138,
   138,   138,   138,   138,   127,   123,     0,     0,    53,   129,
   120,   138,   121,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   149,     0,     0,   152,     0,
     0,   109,   138,   138,   138,   181,   182,   108,   107,     0,
   118,     0,   110,   111,   112,   102,   103,   104,   117,   158,
   114,    89,    86,    85,    87,    88,    78,    79,     0,    77,
    80,    81,    90,    91,    92,    82,    83,    84,    95,    96,
    93,    94,    97,    98,   125,   119,     0,   187,   186,   188,
   185,   189,   190,   138,     0,     0,   138,    36,    32,    31,
    33,    34,    35,    22,     0,     0,    25,    26,    27,    28,
    29,    30,    37,    45,    43,    19,   139,   154,   155,   156,
   157,     0,     0,     0,   140,     0,     0,     0,   143,   148,
   150,     0,   153,   177,     0,     0,     0,   180,   158,   113,
   126,     0,   172,     0,   128,     0,     0,    23,    24,    39,
    41,    40,     0,   144,   146,   145,     0,     0,   142,   151,
   106,   105,     0,   165,   124,     0,   138,     0,    46,   141,
   147,   183,   122,     0,   131,     0,     0,    51,    44,    49,
    50,     0,     0,   130,     0,     0,    53,    47,    48,     0,
    52,     0,     0,     0
};

static const short yydefgoto[] = {   412,
     1,     2,    14,    27,    32,    62,   167,   333,   241,   336,
   373,   396,   402,   403,   408,    33,   220,    77,    79,    81,
    89,    91,    93,    67,    65,    71,    69,    63,    83,    85,
    87,    98,   100,    95,    73,    96,    75,   102,   105,   108,
   103,   306,    24,   216,   221,   106,   109,   118,   136,   137,
   119,   120,   121,    64,   261,   141,   162,   122,   132,   268,
   152,   314
};

static const short yypact[] = {-32768,
-32768,   340,     4,    23,    30,    46,    49,    51,    59,    63,
    62,-32768,    68,    55,-32768,    73,    74,-32768,-32768,-32768,
-32768,-32768,-32768,    34,-32768,-32768,-32768,-32768,-32768,-32768,
    47,    75,   393,-32768,    80,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   549,
-32768,    71,-32768,    37,-32768,    37,-32768,    37,-32768,    37,
-32768,    37,-32768,    37,-32768,    45,-32768,     1,-32768,    37,
-32768,    37,-32768,    37,-32768,    37,-32768,    37,-32768,    37,
-32768,    37,-32768,    37,-32768,-32768,    37,-32768,    84,-32768,
    37,-32768,-32768,   279,-32768,-32768,    84,-32768,-32768,   279,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,     1,     1,     1,     1,     1,     1,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    35,-32768,   825,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   877,    40,   877,-32768,   621,   474,    45,    37,
    37,     8,    45,   474,     1,   474,-32768,-32768,-32768,-32768,
-32768,-32768,     8,     8,    37,    37,    37,    37,     1,-32768,
    37,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    29,   474,    70,-32768,-32768,
-32768,-32768,-32768,    70,   109,   124,   126,   133,   135,   137,
   138,   142,   143,   146,   154,   157,   136,   140,   161,-32768,
    27,    37,    37,    37,    37,    37,    37,    37,    37,    37,
     8,     8,    37,     8,     8,-32768,    37,    37,-32768,    37,
    85,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    92,
-32768,   108,-32768,-32768,-32768,-32768,-32768,-32768,-32768,     1,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   657,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   110,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   713,   279,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,   189,   190,-32768,-32768,-32768,-32768,
-32768,-32768,    60,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    37,    37,    37,-32768,    37,    37,    37,-32768,-32768,
-32768,    37,-32768,-32768,    37,    37,   474,-32768,     1,-32768,
-32768,   877,-32768,   279,-32768,   118,   279,-32768,-32768,-32768,
-32768,-32768,   163,-32768,-32768,-32768,    37,    37,-32768,-32768,
-32768,-32768,   120,-32768,-32768,   125,-32768,   128,-32768,-32768,
-32768,-32768,-32768,   279,-32768,    31,   130,-32768,-32768,-32768,
-32768,   165,   166,-32768,   164,   164,-32768,-32768,-32768,   769,
-32768,   211,   215,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,  -188,   -34,    65,    21,    22,    20,
    18,    28,    17,    33,    41,    32,    48,    44,    39,    42,
    43,    36,    56,    50,    52,    66,    77,-32768,    64,    72,
-32768,-32768,   -25,-32768,  -163,-32768,-32768,   -22,   -66,   -38,
   116,   127,   134,   -24,  -165,   -78,  -107,   318,  -128,  -119,
   172,    76
};


#define	YYLAST		958


static const short yytable[] = {    60,
    31,   223,   166,   128,   133,   113,   114,    15,   270,   131,
   272,   133,   113,    66,    68,    70,    72,    74,    76,    78,
    80,    82,    84,    86,    88,    90,    92,    94,    16,    97,
    99,   101,   104,   107,   110,    17,   398,   157,   115,   116,
   262,   113,   114,   157,   269,   334,   335,   128,    18,   113,
   399,   307,    19,   130,    20,   135,   177,   178,   179,   180,
   181,   182,    21,   273,   274,   158,    22,    23,   400,   401,
    25,   158,    26,    30,   115,   116,   151,   154,    28,    29,
   163,   156,   134,   165,   151,    61,    34,   156,   113,   265,
    35,   112,   168,   370,   371,   372,   271,   169,   170,   171,
   172,   173,   131,   190,   174,   305,   131,   175,   222,   176,
   279,   318,   135,   135,   135,   135,   135,   135,   117,   183,
   184,   185,   186,   187,   188,   189,   129,   319,   191,   320,
   217,   346,   347,   267,   349,   350,   321,   218,   322,   330,
   324,   224,   323,   331,   267,   267,   130,   325,   326,   266,
   130,   327,   135,   308,   309,   310,   311,   312,   313,   328,
   266,   266,   329,   332,   289,   280,   135,   354,    66,    68,
    70,    72,    74,    76,   358,    78,    80,    82,    84,    86,
    88,    90,    92,    94,   315,    97,    99,   101,   107,   110,
   359,   383,   363,   138,   368,   369,   387,   316,   385,   389,
   393,   360,   392,   407,   139,   405,   406,   395,   366,   404,
   413,   140,   267,   267,   414,   267,   267,   409,   215,   159,
   290,   292,   291,   296,   298,   159,   284,   286,   266,   266,
   160,   266,   266,   283,   297,   282,   160,   161,   355,   356,
   357,   293,   285,   161,     0,   294,   301,   295,   287,   157,
   138,   138,   138,   138,   138,   138,   386,   135,   299,   388,
     0,   139,   139,   139,   139,   139,   139,   302,   140,   140,
   140,   140,   140,   140,   288,   300,   303,   158,   164,   362,
   384,   128,   133,   113,   114,   304,   397,     0,     0,   364,
   138,     0,   367,   156,     0,     0,     0,   157,     0,   317,
   157,   139,     0,     0,   138,     0,     0,     0,   140,     0,
     0,     0,     0,     0,     0,   139,   115,   116,     0,     0,
     0,     0,   140,     0,     0,   158,     0,   157,   158,     0,
     0,     0,     0,     0,     0,     0,   135,     0,     0,     0,
     0,   156,     0,     0,   156,     0,     3,     4,     5,     6,
     7,     8,     9,    10,    11,   158,    12,     0,     0,     0,
   155,     0,   394,     0,     0,     0,     0,     0,     0,    13,
     0,   156,   410,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   123,     0,   124,     0,   125,     0,   126,
     0,   127,     0,     0,     0,   138,     0,   142,     0,   143,
     0,   144,    36,   145,     0,   146,   139,   147,     0,   148,
     0,   149,    37,   140,   150,    38,    39,     0,   153,     0,
    40,     0,    41,     0,    42,    43,     0,     0,     0,     0,
     0,   159,     0,     0,    44,    45,    46,    47,    48,    49,
     0,     0,   160,     0,     0,    50,    51,    52,     0,   161,
     0,     0,     0,     0,     0,     0,    53,     0,     0,    54,
     0,     0,    55,    56,     0,     0,     0,    57,     0,     0,
    58,     0,     0,    59,   138,     0,     0,     0,     0,   159,
     0,     0,   159,     0,     0,   139,     0,   263,   264,     0,
   160,     0,   140,   160,     0,     0,     0,   161,     0,     0,
   161,     0,   275,   276,   277,   278,     0,     0,   281,   159,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   160,   242,   243,   244,   245,   246,     0,   161,     0,   247,
   248,   249,   250,   251,   252,   253,   254,     0,   255,   256,
     0,   257,     0,     0,     0,   258,   259,   260,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    36,   337,
   338,   339,   340,   341,   342,   343,   344,   345,   111,     0,
   348,    38,    39,     0,   351,   352,    40,   353,    41,     0,
    42,    43,     0,     0,     0,     0,     0,     0,     0,     0,
    44,    45,    46,    47,    48,    49,     0,     0,     0,     0,
     0,    50,    51,    52,     0,     0,     0,     0,     0,     0,
     0,     0,    53,     0,     0,    54,     0,     0,    55,    56,
     0,     0,     0,    57,     0,     0,    58,     0,     0,    59,
   225,   226,   227,   228,   229,   230,     0,     0,     0,     0,
     0,     0,   231,   232,   233,   234,   235,   236,   237,   238,
   239,   240,     0,     0,     0,     0,     0,     0,     0,   374,
   375,   376,     0,   377,   378,   379,    36,     0,     0,   380,
     0,     0,   381,   382,     0,     0,   361,     0,     0,    38,
    39,     0,     0,     0,    40,     0,    41,     0,    42,    43,
     0,     0,     0,     0,   390,   391,     0,     0,    44,    45,
    46,    47,    48,    49,     0,     0,     0,     0,     0,    50,
    51,    52,     0,     0,     0,     0,     0,     0,     0,     0,
    53,     0,    36,    54,     0,     0,    55,    56,     0,     0,
     0,    57,   365,     0,    58,    38,    39,    59,     0,     0,
    40,     0,    41,     0,    42,    43,     0,     0,     0,     0,
     0,     0,     0,     0,    44,    45,    46,    47,    48,    49,
     0,     0,     0,     0,     0,    50,    51,    52,     0,     0,
     0,     0,     0,     0,     0,     0,    53,     0,    36,    54,
     0,     0,    55,    56,     0,     0,     0,    57,   411,     0,
    58,    38,    39,    59,     0,     0,    40,     0,    41,     0,
    42,    43,     0,     0,     0,     0,     0,     0,     0,     0,
    44,    45,    46,    47,    48,    49,     0,     0,     0,     0,
     0,    50,    51,    52,     0,     0,     0,     0,     0,     0,
     0,     0,    53,     0,   192,    54,     0,     0,    55,    56,
     0,     0,     0,    57,     0,     0,    58,   193,   194,    59,
     0,     0,   195,     0,   196,     0,   197,   198,     0,     0,
     0,     0,     0,     0,   199,     0,   200,   201,   202,   203,
   204,   205,     0,     0,     0,     0,     0,   206,   207,   208,
     0,     0,     0,     0,     0,     0,   192,     0,   209,     0,
     0,   210,     0,     0,   211,   212,     0,     0,     0,   193,
   194,     0,   213,     0,   195,   214,   196,     0,   197,   198,
     0,     0,     0,     0,     0,     0,   219,     0,   200,   201,
   202,   203,   204,   205,     0,     0,     0,     0,     0,   206,
   207,   208,     0,     0,     0,     0,     0,     0,     0,     0,
   209,     0,     0,   210,     0,     0,   211,   212,     0,     0,
     0,     0,     0,     0,   213,     0,     0,   214
};

static const short yycheck[] = {    34,
    26,   165,   110,     3,     4,     5,     6,     4,   174,    76,
   176,     4,     5,    38,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,     6,    54,
    55,    56,    57,    58,    59,     6,     6,   104,    38,    39,
   169,     5,     6,   110,   173,    19,    20,     3,     3,     5,
    20,   217,     4,    76,     4,    78,   135,   136,   137,   138,
   139,   140,     4,   183,   184,   104,     4,     6,    38,    39,
     3,   110,    18,    40,    38,    39,    99,   103,     6,     6,
   106,   104,    82,   109,   107,     6,    40,   110,     5,    82,
    16,    21,   117,    34,    35,    36,   175,   122,   123,   124,
   125,   126,   169,    69,   129,    77,   173,   132,    69,   134,
   189,     3,   135,   136,   137,   138,   139,   140,    82,   144,
   145,   146,   147,   148,   149,   150,    82,     4,   153,     4,
   155,   251,   252,   172,   254,   255,     4,   162,     4,     4,
     3,   166,     6,     4,   183,   184,   169,     6,     6,   172,
   173,     6,   175,    84,    85,    86,    87,    88,    89,     6,
   183,   184,     6,     3,   199,   190,   189,    83,   193,   194,
   195,   196,   197,   198,    83,   200,   201,   202,   203,   204,
   205,   206,   207,   208,   219,   210,   211,   212,   213,   214,
    83,   357,    83,    78,     6,     6,    79,   222,   362,    37,
    76,   280,    83,    40,    78,    41,    41,    80,   316,    80,
     0,    78,   251,   252,     0,   254,   255,   406,   154,   104,
   200,   202,   201,   206,   208,   110,   194,   196,   251,   252,
   104,   254,   255,   193,   207,   192,   110,   104,   263,   264,
   265,   203,   195,   110,    -1,   204,   211,   205,   197,   316,
   135,   136,   137,   138,   139,   140,   364,   280,   209,   367,
    -1,   135,   136,   137,   138,   139,   140,   212,   135,   136,
   137,   138,   139,   140,   198,   210,   213,   316,   107,   305,
   359,     3,     4,     5,     6,   214,   394,    -1,    -1,   314,
   175,    -1,   317,   316,    -1,    -1,    -1,   364,    -1,   224,
   367,   175,    -1,    -1,   189,    -1,    -1,    -1,   175,    -1,
    -1,    -1,    -1,    -1,    -1,   189,    38,    39,    -1,    -1,
    -1,    -1,   189,    -1,    -1,   364,    -1,   394,   367,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,   359,    -1,    -1,    -1,
    -1,   364,    -1,    -1,   367,    -1,     7,     8,     9,    10,
    11,    12,    13,    14,    15,   394,    17,    -1,    -1,    -1,
    82,    -1,   387,    -1,    -1,    -1,    -1,    -1,    -1,    30,
    -1,   394,   407,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    66,    -1,    68,    -1,    70,    -1,    72,
    -1,    74,    -1,    -1,    -1,   280,    -1,    80,    -1,    82,
    -1,    84,    10,    86,    -1,    88,   280,    90,    -1,    92,
    -1,    94,    20,   280,    97,    23,    24,    -1,   101,    -1,
    28,    -1,    30,    -1,    32,    33,    -1,    -1,    -1,    -1,
    -1,   316,    -1,    -1,    42,    43,    44,    45,    46,    47,
    -1,    -1,   316,    -1,    -1,    53,    54,    55,    -1,   316,
    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    -1,    67,
    -1,    -1,    70,    71,    -1,    -1,    -1,    75,    -1,    -1,
    78,    -1,    -1,    81,   359,    -1,    -1,    -1,    -1,   364,
    -1,    -1,   367,    -1,    -1,   359,    -1,   170,   171,    -1,
   364,    -1,   359,   367,    -1,    -1,    -1,   364,    -1,    -1,
   367,    -1,   185,   186,   187,   188,    -1,    -1,   191,   394,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   394,    48,    49,    50,    51,    52,    -1,   394,    -1,    56,
    57,    58,    59,    60,    61,    62,    63,    -1,    65,    66,
    -1,    68,    -1,    -1,    -1,    72,    73,    74,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    10,   242,
   243,   244,   245,   246,   247,   248,   249,   250,    20,    -1,
   253,    23,    24,    -1,   257,   258,    28,   260,    30,    -1,
    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,
    -1,    53,    54,    55,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    64,    -1,    -1,    67,    -1,    -1,    70,    71,
    -1,    -1,    -1,    75,    -1,    -1,    78,    -1,    -1,    81,
    10,    11,    12,    13,    14,    15,    -1,    -1,    -1,    -1,
    -1,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   342,
   343,   344,    -1,   346,   347,   348,    10,    -1,    -1,   352,
    -1,    -1,   355,   356,    -1,    -1,    20,    -1,    -1,    23,
    24,    -1,    -1,    -1,    28,    -1,    30,    -1,    32,    33,
    -1,    -1,    -1,    -1,   377,   378,    -1,    -1,    42,    43,
    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,    53,
    54,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    64,    -1,    10,    67,    -1,    -1,    70,    71,    -1,    -1,
    -1,    75,    20,    -1,    78,    23,    24,    81,    -1,    -1,
    28,    -1,    30,    -1,    32,    33,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    42,    43,    44,    45,    46,    47,
    -1,    -1,    -1,    -1,    -1,    53,    54,    55,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    64,    -1,    10,    67,
    -1,    -1,    70,    71,    -1,    -1,    -1,    75,    20,    -1,
    78,    23,    24,    81,    -1,    -1,    28,    -1,    30,    -1,
    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    42,    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,
    -1,    53,    54,    55,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    64,    -1,    10,    67,    -1,    -1,    70,    71,
    -1,    -1,    -1,    75,    -1,    -1,    78,    23,    24,    81,
    -1,    -1,    28,    -1,    30,    -1,    32,    33,    -1,    -1,
    -1,    -1,    -1,    -1,    40,    -1,    42,    43,    44,    45,
    46,    47,    -1,    -1,    -1,    -1,    -1,    53,    54,    55,
    -1,    -1,    -1,    -1,    -1,    -1,    10,    -1,    64,    -1,
    -1,    67,    -1,    -1,    70,    71,    -1,    -1,    -1,    23,
    24,    -1,    78,    -1,    28,    81,    30,    -1,    32,    33,
    -1,    -1,    -1,    -1,    -1,    -1,    40,    -1,    42,    43,
    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,    53,
    54,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    64,    -1,    -1,    67,    -1,    -1,    70,    71,    -1,    -1,
    -1,    -1,    -1,    -1,    78,    -1,    -1,    81
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
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

#ifndef YYPARSE_RETURN_TYPE
#define YYPARSE_RETURN_TYPE int
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
YYPARSE_RETURN_TYPE yyparse (void);
#endif

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
     int count;
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
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "/usr/share/bison.simple"

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

YYPARSE_RETURN_TYPE
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
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
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
				;
    break;}
case 13:
#line 415 "script.y"
{
				 scriptprop->font=yyvsp[0].str;
				;
    break;}
case 15:
#line 422 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 17:
#line 428 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 20:
#line 441 "script.y"
{ nbobj++;
				  if (nbobj>100)
				  { yyerror("Too many items\n");
				    exit(1);}
				  if ((yyvsp[0].number<1)||(yyvsp[0].number>100))
				  { yyerror("Choose item id between 1 and 100\n");
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
#line 458 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				;
    break;}
case 23:
#line 462 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				;
    break;}
case 24:
#line 466 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				;
    break;}
case 25:
#line 471 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				;
    break;}
case 26:
#line 474 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				;
    break;}
case 27:
#line 477 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				;
    break;}
case 28:
#line 480 "script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[0].str;
				;
    break;}
case 29:
#line 483 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				;
    break;}
case 30:
#line 486 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				;
    break;}
case 31:
#line 489 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 32:
#line 493 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 33:
#line 497 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 34:
#line 501 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 35:
#line 505 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				;
    break;}
case 36:
#line 508 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				;
    break;}
case 39:
#line 514 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				;
    break;}
case 40:
#line 517 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				;
    break;}
case 41:
#line 520 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				;
    break;}
case 42:
#line 526 "script.y"
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
#line 537 "script.y"
{ InitObjTabCase(0); ;
    break;}
case 45:
#line 541 "script.y"
{ InitObjTabCase(1); ;
    break;}
case 49:
#line 548 "script.y"
{ InitCase(-1); ;
    break;}
case 50:
#line 549 "script.y"
{ InitCase(-2); ;
    break;}
case 51:
#line 552 "script.y"
{ InitCase(yyvsp[0].number); ;
    break;}
case 99:
#line 611 "script.y"
{ AddCom(1,1); ;
    break;}
case 100:
#line 613 "script.y"
{ AddCom(2,1);;
    break;}
case 101:
#line 615 "script.y"
{ AddCom(3,1);;
    break;}
case 102:
#line 617 "script.y"
{ AddCom(4,2);;
    break;}
case 103:
#line 619 "script.y"
{ AddCom(21,2);;
    break;}
case 104:
#line 621 "script.y"
{ AddCom(22,2);;
    break;}
case 105:
#line 623 "script.y"
{ AddCom(5,3);;
    break;}
case 106:
#line 625 "script.y"
{ AddCom(6,3);;
    break;}
case 107:
#line 627 "script.y"
{ AddCom(7,2);;
    break;}
case 108:
#line 629 "script.y"
{ AddCom(8,2);;
    break;}
case 109:
#line 631 "script.y"
{ AddCom(9,2);;
    break;}
case 110:
#line 633 "script.y"
{ AddCom(10,2);;
    break;}
case 111:
#line 635 "script.y"
{ AddCom(19,2);;
    break;}
case 112:
#line 637 "script.y"
{ AddCom(24,2);;
    break;}
case 113:
#line 639 "script.y"
{ AddCom(11,2);;
    break;}
case 114:
#line 641 "script.y"
{ AddCom(12,2);;
    break;}
case 115:
#line 643 "script.y"
{ AddCom(13,0);;
    break;}
case 116:
#line 645 "script.y"
{ AddCom(17,1);;
    break;}
case 117:
#line 647 "script.y"
{ AddCom(23,2);;
    break;}
case 118:
#line 649 "script.y"
{ AddCom(18,2);;
    break;}
case 122:
#line 659 "script.y"
{ AddComBloc(14,3,2); ;
    break;}
case 125:
#line 664 "script.y"
{ EmpilerBloc(); ;
    break;}
case 126:
#line 666 "script.y"
{ DepilerBloc(2); ;
    break;}
case 127:
#line 667 "script.y"
{ DepilerBloc(2); ;
    break;}
case 128:
#line 670 "script.y"
{ DepilerBloc(1); ;
    break;}
case 129:
#line 671 "script.y"
{ DepilerBloc(1); ;
    break;}
case 130:
#line 675 "script.y"
{ AddComBloc(15,3,1); ;
    break;}
case 131:
#line 679 "script.y"
{ AddComBloc(16,3,1); ;
    break;}
case 132:
#line 684 "script.y"
{ AddVar(yyvsp[0].str); ;
    break;}
case 133:
#line 686 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 134:
#line 688 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 135:
#line 690 "script.y"
{ AddConstNum(yyvsp[0].number); ;
    break;}
case 136:
#line 692 "script.y"
{ AddConstNum(-1); ;
    break;}
case 137:
#line 694 "script.y"
{ AddConstNum(-2); ;
    break;}
case 138:
#line 696 "script.y"
{ AddLevelBufArg(); ;
    break;}
case 139:
#line 698 "script.y"
{ AddFunct(1,1); ;
    break;}
case 140:
#line 699 "script.y"
{ AddFunct(2,1); ;
    break;}
case 141:
#line 700 "script.y"
{ AddFunct(3,1); ;
    break;}
case 142:
#line 701 "script.y"
{ AddFunct(4,1); ;
    break;}
case 143:
#line 702 "script.y"
{ AddFunct(5,1); ;
    break;}
case 144:
#line 703 "script.y"
{ AddFunct(6,1); ;
    break;}
case 145:
#line 704 "script.y"
{ AddFunct(7,1); ;
    break;}
case 146:
#line 705 "script.y"
{ AddFunct(8,1); ;
    break;}
case 147:
#line 706 "script.y"
{ AddFunct(9,1); ;
    break;}
case 148:
#line 707 "script.y"
{ AddFunct(10,1); ;
    break;}
case 149:
#line 708 "script.y"
{ AddFunct(11,1); ;
    break;}
case 150:
#line 709 "script.y"
{ AddFunct(12,1); ;
    break;}
case 151:
#line 710 "script.y"
{ AddFunct(13,1); ;
    break;}
case 152:
#line 711 "script.y"
{ AddFunct(14,1); ;
    break;}
case 153:
#line 712 "script.y"
{ AddFunct(15,1); ;
    break;}
case 154:
#line 713 "script.y"
{ AddFunct(16,1); ;
    break;}
case 155:
#line 714 "script.y"
{ AddFunct(17,1); ;
    break;}
case 156:
#line 715 "script.y"
{ AddFunct(18,1); ;
    break;}
case 157:
#line 716 "script.y"
{ AddFunct(19,1); ;
    break;}
case 158:
#line 721 "script.y"
{ ;
    break;}
case 185:
#line 767 "script.y"
{ l=1-250000; AddBufArg(&l,1); ;
    break;}
case 186:
#line 768 "script.y"
{ l=2-250000; AddBufArg(&l,1); ;
    break;}
case 187:
#line 769 "script.y"
{ l=3-250000; AddBufArg(&l,1); ;
    break;}
case 188:
#line 770 "script.y"
{ l=4-250000; AddBufArg(&l,1); ;
    break;}
case 189:
#line 771 "script.y"
{ l=5-250000; AddBufArg(&l,1); ;
    break;}
case 190:
#line 772 "script.y"
{ l=6-250000; AddBufArg(&l,1); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "/usr/share/bison.simple"

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
}
#line 775 "script.y"

