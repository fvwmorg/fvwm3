
/*  A Bison parser, made from ../../../extras/FvwmScript/script.y
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
#define	GETVALUE	301
#define	CHVALUE	302
#define	CHVALUEMAX	303
#define	CHVALUEMIN	304
#define	ADD	305
#define	DIV	306
#define	MULT	307
#define	GETTITLE	308
#define	GETOUTPUT	309
#define	STRCOPY	310
#define	NUMTOHEX	311
#define	HEXTONUM	312
#define	QUIT	313
#define	LAUNCHSCRIPT	314
#define	GETSCRIPTFATHER	315
#define	SENDTOSCRIPT	316
#define	RECEIVFROMSCRIPT	317
#define	GET	318
#define	SET	319
#define	SENDSIGN	320
#define	REMAINDEROFDIV	321
#define	GETTIME	322
#define	GETSCRIPTARG	323
#define	IF	324
#define	THEN	325
#define	ELSE	326
#define	FOR	327
#define	TO	328
#define	DO	329
#define	WHILE	330
#define	BEGF	331
#define	ENDF	332
#define	EQUAL	333
#define	INFEQ	334
#define	SUPEQ	335
#define	INF	336
#define	SUP	337
#define	DIFF	338

#line 1 "../../../extras/FvwmScript/script.y"

#include "types.h"

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

 if (NbVar>58) 	
 {
  fprintf(stderr,"Line %d: too many variables (>60)\n",numligne);
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



#line 346 "../../../extras/FvwmScript/script.y"
typedef union {  char *str;
          int number;
       } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		446
#define	YYFLAG		-32768
#define	YYNTBASE	84

#define YYTRANSLATE(x) ((unsigned)(x) <= 338 ? yytranslate[x] : 146)

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
    76,    77,    78,    79,    80,    81,    82,    83
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     6,     7,     8,    12,    16,    21,    26,    30,    34,
    38,    42,    46,    47,    53,    54,    60,    61,    69,    71,
    72,    76,    81,    86,    90,    94,    98,   102,   106,   110,
   114,   118,   122,   126,   130,   134,   135,   138,   141,   144,
   145,   147,   153,   154,   155,   160,   165,   167,   169,   171,
   175,   176,   180,   184,   188,   192,   196,   200,   204,   208,
   212,   216,   220,   224,   228,   232,   236,   240,   244,   248,
   252,   256,   260,   264,   267,   270,   273,   276,   279,   282,
   285,   288,   291,   294,   297,   300,   303,   306,   309,   312,
   315,   318,   321,   324,   327,   330,   333,   336,   341,   346,
   351,   358,   365,   370,   375,   380,   385,   390,   396,   401,
   402,   405,   410,   415,   420,   424,   428,   436,   437,   441,
   442,   446,   448,   452,   454,   464,   472,   474,   476,   478,
   480,   482,   484,   485,   488,   491,   496,   500,   503,   507,
   511,   515,   520,   523,   525,   528,   532,   534,   537,   538,
   541,   544,   547,   550,   553,   556,   562,   564,   566,   568,
   570,   572,   574,   579,   581,   583,   585,   587,   592,   594,
   596,   601,   603,   605,   610,   612,   614,   616,   618,   620,
   622
};

static const short yyrhs[] = {    85,
    86,    87,    88,    89,     0,     0,     0,     7,     4,    86,
     0,    29,     3,    86,     0,     9,     6,     6,    86,     0,
     8,     6,     6,    86,     0,    12,     4,    86,     0,    11,
     4,    86,     0,    13,     4,    86,     0,    14,     4,    86,
     0,    10,     3,    86,     0,     0,    16,   126,    39,   100,
    19,     0,     0,    17,   126,    39,   100,    19,     0,     0,
    15,    90,    20,    91,    93,    94,    89,     0,     6,     0,
     0,    21,     3,    91,     0,    22,     6,     6,    91,     0,
    23,     6,     6,    91,     0,    24,     6,    91,     0,    25,
     6,    91,     0,    26,     6,    91,     0,    27,     4,    91,
     0,    28,     4,    91,     0,    29,     3,    91,     0,    12,
     4,    91,     0,    11,     4,    91,     0,    13,     4,    91,
     0,    14,     4,    91,     0,    10,     3,    91,     0,    30,
    92,    91,     0,     0,    33,    92,     0,    35,    92,     0,
    34,    92,     0,     0,    19,     0,    18,    95,    36,    96,
    19,     0,     0,     0,    97,    40,    99,    96,     0,    98,
    40,    99,    96,     0,    37,     0,    38,     0,     6,     0,
    39,   100,    19,     0,     0,    41,   102,   100,     0,    31,
   118,   100,     0,    32,   120,   100,     0,    42,   103,   100,
     0,    43,   104,   100,     0,    47,   105,   100,     0,    48,
   106,   100,     0,    49,   107,   100,     0,    23,   108,   100,
     0,    22,   109,   100,     0,    27,   111,   100,     0,    29,
   110,   100,     0,    10,   112,   100,     0,    44,   113,   100,
     0,    45,   114,   100,     0,    64,   115,   100,     0,    65,
   116,   100,     0,    58,   117,   100,     0,    61,   119,   100,
     0,    69,   121,   100,     0,    72,   122,   100,     0,    75,
   123,   100,     0,    41,   102,     0,    31,   118,     0,    32,
   120,     0,    42,   103,     0,    43,   104,     0,    47,   105,
     0,    48,   106,     0,    49,   107,     0,    23,   108,     0,
    22,   109,     0,    27,   111,     0,    29,   110,     0,    10,
   112,     0,    44,   113,     0,    45,   114,     0,    64,   115,
     0,    65,   116,     0,    58,   117,     0,    61,   119,     0,
    72,   122,     0,    75,   123,     0,   137,   139,     0,   137,
   141,     0,   137,   141,     0,   137,   141,   137,   141,     0,
   137,   141,   137,   141,     0,   137,   141,   137,   141,     0,
   137,   141,   137,   141,   137,   141,     0,   137,   141,   137,
   141,   137,   141,     0,   137,   141,   137,   142,     0,   137,
   141,   137,   143,     0,   137,   141,   137,   142,     0,   137,
   141,   137,   143,     0,   137,   141,   137,   143,     0,   137,
   144,    63,   137,   139,     0,   137,   141,   137,   141,     0,
     0,   137,   141,     0,   137,   141,   137,   139,     0,   137,
   142,   137,   139,     0,   124,   126,   127,   125,     0,   129,
   126,   128,     0,   130,   126,   128,     0,   137,   140,   137,
   145,   137,   140,    70,     0,     0,    71,   126,   128,     0,
     0,    39,   100,    19,     0,   101,     0,    39,   100,    19,
     0,   101,     0,   137,   144,    63,   137,   140,    73,   137,
   140,    74,     0,   137,   140,   137,   145,   137,   140,    74,
     0,     5,     0,     3,     0,     4,     0,     6,     0,    37,
     0,    38,     0,     0,    46,   141,     0,    53,   141,     0,
    54,   143,   141,   141,     0,    56,   141,   141,     0,    57,
   143,     0,    50,   141,   141,     0,    52,   141,   141,     0,
    51,   141,   141,     0,    55,   143,   141,   141,     0,    59,
   143,     0,    60,     0,    62,   141,     0,    66,   141,   141,
     0,    67,     0,    68,   141,     0,     0,   135,   139,     0,
   136,   139,     0,   131,   139,     0,   133,   139,     0,   132,
   139,     0,   134,   139,     0,    76,   137,   138,    77,   139,
     0,   131,     0,   135,     0,   136,     0,   133,     0,   132,
     0,   134,     0,    76,   137,   138,    77,     0,   135,     0,
   136,     0,   134,     0,   131,     0,    76,   137,   138,    77,
     0,   131,     0,   132,     0,    76,   137,   138,    77,     0,
   131,     0,   133,     0,    76,   137,   138,    77,     0,   131,
     0,    81,     0,    79,     0,    78,     0,    80,     0,    82,
     0,    83,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   369,   372,   376,   377,   380,   383,   388,   393,   396,   399,
   402,   405,   411,   412,   417,   418,   427,   428,   431,   446,
   447,   451,   455,   460,   463,   466,   469,   472,   475,   478,
   481,   484,   487,   490,   493,   495,   496,   499,   502,   508,
   519,   520,   523,   525,   526,   527,   530,   531,   534,   537,
   542,   543,   544,   545,   546,   547,   548,   549,   550,   551,
   552,   553,   554,   555,   556,   557,   558,   559,   560,   561,
   562,   563,   564,   568,   569,   570,   571,   572,   573,   574,
   575,   576,   577,   578,   579,   580,   581,   582,   583,   584,
   585,   586,   587,   588,   591,   593,   595,   597,   599,   601,
   603,   605,   607,   609,   611,   613,   615,   617,   619,   621,
   623,   625,   627,   629,   631,   633,   637,   639,   640,   642,
   644,   645,   648,   649,   653,   657,   662,   664,   666,   668,
   670,   672,   674,   676,   677,   678,   679,   680,   681,   682,
   683,   684,   685,   686,   687,   688,   689,   690,   695,   696,
   697,   698,   699,   700,   701,   702,   707,   708,   709,   710,
   711,   712,   713,   717,   718,   719,   720,   721,   725,   726,
   727,   731,   732,   733,   737,   741,   742,   743,   744,   745,
   746
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","STR","GSTR",
"VAR","NUMBER","WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FONT","FORECOLOR",
"BACKCOLOR","SHADCOLOR","LICOLOR","OBJECT","INIT","PERIODICTASK","MAIN","END",
"PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX","TITLE","SWALLOWEXEC",
"ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","CANBESELECTED","NORELIEFSTRING",
"CASE","SINGLECLIC","DOUBLECLIC","BEG","POINT","EXEC","HIDE","SHOW","CHFORECOLOR",
"CHBACKCOLOR","GETVALUE","CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT",
"GETTITLE","GETOUTPUT","STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT",
"GETSCRIPTFATHER","SENDTOSCRIPT","RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV",
"GETTIME","GETSCRIPTARG","IF","THEN","ELSE","FOR","TO","DO","WHILE","BEGF","ENDF",
"EQUAL","INFEQ","SUPEQ","INF","SUP","DIFF","script","initvar","head","initbloc",
"periodictask","object","id","init","flags","verify","mainloop","addtabcase",
"case","clic","number","bloc","instr","oneinstr","exec","hide","show","chvalue",
"chvaluemax","chvaluemin","position","size","icon","title","font","chforecolor",
"chbackcolor","set","sendsign","quit","warp","sendtoscript","writetofile","ifthenelse",
"loop","while","headif","else","creerbloc","bloc1","bloc2","headloop","headwhile",
"var","str","gstr","num","singleclic2","doubleclic2","addlbuff","function","args",
"arg","numarg","strarg","gstrarg","vararg","compare", NULL
};
#endif

static const short yyr1[] = {     0,
    84,    85,    86,    86,    86,    86,    86,    86,    86,    86,
    86,    86,    87,    87,    88,    88,    89,    89,    90,    91,
    91,    91,    91,    91,    91,    91,    91,    91,    91,    91,
    91,    91,    91,    91,    91,    92,    92,    92,    92,    93,
    94,    94,    95,    96,    96,    96,    97,    97,    98,    99,
   100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
   100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
   100,   100,   100,   101,   101,   101,   101,   101,   101,   101,
   101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
   101,   101,   101,   101,   102,   103,   104,   105,   106,   107,
   108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
   118,   119,   120,   121,   122,   123,   124,   125,   125,   126,
   127,   127,   128,   128,   129,   130,   131,   132,   133,   134,
   135,   136,   137,   138,   138,   138,   138,   138,   138,   138,
   138,   138,   138,   138,   138,   138,   138,   138,   139,   139,
   139,   139,   139,   139,   139,   139,   140,   140,   140,   140,
   140,   140,   140,   141,   141,   141,   141,   141,   142,   142,
   142,   143,   143,   143,   144,   145,   145,   145,   145,   145,
   145
};

static const short yyr2[] = {     0,
     5,     0,     0,     3,     3,     4,     4,     3,     3,     3,
     3,     3,     0,     5,     0,     5,     0,     7,     1,     0,
     3,     4,     4,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     0,     2,     2,     2,     0,
     1,     5,     0,     0,     4,     4,     1,     1,     1,     3,
     0,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     4,     4,     4,
     6,     6,     4,     4,     4,     4,     4,     5,     4,     0,
     2,     4,     4,     4,     3,     3,     7,     0,     3,     0,
     3,     1,     3,     1,     9,     7,     1,     1,     1,     1,
     1,     1,     0,     2,     2,     4,     3,     2,     3,     3,
     3,     4,     2,     1,     2,     3,     1,     2,     0,     2,
     2,     2,     2,     2,     2,     5,     1,     1,     1,     1,
     1,     1,     4,     1,     1,     1,     1,     4,     1,     1,
     4,     1,     1,     4,     1,     1,     1,     1,     1,     1,
     1
};

static const short yydefact[] = {     2,
     3,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    13,     3,     0,     0,     3,     3,     3,     3,     3,     3,
   120,    15,     4,     3,     3,    12,     9,     8,    10,    11,
     5,     0,   120,    17,     7,     6,    51,     0,     0,     1,
   133,   133,   133,   133,   133,   133,   133,   133,   133,   133,
   133,   133,   133,   133,   133,   110,   133,   133,   133,   133,
   133,   133,     0,    51,    19,     0,    51,     0,    51,     0,
    51,     0,    51,     0,    51,     0,    51,     0,    51,     0,
    51,   149,    51,     0,    51,     0,    51,     0,    51,     0,
    51,     0,    51,     0,    51,     0,    51,    51,     0,    51,
     0,    51,     0,    51,   120,     0,    51,   120,     0,    51,
   120,     0,    14,     0,    20,    64,   127,   130,   131,   132,
   133,   167,   166,   164,   165,   133,    61,   133,    60,   133,
    62,   133,    63,   133,    53,   111,    54,   128,   133,   169,
   170,   133,    52,   129,   133,   149,   149,   149,   149,   149,
   149,    95,    55,    96,    56,    97,    65,   133,    66,   133,
    57,   133,    58,   133,    59,   133,    69,    70,   133,    67,
   175,     0,    68,   133,    71,     0,   133,   157,   161,   160,
   162,   158,   159,   133,    72,     0,     0,    73,     0,   133,
    16,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    36,    40,     0,     0,     0,
     0,     0,     0,     0,   149,     0,   152,   154,   153,   155,
   150,   151,     0,     0,     0,     0,     0,   149,   133,     0,
   133,   133,   133,   133,   133,   133,   133,    51,   133,   133,
   133,   133,   133,   133,   133,   133,   110,   133,   133,   133,
   133,   133,   122,   118,     0,     0,    51,   124,   115,   133,
   116,     0,    20,    20,    20,    20,    20,    20,     0,     0,
    20,    20,    20,    20,    20,    20,    36,    36,    36,    20,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   144,     0,     0,   147,     0,     0,   105,   133,   133,
   133,   172,   173,   104,   103,     0,   113,     0,   106,   107,
    98,    99,   100,   112,   149,   109,    86,    83,    82,    84,
    85,    75,    76,     0,    74,    77,    78,    87,    88,    79,
    80,    81,    91,    92,    89,    90,    93,    94,   120,   114,
     0,   178,   177,   179,   176,   180,   181,   133,     0,     0,
   133,    34,    31,    30,    32,    33,    21,    20,    20,    24,
    25,    26,    27,    28,    29,    37,    39,    38,    35,    43,
    41,    17,   134,     0,     0,     0,   135,     0,     0,     0,
   138,   143,   145,     0,   148,   168,     0,     0,     0,   171,
   149,   108,   121,     0,   163,     0,   123,     0,     0,    22,
    23,     0,    18,   139,   141,   140,     0,     0,   137,   146,
   102,   101,     0,   156,   119,     0,   133,     0,    44,   136,
   142,   174,   117,     0,   126,    49,    47,    48,     0,     0,
     0,     0,    42,     0,     0,   125,    51,    44,    44,     0,
    45,    46,    50,     0,     0,     0
};

static const short yydefgoto[] = {   444,
     1,    11,    22,    34,    40,    66,   207,   280,   281,   372,
   402,   429,   430,   431,   438,    63,   258,    81,    83,    85,
    91,    93,    95,    71,    69,    75,    73,    67,    87,    89,
   100,   102,    97,    77,    98,    79,   104,   107,   110,   105,
   340,    32,   254,   259,   108,   111,   122,   147,   148,   123,
   124,   125,    68,   297,   152,   184,   126,   142,   304,   172,
   348
};

static const short yypact[] = {-32768,
   555,    37,    29,    39,    25,    45,    47,    51,    53,    40,
    43,   555,    55,    57,   555,   555,   555,   555,   555,   555,
-32768,    49,-32768,   555,   555,-32768,-32768,-32768,-32768,-32768,
-32768,    36,-32768,    61,-32768,-32768,   465,    46,    62,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    68,   465,-32768,    50,   465,   139,   465,   139,
   465,   139,   465,   139,   465,   139,   465,   139,   465,    19,
   465,    77,   465,   139,   465,   139,   465,   139,   465,   139,
   465,   139,   465,   139,   465,   139,   465,   465,   139,   465,
    84,   465,   139,   465,-32768,   262,   465,-32768,    84,   465,
-32768,   262,-32768,    72,   616,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    77,    77,    77,    77,    77,
    77,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    30,-32768,-32768,-32768,   560,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,   663,    33,-32768,   663,-32768,
-32768,    91,    97,   108,   109,   115,   118,   120,   122,   125,
   128,   134,   142,   145,   149,    76,-32768,   492,    19,   139,
   139,    22,    19,   492,    77,   492,-32768,-32768,-32768,-32768,
-32768,-32768,    22,    22,   139,   139,   139,    77,-32768,   139,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   465,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,    80,   492,   148,   465,-32768,-32768,-32768,
-32768,   148,   616,   616,   616,   616,   616,   616,   156,   163,
   616,   616,   616,   616,   616,   616,    76,    76,    76,   616,
    14,   139,   139,   139,   139,   139,    22,    22,   139,    22,
    22,-32768,   139,   139,-32768,   139,    78,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,   111,-32768,   112,-32768,-32768,
-32768,-32768,-32768,-32768,    77,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,   177,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   136,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   195,   262,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   616,   616,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    61,-32768,   139,   139,   139,-32768,   139,   139,   139,
-32768,-32768,-32768,   139,-32768,-32768,   139,   139,   492,-32768,
    77,-32768,-32768,   663,-32768,   262,-32768,   143,   262,-32768,
-32768,   181,-32768,-32768,-32768,-32768,   139,   139,-32768,-32768,
-32768,-32768,   146,-32768,-32768,   150,-32768,   160,   101,-32768,
-32768,-32768,-32768,   262,-32768,-32768,-32768,-32768,   221,   202,
   204,   172,-32768,   210,   210,-32768,   465,   101,   101,   232,
-32768,-32768,-32768,   253,   256,-32768
};

static const short yypgoto[] = {-32768,
-32768,   278,-32768,-32768,  -112,-32768,   197,   -99,-32768,-32768,
-32768,  -366,-32768,-32768,  -173,   -33,    88,    32,    34,    28,
    26,    27,    38,    48,    41,    52,    58,    70,    63,    65,
    64,    54,    69,    81,    75,    83,-32768,    73,    74,-32768,
-32768,    -3,-32768,  -175,-32768,-32768,    24,   -59,    31,    17,
    35,   129,   -42,  -177,    10,   -89,   292,  -184,    -2,   173,
    21
};


#define	YYLAST		738


static const short yytable[] = {    70,
    72,    74,    76,    78,    80,    82,    84,    86,    88,    90,
    92,    94,    96,   261,    99,   101,   103,   106,   109,   112,
   141,   138,   190,   117,   298,   144,   117,    15,   305,    38,
   114,   370,   371,   116,    13,   127,   306,   129,   308,   131,
    12,   133,    20,   135,    14,   137,   179,   143,    16,   153,
    17,   155,   179,   157,    18,   159,    19,   161,    21,   163,
    24,   165,    25,   167,   168,    33,   170,    65,   173,   115,
   175,   441,   442,   185,    37,    39,   188,   341,   208,   138,
   144,   117,   118,   209,    64,   210,   113,   211,   117,   212,
   191,   213,   229,   263,   139,   260,   214,   301,   149,   215,
   264,   176,   216,   140,   186,   146,   426,   189,   277,   278,
   279,   265,   266,   119,   120,   223,   150,   224,   267,   225,
   268,   226,   181,   227,   171,   269,   228,   270,   181,   178,
   271,   230,   171,   272,   255,   178,   180,   427,   428,   273,
   182,   256,   180,   117,   118,   274,   182,   262,   275,   141,
   339,   276,   145,   141,   386,   217,   218,   219,   220,   221,
   222,   358,   149,   149,   149,   149,   149,   149,   359,   146,
   146,   146,   146,   146,   146,   119,   120,   366,   367,   368,
   150,   150,   150,   150,   150,   150,   315,   390,   391,    70,
    72,    74,    76,    78,    80,   393,    82,    84,    86,    88,
    90,    92,    94,    96,   324,    99,   101,   103,   109,   112,
   151,   413,   395,   397,   121,   417,   419,   350,   415,   423,
   309,   310,   422,   349,   307,   342,   343,   344,   345,   346,
   347,   149,   140,   425,   183,   302,   140,   314,   146,   433,
   183,   434,   303,   435,   149,   436,   302,   302,   437,   150,
   443,   146,   445,   303,   303,   446,   387,   388,   389,   403,
   398,   439,   150,   253,   138,   144,   117,   118,   327,   330,
   325,   331,   318,   326,   151,   151,   151,   151,   151,   151,
   319,   187,   351,   332,   378,   379,   321,   381,   382,    23,
   179,   320,    26,    27,    28,    29,    30,    31,   119,   120,
   317,    35,    36,   336,   328,   396,   416,   329,   399,   418,
   302,   302,   335,   302,   302,   333,   322,   303,   303,   323,
   303,   303,   334,   337,   392,   338,     0,     0,     0,     0,
     0,   149,     0,     0,   432,   394,   179,   177,   146,   179,
     0,     0,     0,   151,     0,     0,     0,     0,     0,   150,
     0,     0,     0,     0,     0,     0,   151,     0,     0,     0,
     0,   128,     0,   130,   179,   132,   181,   134,     0,   136,
     0,     0,     0,   178,   424,   154,     0,   156,     0,   158,
   180,   160,     0,   162,   182,   164,     0,   166,     0,     0,
   169,     0,     0,     0,   174,     0,     0,     0,     0,     0,
   414,     0,     0,   440,     0,     0,     0,   149,     0,     0,
     0,     0,   181,     0,   146,   181,     0,     0,     0,   178,
     0,     0,   178,     0,     0,   150,   180,     0,     0,   180,
   182,     0,     0,   182,     0,     0,     0,     0,     0,     0,
   181,     0,     0,   151,     0,     0,     0,   178,     0,     0,
     0,     0,     0,     0,   180,     0,     0,     0,   182,   352,
   353,   354,   355,   356,   357,     0,     0,   360,   361,   362,
   363,   364,   365,     0,    41,     0,   369,     0,   183,     0,
     0,     0,     0,     0,     0,     0,    42,    43,     0,     0,
     0,    44,     0,    45,     0,    46,    47,     0,     0,     0,
     0,   299,   300,     0,     0,    48,    49,    50,    51,    52,
     0,    53,    54,    55,     0,     0,   311,   312,   313,   151,
     0,   316,    56,     0,   183,    57,     0,   183,    58,    59,
     0,     0,     0,    60,     0,     0,    61,   282,     0,    62,
     0,   283,   284,   285,   286,   287,   288,   289,   290,     0,
   291,   292,   183,   293,   400,   401,     0,   294,   295,   296,
     0,     2,     3,     4,     5,     6,     7,     8,     9,   231,
     0,     0,     0,   373,   374,   375,   376,   377,     0,     0,
   380,   232,   233,    10,   383,   384,   234,   385,   235,     0,
   236,   237,     0,     0,     0,     0,     0,     0,   238,     0,
   239,   240,   241,   242,   243,     0,   244,   245,   246,     0,
     0,     0,     0,     0,     0,     0,     0,   247,     0,     0,
   248,     0,     0,   249,   250,   192,   193,   194,   195,   196,
     0,   251,     0,     0,   252,     0,   197,   198,   199,   200,
   201,   202,   203,   204,   205,   206,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   404,   405,   406,     0,   407,
   408,   409,   231,     0,     0,   410,     0,     0,   411,   412,
     0,     0,     0,     0,   232,   233,     0,     0,     0,   234,
     0,   235,     0,   236,   237,     0,     0,     0,   420,   421,
     0,   257,     0,   239,   240,   241,   242,   243,     0,   244,
   245,   246,     0,     0,     0,     0,     0,     0,     0,     0,
   247,     0,     0,   248,     0,     0,   249,   250,     0,     0,
     0,     0,     0,     0,   251,     0,     0,   252
};

static const short yycheck[] = {    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,   189,    57,    58,    59,    60,    61,    62,
    80,     3,   112,     5,   209,     4,     5,     3,   213,    33,
    64,    18,    19,    67,     6,    69,   214,    71,   216,    73,
     4,    75,     3,    77,     6,    79,   106,    81,     4,    83,
     4,    85,   112,    87,     4,    89,     4,    91,    16,    93,
     6,    95,     6,    97,    98,    17,   100,     6,   102,    20,
   104,   438,   439,   107,    39,    15,   110,   255,   121,     3,
     4,     5,     6,   126,    39,   128,    19,   130,     5,   132,
    19,   134,    63,     3,    76,    63,   139,    76,    82,   142,
     4,   105,   145,    80,   108,    82,     6,   111,    33,    34,
    35,     4,     4,    37,    38,   158,    82,   160,     4,   162,
     3,   164,   106,   166,   101,     6,   169,     6,   112,   106,
     6,   174,   109,     6,   177,   112,   106,    37,    38,     6,
   106,   184,   112,     5,     6,     4,   112,   190,     4,   209,
    71,     3,    76,   213,    77,   146,   147,   148,   149,   150,
   151,     6,   146,   147,   148,   149,   150,   151,     6,   146,
   147,   148,   149,   150,   151,    37,    38,   277,   278,   279,
   146,   147,   148,   149,   150,   151,   229,    77,    77,   232,
   233,   234,   235,   236,   237,    19,   239,   240,   241,   242,
   243,   244,   245,   246,   238,   248,   249,   250,   251,   252,
    82,   389,    77,    19,    76,    73,    36,   260,   394,    70,
   223,   224,    77,   257,   215,    78,    79,    80,    81,    82,
    83,   215,   209,    74,   106,   212,   213,   228,   215,    19,
   112,    40,   212,    40,   228,    74,   223,   224,    39,   215,
    19,   228,     0,   223,   224,     0,   299,   300,   301,   372,
   350,   435,   228,   176,     3,     4,     5,     6,   241,   244,
   239,   245,   232,   240,   146,   147,   148,   149,   150,   151,
   233,   109,   262,   246,   287,   288,   235,   290,   291,    12,
   350,   234,    15,    16,    17,    18,    19,    20,    37,    38,
   231,    24,    25,   250,   242,   348,   396,   243,   351,   399,
   287,   288,   249,   290,   291,   247,   236,   287,   288,   237,
   290,   291,   248,   251,   315,   252,    -1,    -1,    -1,    -1,
    -1,   315,    -1,    -1,   424,   339,   396,    76,   315,   399,
    -1,    -1,    -1,   215,    -1,    -1,    -1,    -1,    -1,   315,
    -1,    -1,    -1,    -1,    -1,    -1,   228,    -1,    -1,    -1,
    -1,    70,    -1,    72,   424,    74,   350,    76,    -1,    78,
    -1,    -1,    -1,   350,   417,    84,    -1,    86,    -1,    88,
   350,    90,    -1,    92,   350,    94,    -1,    96,    -1,    -1,
    99,    -1,    -1,    -1,   103,    -1,    -1,    -1,    -1,    -1,
   391,    -1,    -1,   437,    -1,    -1,    -1,   391,    -1,    -1,
    -1,    -1,   396,    -1,   391,   399,    -1,    -1,    -1,   396,
    -1,    -1,   399,    -1,    -1,   391,   396,    -1,    -1,   399,
   396,    -1,    -1,   399,    -1,    -1,    -1,    -1,    -1,    -1,
   424,    -1,    -1,   315,    -1,    -1,    -1,   424,    -1,    -1,
    -1,    -1,    -1,    -1,   424,    -1,    -1,    -1,   424,   263,
   264,   265,   266,   267,   268,    -1,    -1,   271,   272,   273,
   274,   275,   276,    -1,    10,    -1,   280,    -1,   350,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    22,    23,    -1,    -1,
    -1,    27,    -1,    29,    -1,    31,    32,    -1,    -1,    -1,
    -1,   210,   211,    -1,    -1,    41,    42,    43,    44,    45,
    -1,    47,    48,    49,    -1,    -1,   225,   226,   227,   391,
    -1,   230,    58,    -1,   396,    61,    -1,   399,    64,    65,
    -1,    -1,    -1,    69,    -1,    -1,    72,    46,    -1,    75,
    -1,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
    59,    60,   424,    62,   358,   359,    -1,    66,    67,    68,
    -1,     7,     8,     9,    10,    11,    12,    13,    14,    10,
    -1,    -1,    -1,   282,   283,   284,   285,   286,    -1,    -1,
   289,    22,    23,    29,   293,   294,    27,   296,    29,    -1,
    31,    32,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
    41,    42,    43,    44,    45,    -1,    47,    48,    49,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    58,    -1,    -1,
    61,    -1,    -1,    64,    65,    10,    11,    12,    13,    14,
    -1,    72,    -1,    -1,    75,    -1,    21,    22,    23,    24,
    25,    26,    27,    28,    29,    30,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,   374,   375,   376,    -1,   378,
   379,   380,    10,    -1,    -1,   384,    -1,    -1,   387,   388,
    -1,    -1,    -1,    -1,    22,    23,    -1,    -1,    -1,    27,
    -1,    29,    -1,    31,    32,    -1,    -1,    -1,   407,   408,
    -1,    39,    -1,    41,    42,    43,    44,    45,    -1,    47,
    48,    49,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    58,    -1,    -1,    61,    -1,    -1,    64,    65,    -1,    -1,
    -1,    -1,    -1,    -1,    72,    -1,    -1,    75
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

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
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
#line 372 "../../../extras/FvwmScript/script.y"
{ InitVarGlob(); ;
    break;}
case 4:
#line 377 "../../../extras/FvwmScript/script.y"
{		/* Titre de la fenetre */
				 scriptprop->titlewin=yyvsp[-1].str;
				;
    break;}
case 5:
#line 380 "../../../extras/FvwmScript/script.y"
{
				 scriptprop->icon=yyvsp[-1].str;
				;
    break;}
case 6:
#line 384 "../../../extras/FvwmScript/script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->x=yyvsp[-2].number;
				 scriptprop->y=yyvsp[-1].number;
				;
    break;}
case 7:
#line 389 "../../../extras/FvwmScript/script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->width=yyvsp[-2].number;
				 scriptprop->height=yyvsp[-1].number;
				;
    break;}
case 8:
#line 393 "../../../extras/FvwmScript/script.y"
{ 		/* Couleur de fond */
				 scriptprop->backcolor=yyvsp[-1].str;
				;
    break;}
case 9:
#line 396 "../../../extras/FvwmScript/script.y"
{ 		/* Couleur des lignes */
				 scriptprop->forecolor=yyvsp[-1].str;
				;
    break;}
case 10:
#line 399 "../../../extras/FvwmScript/script.y"
{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=yyvsp[-1].str;
				;
    break;}
case 11:
#line 402 "../../../extras/FvwmScript/script.y"
{ 		/* Couleur des lignes */
				 scriptprop->licolor=yyvsp[-1].str;
				;
    break;}
case 12:
#line 405 "../../../extras/FvwmScript/script.y"
{
				 scriptprop->font=yyvsp[-1].str;
				;
    break;}
case 14:
#line 412 "../../../extras/FvwmScript/script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 16:
#line 418 "../../../extras/FvwmScript/script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 19:
#line 431 "../../../extras/FvwmScript/script.y"
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
				;
    break;}
case 21:
#line 447 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[-1].str;
				 HasType=1;
				;
    break;}
case 22:
#line 451 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-2].number;
				 (*tabobj)[nbobj].height=yyvsp[-1].number;
				;
    break;}
case 23:
#line 455 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-2].number;
				 (*tabobj)[nbobj].y=yyvsp[-1].number;
				 HasPosition=1;
				;
    break;}
case 24:
#line 460 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[-1].number;
				;
    break;}
case 25:
#line 463 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[-1].number;
				;
    break;}
case 26:
#line 466 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[-1].number;
				;
    break;}
case 27:
#line 469 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[-1].str;
				;
    break;}
case 28:
#line 472 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[-1].str;
				;
    break;}
case 29:
#line 475 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[-1].str;
				;
    break;}
case 30:
#line 478 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[-1].str;
				;
    break;}
case 31:
#line 481 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[-1].str;
				;
    break;}
case 32:
#line 484 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[-1].str;
				;
    break;}
case 33:
#line 487 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].licolor=yyvsp[-1].str;
				;
    break;}
case 34:
#line 490 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[-1].str;
				;
    break;}
case 37:
#line 496 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				;
    break;}
case 38:
#line 499 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				;
    break;}
case 39:
#line 502 "../../../extras/FvwmScript/script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				;
    break;}
case 40:
#line 508 "../../../extras/FvwmScript/script.y"
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
case 41:
#line 519 "../../../extras/FvwmScript/script.y"
{ InitObjTabCase(0); ;
    break;}
case 43:
#line 523 "../../../extras/FvwmScript/script.y"
{ InitObjTabCase(1); ;
    break;}
case 47:
#line 530 "../../../extras/FvwmScript/script.y"
{ InitCase(-1); ;
    break;}
case 48:
#line 531 "../../../extras/FvwmScript/script.y"
{ InitCase(-2); ;
    break;}
case 49:
#line 534 "../../../extras/FvwmScript/script.y"
{ InitCase(yyvsp[0].number); ;
    break;}
case 95:
#line 591 "../../../extras/FvwmScript/script.y"
{ AddCom(1,1); ;
    break;}
case 96:
#line 593 "../../../extras/FvwmScript/script.y"
{ AddCom(2,1);;
    break;}
case 97:
#line 595 "../../../extras/FvwmScript/script.y"
{ AddCom(3,1);;
    break;}
case 98:
#line 597 "../../../extras/FvwmScript/script.y"
{ AddCom(4,2);;
    break;}
case 99:
#line 599 "../../../extras/FvwmScript/script.y"
{ AddCom(21,2);;
    break;}
case 100:
#line 601 "../../../extras/FvwmScript/script.y"
{ AddCom(22,2);;
    break;}
case 101:
#line 603 "../../../extras/FvwmScript/script.y"
{ AddCom(5,3);;
    break;}
case 102:
#line 605 "../../../extras/FvwmScript/script.y"
{ AddCom(6,3);;
    break;}
case 103:
#line 607 "../../../extras/FvwmScript/script.y"
{ AddCom(7,2);;
    break;}
case 104:
#line 609 "../../../extras/FvwmScript/script.y"
{ AddCom(8,2);;
    break;}
case 105:
#line 611 "../../../extras/FvwmScript/script.y"
{ AddCom(9,2);;
    break;}
case 106:
#line 613 "../../../extras/FvwmScript/script.y"
{ AddCom(10,2);;
    break;}
case 107:
#line 615 "../../../extras/FvwmScript/script.y"
{ AddCom(19,2);;
    break;}
case 108:
#line 617 "../../../extras/FvwmScript/script.y"
{ AddCom(11,2);;
    break;}
case 109:
#line 619 "../../../extras/FvwmScript/script.y"
{ AddCom(12,2);;
    break;}
case 110:
#line 621 "../../../extras/FvwmScript/script.y"
{ AddCom(13,0);;
    break;}
case 111:
#line 623 "../../../extras/FvwmScript/script.y"
{ AddCom(17,1);;
    break;}
case 112:
#line 625 "../../../extras/FvwmScript/script.y"
{ AddCom(23,2);;
    break;}
case 113:
#line 627 "../../../extras/FvwmScript/script.y"
{ AddCom(18,2);;
    break;}
case 117:
#line 637 "../../../extras/FvwmScript/script.y"
{ AddComBloc(14,3,2); ;
    break;}
case 120:
#line 642 "../../../extras/FvwmScript/script.y"
{ EmpilerBloc(); ;
    break;}
case 121:
#line 644 "../../../extras/FvwmScript/script.y"
{ DepilerBloc(2); ;
    break;}
case 122:
#line 645 "../../../extras/FvwmScript/script.y"
{ DepilerBloc(2); ;
    break;}
case 123:
#line 648 "../../../extras/FvwmScript/script.y"
{ DepilerBloc(1); ;
    break;}
case 124:
#line 649 "../../../extras/FvwmScript/script.y"
{ DepilerBloc(1); ;
    break;}
case 125:
#line 653 "../../../extras/FvwmScript/script.y"
{ AddComBloc(15,3,1); ;
    break;}
case 126:
#line 657 "../../../extras/FvwmScript/script.y"
{ AddComBloc(16,3,1); ;
    break;}
case 127:
#line 662 "../../../extras/FvwmScript/script.y"
{ AddVar(yyvsp[0].str); ;
    break;}
case 128:
#line 664 "../../../extras/FvwmScript/script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 129:
#line 666 "../../../extras/FvwmScript/script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 130:
#line 668 "../../../extras/FvwmScript/script.y"
{ AddConstNum(yyvsp[0].number); ;
    break;}
case 131:
#line 670 "../../../extras/FvwmScript/script.y"
{ AddConstNum(-1); ;
    break;}
case 132:
#line 672 "../../../extras/FvwmScript/script.y"
{ AddConstNum(-2); ;
    break;}
case 133:
#line 674 "../../../extras/FvwmScript/script.y"
{ AddLevelBufArg(); ;
    break;}
case 134:
#line 676 "../../../extras/FvwmScript/script.y"
{ AddFunct(1,1); ;
    break;}
case 135:
#line 677 "../../../extras/FvwmScript/script.y"
{ AddFunct(2,1); ;
    break;}
case 136:
#line 678 "../../../extras/FvwmScript/script.y"
{ AddFunct(3,1); ;
    break;}
case 137:
#line 679 "../../../extras/FvwmScript/script.y"
{ AddFunct(4,1); ;
    break;}
case 138:
#line 680 "../../../extras/FvwmScript/script.y"
{ AddFunct(5,1); ;
    break;}
case 139:
#line 681 "../../../extras/FvwmScript/script.y"
{ AddFunct(6,1); ;
    break;}
case 140:
#line 682 "../../../extras/FvwmScript/script.y"
{ AddFunct(7,1); ;
    break;}
case 141:
#line 683 "../../../extras/FvwmScript/script.y"
{ AddFunct(8,1); ;
    break;}
case 142:
#line 684 "../../../extras/FvwmScript/script.y"
{ AddFunct(9,1); ;
    break;}
case 143:
#line 685 "../../../extras/FvwmScript/script.y"
{ AddFunct(10,1); ;
    break;}
case 144:
#line 686 "../../../extras/FvwmScript/script.y"
{ AddFunct(11,1); ;
    break;}
case 145:
#line 687 "../../../extras/FvwmScript/script.y"
{ AddFunct(12,1); ;
    break;}
case 146:
#line 688 "../../../extras/FvwmScript/script.y"
{ AddFunct(13,1); ;
    break;}
case 147:
#line 689 "../../../extras/FvwmScript/script.y"
{ AddFunct(14,1); ;
    break;}
case 148:
#line 690 "../../../extras/FvwmScript/script.y"
{ AddFunct(15,1); ;
    break;}
case 149:
#line 695 "../../../extras/FvwmScript/script.y"
{ ;
    break;}
case 176:
#line 741 "../../../extras/FvwmScript/script.y"
{ l=1-250000; AddBufArg(&l,1); ;
    break;}
case 177:
#line 742 "../../../extras/FvwmScript/script.y"
{ l=2-250000; AddBufArg(&l,1); ;
    break;}
case 178:
#line 743 "../../../extras/FvwmScript/script.y"
{ l=3-250000; AddBufArg(&l,1); ;
    break;}
case 179:
#line 744 "../../../extras/FvwmScript/script.y"
{ l=4-250000; AddBufArg(&l,1); ;
    break;}
case 180:
#line 745 "../../../extras/FvwmScript/script.y"
{ l=5-250000; AddBufArg(&l,1); ;
    break;}
case 181:
#line 746 "../../../extras/FvwmScript/script.y"
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
#line 749 "../../../extras/FvwmScript/script.y"















