
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
#define	QUITFUNC	273
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
#define	NOFOCUS	290
#define	NORELIEFSTRING	291
#define	CENTER	292
#define	LEFT	293
#define	RIGHT	294
#define	CASE	295
#define	SINGLECLIC	296
#define	DOUBLECLIC	297
#define	BEG	298
#define	POINT	299
#define	EXEC	300
#define	HIDE	301
#define	SHOW	302
#define	CHFORECOLOR	303
#define	CHBACKCOLOR	304
#define	CHCOLORSET	305
#define	KEY	306
#define	GETVALUE	307
#define	GETMINVALUE	308
#define	GETMAXVALUE	309
#define	GETFORE	310
#define	GETBACK	311
#define	GETHILIGHT	312
#define	GETSHADOW	313
#define	CHVALUE	314
#define	CHVALUEMAX	315
#define	CHVALUEMIN	316
#define	ADD	317
#define	DIV	318
#define	MULT	319
#define	GETTITLE	320
#define	GETOUTPUT	321
#define	STRCOPY	322
#define	NUMTOHEX	323
#define	HEXTONUM	324
#define	QUIT	325
#define	LAUNCHSCRIPT	326
#define	GETSCRIPTFATHER	327
#define	SENDTOSCRIPT	328
#define	RECEIVFROMSCRIPT	329
#define	GET	330
#define	SET	331
#define	SENDSIGN	332
#define	REMAINDEROFDIV	333
#define	GETTIME	334
#define	GETSCRIPTARG	335
#define	GETPID	336
#define	SENDMSGANDGET	337
#define	PARSE	338
#define	LASTSTRING	339
#define	IF	340
#define	THEN	341
#define	ELSE	342
#define	FOR	343
#define	TO	344
#define	DO	345
#define	WHILE	346
#define	BEGF	347
#define	ENDF	348
#define	EQUAL	349
#define	INFEQ	350
#define	SUPEQ	351
#define	INF	352
#define	SUP	353
#define	DIFF	354

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
extern char* ScriptName;

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
 fprintf(stderr,"[%s] Line %d: %s\n",ScriptName,numligne,errmsg);
 return 0;
}



#line 351 "script.y"
typedef union {  char *str;
          int number;
       } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		450
#define	YYFLAG		-32768
#define	YYNTBASE	101

#define YYTRANSLATE(x) ((unsigned)(x) <= 354 ? yytranslate[x] : 166)

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
    87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
    97,    98,    99,   100
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     7,     8,     9,    13,    17,    22,    27,    31,    35,
    39,    43,    47,    51,    52,    58,    59,    65,    66,    72,
    73,    81,    83,    84,    88,    93,    98,   102,   106,   110,
   114,   118,   122,   126,   130,   134,   138,   142,   146,   150,
   151,   154,   157,   160,   163,   166,   169,   170,   172,   178,
   179,   180,   185,   190,   192,   194,   196,   200,   201,   205,
   209,   213,   217,   221,   225,   229,   233,   237,   241,   245,
   249,   253,   257,   261,   265,   269,   273,   277,   281,   285,
   289,   293,   297,   300,   303,   306,   309,   312,   315,   318,
   321,   324,   327,   330,   333,   336,   339,   342,   345,   348,
   351,   354,   357,   360,   363,   366,   369,   372,   375,   380,
   385,   390,   397,   404,   409,   414,   419,   424,   429,   434,
   440,   445,   446,   449,   454,   459,   470,   475,   479,   483,
   491,   492,   496,   497,   501,   503,   507,   509,   519,   527,
   529,   531,   533,   535,   537,   539,   540,   543,   546,   551,
   555,   558,   562,   566,   570,   575,   578,   580,   583,   587,
   589,   592,   595,   598,   601,   604,   607,   610,   612,   617,
   621,   623,   624,   627,   630,   633,   636,   639,   642,   648,
   650,   652,   654,   656,   658,   660,   665,   667,   669,   671,
   673,   678,   680,   682,   687,   689,   691,   696,   698,   700,
   702,   704,   706,   708
};

static const short yyrhs[] = {   102,
   103,   104,   105,   106,   107,     0,     0,     0,   103,     7,
     4,     0,   103,    31,     3,     0,   103,     9,     6,     6,
     0,   103,     8,     6,     6,     0,   103,    12,     4,     0,
   103,    11,     4,     0,   103,    13,     4,     0,   103,    14,
     4,     0,   103,    15,     6,     0,   103,    10,     3,     0,
     0,    17,   146,    44,   118,    21,     0,     0,    18,   146,
    44,   118,    21,     0,     0,    19,   146,    44,   118,    21,
     0,     0,   107,    16,   108,    22,   109,   111,   112,     0,
     6,     0,     0,   109,    23,     3,     0,   109,    24,     6,
     6,     0,   109,    25,     6,     6,     0,   109,    26,     6,
     0,   109,    27,     6,     0,   109,    28,     6,     0,   109,
    29,     4,     0,   109,    30,     4,     0,   109,    31,     3,
     0,   109,    12,     4,     0,   109,    11,     4,     0,   109,
    13,     4,     0,   109,    14,     4,     0,   109,    15,     6,
     0,   109,    10,     3,     0,   109,    32,   110,     0,     0,
   110,    35,     0,   110,    37,     0,   110,    36,     0,   110,
    38,     0,   110,    39,     0,   110,    40,     0,     0,    21,
     0,    20,   113,    41,   114,    21,     0,     0,     0,   114,
   115,    45,   117,     0,   114,   116,    45,   117,     0,    42,
     0,    43,     0,     6,     0,    44,   118,    21,     0,     0,
   118,    46,   120,     0,   118,    33,   137,     0,   118,    34,
   139,     0,   118,    47,   121,     0,   118,    48,   122,     0,
   118,    60,   123,     0,   118,    61,   124,     0,   118,    62,
   125,     0,   118,    25,   126,     0,   118,    24,   127,     0,
   118,    29,   129,     0,   118,    31,   128,     0,   118,    10,
   130,     0,   118,    49,   131,     0,   118,    50,   132,     0,
   118,    51,   133,     0,   118,    77,   134,     0,   118,    78,
   135,     0,   118,    71,   136,     0,   118,    74,   138,     0,
   118,    86,   141,     0,   118,    89,   142,     0,   118,    92,
   143,     0,   118,    52,   140,     0,    46,   120,     0,    33,
   137,     0,    34,   139,     0,    47,   121,     0,    48,   122,
     0,    60,   123,     0,    61,   124,     0,    62,   125,     0,
    25,   126,     0,    24,   127,     0,    29,   129,     0,    31,
   128,     0,    10,   130,     0,    49,   131,     0,    50,   132,
     0,    51,   133,     0,    77,   134,     0,    78,   135,     0,
    71,   136,     0,    74,   138,     0,    89,   142,     0,    92,
   143,     0,    52,   140,     0,   157,   159,     0,   157,   161,
     0,   157,   161,     0,   157,   161,   157,   161,     0,   157,
   161,   157,   161,     0,   157,   161,   157,   161,     0,   157,
   161,   157,   161,   157,   161,     0,   157,   161,   157,   161,
   157,   161,     0,   157,   161,   157,   162,     0,   157,   161,
   157,   163,     0,   157,   161,   157,   162,     0,   157,   161,
   157,   163,     0,   157,   161,   157,   163,     0,   157,   161,
   157,   161,     0,   157,   164,    76,   157,   159,     0,   157,
   161,   157,   161,     0,     0,   157,   161,     0,   157,   161,
   157,   159,     0,   157,   162,   157,   159,     0,   157,   162,
   157,   162,   157,   161,   157,   161,   157,   159,     0,   144,
   146,   147,   145,     0,   149,   146,   148,     0,   150,   146,
   148,     0,   157,   160,   157,   165,   157,   160,    87,     0,
     0,    88,   146,   148,     0,     0,    44,   118,    21,     0,
   119,     0,    44,   118,    21,     0,   119,     0,   157,   164,
    76,   157,   160,    90,   157,   160,    91,     0,   157,   160,
   157,   165,   157,   160,    91,     0,     5,     0,     3,     0,
     4,     0,     6,     0,    42,     0,    43,     0,     0,    53,
   161,     0,    66,   161,     0,    67,   163,   161,   161,     0,
    69,   161,   161,     0,    70,   163,     0,    63,   161,   161,
     0,    65,   161,   161,     0,    64,   161,   161,     0,    68,
   163,   161,   161,     0,    72,   163,     0,    73,     0,    75,
   161,     0,    79,   161,   161,     0,    80,     0,    81,   161,
     0,    56,   161,     0,    57,   161,     0,    58,   161,     0,
    59,   161,     0,    54,   161,     0,    55,   161,     0,    82,
     0,    83,   163,   163,   161,     0,    84,   163,   161,     0,
    85,     0,     0,   155,   159,     0,   156,   159,     0,   151,
   159,     0,   153,   159,     0,   152,   159,     0,   154,   159,
     0,    93,   157,   158,    94,   159,     0,   151,     0,   155,
     0,   156,     0,   153,     0,   152,     0,   154,     0,    93,
   157,   158,    94,     0,   155,     0,   156,     0,   154,     0,
   151,     0,    93,   157,   158,    94,     0,   151,     0,   152,
     0,    93,   157,   158,    94,     0,   151,     0,   153,     0,
    93,   157,   158,    94,     0,   151,     0,    98,     0,    96,
     0,    95,     0,    97,     0,    99,     0,   100,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   376,   379,   383,   384,   387,   390,   395,   400,   404,   408,
   412,   416,   420,   426,   427,   432,   433,   438,   439,   448,
   449,   452,   468,   469,   473,   477,   482,   485,   488,   491,
   494,   497,   500,   504,   508,   512,   516,   520,   523,   525,
   526,   529,   532,   535,   538,   541,   547,   558,   559,   562,
   564,   565,   566,   569,   570,   573,   576,   581,   582,   583,
   584,   585,   586,   587,   588,   589,   590,   591,   592,   593,
   594,   595,   596,   597,   598,   599,   600,   601,   602,   603,
   604,   605,   609,   610,   611,   612,   613,   614,   615,   616,
   617,   618,   619,   620,   621,   622,   623,   624,   625,   626,
   627,   628,   629,   630,   631,   634,   636,   638,   640,   642,
   644,   646,   648,   650,   652,   654,   656,   658,   660,   662,
   664,   666,   668,   670,   672,   674,   676,   678,   680,   684,
   686,   687,   689,   691,   692,   695,   696,   700,   704,   709,
   711,   713,   715,   717,   719,   721,   723,   724,   725,   726,
   727,   728,   729,   730,   731,   732,   733,   734,   735,   736,
   737,   738,   739,   740,   741,   742,   743,   744,   745,   746,
   747,   752,   753,   754,   755,   756,   757,   758,   759,   764,
   765,   766,   767,   768,   769,   770,   774,   775,   776,   777,
   778,   782,   783,   784,   788,   789,   790,   794,   798,   799,
   800,   801,   802,   803
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","STR","GSTR",
"VAR","NUMBER","WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FONT","FORECOLOR",
"BACKCOLOR","SHADCOLOR","LICOLOR","COLORSET","OBJECT","INIT","PERIODICTASK",
"QUITFUNC","MAIN","END","PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX",
"TITLE","SWALLOWEXEC","ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","NOFOCUS",
"NORELIEFSTRING","CENTER","LEFT","RIGHT","CASE","SINGLECLIC","DOUBLECLIC","BEG",
"POINT","EXEC","HIDE","SHOW","CHFORECOLOR","CHBACKCOLOR","CHCOLORSET","KEY",
"GETVALUE","GETMINVALUE","GETMAXVALUE","GETFORE","GETBACK","GETHILIGHT","GETSHADOW",
"CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT","GETTITLE","GETOUTPUT",
"STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT","GETSCRIPTFATHER","SENDTOSCRIPT",
"RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV","GETTIME","GETSCRIPTARG",
"GETPID","SENDMSGANDGET","PARSE","LASTSTRING","IF","THEN","ELSE","FOR","TO",
"DO","WHILE","BEGF","ENDF","EQUAL","INFEQ","SUPEQ","INF","SUP","DIFF","script",
"initvar","head","initbloc","periodictask","quitfunc","object","id","init","flags",
"verify","mainloop","addtabcase","case","clic","number","bloc","instr","oneinstr",
"exec","hide","show","chvalue","chvaluemax","chvaluemin","position","size","icon",
"title","font","chforecolor","chbackcolor","chcolorset","set","sendsign","quit",
"warp","sendtoscript","writetofile","key","ifthenelse","loop","while","headif",
"else","creerbloc","bloc1","bloc2","headloop","headwhile","var","str","gstr",
"num","singleclic2","doubleclic2","addlbuff","function","args","arg","numarg",
"strarg","gstrarg","vararg","compare", NULL
};
#endif

static const short yyr1[] = {     0,
   101,   102,   103,   103,   103,   103,   103,   103,   103,   103,
   103,   103,   103,   104,   104,   105,   105,   106,   106,   107,
   107,   108,   109,   109,   109,   109,   109,   109,   109,   109,
   109,   109,   109,   109,   109,   109,   109,   109,   109,   110,
   110,   110,   110,   110,   110,   110,   111,   112,   112,   113,
   114,   114,   114,   115,   115,   116,   117,   118,   118,   118,
   118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
   118,   118,   118,   118,   118,   118,   118,   118,   118,   118,
   118,   118,   119,   119,   119,   119,   119,   119,   119,   119,
   119,   119,   119,   119,   119,   119,   119,   119,   119,   119,
   119,   119,   119,   119,   119,   120,   121,   122,   123,   124,
   125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
   135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
   145,   145,   146,   147,   147,   148,   148,   149,   150,   151,
   152,   153,   154,   155,   156,   157,   158,   158,   158,   158,
   158,   158,   158,   158,   158,   158,   158,   158,   158,   158,
   158,   158,   158,   158,   158,   158,   158,   158,   158,   158,
   158,   159,   159,   159,   159,   159,   159,   159,   159,   160,
   160,   160,   160,   160,   160,   160,   161,   161,   161,   161,
   161,   162,   162,   162,   163,   163,   163,   164,   165,   165,
   165,   165,   165,   165
};

static const short yyr2[] = {     0,
     6,     0,     0,     3,     3,     4,     4,     3,     3,     3,
     3,     3,     3,     0,     5,     0,     5,     0,     5,     0,
     7,     1,     0,     3,     4,     4,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     0,
     2,     2,     2,     2,     2,     2,     0,     1,     5,     0,
     0,     4,     4,     1,     1,     1,     3,     0,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     4,     4,
     4,     6,     6,     4,     4,     4,     4,     4,     4,     5,
     4,     0,     2,     4,     4,    10,     4,     3,     3,     7,
     0,     3,     0,     3,     1,     3,     1,     9,     7,     1,
     1,     1,     1,     1,     1,     0,     2,     2,     4,     3,
     2,     3,     3,     3,     4,     2,     1,     2,     3,     1,
     2,     2,     2,     2,     2,     2,     2,     1,     4,     3,
     1,     0,     2,     2,     2,     2,     2,     2,     5,     1,
     1,     1,     1,     1,     1,     4,     1,     1,     1,     1,
     4,     1,     1,     4,     1,     1,     4,     1,     1,     1,
     1,     1,     1,     1
};

static const short yydefact[] = {     2,
     3,    14,     0,     0,     0,     0,     0,     0,     0,     0,
     0,   133,     0,    16,     4,     0,     0,    13,     9,     8,
    10,    11,    12,     0,     5,   133,    18,     7,     6,    58,
     0,   133,    20,     0,    58,     0,     1,   146,    15,   146,
   146,   146,   146,   146,   146,   146,   146,   146,   146,   146,
   146,   146,   146,   146,   146,   122,   146,   146,   146,   146,
   146,   146,     0,    58,     0,    71,     0,    68,     0,    67,
     0,    69,     0,    70,     0,    60,     0,    61,     0,    59,
   172,    62,     0,    63,     0,    72,     0,    73,     0,    74,
     0,    82,     0,    64,     0,    65,     0,    66,     0,    77,
    78,     0,    75,     0,    76,     0,    79,   133,     0,    80,
   133,     0,    81,   133,     0,    17,     0,    22,     0,   140,
   143,   144,   145,   146,   190,   189,   187,   188,   146,   146,
   146,   146,   146,   123,   141,   146,   192,   193,   146,   142,
   146,   172,   172,   172,   172,   172,   172,   106,   107,   108,
   146,   146,   146,   146,   146,   146,   146,   146,   198,     0,
   146,     0,   146,   180,   184,   183,   185,   181,   182,   146,
     0,     0,     0,   146,    19,    23,     0,     0,     0,     0,
     0,     0,     0,   172,     0,   175,   177,   176,   178,   173,
   174,     0,     0,     0,     0,     0,     0,     0,   172,   146,
     0,   146,   146,   146,   146,   146,   146,   146,    58,   146,
   146,   146,   146,   146,   146,   146,   146,   146,   146,   122,
   146,   146,   146,   146,   146,   135,   131,     0,     0,    58,
   137,   128,   146,   129,     0,    47,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   157,     0,     0,   160,     0,   168,     0,     0,
   171,     0,   116,   146,   146,   146,   195,   196,   115,   114,
     0,   125,     0,   117,   118,   119,   146,   109,   110,   111,
   124,   172,   121,    95,    92,    91,    93,    94,    84,    85,
     0,    83,    86,    87,    96,    97,    98,   105,    88,    89,
    90,   101,   102,    99,   100,   103,   104,   133,   127,     0,
   201,   200,   202,   199,   203,   204,   146,     0,     0,   146,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    40,     0,   147,   166,   167,
   162,   163,   164,   165,     0,     0,     0,   148,     0,     0,
     0,   151,   156,   158,     0,   161,     0,     0,   191,     0,
     0,     0,   194,   172,     0,   120,   134,     0,   186,     0,
   136,     0,     0,    38,    34,    33,    35,    36,    37,    24,
     0,     0,    27,    28,    29,    30,    31,    32,    39,    50,
    48,    21,   152,   154,   153,     0,     0,   150,   159,     0,
   170,   113,   112,     0,   179,   146,   132,     0,   146,     0,
    25,    26,    41,    43,    42,    44,    45,    46,     0,   149,
   155,   169,   197,     0,   130,     0,   139,    51,   146,     0,
     0,   172,   138,    56,    49,    54,    55,     0,     0,   126,
     0,     0,    58,    52,    53,     0,    57,     0,     0,     0
};

static const short yydefgoto[] = {   448,
     1,     2,    14,    27,    33,    37,   119,   236,   389,   337,
   392,   419,   431,   438,   439,   444,    34,   231,    80,    82,
    84,    94,    96,    98,    70,    68,    74,    72,    66,    86,
    88,    90,   103,   105,   100,    76,   101,    78,    92,   107,
   110,   113,   108,   309,    24,   227,   232,   111,   114,   125,
   143,   144,   126,   127,   128,    67,   262,   148,   170,   129,
   139,   269,   160,   317
};

static const short yypact[] = {-32768,
-32768,   496,    -2,    24,    25,     1,     6,    28,    45,    53,
    52,-32768,    57,    44,-32768,    58,    60,-32768,-32768,-32768,
-32768,-32768,-32768,    30,-32768,-32768,    59,-32768,-32768,-32768,
    32,-32768,-32768,   596,-32768,    36,    66,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   669,-32768,    89,-32768,    78,-32768,    78,-32768,
    78,-32768,    78,-32768,    78,-32768,    78,-32768,     0,-32768,
    65,-32768,    78,-32768,    78,-32768,    78,-32768,    78,-32768,
    78,-32768,     0,-32768,    78,-32768,    78,-32768,    78,-32768,
-32768,    78,-32768,    67,-32768,    78,-32768,-32768,    96,-32768,
-32768,    67,-32768,-32768,    96,-32768,   735,-32768,    76,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    65,    65,    65,    65,    65,    65,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    29,
-32768,   999,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
  1056,    33,  1056,-32768,-32768,-32768,  1096,     0,    78,    78,
     4,     0,  1096,    65,  1096,-32768,-32768,-32768,-32768,-32768,
-32768,     4,     4,    78,     0,    78,    78,    78,    65,-32768,
    78,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,    18,  1096,   -81,-32768,
-32768,-32768,-32768,-32768,   -81,   559,    78,    78,    78,    78,
    78,    78,    78,    78,    78,    78,    78,     4,     4,    78,
     4,     4,-32768,    78,    78,-32768,    78,-32768,     4,     4,
-32768,    16,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    34,-32768,    37,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    65,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   801,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    39,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   867,    96,-32768,
   115,   123,   126,   131,   132,   136,   134,   137,   146,   149,
   151,   153,   156,   157,   159,-32768,     7,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,    78,    78,    78,-32768,    78,    78,
    78,-32768,-32768,-32768,    78,-32768,     4,    78,-32768,    78,
    78,  1096,-32768,    65,    78,-32768,-32768,  1056,-32768,    96,
-32768,    74,    96,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   160,   166,-32768,-32768,-32768,-32768,-32768,-32768,   -14,-32768,
-32768,-32768,-32768,-32768,-32768,    78,    78,-32768,-32768,    78,
-32768,-32768,-32768,    71,-32768,-32768,-32768,    86,-32768,    84,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   135,-32768,
-32768,-32768,-32768,    78,-32768,    96,-32768,-32768,-32768,    87,
    98,    65,-32768,-32768,-32768,-32768,-32768,   138,   140,-32768,
   133,   133,-32768,-32768,-32768,   933,-32768,   179,   180,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,  -261,   -35,    20,   -18,   -17,
     8,    -4,     3,    19,    15,    22,    17,    38,    40,     9,
    27,    31,    42,    43,    35,    46,    48,    63,    23,-32768,
    26,    47,-32768,-32768,   -20,-32768,  -166,-32768,-32768,   175,
   183,   223,   182,   249,   296,    -7,  -172,   -31,   -95,   -10,
   -92,    56,   148,    50
};


#define	YYLAST		1181


static const short yytable[] = {    63,
   154,    15,   135,    18,   120,    31,   234,   140,   120,    19,
   271,    36,   273,   311,   312,   313,   314,   315,   316,   174,
   413,   414,   415,   416,   417,   418,   390,   391,   117,    16,
    17,    20,    69,    71,    73,    75,    77,    79,    81,    83,
    85,    87,    89,    91,    93,    95,    97,    99,    21,   102,
   104,   106,   109,   112,   115,   310,    22,    23,   130,    25,
   131,    26,   132,    28,   133,    29,   134,   135,   140,   120,
   121,   120,   149,    30,   150,    35,   151,    32,   152,    64,
   153,    65,   120,   121,   155,   263,   156,   162,   157,   270,
   171,   158,   136,   173,   118,   161,   266,   176,   135,   140,
   120,   121,   277,   434,   200,   308,   122,   123,   233,   359,
   186,   187,   188,   189,   190,   191,   177,   374,   435,   122,
   123,   178,   179,   180,   181,   182,   375,   363,   183,   376,
   364,   184,   369,   185,   377,   378,   380,   122,   123,   436,
   437,   379,   381,   192,   193,   194,   195,   196,   197,   198,
   199,   382,   272,   201,   383,   228,   384,   141,   385,   386,
   387,   388,   229,   409,   423,   411,   235,   281,   264,   265,
   124,   412,   425,   291,   427,   428,   443,   433,   449,   450,
   445,   226,   441,   276,   442,   278,   279,   280,   163,   404,
   283,   292,   282,   293,   318,    69,    71,    73,    75,    77,
    79,   407,    81,    83,    85,    87,    89,    91,    93,    95,
    97,    99,   299,   102,   104,   106,   112,   115,   286,   294,
   300,   295,   288,   372,   285,   319,   338,   339,   340,   341,
   342,   343,   344,   345,   346,   347,   348,   301,   298,   351,
   296,   284,   287,   354,   355,   297,   356,   274,   275,   306,
   366,     0,   289,   137,   302,   142,   360,   361,   362,   172,
     0,   138,   145,   304,     0,   305,     0,   137,   303,   365,
   290,   307,     0,     0,   408,   138,     0,   410,   159,     0,
     0,     0,     0,   164,   320,     0,   159,   368,     0,   164,
   167,   165,     0,     0,     0,     0,   167,   165,     0,     0,
     0,     0,     0,   349,   350,     0,   352,   353,     0,   370,
     0,     0,   373,     0,   357,   358,   142,   142,   142,   142,
   142,   142,     0,   145,   145,   145,   145,   145,   145,   146,
   430,   166,   405,     0,   393,   394,   395,   166,   396,   397,
   398,     0,     0,     0,   399,     0,     0,   401,     0,   402,
   403,     0,   137,     0,   406,   267,   137,   168,   142,     0,
   138,     0,     0,   168,   138,   145,   267,   267,     0,   137,
     0,     0,     0,   142,     0,     0,   147,   138,     0,     0,
   145,     0,     0,     0,     0,   420,   421,     0,     0,   422,
   146,   146,   146,   146,   146,   146,     0,     0,   424,     0,
   440,   426,     0,   268,   169,     0,     0,   446,     0,     0,
   169,     0,   400,   429,   268,   268,     0,     0,     0,     0,
     0,   432,   267,   267,     0,   267,   267,     0,     0,     0,
     0,     0,   146,   267,   267,     0,     0,   147,   147,   147,
   147,   147,   147,     0,     0,     0,     0,   146,     0,     0,
     0,     0,     0,     0,     0,     0,   142,     0,     0,     0,
     0,     0,     0,   145,     0,     0,     0,     0,     0,     0,
   268,   268,     0,   268,   268,     0,     0,     0,     0,   147,
     0,   268,   268,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   164,   147,     0,     0,     0,     0,     0,
   167,   165,     3,     4,     5,     6,     7,     8,     9,    10,
    11,     0,    12,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
   146,   267,     0,     0,     0,     0,     0,     0,   142,     0,
     0,   166,     0,     0,   164,   145,     0,   164,     0,     0,
     0,   167,   165,     0,   167,   165,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   168,   321,   322,
   323,   324,   325,   326,     0,     0,     0,   147,     0,   268,
     0,   327,   328,   329,   330,   331,   332,   333,   334,   335,
   336,     0,   166,     0,     0,   166,     0,     0,     0,     0,
   164,     0,     0,     0,     0,    38,   142,   167,   165,     0,
     0,     0,   146,   145,   169,     0,    39,     0,   168,    40,
    41,   168,     0,     0,    42,     0,    43,     0,    44,    45,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    46,    47,    48,    49,    50,    51,    52,   166,     0,
     0,     0,     0,     0,     0,    53,    54,    55,     0,   147,
     0,     0,     0,     0,     0,   169,    56,     0,   169,    57,
     0,     0,    58,    59,   168,     0,     0,     0,    38,     0,
   146,    60,     0,     0,    61,     0,     0,    62,     0,   116,
     0,     0,    40,    41,     0,     0,     0,    42,     0,    43,
     0,    44,    45,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    46,    47,    48,    49,    50,    51,
    52,   169,     0,     0,     0,     0,     0,   147,    53,    54,
    55,     0,     0,     0,     0,     0,     0,     0,     0,    56,
     0,     0,    57,     0,    38,    58,    59,     0,     0,     0,
     0,     0,     0,     0,    60,   175,     0,    61,    40,    41,
    62,     0,     0,    42,     0,    43,     0,    44,    45,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    46,    47,    48,    49,    50,    51,    52,     0,     0,     0,
     0,     0,     0,     0,    53,    54,    55,     0,     0,     0,
     0,     0,     0,     0,     0,    56,     0,     0,    57,     0,
    38,    58,    59,     0,     0,     0,     0,     0,     0,     0,
    60,   367,     0,    61,    40,    41,    62,     0,     0,    42,
     0,    43,     0,    44,    45,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    46,    47,    48,    49,
    50,    51,    52,     0,     0,     0,     0,     0,     0,     0,
    53,    54,    55,     0,     0,     0,     0,     0,     0,     0,
     0,    56,     0,     0,    57,     0,    38,    58,    59,     0,
     0,     0,     0,     0,     0,     0,    60,   371,     0,    61,
    40,    41,    62,     0,     0,    42,     0,    43,     0,    44,
    45,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    46,    47,    48,    49,    50,    51,    52,     0,
     0,     0,     0,     0,     0,     0,    53,    54,    55,     0,
     0,     0,     0,     0,     0,     0,     0,    56,     0,     0,
    57,     0,    38,    58,    59,     0,     0,     0,     0,     0,
     0,     0,    60,   447,     0,    61,    40,    41,    62,     0,
     0,    42,     0,    43,     0,    44,    45,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    46,    47,
    48,    49,    50,    51,    52,     0,     0,     0,     0,     0,
     0,     0,    53,    54,    55,     0,     0,     0,     0,     0,
     0,     0,     0,    56,     0,     0,    57,     0,   202,    58,
    59,     0,     0,     0,     0,     0,     0,     0,    60,     0,
     0,    61,   203,   204,    62,     0,     0,   205,     0,   206,
     0,   207,   208,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   209,     0,   210,   211,   212,   213,   214,   215,
   216,     0,     0,     0,     0,     0,     0,     0,   217,   218,
   219,     0,     0,     0,     0,   202,     0,     0,     0,   220,
     0,     0,   221,     0,     0,   222,   223,     0,     0,   203,
   204,     0,     0,     0,   205,     0,   206,   224,   207,   208,
   225,     0,     0,     0,     0,     0,     0,     0,     0,   230,
     0,   210,   211,   212,   213,   214,   215,   216,     0,     0,
     0,     0,     0,     0,     0,   217,   218,   219,     0,     0,
     0,     0,     0,     0,     0,     0,   220,     0,     0,   221,
     0,     0,   222,   223,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   224,     0,     0,   225,   237,   238,
   239,   240,   241,   242,   243,     0,     0,     0,   244,   245,
   246,   247,   248,   249,   250,   251,     0,   252,   253,     0,
   254,     0,     0,     0,   255,   256,   257,   258,   259,   260,
   261
};

static const short yycheck[] = {    35,
    93,     4,     3,     3,     5,    26,   173,     4,     5,     4,
   183,    32,   185,    95,    96,    97,    98,    99,   100,   115,
    35,    36,    37,    38,    39,    40,    20,    21,    64,     6,
     6,     4,    40,    41,    42,    43,    44,    45,    46,    47,
    48,    49,    50,    51,    52,    53,    54,    55,     4,    57,
    58,    59,    60,    61,    62,   228,     4,     6,    69,     3,
    71,    18,    73,     6,    75,     6,    77,     3,     4,     5,
     6,     5,    83,    44,    85,    44,    87,    19,    89,    44,
    91,    16,     5,     6,    95,   178,    97,   108,    99,   182,
   111,   102,    93,   114,     6,   106,    93,    22,     3,     4,
     5,     6,   195,     6,    76,    88,    42,    43,    76,    94,
   142,   143,   144,   145,   146,   147,   124,     3,    21,    42,
    43,   129,   130,   131,   132,   133,     4,    94,   136,     4,
    94,   139,    94,   141,     4,     4,     3,    42,    43,    42,
    43,     6,     6,   151,   152,   153,   154,   155,   156,   157,
   158,     6,   184,   161,     6,   163,     6,    93,     6,     4,
     4,     3,   170,    90,    94,     6,   174,   199,   179,   180,
    93,     6,    87,   209,    91,    41,    44,    91,     0,     0,
   442,   162,    45,   194,    45,   196,   197,   198,    93,   362,
   201,   210,   200,   211,   230,   203,   204,   205,   206,   207,
   208,   368,   210,   211,   212,   213,   214,   215,   216,   217,
   218,   219,   217,   221,   222,   223,   224,   225,   204,   212,
   218,   213,   206,   319,   203,   233,   237,   238,   239,   240,
   241,   242,   243,   244,   245,   246,   247,   219,   216,   250,
   214,   202,   205,   254,   255,   215,   257,   192,   193,   224,
   282,    -1,   207,    79,   220,    81,   264,   265,   266,   112,
    -1,    79,    81,   222,    -1,   223,    -1,    93,   221,   277,
   208,   225,    -1,    -1,   370,    93,    -1,   373,   104,    -1,
    -1,    -1,    -1,   109,   235,    -1,   112,   308,    -1,   115,
   109,   109,    -1,    -1,    -1,    -1,   115,   115,    -1,    -1,
    -1,    -1,    -1,   248,   249,    -1,   251,   252,    -1,   317,
    -1,    -1,   320,    -1,   259,   260,   142,   143,   144,   145,
   146,   147,    -1,   142,   143,   144,   145,   146,   147,    81,
   426,   109,   364,    -1,   345,   346,   347,   115,   349,   350,
   351,    -1,    -1,    -1,   355,    -1,    -1,   358,    -1,   360,
   361,    -1,   178,    -1,   365,   181,   182,   109,   184,    -1,
   178,    -1,    -1,   115,   182,   184,   192,   193,    -1,   195,
    -1,    -1,    -1,   199,    -1,    -1,    81,   195,    -1,    -1,
   199,    -1,    -1,    -1,    -1,   396,   397,    -1,    -1,   400,
   142,   143,   144,   145,   146,   147,    -1,    -1,   406,    -1,
   432,   409,    -1,   181,   109,    -1,    -1,   443,    -1,    -1,
   115,    -1,   357,   424,   192,   193,    -1,    -1,    -1,    -1,
    -1,   429,   248,   249,    -1,   251,   252,    -1,    -1,    -1,
    -1,    -1,   184,   259,   260,    -1,    -1,   142,   143,   144,
   145,   146,   147,    -1,    -1,    -1,    -1,   199,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,   282,    -1,    -1,    -1,
    -1,    -1,    -1,   282,    -1,    -1,    -1,    -1,    -1,    -1,
   248,   249,    -1,   251,   252,    -1,    -1,    -1,    -1,   184,
    -1,   259,   260,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,   319,   199,    -1,    -1,    -1,    -1,    -1,
   319,   319,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    -1,    17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,
   282,   357,    -1,    -1,    -1,    -1,    -1,    -1,   364,    -1,
    -1,   319,    -1,    -1,   370,   364,    -1,   373,    -1,    -1,
    -1,   370,   370,    -1,   373,   373,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,   319,    10,    11,
    12,    13,    14,    15,    -1,    -1,    -1,   282,    -1,   357,
    -1,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    -1,   370,    -1,    -1,   373,    -1,    -1,    -1,    -1,
   426,    -1,    -1,    -1,    -1,    10,   432,   426,   426,    -1,
    -1,    -1,   364,   432,   319,    -1,    21,    -1,   370,    24,
    25,   373,    -1,    -1,    29,    -1,    31,    -1,    33,    34,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    46,    47,    48,    49,    50,    51,    52,   426,    -1,
    -1,    -1,    -1,    -1,    -1,    60,    61,    62,    -1,   364,
    -1,    -1,    -1,    -1,    -1,   370,    71,    -1,   373,    74,
    -1,    -1,    77,    78,   426,    -1,    -1,    -1,    10,    -1,
   432,    86,    -1,    -1,    89,    -1,    -1,    92,    -1,    21,
    -1,    -1,    24,    25,    -1,    -1,    -1,    29,    -1,    31,
    -1,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    46,    47,    48,    49,    50,    51,
    52,   426,    -1,    -1,    -1,    -1,    -1,   432,    60,    61,
    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,
    -1,    -1,    74,    -1,    10,    77,    78,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    86,    21,    -1,    89,    24,    25,
    92,    -1,    -1,    29,    -1,    31,    -1,    33,    34,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    46,    47,    48,    49,    50,    51,    52,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    60,    61,    62,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    74,    -1,
    10,    77,    78,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    86,    21,    -1,    89,    24,    25,    92,    -1,    -1,    29,
    -1,    31,    -1,    33,    34,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,    48,    49,
    50,    51,    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    60,    61,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    71,    -1,    -1,    74,    -1,    10,    77,    78,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    86,    21,    -1,    89,
    24,    25,    92,    -1,    -1,    29,    -1,    31,    -1,    33,
    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    46,    47,    48,    49,    50,    51,    52,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    60,    61,    62,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,
    74,    -1,    10,    77,    78,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    86,    21,    -1,    89,    24,    25,    92,    -1,
    -1,    29,    -1,    31,    -1,    33,    34,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    46,    47,
    48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    60,    61,    62,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    71,    -1,    -1,    74,    -1,    10,    77,
    78,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,
    -1,    89,    24,    25,    92,    -1,    -1,    29,    -1,    31,
    -1,    33,    34,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    44,    -1,    46,    47,    48,    49,    50,    51,
    52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,    61,
    62,    -1,    -1,    -1,    -1,    10,    -1,    -1,    -1,    71,
    -1,    -1,    74,    -1,    -1,    77,    78,    -1,    -1,    24,
    25,    -1,    -1,    -1,    29,    -1,    31,    89,    33,    34,
    92,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    44,
    -1,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    60,    61,    62,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    74,
    -1,    -1,    77,    78,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    89,    -1,    -1,    92,    53,    54,
    55,    56,    57,    58,    59,    -1,    -1,    -1,    63,    64,
    65,    66,    67,    68,    69,    70,    -1,    72,    73,    -1,
    75,    -1,    -1,    -1,    79,    80,    81,    82,    83,    84,
    85
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
#line 379 "script.y"
{ InitVarGlob(); ;
    break;}
case 4:
#line 384 "script.y"
{		/* Titre de la fenetre */
				 scriptprop->titlewin=yyvsp[0].str;
				;
    break;}
case 5:
#line 387 "script.y"
{
				 scriptprop->icon=yyvsp[0].str;
				;
    break;}
case 6:
#line 391 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->x=yyvsp[-1].number;
				 scriptprop->y=yyvsp[0].number;
				;
    break;}
case 7:
#line 396 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->width=yyvsp[-1].number;
				 scriptprop->height=yyvsp[0].number;
				;
    break;}
case 8:
#line 400 "script.y"
{ 		/* Couleur de fond */
				 scriptprop->backcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 9:
#line 404 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->forecolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 10:
#line 408 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 11:
#line 412 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->hilicolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				;
    break;}
case 12:
#line 416 "script.y"
{
				 scriptprop->colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				;
    break;}
case 13:
#line 420 "script.y"
{
				 scriptprop->font=yyvsp[0].str;
				;
    break;}
case 15:
#line 427 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 17:
#line 433 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 19:
#line 439 "script.y"
{
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--; 
				;
    break;}
case 22:
#line 452 "script.y"
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
case 24:
#line 469 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				;
    break;}
case 25:
#line 473 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				;
    break;}
case 26:
#line 477 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				;
    break;}
case 27:
#line 482 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				;
    break;}
case 28:
#line 485 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				;
    break;}
case 29:
#line 488 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				;
    break;}
case 30:
#line 491 "script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[0].str;
				;
    break;}
case 31:
#line 494 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				;
    break;}
case 32:
#line 497 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				;
    break;}
case 33:
#line 500 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 34:
#line 504 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 35:
#line 508 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 36:
#line 512 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 37:
#line 516 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				;
    break;}
case 38:
#line 520 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				;
    break;}
case 41:
#line 526 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				;
    break;}
case 42:
#line 529 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				;
    break;}
case 43:
#line 532 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				;
    break;}
case 44:
#line 535 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				;
    break;}
case 45:
#line 538 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				;
    break;}
case 46:
#line 541 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
				;
    break;}
case 47:
#line 547 "script.y"
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
case 48:
#line 558 "script.y"
{ InitObjTabCase(0); ;
    break;}
case 50:
#line 562 "script.y"
{ InitObjTabCase(1); ;
    break;}
case 54:
#line 569 "script.y"
{ InitCase(-1); ;
    break;}
case 55:
#line 570 "script.y"
{ InitCase(-2); ;
    break;}
case 56:
#line 573 "script.y"
{ InitCase(yyvsp[0].number); ;
    break;}
case 106:
#line 634 "script.y"
{ AddCom(1,1); ;
    break;}
case 107:
#line 636 "script.y"
{ AddCom(2,1);;
    break;}
case 108:
#line 638 "script.y"
{ AddCom(3,1);;
    break;}
case 109:
#line 640 "script.y"
{ AddCom(4,2);;
    break;}
case 110:
#line 642 "script.y"
{ AddCom(21,2);;
    break;}
case 111:
#line 644 "script.y"
{ AddCom(22,2);;
    break;}
case 112:
#line 646 "script.y"
{ AddCom(5,3);;
    break;}
case 113:
#line 648 "script.y"
{ AddCom(6,3);;
    break;}
case 114:
#line 650 "script.y"
{ AddCom(7,2);;
    break;}
case 115:
#line 652 "script.y"
{ AddCom(8,2);;
    break;}
case 116:
#line 654 "script.y"
{ AddCom(9,2);;
    break;}
case 117:
#line 656 "script.y"
{ AddCom(10,2);;
    break;}
case 118:
#line 658 "script.y"
{ AddCom(19,2);;
    break;}
case 119:
#line 660 "script.y"
{ AddCom(24,2);;
    break;}
case 120:
#line 662 "script.y"
{ AddCom(11,2);;
    break;}
case 121:
#line 664 "script.y"
{ AddCom(12,2);;
    break;}
case 122:
#line 666 "script.y"
{ AddCom(13,0);;
    break;}
case 123:
#line 668 "script.y"
{ AddCom(17,1);;
    break;}
case 124:
#line 670 "script.y"
{ AddCom(23,2);;
    break;}
case 125:
#line 672 "script.y"
{ AddCom(18,2);;
    break;}
case 126:
#line 674 "script.y"
{ AddCom(25,5);;
    break;}
case 130:
#line 684 "script.y"
{ AddComBloc(14,3,2); ;
    break;}
case 133:
#line 689 "script.y"
{ EmpilerBloc(); ;
    break;}
case 134:
#line 691 "script.y"
{ DepilerBloc(2); ;
    break;}
case 135:
#line 692 "script.y"
{ DepilerBloc(2); ;
    break;}
case 136:
#line 695 "script.y"
{ DepilerBloc(1); ;
    break;}
case 137:
#line 696 "script.y"
{ DepilerBloc(1); ;
    break;}
case 138:
#line 700 "script.y"
{ AddComBloc(15,3,1); ;
    break;}
case 139:
#line 704 "script.y"
{ AddComBloc(16,3,1); ;
    break;}
case 140:
#line 709 "script.y"
{ AddVar(yyvsp[0].str); ;
    break;}
case 141:
#line 711 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 142:
#line 713 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 143:
#line 715 "script.y"
{ AddConstNum(yyvsp[0].number); ;
    break;}
case 144:
#line 717 "script.y"
{ AddConstNum(-1); ;
    break;}
case 145:
#line 719 "script.y"
{ AddConstNum(-2); ;
    break;}
case 146:
#line 721 "script.y"
{ AddLevelBufArg(); ;
    break;}
case 147:
#line 723 "script.y"
{ AddFunct(1,1); ;
    break;}
case 148:
#line 724 "script.y"
{ AddFunct(2,1); ;
    break;}
case 149:
#line 725 "script.y"
{ AddFunct(3,1); ;
    break;}
case 150:
#line 726 "script.y"
{ AddFunct(4,1); ;
    break;}
case 151:
#line 727 "script.y"
{ AddFunct(5,1); ;
    break;}
case 152:
#line 728 "script.y"
{ AddFunct(6,1); ;
    break;}
case 153:
#line 729 "script.y"
{ AddFunct(7,1); ;
    break;}
case 154:
#line 730 "script.y"
{ AddFunct(8,1); ;
    break;}
case 155:
#line 731 "script.y"
{ AddFunct(9,1); ;
    break;}
case 156:
#line 732 "script.y"
{ AddFunct(10,1); ;
    break;}
case 157:
#line 733 "script.y"
{ AddFunct(11,1); ;
    break;}
case 158:
#line 734 "script.y"
{ AddFunct(12,1); ;
    break;}
case 159:
#line 735 "script.y"
{ AddFunct(13,1); ;
    break;}
case 160:
#line 736 "script.y"
{ AddFunct(14,1); ;
    break;}
case 161:
#line 737 "script.y"
{ AddFunct(15,1); ;
    break;}
case 162:
#line 738 "script.y"
{ AddFunct(16,1); ;
    break;}
case 163:
#line 739 "script.y"
{ AddFunct(17,1); ;
    break;}
case 164:
#line 740 "script.y"
{ AddFunct(18,1); ;
    break;}
case 165:
#line 741 "script.y"
{ AddFunct(19,1); ;
    break;}
case 166:
#line 742 "script.y"
{ AddFunct(20,1); ;
    break;}
case 167:
#line 743 "script.y"
{ AddFunct(21,1); ;
    break;}
case 168:
#line 744 "script.y"
{ AddFunct(22,1); ;
    break;}
case 169:
#line 745 "script.y"
{ AddFunct(23,1); ;
    break;}
case 170:
#line 746 "script.y"
{ AddFunct(24,1); ;
    break;}
case 171:
#line 747 "script.y"
{ AddFunct(25,1); ;
    break;}
case 172:
#line 752 "script.y"
{ ;
    break;}
case 199:
#line 798 "script.y"
{ l=1-250000; AddBufArg(&l,1); ;
    break;}
case 200:
#line 799 "script.y"
{ l=2-250000; AddBufArg(&l,1); ;
    break;}
case 201:
#line 800 "script.y"
{ l=3-250000; AddBufArg(&l,1); ;
    break;}
case 202:
#line 801 "script.y"
{ l=4-250000; AddBufArg(&l,1); ;
    break;}
case 203:
#line 802 "script.y"
{ l=5-250000; AddBufArg(&l,1); ;
    break;}
case 204:
#line 803 "script.y"
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
#line 806 "script.y"

