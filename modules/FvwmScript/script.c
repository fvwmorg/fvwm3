
/*  A Bison parser, made from script.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	STR	257
#define	GSTR	258
#define	VAR	259
#define	FONT	260
#define	NUMBER	261
#define	WINDOWTITLE	262
#define	WINDOWSIZE	263
#define	WINDOWPOSITION	264
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
#define	CHFONT	303
#define	CHFORECOLOR	304
#define	CHBACKCOLOR	305
#define	CHCOLORSET	306
#define	KEY	307
#define	GETVALUE	308
#define	GETMINVALUE	309
#define	GETMAXVALUE	310
#define	GETFORE	311
#define	GETBACK	312
#define	GETHILIGHT	313
#define	GETSHADOW	314
#define	CHVALUE	315
#define	CHVALUEMAX	316
#define	CHVALUEMIN	317
#define	ADD	318
#define	DIV	319
#define	MULT	320
#define	GETTITLE	321
#define	GETOUTPUT	322
#define	STRCOPY	323
#define	NUMTOHEX	324
#define	HEXTONUM	325
#define	QUIT	326
#define	LAUNCHSCRIPT	327
#define	GETSCRIPTFATHER	328
#define	SENDTOSCRIPT	329
#define	RECEIVFROMSCRIPT	330
#define	GET	331
#define	SET	332
#define	SENDSIGN	333
#define	REMAINDEROFDIV	334
#define	GETTIME	335
#define	GETSCRIPTARG	336
#define	GETPID	337
#define	SENDMSGANDGET	338
#define	PARSE	339
#define	LASTSTRING	340
#define	IF	341
#define	THEN	342
#define	ELSE	343
#define	FOR	344
#define	TO	345
#define	DO	346
#define	WHILE	347
#define	BEGF	348
#define	ENDF	349
#define	EQUAL	350
#define	INFEQ	351
#define	SUPEQ	352
#define	INF	353
#define	SUP	354
#define	DIFF	355

#line 1 "script.y"

#include "types.h"

#define MAX_VARS 5120
extern int numligne;
ScriptProp *scriptprop;
int nbobj=-1;			/* Nombre d'objets */
int HasPosition,HasType=0;
TabObj *tabobj;		/* Tableau d'objets, limite=1000 */
int TabIdObj[1001];	/* Tableau d'indice des objets */
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
typedef union {	 char *str;
	  int number;
       } YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		448
#define	YYFLAG		-32768
#define	YYNTBASE	102

#define YYTRANSLATE(x) ((unsigned)(x) <= 355 ? yytranslate[x] : 167)

static const char yytranslate[] = {	0,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	1,     3,     4,     5,	    6,
     7,	    8,	   9,	 10,	11,    12,    13,    14,    15,	   16,
    17,	   18,	  19,	 20,	21,    22,    23,    24,    25,	   26,
    27,	   28,	  29,	 30,	31,    32,    33,    34,    35,	   36,
    37,	   38,	  39,	 40,	41,    42,    43,    44,    45,	   46,
    47,	   48,	  49,	 50,	51,    52,    53,    54,    55,	   56,
    57,	   58,	  59,	 60,	61,    62,    63,    64,    65,	   66,
    67,	   68,	  69,	 70,	71,    72,    73,    74,    75,	   76,
    77,	   78,	  79,	 80,	81,    82,    83,    84,    85,	   86,
    87,	   88,	  89,	 90,	91,    92,    93,    94,    95,	   96,
    97,	   98,	  99,	100,   101
};

#if YYDEBUG != 0
static const short yyprhs[] = {	    0,
     0,	    7,	   8,	  9,	13,    17,    22,    27,    31,	   35,
    39,	   43,	  47,	 50,	51,    57,    58,    64,    65,	   71,
    72,	   80,	  82,	 83,	87,    92,    97,   101,   105,	  109,
   113,	  117,	 121,	125,   129,   133,   137,   141,   144,	  148,
   149,	  152,	 155,	158,   161,   164,   167,   168,   170,	  176,
   177,	  178,	 183,	188,   190,   192,   194,   198,   199,	  203,
   207,	  211,	 215,	219,   223,   227,   231,   235,   239,	  243,
   247,	  251,	 255,	259,   263,   267,   271,   275,   279,	  283,
   287,	  291,	 295,	298,   301,   304,   307,   310,   313,	  316,
   319,	  322,	 325,	328,   331,   334,   337,   340,   343,	  346,
   349,	  352,	 355,	358,   361,   364,   367,   370,   373,	  378,
   383,	  388,	 395,	402,   407,   412,   417,   422,   427,	  432,
   438,	  443,	 444,	447,   452,   457,   468,   473,   477,	  481,
   489,	  490,	 494,	495,   499,   501,   505,   507,   517,	  525,
   527,	  529,	 531,	533,   535,   537,   538,   541,   544,	  549,
   553,	  556,	 560,	564,   568,   573,   576,   578,   581,	  585,
   587,	  590,	 593,	596,   599,   602,   605,   608,   610,	  615,
   619,	  621,	 622,	625,   628,   631,   634,   637,   640,	  646,
   648,	  650,	 652,	654,   656,   658,   663,   665,   667,	  669,
   671,	  676,	 678,	680,   685,   687,   689,   694,   696,	  698,
   700,	  702,	 704,	706
};

static const short yyrhs[] = {	 103,
   104,	  105,	 106,	107,   108,	0,     0,     0,   104,	    8,
     4,	    0,	 104,	 31,	 3,	0,   104,    10,     7,	    7,
     0,	  104,	   9,	  7,	 7,	0,   104,    12,     4,	    0,
   104,	   11,	   4,	  0,   104,    13,     4,     0,   104,	   14,
     4,	    0,	 104,	 15,	 7,	0,   104,     6,     0,	    0,
    17,	  147,	  44,	119,	21,	0,     0,    18,   147,	   44,
   119,	   21,	   0,	  0,	19,   147,    44,   119,    21,	    0,
     0,	  108,	  16,	109,	22,   110,   112,   113,     0,	    7,
     0,	    0,	 110,	 23,	 3,	0,   110,    24,     7,	    7,
     0,	  110,	  25,	  7,	 7,	0,   110,    26,     7,	    0,
   110,	   27,	   7,	  0,   110,    28,     7,     0,   110,	   29,
     4,	    0,	 110,	 30,	 4,	0,   110,    31,     3,	    0,
   110,	   12,	   4,	  0,   110,    11,     4,     0,   110,	   13,
     4,	    0,	 110,	 14,	 4,	0,   110,    15,     7,	    0,
   110,	    6,	   0,	110,	32,   111,     0,     0,   111,	   35,
     0,	  111,	  37,	  0,   111,    36,     0,   111,    38,	    0,
   111,	   39,	   0,	111,	40,	0,     0,    21,     0,	   20,
   114,	   41,	 115,	 21,	 0,	0,     0,   115,   116,	   45,
   118,	    0,	 115,	117,	45,   118,     0,    42,     0,	   43,
     0,	    7,	   0,	 44,   119,    21,     0,     0,   119,	   46,
   121,	    0,	 119,	 33,   138,	0,   119,    34,   140,	    0,
   119,	   47,	 122,	  0,   119,    48,   123,     0,   119,	   61,
   124,	    0,	 119,	 62,   125,	0,   119,    63,   126,	    0,
   119,	   25,	 127,	  0,   119,    24,   128,     0,   119,	   29,
   130,	    0,	 119,	 31,   129,	0,   119,    49,   131,	    0,
   119,	   50,	 132,	  0,   119,    51,   133,     0,   119,	   52,
   134,	    0,	 119,	 78,   135,	0,   119,    79,   136,	    0,
   119,	   72,	 137,	  0,   119,    75,   139,     0,   119,	   87,
   142,	    0,	 119,	 90,   143,	0,   119,    93,   144,	    0,
   119,	   53,	 141,	  0,	46,   121,     0,    33,   138,	    0,
    34,	  140,	   0,	 47,   122,	0,    48,   123,     0,	   61,
   124,	    0,	  62,	125,	 0,    63,   126,     0,    25,	  127,
     0,	   24,	 128,	  0,	29,   130,     0,    31,   129,	    0,
    49,	  131,	   0,	 50,   132,	0,    51,   133,     0,	   52,
   134,	    0,	  78,	135,	 0,    79,   136,     0,    72,	  137,
     0,	   75,	 139,	  0,	90,   143,     0,    93,   144,	    0,
    53,	  141,	   0,	158,   160,	0,   158,   162,     0,	  158,
   162,	    0,	 158,	162,   158,   162,     0,   158,   162,	  158,
   162,	    0,	 158,	162,   158,   162,     0,   158,   162,	  158,
   162,	  158,	 162,	  0,   158,   162,   158,   162,   158,	  162,
     0,	  158,	 162,	158,   163,	0,   158,   162,   158,	  164,
     0,	  158,	 162,	158,   160,	0,   158,   162,   158,	  164,
     0,	  158,	 162,	158,   164,	0,   158,   162,   158,	  162,
     0,	  158,	 165,	 77,   158,   160,     0,   158,   162,	  158,
   162,	    0,	   0,	158,   162,	0,   158,   162,   158,	  160,
     0,	  158,	 163,	158,   160,	0,   158,   163,   158,	  163,
   158,	  162,	 158,	162,   158,   160,     0,   145,   147,	  148,
   146,	    0,	 150,	147,   149,	0,   151,   147,   149,	    0,
   158,	  161,	 158,	166,   158,   161,    88,     0,     0,	   89,
   147,	  149,	   0,	  0,	44,   119,    21,     0,   120,	    0,
    44,	  119,	  21,	  0,   120,	0,   158,   165,    77,	  158,
   161,	   91,	 158,	161,	92,	0,   158,   161,   158,	  166,
   158,	  161,	  92,	  0,	 5,	0,     3,     0,     4,	    0,
     7,	    0,	  42,	  0,	43,	0,     0,    54,   162,	    0,
    67,	  162,	   0,	 68,   164,   162,   162,     0,    70,	  162,
   162,	    0,	  71,	164,	 0,    64,   162,   162,     0,	   66,
   162,	  162,	   0,	 65,   162,   162,     0,    69,   164,	  162,
   162,	    0,	  73,	164,	 0,    74,     0,    76,   162,	    0,
    80,	  162,	 162,	  0,	81,	0,    82,   162,     0,	   57,
   162,	    0,	  58,	162,	 0,    59,   162,     0,    60,	  162,
     0,	   55,	 162,	  0,	56,   162,     0,    83,     0,	   84,
   164,	  164,	 162,	  0,	85,   164,   162,     0,    86,	    0,
     0,	  156,	 160,	  0,   157,   160,     0,   152,   160,	    0,
   154,	  160,	   0,	153,   160,	0,   155,   160,     0,	   94,
   158,	  159,	  95,	160,	 0,   152,     0,   156,     0,	  157,
     0,	  154,	   0,	153,	 0,   155,     0,    94,   158,	  159,
    95,	    0,	 156,	  0,   157,	0,   155,     0,   152,	    0,
    94,	  158,	 159,	 95,	 0,   152,     0,   153,     0,	   94,
   158,	  159,	  95,	  0,   152,	0,   154,     0,    94,	  158,
   159,	   95,	   0,	152,	 0,    99,     0,    97,     0,	   96,
     0,	   98,	   0,	100,	 0,   101,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   376,	  379,	 383,	385,   390,   394,   400,   406,   412,	  418,
   424,	  430,	 435,	441,   442,   447,   448,   453,   454,	  463,
   464,	  467,	 483,	484,   488,   492,   497,   500,   503,	  506,
   509,	  512,	 515,	519,   523,   527,   531,   535,   538,	  540,
   541,	  544,	 547,	550,   553,   556,   562,   573,   574,	  577,
   579,	  580,	 581,	584,   585,   588,   591,   596,   597,	  598,
   599,	  600,	 601,	602,   603,   604,   605,   606,   607,	  608,
   609,	  610,	 611,	612,   613,   614,   615,   616,   617,	  618,
   619,	  620,	 624,	625,   626,   627,   628,   629,   630,	  631,
   632,	  633,	 634,	635,   636,   637,   638,   639,   640,	  641,
   642,	  643,	 644,	645,   646,   649,   651,   653,   655,	  657,
   659,	  661,	 663,	665,   667,   669,   671,   673,   675,	  677,
   679,	  681,	 683,	685,   687,   689,   691,   693,   695,	  699,
   701,	  702,	 704,	706,   707,   710,   711,   715,   719,	  724,
   726,	  728,	 730,	732,   734,   736,   738,   739,   740,	  741,
   742,	  743,	 744,	745,   746,   747,   748,   749,   750,	  751,
   752,	  753,	 754,	755,   756,   757,   758,   759,   760,	  761,
   762,	  767,	 768,	769,   770,   771,   772,   773,   774,	  778,
   779,	  780,	 781,	782,   783,   784,   788,   789,   790,	  791,
   792,	  796,	 797,	798,   802,   803,   804,   808,   812,	  813,
   814,	  815,	 816,	817
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {	  "$","error","$undefined.","STR","GSTR",
"VAR","FONT","NUMBER","WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FORECOLOR",
"BACKCOLOR","SHADCOLOR","LICOLOR","COLORSET","OBJECT","INIT","PERIODICTASK",
"QUITFUNC","MAIN","END","PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX",
"TITLE","SWALLOWEXEC","ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","NOFOCUS",
"NORELIEFSTRING","CENTER","LEFT","RIGHT","CASE","SINGLECLIC","DOUBLECLIC","BEG",
"POINT","EXEC","HIDE","SHOW","CHFONT","CHFORECOLOR","CHBACKCOLOR","CHCOLORSET",
"KEY","GETVALUE","GETMINVALUE","GETMAXVALUE","GETFORE","GETBACK","GETHILIGHT",
"GETSHADOW","CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT","GETTITLE",
"GETOUTPUT","STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT","GETSCRIPTFATHER",
"SENDTOSCRIPT","RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV","GETTIME",
"GETSCRIPTARG","GETPID","SENDMSGANDGET","PARSE","LASTSTRING","IF","THEN","ELSE",
"FOR","TO","DO","WHILE","BEGF","ENDF","EQUAL","INFEQ","SUPEQ","INF","SUP","DIFF",
"script","initvar","head","initbloc","periodictask","quitfunc","object","id",
"init","flags","verify","mainloop","addtabcase","case","clic","number","bloc",
"instr","oneinstr","exec","hide","show","chvalue","chvaluemax","chvaluemin",
"position","size","icon","title","font","chforecolor","chbackcolor","chcolorset",
"set","sendsign","quit","warp","sendtoscript","writetofile","key","ifthenelse",
"loop","while","headif","else","creerbloc","bloc1","bloc2","headloop","headwhile",
"var","str","gstr","num","singleclic2","doubleclic2","addlbuff","function","args",
"arg","numarg","strarg","gstrarg","vararg","compare", NULL
};
#endif

static const short yyr1[] = {	  0,
   102,	  103,	 104,	104,   104,   104,   104,   104,   104,	  104,
   104,	  104,	 104,	105,   105,   106,   106,   107,   107,	  108,
   108,	  109,	 110,	110,   110,   110,   110,   110,   110,	  110,
   110,	  110,	 110,	110,   110,   110,   110,   110,   110,	  111,
   111,	  111,	 111,	111,   111,   111,   112,   113,   113,	  114,
   115,	  115,	 115,	116,   116,   117,   118,   119,   119,	  119,
   119,	  119,	 119,	119,   119,   119,   119,   119,   119,	  119,
   119,	  119,	 119,	119,   119,   119,   119,   119,   119,	  119,
   119,	  119,	 120,	120,   120,   120,   120,   120,   120,	  120,
   120,	  120,	 120,	120,   120,   120,   120,   120,   120,	  120,
   120,	  120,	 120,	120,   120,   121,   122,   123,   124,	  125,
   126,	  127,	 128,	129,   130,   131,   132,   133,   134,	  135,
   136,	  137,	 138,	139,   140,   141,   142,   143,   144,	  145,
   146,	  146,	 147,	148,   148,   149,   149,   150,   151,	  152,
   153,	  154,	 155,	156,   157,   158,   159,   159,   159,	  159,
   159,	  159,	 159,	159,   159,   159,   159,   159,   159,	  159,
   159,	  159,	 159,	159,   159,   159,   159,   159,   159,	  159,
   159,	  160,	 160,	160,   160,   160,   160,   160,   160,	  161,
   161,	  161,	 161,	161,   161,   161,   162,   162,   162,	  162,
   162,	  163,	 163,	163,   164,   164,   164,   165,   166,	  166,
   166,	  166,	 166,	166
};

static const short yyr2[] = {	  0,
     6,	    0,	   0,	  3,	 3,	4,     4,     3,     3,	    3,
     3,	    3,	   2,	  0,	 5,	0,     5,     0,     5,	    0,
     7,	    1,	   0,	  3,	 4,	4,     3,     3,     3,	    3,
     3,	    3,	   3,	  3,	 3,	3,     3,     2,     3,	    0,
     2,	    2,	   2,	  2,	 2,	2,     0,     1,     5,	    0,
     0,	    4,	   4,	  1,	 1,	1,     3,     0,     3,	    3,
     3,	    3,	   3,	  3,	 3,	3,     3,     3,     3,	    3,
     3,	    3,	   3,	  3,	 3,	3,     3,     3,     3,	    3,
     3,	    3,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     2,	    2,
     2,	    2,	   2,	  2,	 2,	2,     2,     2,     4,	    4,
     4,	    6,	   6,	  4,	 4,	4,     4,     4,     4,	    5,
     4,	    0,	   2,	  4,	 4,    10,     4,     3,     3,	    7,
     0,	    3,	   0,	  3,	 1,	3,     1,     9,     7,	    1,
     1,	    1,	   1,	  1,	 1,	0,     2,     2,     4,	    3,
     2,	    3,	   3,	  3,	 4,	2,     1,     2,     3,	    1,
     2,	    2,	   2,	  2,	 2,	2,     2,     1,     4,	    3,
     1,	    0,	   2,	  2,	 2,	2,     2,     2,     5,	    1,
     1,	    1,	   1,	  1,	 1,	4,     1,     1,     1,	    1,
     4,	    1,	   1,	  4,	 1,	1,     4,     1,     1,	    1,
     1,	    1,	   1,	  1
};

static const short yydefact[] = {     2,
     3,	   14,	  13,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	  133,	   0,	 16,	 4,	0,     0,     9,     8,	   10,
    11,	   12,	   0,	  5,   133,    18,     7,     6,    58,	    0,
   133,	   20,	   0,	 58,	 0,	1,    15,   146,   146,	  146,
   146,	  146,	 146,	146,   146,   146,   146,   146,   146,	  146,
   146,	  146,	 146,	146,   122,   146,   146,   146,   146,	  146,
   146,	    0,	  58,	  0,	68,	0,    67,     0,    69,	    0,
    70,	    0,	  60,	  0,	61,	0,    59,   172,    62,	    0,
    63,	    0,	  71,	  0,	72,	0,    73,     0,    74,	    0,
    82,	    0,	  64,	  0,	65,	0,    66,     0,    77,	   78,
     0,	   75,	   0,	 76,	 0,    79,   133,     0,    80,	  133,
     0,	   81,	 133,	  0,	17,	0,    22,     0,   140,	  143,
   144,	  145,	 146,	190,   189,   187,   188,   146,   146,	  146,
   146,	  123,	 141,	146,   192,   193,   146,   142,   146,	  172,
   172,	  172,	 172,	172,   172,   106,   107,   108,   146,	  146,
   146,	  146,	 146,	146,   146,   146,   146,   198,     0,	  146,
     0,	  146,	 180,	184,   183,   185,   181,   182,   146,	    0,
     0,	    0,	 146,	 19,	23,	0,     0,     0,     0,	    0,
     0,	  172,	   0,	175,   177,   176,   178,   173,   174,	  172,
     0,	    0,	   0,	  0,	 0,	0,     0,   172,   146,	    0,
   146,	  146,	 146,	146,   146,   146,    58,   146,   146,	  146,
   146,	  146,	 146,	146,   146,   146,   146,   146,   122,	  146,
   146,	  146,	 146,	146,   135,   131,     0,     0,    58,	  137,
   128,	  146,	 129,	  0,	47,	0,     0,     0,     0,	    0,
     0,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	  157,	   0,	  0,   160,	0,   168,     0,     0,	  171,
     0,	  146,	 146,	146,   195,   196,   115,   114,     0,	  125,
     0,	  116,	 117,	118,   119,   146,   109,   110,   111,	  124,
   172,	  121,	  92,	 91,	93,    94,    84,    85,     0,	   83,
    86,	   87,	  95,	 96,	97,    98,   105,    88,    89,	   90,
   101,	  102,	  99,	100,   103,   104,   133,   127,     0,	  201,
   200,	  202,	 199,	203,   204,   146,     0,     0,   146,	   38,
     0,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	    0,	   0,	  0,	40,	0,   147,   166,   167,	  162,
   163,	  164,	 165,	  0,	 0,	0,   148,     0,     0,	    0,
   151,	  156,	 158,	  0,   161,	0,     0,   191,     0,	    0,
     0,	  194,	 172,	  0,   120,   134,     0,   186,     0,	  136,
     0,	    0,	  34,	 33,	35,    36,    37,    24,     0,	    0,
    27,	   28,	  29,	 30,	31,    32,    39,    50,    48,	   21,
   152,	  154,	 153,	  0,	 0,   150,   159,     0,   170,	  113,
   112,	    0,	 179,	146,   132,	0,   146,     0,    25,	   26,
    41,	   43,	  42,	 44,	45,    46,     0,   149,   155,	  169,
   197,	    0,	 130,	  0,   139,    51,   146,     0,     0,	  172,
   138,	   56,	  49,	 54,	55,	0,     0,   126,     0,	    0,
    58,	   52,	  53,	  0,	57,	0,     0,     0
};

static const short yydefgoto[] = {   446,
     1,	    2,	  14,	 26,	32,    36,   118,   235,   387,	  336,
   390,	  417,	 429,	436,   437,   442,    33,   230,    77,	   79,
    81,	   93,	  95,	 97,	67,    65,    71,    69,    83,	   85,
    87,	   89,	 102,	104,	99,    73,   100,    75,    91,	  106,
   109,	  112,	 107,	308,	23,   226,   231,   110,   113,	  124,
   141,	  142,	 125,	126,   127,    66,   261,   146,   169,	  128,
   137,	  267,	 159,	316
};

static const short yypact[] = {-32768,
-32768,	  503,-32768,	 21,	19,    20,    27,    52,    53,	   56,
    55,-32768,	  -2,	 51,-32768,    63,    65,-32768,-32768,-32768,
-32768,-32768,	  32,-32768,-32768,    61,-32768,-32768,-32768,	   38,
-32768,-32768,	 601,-32768,	39,    68,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,	  668,-32768,	 79,-32768,    25,-32768,    25,-32768,	   25,
-32768,	   25,-32768,	 25,-32768,    -1,-32768,    98,-32768,	   25,
-32768,	   25,-32768,	 25,-32768,    25,-32768,    25,-32768,	   25,
-32768,	   -1,-32768,	 25,-32768,    25,-32768,    25,-32768,-32768,
    25,-32768,	  83,-32768,	25,-32768,-32768,   260,-32768,-32768,
    83,-32768,-32768,	260,-32768,   720,-32768,    69,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,	   98,
    98,	   98,	  98,	 98,	98,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    13,-32768,
   928,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,	  980,
    17,	  980,-32768,-32768,-32768,  1020,    25,    25,     4,	   -1,
  1020,	   98,	1020,-32768,-32768,-32768,-32768,-32768,-32768,	   98,
     4,	    4,	  25,	 -1,	25,    25,    25,    98,-32768,	   25,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,	8,  1020,   -86,-32768,-32768,
-32768,-32768,-32768,	-86,  1101,    25,    25,    25,    25,	   25,
    25,	   25,	  25,	 25,	25,    25,     4,     4,    25,	    4,
     4,-32768,	  25,	 25,-32768,    25,-32768,     4,     4,-32768,
    11,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    23,-32768,
    26,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    98,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   772,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    31,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   824,   260,-32768,-32768,
   104,	  116,	 123,	125,   127,   129,   128,   130,   132,	  135,
   145,	  126,	 149,	133,-32768,	3,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,	 25,	25,    25,-32768,    25,    25,	   25,
-32768,-32768,-32768,	 25,-32768,	4,    25,-32768,    25,	   25,
  1020,-32768,	  98,	 25,-32768,-32768,   980,-32768,   260,-32768,
    64,	  260,-32768,-32768,-32768,-32768,-32768,-32768,   151,	  153,
-32768,-32768,-32768,-32768,-32768,-32768,    76,-32768,-32768,-32768,
-32768,-32768,-32768,	 25,	25,-32768,-32768,    25,-32768,-32768,
-32768,	   66,-32768,-32768,-32768,    71,-32768,    70,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   124,-32768,-32768,-32768,
-32768,	   25,-32768,	260,-32768,-32768,-32768,    72,    57,	   98,
-32768,-32768,-32768,-32768,-32768,   121,   131,-32768,   134,	  134,
-32768,-32768,-32768,	876,-32768,   170,   171,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,  -268,   -34,    14,   -31,	  -35,
   -30,	  -37,	 -36,	-33,   -20,   -12,   -14,    10,   -17,	    7,
     9,	    6,	   0,	  1,	 5,    33,    22,    35,    34,-32768,
     2,	   28,-32768,-32768,	-3,-32768,  -166,-32768,-32768,	  177,
   167,	  152,	 184,	298,   309,    -6,  -178,  -124,    24,	   -9,
   -85,	  213,	 137,	 12
};


#define	YYLAST		1133


static const short yytable[] = {    62,
    24,	  133,	 269,	119,   271,   233,   153,   138,   119,	  310,
   311,	  312,	 313,	314,   315,   184,   185,   186,   187,	  188,
   189,	   30,	 388,	389,	15,    16,    17,    35,   116,	  119,
    18,	  120,	  68,	 70,	72,    74,    76,    78,    80,	   82,
    84,	   86,	  88,	 90,	92,    94,    96,    98,   309,	  101,
   103,	  105,	 108,	111,   114,    19,    20,   270,   129,	   21,
   130,	   22,	 131,	432,   132,   272,   121,   122,    25,	   27,
   147,	   28,	 148,	280,   149,    29,   150,   433,   151,	   31,
   152,	   34,	  63,	 64,   154,   117,   155,   119,   156,	  199,
   175,	  157,	 134,	232,   268,   160,   307,   264,   434,	  435,
   133,	  138,	 119,	161,   120,   358,   170,   373,   276,	  172,
   411,	  412,	 413,	414,   415,   416,   176,   362,   123,	  374,
   363,	  177,	 178,	179,   180,   368,   375,   181,   376,	  384,
   182,	  378,	 183,	377,   379,   386,   380,   173,   381,	  121,
   122,	  382,	 190,	191,   192,   193,   194,   195,   196,	  197,
   198,	  383,	 385,	200,   407,   227,   365,   409,   423,	  410,
   421,	  425,	 228,	431,   426,   439,   234,   262,   263,	  447,
   448,	  443,	 289,	291,   225,   440,   290,   441,   298,	  292,
   299,	  284,	 402,	275,   300,   277,   278,   279,   283,	  286,
   282,	  139,	 281,	293,   317,    68,    70,    72,    74,	   76,
   405,	   78,	  80,	 82,	84,    86,    88,    90,    92,	   94,
    96,	   98,	 285,	101,   103,   105,   111,   114,   294,	  296,
   303,	  295,	 304,	301,   305,   318,   337,   338,   339,	  340,
   341,	  342,	 343,	344,   345,   346,   347,   287,   403,	  350,
   288,	  302,	 136,	353,   354,   319,   355,   171,   297,	    0,
     0,	  306,	 135,	  0,   140,   359,   360,   361,   136,	  165,
     0,	  143,	 133,	138,   119,   165,   120,     0,   135,	  364,
     0,	    0,	   0,	  0,   164,	0,     0,     0,     0,	  158,
   164,	    0,	   0,	  0,   163,	0,     0,   158,     0,	    0,
   163,	  166,	   0,	  0,	 0,	0,     0,   166,     0,	    0,
     0,	  121,	 122,	367,	 0,   438,     0,     0,     0,	  369,
     0,	    0,	 372,	  0,	 0,	0,   140,   140,   140,	  140,
   140,	  140,	   0,	143,   143,   143,   143,   143,   143,	    0,
   266,	    0,	   0,	  0,   391,   392,   393,     0,   394,	  395,
   396,	  371,	 266,	266,   397,	0,   136,   399,     0,	  400,
   401,	    0,	   0,	162,   404,   265,   135,     0,   140,	    0,
   136,	    0,	   0,	  0,	 0,   143,   140,   265,   265,	    0,
   135,	    0,	   0,	143,   140,   144,     0,     0,     0,	    0,
     0,	  143,	   0,	  0,   418,   419,   145,     0,   420,	    0,
     0,	    0,	 406,	  0,	 0,   408,     0,   422,   266,	  266,
   424,	  266,	 266,	273,   274,   167,   444,     0,     0,	  266,
   266,	  167,	 427,	  0,	 0,	0,   168,     0,     0,	    0,
   430,	    0,	 168,	265,   265,	0,   265,   265,     0,	    0,
     0,	    0,	   0,	  0,   265,   265,     0,   144,   144,	  144,
   144,	  144,	 144,	  0,	 0,	0,     0,   428,   145,	  145,
   145,	  145,	 145,	145,	 0,	0,     0,   140,     0,	  348,
   349,	    0,	 351,	352,   143,	0,     0,     0,     0,	  165,
   356,	  357,	   0,	  0,	 0,	0,     0,     0,     0,	  144,
     0,	    0,	   0,	  0,   164,	0,     0,   144,     0,	    0,
   145,	    0,	   0,	  0,   163,   144,     0,     0,   145,	    0,
     0,	  166,	   0,	  0,	 0,	0,   145,   266,     3,	    0,
     4,	    5,	   6,	  7,	 8,	9,    10,    11,     0,	   12,
   165,	    0,	   0,	165,	 0,	0,     0,     0,     0,	    0,
     0,	    0,	 265,	 13,	 0,   164,     0,     0,   164,	  140,
     0,	    0,	   0,	  0,	 0,   163,   143,     0,   163,	    0,
     0,	    0,	 166,	  0,	 0,   166,     0,     0,     0,	    0,
     0,	    0,	   0,	  0,	 0,	0,     0,     0,   398,	    0,
     0,	    0,	   0,	  0,	 0,   165,     0,     0,   144,	    0,
     0,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	  145,
   164,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	    0,
   163,	    0,	   0,	  0,	 0,	0,   140,   166,     0,	    0,
     0,	    0,	   0,	143,	 0,   167,     0,     0,     0,	    0,
     0,	   37,	   0,	  0,	38,    39,   168,     0,     0,	   40,
     0,	   41,	   0,	 42,	43,	0,     0,     0,     0,	    0,
     0,	    0,	   0,	  0,	 0,	0,    44,    45,    46,	   47,
    48,	   49,	  50,	 51,	 0,	0,     0,     0,     0,	    0,
   144,	   52,	  53,	 54,	 0,	0,   167,     0,     0,	  167,
     0,	  145,	  55,	  0,	 0,    56,     0,   168,    57,	   58,
   168,	    0,	   0,	  0,	 0,	0,     0,    59,   115,	    0,
    60,	   38,	  39,	 61,	 0,	0,    40,     0,    41,	    0,
    42,	   43,	   0,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	    0,	   0,	 44,	45,    46,    47,    48,    49,	   50,
    51,	  167,	   0,	  0,	 0,	0,     0,   144,    52,	   53,
    54,	    0,	 168,	  0,	 0,	0,     0,     0,   145,	   55,
   174,	    0,	  56,	 38,	39,    57,    58,     0,    40,	    0,
    41,	    0,	  42,	 43,	59,	0,     0,    60,     0,	    0,
    61,	    0,	   0,	  0,	 0,    44,    45,    46,    47,	   48,
    49,	   50,	  51,	  0,	 0,	0,     0,     0,     0,	    0,
    52,	   53,	  54,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	   55,	 366,	  0,	56,    38,    39,    57,    58,	    0,
    40,	    0,	  41,	  0,	42,    43,    59,     0,     0,	   60,
     0,	    0,	  61,	  0,	 0,	0,     0,    44,    45,	   46,
    47,	   48,	  49,	 50,	51,	0,     0,     0,     0,	    0,
     0,	    0,	  52,	 53,	54,	0,     0,     0,     0,	    0,
     0,	    0,	   0,	 55,   370,	0,    56,    38,    39,	   57,
    58,	    0,	  40,	  0,	41,	0,    42,    43,    59,	    0,
     0,	   60,	   0,	  0,	61,	0,     0,     0,     0,	   44,
    45,	   46,	  47,	 48,	49,    50,    51,     0,     0,	    0,
     0,	    0,	   0,	  0,	52,    53,    54,     0,     0,	    0,
     0,	    0,	   0,	  0,	 0,    55,   445,     0,    56,	   38,
    39,	   57,	  58,	  0,	40,	0,    41,     0,    42,	   43,
    59,	    0,	   0,	 60,	 0,	0,    61,     0,     0,	    0,
     0,	   44,	  45,	 46,	47,    48,    49,    50,    51,	    0,
     0,	    0,	   0,	  0,	 0,	0,    52,    53,    54,	    0,
     0,	    0,	   0,	  0,	 0,	0,     0,    55,     0,	    0,
    56,	  201,	 202,	 57,	58,	0,   203,     0,   204,	    0,
   205,	  206,	  59,	  0,	 0,    60,     0,     0,    61,	    0,
     0,	  207,	   0,	208,   209,   210,   211,   212,   213,	  214,
   215,	    0,	   0,	  0,	 0,	0,     0,     0,   216,	  217,
   218,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	  219,
     0,	    0,	 220,	201,   202,   221,   222,     0,   203,	    0,
   204,	    0,	 205,	206,	 0,	0,     0,   223,     0,	    0,
   224,	    0,	   0,	229,	 0,   208,   209,   210,   211,	  212,
   213,	  214,	 215,	  0,	 0,	0,     0,     0,     0,	    0,
   216,	  217,	 218,	  0,	 0,	0,     0,     0,     0,	    0,
     0,	  219,	   0,	  0,   220,	0,     0,   221,   222,	    0,
     0,	    0,	   0,	  0,	 0,	0,     0,     0,     0,	  223,
     0,	    0,	 224,	236,   237,   238,   239,   240,   241,	  242,
     0,	    0,	   0,	243,   244,   245,   246,   247,   248,	  249,
   250,	    0,	 251,	252,	 0,   253,     0,     0,     0,	  254,
   255,	  256,	 257,	258,   259,   260,   320,     0,     0,	    0,
     0,	  321,	 322,	323,   324,   325,     0,     0,     0,	    0,
     0,	    0,	   0,	326,   327,   328,   329,   330,   331,	  332,
   333,	  334,	 335
};

static const short yycheck[] = {    34,
     3,	    3,	 181,	  5,   183,   172,    92,     4,     5,	   96,
    97,	   98,	  99,	100,   101,   140,   141,   142,   143,	  144,
   145,	   25,	  20,	 21,	 4,	7,     7,    31,    63,	    5,
     4,	    7,	  39,	 40,	41,    42,    43,    44,    45,	   46,
    47,	   48,	  49,	 50,	51,    52,    53,    54,   227,	   56,
    57,	   58,	  59,	 60,	61,	4,     4,   182,    68,	    4,
    70,	    7,	  72,	  7,	74,   190,    42,    43,    18,	    7,
    80,	    7,	  82,	198,	84,    44,    86,    21,    88,	   19,
    90,	   44,	  44,	 16,	94,	7,    96,     5,    98,	   77,
    22,	  101,	  94,	 77,   180,   105,    89,    94,    42,	   43,
     3,	    4,	   5,	107,	 7,    95,   110,     4,   194,	  113,
    35,	   36,	  37,	 38,	39,    40,   123,    95,    94,	    4,
    95,	  128,	 129,	130,   131,    95,     4,   134,     4,	    4,
   137,	    3,	 139,	  7,	 7,	3,     7,   114,     7,	   42,
    43,	    7,	 149,	150,   151,   152,   153,   154,   155,	  156,
   157,	    7,	   4,	160,	91,   162,   281,     7,    88,	    7,
    95,	   92,	 169,	 92,	41,    45,   173,   177,   178,	    0,
     0,	  440,	 207,	209,   161,    45,   208,    44,   216,	  210,
   217,	  202,	 361,	193,   218,   195,   196,   197,   201,	  204,
   200,	   94,	 199,	211,   229,   202,   203,   204,   205,	  206,
   367,	  208,	 209,	210,   211,   212,   213,   214,   215,	  216,
   217,	  218,	 203,	220,   221,   222,   223,   224,   212,	  214,
   221,	  213,	 222,	219,   223,   232,   236,   237,   238,	  239,
   240,	  241,	 242,	243,   244,   245,   246,   205,   363,	  249,
   206,	  220,	  76,	253,   254,   234,   256,   111,   215,	   -1,
    -1,	  224,	  76,	 -1,	78,   262,   263,   264,    92,	  108,
    -1,	   78,	   3,	  4,	 5,   114,     7,    -1,    92,	  276,
    -1,	   -1,	  -1,	 -1,   108,    -1,    -1,    -1,    -1,	  103,
   114,	   -1,	  -1,	 -1,   108,    -1,    -1,   111,    -1,	   -1,
   114,	  108,	  -1,	 -1,	-1,    -1,    -1,   114,    -1,	   -1,
    -1,	   42,	  43,	307,	-1,   430,    -1,    -1,    -1,	  316,
    -1,	   -1,	 319,	 -1,	-1,    -1,   140,   141,   142,	  143,
   144,	  145,	  -1,	140,   141,   142,   143,   144,   145,	   -1,
   179,	   -1,	  -1,	 -1,   344,   345,   346,    -1,   348,	  349,
   350,	  318,	 191,	192,   354,    -1,   180,   357,    -1,	  359,
   360,	   -1,	  -1,	 94,   364,   179,   180,    -1,   182,	   -1,
   194,	   -1,	  -1,	 -1,	-1,   182,   190,   191,   192,	   -1,
   194,	   -1,	  -1,	190,   198,    78,    -1,    -1,    -1,	   -1,
    -1,	  198,	  -1,	 -1,   394,   395,    78,    -1,   398,	   -1,
    -1,	   -1,	 369,	 -1,	-1,   372,    -1,   404,   247,	  248,
   407,	  250,	 251,	191,   192,   108,   441,    -1,    -1,	  258,
   259,	  114,	 422,	 -1,	-1,    -1,   108,    -1,    -1,	   -1,
   427,	   -1,	 114,	247,   248,    -1,   250,   251,    -1,	   -1,
    -1,	   -1,	  -1,	 -1,   258,   259,    -1,   140,   141,	  142,
   143,	  144,	 145,	 -1,	-1,    -1,    -1,   424,   140,	  141,
   142,	  143,	 144,	145,	-1,    -1,    -1,   281,    -1,	  247,
   248,	   -1,	 250,	251,   281,    -1,    -1,    -1,    -1,	  318,
   258,	  259,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	  182,
    -1,	   -1,	  -1,	 -1,   318,    -1,    -1,   190,    -1,	   -1,
   182,	   -1,	  -1,	 -1,   318,   198,    -1,    -1,   190,	   -1,
    -1,	  318,	  -1,	 -1,	-1,    -1,   198,   356,     6,	   -1,
     8,	    9,	  10,	 11,	12,    13,    14,    15,    -1,	   17,
   369,	   -1,	  -1,	372,	-1,    -1,    -1,    -1,    -1,	   -1,
    -1,	   -1,	 356,	 31,	-1,   369,    -1,    -1,   372,	  363,
    -1,	   -1,	  -1,	 -1,	-1,   369,   363,    -1,   372,	   -1,
    -1,	   -1,	 369,	 -1,	-1,   372,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,   356,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,   424,    -1,    -1,   281,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	  281,
   424,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
   424,	   -1,	  -1,	 -1,	-1,    -1,   430,   424,    -1,	   -1,
    -1,	   -1,	  -1,	430,	-1,   318,    -1,    -1,    -1,	   -1,
    -1,	   21,	  -1,	 -1,	24,    25,   318,    -1,    -1,	   29,
    -1,	   31,	  -1,	 33,	34,    -1,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    46,    47,    48,	   49,
    50,	   51,	  52,	 53,	-1,    -1,    -1,    -1,    -1,	   -1,
   363,	   61,	  62,	 63,	-1,    -1,   369,    -1,    -1,	  372,
    -1,	  363,	  72,	 -1,	-1,    75,    -1,   369,    78,	   79,
   372,	   -1,	  -1,	 -1,	-1,    -1,    -1,    87,    21,	   -1,
    90,	   24,	  25,	 93,	-1,    -1,    29,    -1,    31,	   -1,
    33,	   34,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 46,	47,    48,    49,    50,    51,	   52,
    53,	  424,	  -1,	 -1,	-1,    -1,    -1,   430,    61,	   62,
    63,	   -1,	 424,	 -1,	-1,    -1,    -1,    -1,   430,	   72,
    21,	   -1,	  75,	 24,	25,    78,    79,    -1,    29,	   -1,
    31,	   -1,	  33,	 34,	87,    -1,    -1,    90,    -1,	   -1,
    93,	   -1,	  -1,	 -1,	-1,    46,    47,    48,    49,	   50,
    51,	   52,	  53,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
    61,	   62,	  63,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
    -1,	   72,	  21,	 -1,	75,    24,    25,    78,    79,	   -1,
    29,	   -1,	  31,	 -1,	33,    34,    87,    -1,    -1,	   90,
    -1,	   -1,	  93,	 -1,	-1,    -1,    -1,    46,    47,	   48,
    49,	   50,	  51,	 52,	53,    -1,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  61,	 62,	63,    -1,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 72,	21,    -1,    75,    24,    25,	   78,
    79,	   -1,	  29,	 -1,	31,    -1,    33,    34,    87,	   -1,
    -1,	   90,	  -1,	 -1,	93,    -1,    -1,    -1,    -1,	   46,
    47,	   48,	  49,	 50,	51,    52,    53,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 -1,	61,    62,    63,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    72,    21,    -1,    75,	   24,
    25,	   78,	  79,	 -1,	29,    -1,    31,    -1,    33,	   34,
    87,	   -1,	  -1,	 90,	-1,    -1,    93,    -1,    -1,	   -1,
    -1,	   46,	  47,	 48,	49,    50,    51,    52,    53,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    61,    62,    63,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    -1,    72,    -1,	   -1,
    75,	   24,	  25,	 78,	79,    -1,    29,    -1,    31,	   -1,
    33,	   34,	  87,	 -1,	-1,    90,    -1,    -1,    93,	   -1,
    -1,	   44,	  -1,	 46,	47,    48,    49,    50,    51,	   52,
    53,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,    61,	   62,
    63,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	   72,
    -1,	   -1,	  75,	 24,	25,    78,    79,    -1,    29,	   -1,
    31,	   -1,	  33,	 34,	-1,    -1,    -1,    90,    -1,	   -1,
    93,	   -1,	  -1,	 44,	-1,    46,    47,    48,    49,	   50,
    51,	   52,	  53,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
    61,	   62,	  63,	 -1,	-1,    -1,    -1,    -1,    -1,	   -1,
    -1,	   72,	  -1,	 -1,	75,    -1,    -1,    78,    79,	   -1,
    -1,	   -1,	  -1,	 -1,	-1,    -1,    -1,    -1,    -1,	   90,
    -1,	   -1,	  93,	 54,	55,    56,    57,    58,    59,	   60,
    -1,	   -1,	  -1,	 64,	65,    66,    67,    68,    69,	   70,
    71,	   -1,	  73,	 74,	-1,    76,    -1,    -1,    -1,	   80,
    81,	   82,	  83,	 84,	85,    86,     6,    -1,    -1,	   -1,
    -1,	   11,	  12,	 13,	14,    15,    -1,    -1,    -1,	   -1,
    -1,	   -1,	  -1,	 23,	24,    25,    26,    27,    28,	   29,
    30,	   31,	  32
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
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.	*/

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
   since that symbol is in the user namespace.	*/
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.	 rms, 2 May 1997.  */
/* #include <malloc.h>	*/
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
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()	(!!yyerrstatus)
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

int yynerrs;			/*  number of parse errors so far	*/
#endif	/* not YYPURE */

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

/* Define __yy_memcpy.	Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.	 With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.	*/
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
   in available built-in functions on various systems.	*/
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
   to the proper pointer type.	*/

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
     The wasted elements are never initialized.	 */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in	yystate	 .  */
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
	 but that might be undefined if yyoverflow is a macro.	*/
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
      /* Extend the stack our own way.	*/
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
     or a valid token in external form.	 */

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

  /* Shift the lookahead token.	 */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.	*/
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

/* Do the default action for the current state.	 */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.	 */
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
#line 386 "script.y"
{
	/* Titre de la fenetre */
	scriptprop->titlewin=yyvsp[0].str;
;
    break;}
case 5:
#line 391 "script.y"
{
	scriptprop->icon=yyvsp[0].str;
;
    break;}
case 6:
#line 395 "script.y"
{
	/* Position et taille de la fenetre */
	scriptprop->x=yyvsp[-1].number;
	scriptprop->y=yyvsp[0].number;
;
    break;}
case 7:
#line 401 "script.y"
{
	/* Position et taille de la fenetre */
	scriptprop->width=yyvsp[-1].number;
	scriptprop->height=yyvsp[0].number;
;
    break;}
case 8:
#line 407 "script.y"
{
	/* Couleur de fond */
	scriptprop->backcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
;
    break;}
case 9:
#line 413 "script.y"
{
	/* Couleur des lignes */
	scriptprop->forecolor=yyvsp[0].str;
	scriptprop->colorset = -1;
;
    break;}
case 10:
#line 419 "script.y"
{
	/* Couleur des lignes */
	scriptprop->shadcolor=yyvsp[0].str;
	scriptprop->colorset = -1;
;
    break;}
case 11:
#line 425 "script.y"
{
	/* Couleur des lignes */
	scriptprop->hilicolor=yyvsp[0].str;
	scriptprop->colorset = -1;
;
    break;}
case 12:
#line 431 "script.y"
{
	scriptprop->colorset = yyvsp[0].number;
	AllocColorset(yyvsp[0].number);
;
    break;}
case 13:
#line 436 "script.y"
{
	scriptprop->font=yyvsp[0].str;
;
    break;}
case 15:
#line 442 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--;
				;
    break;}
case 17:
#line 448 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--;
				;
    break;}
case 19:
#line 454 "script.y"
{
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--;
				;
    break;}
case 22:
#line 467 "script.y"
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
#line 484 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				;
    break;}
case 25:
#line 488 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				;
    break;}
case 26:
#line 492 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				;
    break;}
case 27:
#line 497 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				;
    break;}
case 28:
#line 500 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				;
    break;}
case 29:
#line 503 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				;
    break;}
case 30:
#line 506 "script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[0].str;
				;
    break;}
case 31:
#line 509 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				;
    break;}
case 32:
#line 512 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				;
    break;}
case 33:
#line 515 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 34:
#line 519 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 35:
#line 523 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 36:
#line 527 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				;
    break;}
case 37:
#line 531 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				;
    break;}
case 38:
#line 535 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				;
    break;}
case 41:
#line 541 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				;
    break;}
case 42:
#line 544 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				;
    break;}
case 43:
#line 547 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				;
    break;}
case 44:
#line 550 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				;
    break;}
case 45:
#line 553 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				;
    break;}
case 46:
#line 556 "script.y"
{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
				;
    break;}
case 47:
#line 562 "script.y"
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
#line 573 "script.y"
{ InitObjTabCase(0); ;
    break;}
case 50:
#line 577 "script.y"
{ InitObjTabCase(1); ;
    break;}
case 54:
#line 584 "script.y"
{ InitCase(-1); ;
    break;}
case 55:
#line 585 "script.y"
{ InitCase(-2); ;
    break;}
case 56:
#line 588 "script.y"
{ InitCase(yyvsp[0].number); ;
    break;}
case 106:
#line 649 "script.y"
{ AddCom(1,1); ;
    break;}
case 107:
#line 651 "script.y"
{ AddCom(2,1);;
    break;}
case 108:
#line 653 "script.y"
{ AddCom(3,1);;
    break;}
case 109:
#line 655 "script.y"
{ AddCom(4,2);;
    break;}
case 110:
#line 657 "script.y"
{ AddCom(21,2);;
    break;}
case 111:
#line 659 "script.y"
{ AddCom(22,2);;
    break;}
case 112:
#line 661 "script.y"
{ AddCom(5,3);;
    break;}
case 113:
#line 663 "script.y"
{ AddCom(6,3);;
    break;}
case 114:
#line 665 "script.y"
{ AddCom(7,2);;
    break;}
case 115:
#line 667 "script.y"
{ AddCom(8,2);;
    break;}
case 116:
#line 669 "script.y"
{ AddCom(9,2);;
    break;}
case 117:
#line 671 "script.y"
{ AddCom(10,2);;
    break;}
case 118:
#line 673 "script.y"
{ AddCom(19,2);;
    break;}
case 119:
#line 675 "script.y"
{ AddCom(24,2);;
    break;}
case 120:
#line 677 "script.y"
{ AddCom(11,2);;
    break;}
case 121:
#line 679 "script.y"
{ AddCom(12,2);;
    break;}
case 122:
#line 681 "script.y"
{ AddCom(13,0);;
    break;}
case 123:
#line 683 "script.y"
{ AddCom(17,1);;
    break;}
case 124:
#line 685 "script.y"
{ AddCom(23,2);;
    break;}
case 125:
#line 687 "script.y"
{ AddCom(18,2);;
    break;}
case 126:
#line 689 "script.y"
{ AddCom(25,5);;
    break;}
case 130:
#line 699 "script.y"
{ AddComBloc(14,3,2); ;
    break;}
case 133:
#line 704 "script.y"
{ EmpilerBloc(); ;
    break;}
case 134:
#line 706 "script.y"
{ DepilerBloc(2); ;
    break;}
case 135:
#line 707 "script.y"
{ DepilerBloc(2); ;
    break;}
case 136:
#line 710 "script.y"
{ DepilerBloc(1); ;
    break;}
case 137:
#line 711 "script.y"
{ DepilerBloc(1); ;
    break;}
case 138:
#line 715 "script.y"
{ AddComBloc(15,3,1); ;
    break;}
case 139:
#line 719 "script.y"
{ AddComBloc(16,3,1); ;
    break;}
case 140:
#line 724 "script.y"
{ AddVar(yyvsp[0].str); ;
    break;}
case 141:
#line 726 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 142:
#line 728 "script.y"
{ AddConstStr(yyvsp[0].str); ;
    break;}
case 143:
#line 730 "script.y"
{ AddConstNum(yyvsp[0].number); ;
    break;}
case 144:
#line 732 "script.y"
{ AddConstNum(-1); ;
    break;}
case 145:
#line 734 "script.y"
{ AddConstNum(-2); ;
    break;}
case 146:
#line 736 "script.y"
{ AddLevelBufArg(); ;
    break;}
case 147:
#line 738 "script.y"
{ AddFunct(1,1); ;
    break;}
case 148:
#line 739 "script.y"
{ AddFunct(2,1); ;
    break;}
case 149:
#line 740 "script.y"
{ AddFunct(3,1); ;
    break;}
case 150:
#line 741 "script.y"
{ AddFunct(4,1); ;
    break;}
case 151:
#line 742 "script.y"
{ AddFunct(5,1); ;
    break;}
case 152:
#line 743 "script.y"
{ AddFunct(6,1); ;
    break;}
case 153:
#line 744 "script.y"
{ AddFunct(7,1); ;
    break;}
case 154:
#line 745 "script.y"
{ AddFunct(8,1); ;
    break;}
case 155:
#line 746 "script.y"
{ AddFunct(9,1); ;
    break;}
case 156:
#line 747 "script.y"
{ AddFunct(10,1); ;
    break;}
case 157:
#line 748 "script.y"
{ AddFunct(11,1); ;
    break;}
case 158:
#line 749 "script.y"
{ AddFunct(12,1); ;
    break;}
case 159:
#line 750 "script.y"
{ AddFunct(13,1); ;
    break;}
case 160:
#line 751 "script.y"
{ AddFunct(14,1); ;
    break;}
case 161:
#line 752 "script.y"
{ AddFunct(15,1); ;
    break;}
case 162:
#line 753 "script.y"
{ AddFunct(16,1); ;
    break;}
case 163:
#line 754 "script.y"
{ AddFunct(17,1); ;
    break;}
case 164:
#line 755 "script.y"
{ AddFunct(18,1); ;
    break;}
case 165:
#line 756 "script.y"
{ AddFunct(19,1); ;
    break;}
case 166:
#line 757 "script.y"
{ AddFunct(20,1); ;
    break;}
case 167:
#line 758 "script.y"
{ AddFunct(21,1); ;
    break;}
case 168:
#line 759 "script.y"
{ AddFunct(22,1); ;
    break;}
case 169:
#line 760 "script.y"
{ AddFunct(23,1); ;
    break;}
case 170:
#line 761 "script.y"
{ AddFunct(24,1); ;
    break;}
case 171:
#line 762 "script.y"
{ AddFunct(25,1); ;
    break;}
case 172:
#line 767 "script.y"
{ ;
    break;}
case 199:
#line 812 "script.y"
{ l=1-250000; AddBufArg(&l,1); ;
    break;}
case 200:
#line 813 "script.y"
{ l=2-250000; AddBufArg(&l,1); ;
    break;}
case 201:
#line 814 "script.y"
{ l=3-250000; AddBufArg(&l,1); ;
    break;}
case 202:
#line 815 "script.y"
{ l=4-250000; AddBufArg(&l,1); ;
    break;}
case 203:
#line 816 "script.y"
{ l=5-250000; AddBufArg(&l,1); ;
    break;}
case 204:
#line 817 "script.y"
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
      /* if just tried and failed to reuse lookahead token after an error, discard it.	*/

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
     should shift them.	 */
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
#line 820 "script.y"

