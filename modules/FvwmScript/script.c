#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "script.y"
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
#line 358 "y.tab.c"
#define STR 257
#define GSTR 258
#define VAR 259
#define NUMBER 260
#define WINDOWTITLE 261
#define WINDOWSIZE 262
#define WINDOWPOSITION 263
#define FONT 264
#define FORECOLOR 265
#define BACKCOLOR 266
#define SHADCOLOR 267
#define LICOLOR 268
#define COLORSET 269
#define OBJECT 270
#define INIT 271
#define PERIODICTASK 272
#define MAIN 273
#define END 274
#define PROP 275
#define TYPE 276
#define SIZE 277
#define POSITION 278
#define VALUE 279
#define VALUEMIN 280
#define VALUEMAX 281
#define TITLE 282
#define SWALLOWEXEC 283
#define ICON 284
#define FLAGS 285
#define WARP 286
#define WRITETOFILE 287
#define HIDDEN 288
#define CANBESELECTED 289
#define NORELIEFSTRING 290
#define CASE 291
#define SINGLECLIC 292
#define DOUBLECLIC 293
#define BEG 294
#define POINT 295
#define EXEC 296
#define HIDE 297
#define SHOW 298
#define CHFORECOLOR 299
#define CHBACKCOLOR 300
#define CHCOLORSET 301
#define GETVALUE 302
#define GETMINVALUE 303
#define GETMAXVALUE 304
#define GETFORE 305
#define GETBACK 306
#define GETHILIGHT 307
#define GETSHADOW 308
#define CHVALUE 309
#define CHVALUEMAX 310
#define CHVALUEMIN 311
#define ADD 312
#define DIV 313
#define MULT 314
#define GETTITLE 315
#define GETOUTPUT 316
#define STRCOPY 317
#define NUMTOHEX 318
#define HEXTONUM 319
#define QUIT 320
#define LAUNCHSCRIPT 321
#define GETSCRIPTFATHER 322
#define SENDTOSCRIPT 323
#define RECEIVFROMSCRIPT 324
#define GET 325
#define SET 326
#define SENDSIGN 327
#define REMAINDEROFDIV 328
#define GETTIME 329
#define GETSCRIPTARG 330
#define IF 331
#define THEN 332
#define ELSE 333
#define FOR 334
#define TO 335
#define DO 336
#define WHILE 337
#define BEGF 338
#define ENDF 339
#define EQUAL 340
#define INFEQ 341
#define SUPEQ 342
#define INF 343
#define SUP 344
#define DIFF 345
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    1,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    3,    3,    4,    4,    5,    5,    8,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,   12,   12,   12,
   12,   10,   11,   11,   13,   14,   14,   14,   15,   15,
   17,   16,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,   41,   41,   41,   41,
   41,   41,   41,   41,   41,   41,   41,   41,   41,   41,
   41,   41,   41,   41,   41,   41,   41,   41,   18,   21,
   22,   23,   24,   25,   26,   27,   29,   28,   30,   31,
   32,   33,   34,   35,   36,   19,   37,   20,   38,   39,
   40,   48,   50,   50,    6,   49,   49,   52,   52,   51,
   53,   56,   57,   58,   59,   60,   61,   42,   62,   62,
   62,   62,   62,   62,   62,   62,   62,   62,   62,   62,
   62,   62,   62,   62,   62,   62,   62,   62,   62,   43,
   43,   43,   43,   43,   43,   43,   43,   54,   54,   54,
   54,   54,   54,   54,   44,   44,   44,   44,   44,   45,
   45,   45,   46,   46,   46,   47,   55,   55,   55,   55,
   55,   55,
};
short yylen[] = {                                         2,
    5,    0,    0,    3,    3,    4,    4,    3,    3,    3,
    3,    3,    3,    0,    5,    0,    5,    0,    7,    1,
    0,    3,    4,    4,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    0,    2,    2,
    2,    0,    1,    5,    0,    0,    4,    4,    1,    1,
    1,    3,    0,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    4,    4,    4,    6,    6,    4,    4,    4,    4,
    4,    4,    5,    4,    0,    2,    4,    4,    4,    3,
    3,    7,    0,    3,    0,    3,    1,    3,    1,    9,
    7,    1,    1,    1,    1,    1,    1,    0,    2,    2,
    4,    3,    2,    3,    3,    3,    4,    2,    1,    2,
    3,    1,    2,    2,    2,    2,    2,    2,    2,    0,
    2,    2,    2,    2,    2,    2,    5,    1,    1,    1,
    1,    1,    1,    4,    1,    1,    1,    1,    4,    1,
    1,    4,    1,    1,    4,    1,    1,    1,    1,    1,
    1,    1,
};
short yydefred[] = {                                      2,
    0,    3,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  125,    0,    0,    4,    0,    0,   13,    9,
    8,   10,   11,   12,    0,    5,  125,   18,    7,    6,
   53,    0,    0,    0,   53,    0,  138,   15,  138,  138,
  138,  138,  138,  138,  138,  138,  138,  138,  138,  138,
  138,  138,  138,  115,  138,  138,  138,  138,  138,  138,
    0,   20,    0,   66,    0,   63,    0,   62,    0,   64,
    0,   65,    0,   55,    0,   56,    0,   54,    0,   57,
    0,   58,    0,   67,    0,   68,    0,   69,    0,   59,
    0,   60,    0,   61,    0,   72,   73,    0,   70,    0,
   71,    0,   74,    0,  125,   75,    0,  125,   76,    0,
  125,   17,   21,  132,  135,  136,  137,  138,  138,  178,
  177,  175,  176,  138,  138,  138,  138,  116,  133,  138,
  138,  180,  181,  134,  138,   99,    0,    0,    0,    0,
    0,    0,  100,  101,  138,  138,  138,  138,  138,  138,
  138,    0,  186,  138,  138,  138,  168,  172,  171,  173,
  169,  170,    0,    0,    0,  138,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  163,  165,  164,
  166,  161,  162,    0,    0,    0,    0,    0,    0,    0,
  138,    0,    0,    0,  138,  138,  138,  138,  138,  138,
  138,   53,  138,  138,  138,  138,  138,  138,  138,  138,
  138,  115,  138,  138,  138,  138,  138,  127,    0,  138,
   53,  129,  120,    0,  121,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   38,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  149,    0,
    0,  152,    0,    0,  109,  138,  138,  138,  108,  183,
  184,  107,    0,  118,    0,  110,  111,  112,  102,  103,
  104,  117,    0,  114,    0,  189,  188,  190,  187,  191,
  192,  138,   89,   86,   85,   87,   88,   78,   79,    0,
   77,   80,   81,   90,   91,   92,   82,   83,   84,   95,
   96,   93,   94,   97,   98,  125,  119,    0,    0,  138,
   36,   32,   31,   33,   34,   35,   22,    0,    0,   25,
   26,   27,   28,   29,   30,    0,   45,   43,   19,  139,
  158,  159,  154,  155,  156,  157,    0,    0,    0,  140,
    0,    0,    0,  143,  148,  150,    0,  153,  179,    0,
    0,    0,  182,    0,  113,  174,    0,  126,    0,    0,
  128,    0,   23,   24,   39,   41,   40,    0,  144,  146,
  145,    0,    0,  142,  151,  106,  105,    0,  167,    0,
  124,  138,    0,   46,  141,  147,  185,  122,    0,  131,
    0,    0,   51,   44,   49,   50,    0,    0,  130,    0,
    0,   53,   47,   48,    0,   52,
};
short yydgoto[] = {                                       1,
    2,    3,   15,   28,   33,   25,   34,   63,  168,  242,
  339,  336,  378,  401,  407,  413,  408,   78,   74,   76,
   80,   82,   90,   92,   94,   68,   66,   70,   72,   64,
   84,   86,   88,   99,  101,   96,   97,  103,  106,  109,
  222,   65,  136,  119,  131,  269,  152,  105,  219,  317,
  108,  223,  111,  156,  292,  120,  138,  139,  121,  122,
  123,  264,
};
short yysindex[] = {                                      0,
    0,    0,  262, -256, -252, -249, -208, -207, -200, -199,
 -187, -188,    0, -181, -194,    0, -178, -175,    0,    0,
    0,    0,    0,    0, -215,    0,    0,    0,    0,    0,
    0, -206, -184,  338,    0, -163,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  438,    0, -172,    0, -185,    0, -185,    0, -185,    0,
 -185,    0, -185,    0, -185,    0, -247,    0, -203,    0,
 -185,    0, -185,    0, -185,    0, -185,    0, -185,    0,
 -185,    0, -185,    0, -185,    0,    0, -185,    0, -165,
    0, -185,    0, -197,    0,    0, -165,    0,    0, -197,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -203, -203, -203, -203,
 -203, -203,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -209,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  728, -198,  782,    0,  782,  305,   78, -247,
 -185, -185, -220, -247,   78, -203,   78,    0,    0,    0,
    0,    0,    0, -220, -220, -185, -185, -185, -185, -203,
    0, -185,   78, -297,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -216,    0,
    0,    0,    0, -297,    0, -131, -127, -126, -124, -122,
 -121, -119, -118, -112, -108, -106, -104, -103,  -99,  -97,
    0, -269, -185, -185, -185, -185, -185, -185, -185, -185,
 -185, -185, -185, -220, -220, -185, -220, -220,    0, -185,
 -185,    0, -185, -173,    0,    0,    0,    0,    0,    0,
    0,    0, -170,    0, -147,    0,    0,    0,    0,    0,
    0,    0, -203,    0, -146,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  496,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -197,  554,    0,
    0,    0,    0,    0,    0,    0,    0,  -96,  -84,    0,
    0,    0,    0,    0,    0, -222,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -185, -185, -185,    0,
 -185, -185, -185,    0,    0,    0, -185,    0,    0, -185,
 -185,   78,    0, -203,    0,    0, -197,    0,  782, -174,
    0, -197,    0,    0,    0,    0,    0,  -89,    0,    0,
    0, -185, -185,    0,    0,    0,    0, -135,    0, -116,
    0,    0, -125,    0,    0,    0,    0,    0, -197,    0,
  -41, -123,    0,    0,    0,    0,  -70,  -66,    0,  -59,
  -59,    0,    0,    0,  612,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    6,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  238,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  181,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  181,  181,  181,  181,
  181,  181,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -233,    0,    0,
    0,    0,    0,    0,    0,  181,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  181,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  670,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  181,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  283,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  181,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,    0,    0,    0,    0,  -24,  -35,    0,    0,    0,
    0,    0,    0,    0,    0, -164,    0,   36,   45,   48,
   42,   49,   44,   47,   54,   70,   59,   71,   69,   85,
   75,   76,   74,   72,   73,   77,   80,    0,   79,   67,
  124,  -26,   68,  348, -105,  -34,  189,    0,    0,    0,
    0, -166,    0,  -82,   81,  -27,  -68,  -40,   58,  122,
  133, -140,
};
#define YYTABLESIZE 1119
short yytable[] = {                                      61,
  225,   16,   32,  337,  338,   16,   14,   17,  133,  129,
   18,  114,   67,   69,   71,   73,   75,   77,   79,   81,
   83,   85,   87,   89,   91,   93,   95,  166,   98,  100,
  102,  104,  107,  110,  273,  158,  275,  134,  114,   42,
   42,  158,  286,  287,  288,  289,  290,  291,   19,  132,
   20,  137,  285,  129,  134,  114,  115,   21,   22,  129,
  134,  114,  115,  159,  265,  375,  376,  377,  272,  159,
   23,   24,  153,  114,  115,   26,  157,   27,   31,  153,
  163,   29,  157,  165,   30,   36,  167,   35,  116,  117,
  130,  169,  170,  114,  116,  117,   62,  171,  172,  173,
  174,  133,  113,  175,  176,  133,  116,  117,  177,  137,
  137,  137,  137,  137,  137,  191,  316,  268,  184,  185,
  186,  187,  188,  189,  190,  321,  220,  192,  193,  194,
  322,  323,  271,  324,  135,  325,  140,  327,  326,  224,
  155,  328,  132,  271,  271,  270,  132,  329,  137,  276,
  277,  330,  118,  331,  333,  332,  270,  270,  334,  335,
  392,  160,  137,  373,  283,  359,  300,  160,  363,   67,
   69,   71,   73,   75,   77,  374,   79,   81,   83,   85,
   87,   89,   91,   93,   95,  319,   98,  100,  102,  107,
  110,  364,  366,  318,  140,  140,  140,  140,  140,  140,
  141,  394,  391,  397,  178,  179,  180,  181,  182,  183,
  400,  142,  409,  271,  271,  398,  271,  271,  403,  351,
  352,  388,  354,  355,  410,  161,  270,  270,  411,  270,
  270,  161,  404,  140,  412,  370,  162,    1,  301,  360,
  361,  362,  162,  274,  298,  302,  414,  140,  299,  158,
  405,  406,  307,  303,  294,  137,  308,  282,  141,  141,
  141,  141,  141,  141,  309,  367,  295,  297,  296,  142,
  142,  142,  142,  142,  142,   16,   14,  159,   14,  293,
  304,  306,  305,  315,  390,  312,  218,  313,  310,  393,
  157,  369,  311,  372,  314,  164,    0,  141,  158,    0,
    0,    0,    0,  158,  320,    0,    0,    0,  142,    0,
    0,  141,    0,    0,    0,    0,  402,    0,    0,    0,
    0,    0,  142,    0,    0,    0,  159,    0,    0,    0,
  158,  159,    0,    0,    0,    0,  137,    0,    0,  157,
  140,    0,    0,    0,  157,    0,    0,    0,    0,    0,
  365,    0,    0,    0,    0,    0,    0,    0,  159,    0,
    0,    0,    0,    0,    0,  399,    0,    0,    0,    0,
    0,  157,    0,    0,    0,  160,  415,    0,    0,  243,
  244,  245,  246,  247,  248,  249,    0,    0,    0,  250,
  251,  252,  253,  254,  255,  256,  257,    0,  258,  259,
    0,  260,    0,    0,  141,  261,  262,  263,    0,    0,
    0,    0,    0,    0,  124,  142,  125,    0,  126,    0,
  127,  140,  128,    0,  160,    0,    0,    0,  143,  160,
  144,  389,  145,    0,  146,    0,  147,    0,  148,  161,
  149,    0,  150,    0,  160,  151,    0,    0,    0,  154,
  162,    0,    0,    0,  160,    0,  160,  160,  160,    0,
    0,    0,  160,    0,  160,    0,  160,  160,    0,    0,
    0,    0,    0,    0,    0,    0,  160,  160,  160,  160,
  160,  160,    0,    0,    0,  141,    0,    0,  161,  160,
  160,  160,    0,  161,    0,    0,  142,    0,    0,  162,
  160,    0,    0,  160,  162,    0,  160,  160,    0,    0,
    0,  160,    0,  160,  160,    0,    0,  160,  266,  267,
  161,    0,    4,    5,    6,    7,    8,    9,   10,   11,
   12,  162,   13,  278,  279,  280,  281,    0,    0,  284,
    0,    0,    0,    0,    0,   14,   37,   37,   37,   37,
   37,   37,    0,    0,    0,   37,   37,    0,   37,   37,
   37,   37,   37,   37,   37,   37,   37,   37,  226,  227,
  228,  229,  230,  231,    0,    0,    0,    0,    0,    0,
  232,  233,  234,  235,  236,  237,  238,  239,  240,  241,
  340,  341,  342,  343,  344,  345,  346,  347,  348,  349,
  350,   37,    0,  353,    0,    0,    0,  356,  357,    0,
  358,   38,    0,    0,   39,   40,    0,    0,    0,   41,
    0,   42,    0,   43,   44,    0,    0,    0,    0,    0,
    0,    0,    0,   45,   46,   47,   48,   49,   50,    0,
    0,    0,    0,    0,    0,    0,   51,   52,   53,    0,
    0,    0,    0,    0,    0,    0,    0,   54,    0,    0,
   55,    0,    0,   56,   57,    0,    0,    0,   58,    0,
    0,   59,    0,    0,   60,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  379,  380,  381,    0,  382,  383,
  384,   37,    0,    0,  385,    0,    0,  386,  387,    0,
    0,  112,    0,    0,   39,   40,    0,    0,    0,   41,
    0,   42,    0,   43,   44,    0,    0,    0,    0,  395,
  396,    0,    0,   45,   46,   47,   48,   49,   50,    0,
    0,    0,    0,    0,    0,    0,   51,   52,   53,    0,
    0,    0,    0,    0,    0,    0,    0,   54,    0,   37,
   55,    0,    0,   56,   57,    0,    0,    0,   58,  368,
    0,   59,   39,   40,   60,    0,    0,   41,    0,   42,
    0,   43,   44,    0,    0,    0,    0,    0,    0,    0,
    0,   45,   46,   47,   48,   49,   50,    0,    0,    0,
    0,    0,    0,    0,   51,   52,   53,    0,    0,    0,
    0,    0,    0,    0,    0,   54,    0,   37,   55,    0,
    0,   56,   57,    0,    0,    0,   58,  371,    0,   59,
   39,   40,   60,    0,    0,   41,    0,   42,    0,   43,
   44,    0,    0,    0,    0,    0,    0,    0,    0,   45,
   46,   47,   48,   49,   50,    0,    0,    0,    0,    0,
    0,    0,   51,   52,   53,    0,    0,    0,    0,    0,
    0,    0,    0,   54,    0,   37,   55,    0,    0,   56,
   57,    0,    0,    0,   58,  416,    0,   59,   39,   40,
   60,    0,    0,   41,    0,   42,    0,   43,   44,    0,
    0,    0,    0,    0,    0,    0,    0,   45,   46,   47,
   48,   49,   50,    0,    0,    0,    0,    0,    0,    0,
   51,   52,   53,    0,    0,    0,    0,    0,    0,    0,
    0,   54,    0,  123,   55,    0,    0,   56,   57,    0,
    0,    0,   58,  123,    0,   59,  123,  123,   60,    0,
    0,  123,    0,  123,    0,  123,  123,    0,    0,    0,
    0,    0,    0,    0,    0,  123,  123,  123,  123,  123,
  123,    0,    0,    0,    0,    0,    0,    0,  123,  123,
  123,    0,    0,    0,    0,    0,    0,    0,    0,  123,
    0,  195,  123,    0,    0,  123,  123,    0,    0,    0,
  123,    0,    0,  123,  196,  197,  123,    0,    0,  198,
    0,  199,    0,  200,  201,    0,    0,    0,    0,    0,
    0,  202,    0,  203,  204,  205,  206,  207,  208,    0,
    0,    0,    0,    0,    0,    0,  209,  210,  211,    0,
    0,    0,    0,    0,    0,  195,    0,  212,    0,    0,
  213,    0,    0,  214,  215,    0,    0,    0,  196,  197,
    0,  216,    0,  198,  217,  199,    0,  200,  201,    0,
    0,    0,    0,    0,    0,  221,    0,  203,  204,  205,
  206,  207,  208,    0,    0,    0,    0,    0,    0,    0,
  209,  210,  211,    0,    0,    0,    0,    0,    0,    0,
    0,  212,    0,    0,  213,    0,    0,  214,  215,    0,
    0,    0,    0,    0,    0,  216,    0,    0,  217,
};
short yycheck[] = {                                      35,
  167,  258,   27,  273,  274,    0,    0,  260,   77,  257,
  260,  259,   39,   40,   41,   42,   43,   44,   45,   46,
   47,   48,   49,   50,   51,   52,   53,  110,   55,   56,
   57,   58,   59,   60,  175,  104,  177,  258,  259,  273,
  274,  110,  340,  341,  342,  343,  344,  345,  257,   77,
  258,   79,  193,  257,  258,  259,  260,  258,  258,  257,
  258,  259,  260,  104,  170,  288,  289,  290,  174,  110,
  258,  260,  100,  259,  260,  257,  104,  272,  294,  107,
  105,  260,  110,  108,  260,  270,  111,  294,  292,  293,
  338,  118,  119,  259,  292,  293,  260,  124,  125,  126,
  127,  170,  275,  130,  131,  174,  292,  293,  135,  137,
  138,  139,  140,  141,  142,  325,  333,  338,  145,  146,
  147,  148,  149,  150,  151,  257,  325,  154,  155,  156,
  258,  258,  173,  258,  338,  258,   79,  257,  260,  166,
  338,  260,  170,  184,  185,  173,  174,  260,  176,  184,
  185,  260,  338,  260,  258,  260,  184,  185,  258,  257,
  335,  104,  190,  260,  191,  339,  202,  110,  339,  196,
  197,  198,  199,  200,  201,  260,  203,  204,  205,  206,
  207,  208,  209,  210,  211,  221,  213,  214,  215,  216,
  217,  339,  339,  220,  137,  138,  139,  140,  141,  142,
   79,  291,  369,  339,  137,  138,  139,  140,  141,  142,
  336,   79,  336,  254,  255,  332,  257,  258,  260,  254,
  255,  362,  257,  258,  295,  104,  254,  255,  295,  257,
  258,  110,  274,  176,  294,  318,  104,    0,  203,  266,
  267,  268,  110,  176,  200,  204,  411,  190,  201,  318,
  292,  293,  209,  205,  196,  283,  210,  190,  137,  138,
  139,  140,  141,  142,  211,  292,  197,  199,  198,  137,
  138,  139,  140,  141,  142,  270,  270,  318,  272,  195,
  206,  208,  207,  217,  367,  214,  163,  215,  212,  372,
  318,  316,  213,  320,  216,  107,   -1,  176,  367,   -1,
   -1,   -1,   -1,  372,  224,   -1,   -1,   -1,  176,   -1,
   -1,  190,   -1,   -1,   -1,   -1,  399,   -1,   -1,   -1,
   -1,   -1,  190,   -1,   -1,   -1,  367,   -1,   -1,   -1,
  399,  372,   -1,   -1,   -1,   -1,  364,   -1,   -1,  367,
  283,   -1,   -1,   -1,  372,   -1,   -1,   -1,   -1,   -1,
  283,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  399,   -1,
   -1,   -1,   -1,   -1,   -1,  392,   -1,   -1,   -1,   -1,
   -1,  399,   -1,   -1,   -1,  318,  412,   -1,   -1,  302,
  303,  304,  305,  306,  307,  308,   -1,   -1,   -1,  312,
  313,  314,  315,  316,  317,  318,  319,   -1,  321,  322,
   -1,  324,   -1,   -1,  283,  328,  329,  330,   -1,   -1,
   -1,   -1,   -1,   -1,   67,  283,   69,   -1,   71,   -1,
   73,  364,   75,   -1,  367,   -1,   -1,   -1,   81,  372,
   83,  364,   85,   -1,   87,   -1,   89,   -1,   91,  318,
   93,   -1,   95,   -1,  264,   98,   -1,   -1,   -1,  102,
  318,   -1,   -1,   -1,  274,   -1,  399,  277,  278,   -1,
   -1,   -1,  282,   -1,  284,   -1,  286,  287,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  296,  297,  298,  299,
  300,  301,   -1,   -1,   -1,  364,   -1,   -1,  367,  309,
  310,  311,   -1,  372,   -1,   -1,  364,   -1,   -1,  367,
  320,   -1,   -1,  323,  372,   -1,  326,  327,   -1,   -1,
   -1,  331,   -1,  333,  334,   -1,   -1,  337,  171,  172,
  399,   -1,  261,  262,  263,  264,  265,  266,  267,  268,
  269,  399,  271,  186,  187,  188,  189,   -1,   -1,  192,
   -1,   -1,   -1,   -1,   -1,  284,  264,  265,  266,  267,
  268,  269,   -1,   -1,   -1,  273,  274,   -1,  276,  277,
  278,  279,  280,  281,  282,  283,  284,  285,  264,  265,
  266,  267,  268,  269,   -1,   -1,   -1,   -1,   -1,   -1,
  276,  277,  278,  279,  280,  281,  282,  283,  284,  285,
  243,  244,  245,  246,  247,  248,  249,  250,  251,  252,
  253,  264,   -1,  256,   -1,   -1,   -1,  260,  261,   -1,
  263,  274,   -1,   -1,  277,  278,   -1,   -1,   -1,  282,
   -1,  284,   -1,  286,  287,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  296,  297,  298,  299,  300,  301,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  309,  310,  311,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  320,   -1,   -1,
  323,   -1,   -1,  326,  327,   -1,   -1,   -1,  331,   -1,
   -1,  334,   -1,   -1,  337,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  347,  348,  349,   -1,  351,  352,
  353,  264,   -1,   -1,  357,   -1,   -1,  360,  361,   -1,
   -1,  274,   -1,   -1,  277,  278,   -1,   -1,   -1,  282,
   -1,  284,   -1,  286,  287,   -1,   -1,   -1,   -1,  382,
  383,   -1,   -1,  296,  297,  298,  299,  300,  301,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  309,  310,  311,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  320,   -1,  264,
  323,   -1,   -1,  326,  327,   -1,   -1,   -1,  331,  274,
   -1,  334,  277,  278,  337,   -1,   -1,  282,   -1,  284,
   -1,  286,  287,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  296,  297,  298,  299,  300,  301,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  309,  310,  311,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  320,   -1,  264,  323,   -1,
   -1,  326,  327,   -1,   -1,   -1,  331,  274,   -1,  334,
  277,  278,  337,   -1,   -1,  282,   -1,  284,   -1,  286,
  287,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  296,
  297,  298,  299,  300,  301,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  309,  310,  311,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  320,   -1,  264,  323,   -1,   -1,  326,
  327,   -1,   -1,   -1,  331,  274,   -1,  334,  277,  278,
  337,   -1,   -1,  282,   -1,  284,   -1,  286,  287,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  296,  297,  298,
  299,  300,  301,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  309,  310,  311,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  320,   -1,  264,  323,   -1,   -1,  326,  327,   -1,
   -1,   -1,  331,  274,   -1,  334,  277,  278,  337,   -1,
   -1,  282,   -1,  284,   -1,  286,  287,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  296,  297,  298,  299,  300,
  301,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  309,  310,
  311,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  320,
   -1,  264,  323,   -1,   -1,  326,  327,   -1,   -1,   -1,
  331,   -1,   -1,  334,  277,  278,  337,   -1,   -1,  282,
   -1,  284,   -1,  286,  287,   -1,   -1,   -1,   -1,   -1,
   -1,  294,   -1,  296,  297,  298,  299,  300,  301,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  309,  310,  311,   -1,
   -1,   -1,   -1,   -1,   -1,  264,   -1,  320,   -1,   -1,
  323,   -1,   -1,  326,  327,   -1,   -1,   -1,  277,  278,
   -1,  334,   -1,  282,  337,  284,   -1,  286,  287,   -1,
   -1,   -1,   -1,   -1,   -1,  294,   -1,  296,  297,  298,
  299,  300,  301,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  309,  310,  311,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  320,   -1,   -1,  323,   -1,   -1,  326,  327,   -1,
   -1,   -1,   -1,   -1,   -1,  334,   -1,   -1,  337,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 345
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"STR","GSTR","VAR","NUMBER",
"WINDOWTITLE","WINDOWSIZE","WINDOWPOSITION","FONT","FORECOLOR","BACKCOLOR",
"SHADCOLOR","LICOLOR","COLORSET","OBJECT","INIT","PERIODICTASK","MAIN","END",
"PROP","TYPE","SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX","TITLE",
"SWALLOWEXEC","ICON","FLAGS","WARP","WRITETOFILE","HIDDEN","CANBESELECTED",
"NORELIEFSTRING","CASE","SINGLECLIC","DOUBLECLIC","BEG","POINT","EXEC","HIDE",
"SHOW","CHFORECOLOR","CHBACKCOLOR","CHCOLORSET","GETVALUE","GETMINVALUE",
"GETMAXVALUE","GETFORE","GETBACK","GETHILIGHT","GETSHADOW","CHVALUE",
"CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT","GETTITLE","GETOUTPUT","STRCOPY",
"NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT","GETSCRIPTFATHER","SENDTOSCRIPT",
"RECEIVFROMSCRIPT","GET","SET","SENDSIGN","REMAINDEROFDIV","GETTIME",
"GETSCRIPTARG","IF","THEN","ELSE","FOR","TO","DO","WHILE","BEGF","ENDF","EQUAL",
"INFEQ","SUPEQ","INF","SUP","DIFF",
};
char *yyrule[] = {
"$accept : script",
"script : initvar head initbloc periodictask object",
"initvar :",
"head :",
"head : head WINDOWTITLE GSTR",
"head : head ICON STR",
"head : head WINDOWPOSITION NUMBER NUMBER",
"head : head WINDOWSIZE NUMBER NUMBER",
"head : head BACKCOLOR GSTR",
"head : head FORECOLOR GSTR",
"head : head SHADCOLOR GSTR",
"head : head LICOLOR GSTR",
"head : head COLORSET NUMBER",
"head : head FONT STR",
"initbloc :",
"initbloc : INIT creerbloc BEG instr END",
"periodictask :",
"periodictask : PERIODICTASK creerbloc BEG instr END",
"object :",
"object : object OBJECT id PROP init verify mainloop",
"id : NUMBER",
"init :",
"init : init TYPE STR",
"init : init SIZE NUMBER NUMBER",
"init : init POSITION NUMBER NUMBER",
"init : init VALUE NUMBER",
"init : init VALUEMIN NUMBER",
"init : init VALUEMAX NUMBER",
"init : init TITLE GSTR",
"init : init SWALLOWEXEC GSTR",
"init : init ICON STR",
"init : init BACKCOLOR GSTR",
"init : init FORECOLOR GSTR",
"init : init SHADCOLOR GSTR",
"init : init LICOLOR GSTR",
"init : init COLORSET NUMBER",
"init : init FONT STR",
"init : init FLAGS flags",
"flags :",
"flags : flags HIDDEN",
"flags : flags NORELIEFSTRING",
"flags : flags CANBESELECTED",
"verify :",
"mainloop : END",
"mainloop : MAIN addtabcase CASE case END",
"addtabcase :",
"case :",
"case : case clic POINT bloc",
"case : case number POINT bloc",
"clic : SINGLECLIC",
"clic : DOUBLECLIC",
"number : NUMBER",
"bloc : BEG instr END",
"instr :",
"instr : instr EXEC exec",
"instr : instr WARP warp",
"instr : instr WRITETOFILE writetofile",
"instr : instr HIDE hide",
"instr : instr SHOW show",
"instr : instr CHVALUE chvalue",
"instr : instr CHVALUEMAX chvaluemax",
"instr : instr CHVALUEMIN chvaluemin",
"instr : instr POSITION position",
"instr : instr SIZE size",
"instr : instr TITLE title",
"instr : instr ICON icon",
"instr : instr FONT font",
"instr : instr CHFORECOLOR chforecolor",
"instr : instr CHBACKCOLOR chbackcolor",
"instr : instr CHCOLORSET chcolorset",
"instr : instr SET set",
"instr : instr SENDSIGN sendsign",
"instr : instr QUIT quit",
"instr : instr SENDTOSCRIPT sendtoscript",
"instr : instr IF ifthenelse",
"instr : instr FOR loop",
"instr : instr WHILE while",
"oneinstr : EXEC exec",
"oneinstr : WARP warp",
"oneinstr : WRITETOFILE writetofile",
"oneinstr : HIDE hide",
"oneinstr : SHOW show",
"oneinstr : CHVALUE chvalue",
"oneinstr : CHVALUEMAX chvaluemax",
"oneinstr : CHVALUEMIN chvaluemin",
"oneinstr : POSITION position",
"oneinstr : SIZE size",
"oneinstr : TITLE title",
"oneinstr : ICON icon",
"oneinstr : FONT font",
"oneinstr : CHFORECOLOR chforecolor",
"oneinstr : CHBACKCOLOR chbackcolor",
"oneinstr : CHCOLORSET chcolorset",
"oneinstr : SET set",
"oneinstr : SENDSIGN sendsign",
"oneinstr : QUIT quit",
"oneinstr : SENDTOSCRIPT sendtoscript",
"oneinstr : FOR loop",
"oneinstr : WHILE while",
"exec : addlbuff args",
"hide : addlbuff numarg",
"show : addlbuff numarg",
"chvalue : addlbuff numarg addlbuff numarg",
"chvaluemax : addlbuff numarg addlbuff numarg",
"chvaluemin : addlbuff numarg addlbuff numarg",
"position : addlbuff numarg addlbuff numarg addlbuff numarg",
"size : addlbuff numarg addlbuff numarg addlbuff numarg",
"icon : addlbuff numarg addlbuff strarg",
"title : addlbuff numarg addlbuff gstrarg",
"font : addlbuff numarg addlbuff strarg",
"chforecolor : addlbuff numarg addlbuff gstrarg",
"chbackcolor : addlbuff numarg addlbuff gstrarg",
"chcolorset : addlbuff numarg addlbuff numarg",
"set : addlbuff vararg GET addlbuff args",
"sendsign : addlbuff numarg addlbuff numarg",
"quit :",
"warp : addlbuff numarg",
"sendtoscript : addlbuff numarg addlbuff args",
"writetofile : addlbuff strarg addlbuff args",
"ifthenelse : headif creerbloc bloc1 else",
"loop : headloop creerbloc bloc2",
"while : headwhile creerbloc bloc2",
"headif : addlbuff arg addlbuff compare addlbuff arg THEN",
"else :",
"else : ELSE creerbloc bloc2",
"creerbloc :",
"bloc1 : BEG instr END",
"bloc1 : oneinstr",
"bloc2 : BEG instr END",
"bloc2 : oneinstr",
"headloop : addlbuff vararg GET addlbuff arg TO addlbuff arg DO",
"headwhile : addlbuff arg addlbuff compare addlbuff arg DO",
"var : VAR",
"str : STR",
"gstr : GSTR",
"num : NUMBER",
"singleclic2 : SINGLECLIC",
"doubleclic2 : DOUBLECLIC",
"addlbuff :",
"function : GETVALUE numarg",
"function : GETTITLE numarg",
"function : GETOUTPUT gstrarg numarg numarg",
"function : NUMTOHEX numarg numarg",
"function : HEXTONUM gstrarg",
"function : ADD numarg numarg",
"function : MULT numarg numarg",
"function : DIV numarg numarg",
"function : STRCOPY gstrarg numarg numarg",
"function : LAUNCHSCRIPT gstrarg",
"function : GETSCRIPTFATHER",
"function : RECEIVFROMSCRIPT numarg",
"function : REMAINDEROFDIV numarg numarg",
"function : GETTIME",
"function : GETSCRIPTARG numarg",
"function : GETFORE numarg",
"function : GETBACK numarg",
"function : GETHILIGHT numarg",
"function : GETSHADOW numarg",
"function : GETMINVALUE numarg",
"function : GETMAXVALUE numarg",
"args :",
"args : singleclic2 args",
"args : doubleclic2 args",
"args : var args",
"args : gstr args",
"args : str args",
"args : num args",
"args : BEGF addlbuff function ENDF args",
"arg : var",
"arg : singleclic2",
"arg : doubleclic2",
"arg : gstr",
"arg : str",
"arg : num",
"arg : BEGF addlbuff function ENDF",
"numarg : singleclic2",
"numarg : doubleclic2",
"numarg : num",
"numarg : var",
"numarg : BEGF addlbuff function ENDF",
"strarg : var",
"strarg : str",
"strarg : BEGF addlbuff function ENDF",
"gstrarg : var",
"gstrarg : gstr",
"gstrarg : BEGF addlbuff function ENDF",
"vararg : var",
"compare : INF",
"compare : INFEQ",
"compare : EQUAL",
"compare : SUPEQ",
"compare : SUP",
"compare : DIFF",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 375 "script.y"
{ InitVarGlob(); }
break;
case 4:
#line 380 "script.y"
{		/* Titre de la fenetre */
				 scriptprop->titlewin=yyvsp[0].str;
				}
break;
case 5:
#line 383 "script.y"
{
				 scriptprop->icon=yyvsp[0].str;
				}
break;
case 6:
#line 387 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->x=yyvsp[-1].number;
				 scriptprop->y=yyvsp[0].number;
				}
break;
case 7:
#line 392 "script.y"
{		/* Position et taille de la fenetre */
				 scriptprop->width=yyvsp[-1].number;
				 scriptprop->height=yyvsp[0].number;
				}
break;
case 8:
#line 396 "script.y"
{ 		/* Couleur de fond */
				 scriptprop->backcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				}
break;
case 9:
#line 400 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->forecolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				}
break;
case 10:
#line 404 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				}
break;
case 11:
#line 408 "script.y"
{ 		/* Couleur des lignes */
				 scriptprop->hilicolor=yyvsp[0].str;
				 scriptprop->colorset = -1;
				}
break;
case 12:
#line 412 "script.y"
{
				 scriptprop->colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				}
break;
case 13:
#line 416 "script.y"
{
				 scriptprop->font=yyvsp[0].str;
				}
break;
case 15:
#line 423 "script.y"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				}
break;
case 17:
#line 429 "script.y"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				}
break;
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
				}
break;
case 22:
#line 459 "script.y"
{
				 (*tabobj)[nbobj].type=yyvsp[0].str;
				 HasType=1;
				}
break;
case 23:
#line 463 "script.y"
{
				 (*tabobj)[nbobj].width=yyvsp[-1].number;
				 (*tabobj)[nbobj].height=yyvsp[0].number;
				}
break;
case 24:
#line 467 "script.y"
{
				 (*tabobj)[nbobj].x=yyvsp[-1].number;
				 (*tabobj)[nbobj].y=yyvsp[0].number;
				 HasPosition=1;
				}
break;
case 25:
#line 472 "script.y"
{
				 (*tabobj)[nbobj].value=yyvsp[0].number;
				}
break;
case 26:
#line 475 "script.y"
{
				 (*tabobj)[nbobj].value2=yyvsp[0].number;
				}
break;
case 27:
#line 478 "script.y"
{
				 (*tabobj)[nbobj].value3=yyvsp[0].number;
				}
break;
case 28:
#line 481 "script.y"
{
				 (*tabobj)[nbobj].title=yyvsp[0].str;
				}
break;
case 29:
#line 484 "script.y"
{
				 (*tabobj)[nbobj].swallow=yyvsp[0].str;
				}
break;
case 30:
#line 487 "script.y"
{
				 (*tabobj)[nbobj].icon=yyvsp[0].str;
				}
break;
case 31:
#line 490 "script.y"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
break;
case 32:
#line 494 "script.y"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
break;
case 33:
#line 498 "script.y"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
break;
case 34:
#line 502 "script.y"
{
				 (*tabobj)[nbobj].hilicolor=yyvsp[0].str;
				 (*tabobj)[nbobj].colorset = -1;
				}
break;
case 35:
#line 506 "script.y"
{
				 (*tabobj)[nbobj].colorset = yyvsp[0].number;
				 AllocColorset(yyvsp[0].number);
				}
break;
case 36:
#line 510 "script.y"
{
				 (*tabobj)[nbobj].font=yyvsp[0].str;
				}
break;
case 39:
#line 516 "script.y"
{
				 (*tabobj)[nbobj].flags[0]=True;
				}
break;
case 40:
#line 519 "script.y"
{
				 (*tabobj)[nbobj].flags[1]=True;
				}
break;
case 41:
#line 522 "script.y"
{
				 (*tabobj)[nbobj].flags[2]=True;
				}
break;
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
				 }
break;
case 43:
#line 539 "script.y"
{ InitObjTabCase(0); }
break;
case 45:
#line 543 "script.y"
{ InitObjTabCase(1); }
break;
case 49:
#line 550 "script.y"
{ InitCase(-1); }
break;
case 50:
#line 551 "script.y"
{ InitCase(-2); }
break;
case 51:
#line 554 "script.y"
{ InitCase(yyvsp[0].number); }
break;
case 99:
#line 613 "script.y"
{ AddCom(1,1); }
break;
case 100:
#line 615 "script.y"
{ AddCom(2,1);}
break;
case 101:
#line 617 "script.y"
{ AddCom(3,1);}
break;
case 102:
#line 619 "script.y"
{ AddCom(4,2);}
break;
case 103:
#line 621 "script.y"
{ AddCom(21,2);}
break;
case 104:
#line 623 "script.y"
{ AddCom(22,2);}
break;
case 105:
#line 625 "script.y"
{ AddCom(5,3);}
break;
case 106:
#line 627 "script.y"
{ AddCom(6,3);}
break;
case 107:
#line 629 "script.y"
{ AddCom(7,2);}
break;
case 108:
#line 631 "script.y"
{ AddCom(8,2);}
break;
case 109:
#line 633 "script.y"
{ AddCom(9,2);}
break;
case 110:
#line 635 "script.y"
{ AddCom(10,2);}
break;
case 111:
#line 637 "script.y"
{ AddCom(19,2);}
break;
case 112:
#line 639 "script.y"
{ AddCom(24,2);}
break;
case 113:
#line 641 "script.y"
{ AddCom(11,2);}
break;
case 114:
#line 643 "script.y"
{ AddCom(12,2);}
break;
case 115:
#line 645 "script.y"
{ AddCom(13,0);}
break;
case 116:
#line 647 "script.y"
{ AddCom(17,1);}
break;
case 117:
#line 649 "script.y"
{ AddCom(23,2);}
break;
case 118:
#line 651 "script.y"
{ AddCom(18,2);}
break;
case 122:
#line 661 "script.y"
{ AddComBloc(14,3,2); }
break;
case 125:
#line 666 "script.y"
{ EmpilerBloc(); }
break;
case 126:
#line 668 "script.y"
{ DepilerBloc(2); }
break;
case 127:
#line 669 "script.y"
{ DepilerBloc(2); }
break;
case 128:
#line 672 "script.y"
{ DepilerBloc(1); }
break;
case 129:
#line 673 "script.y"
{ DepilerBloc(1); }
break;
case 130:
#line 677 "script.y"
{ AddComBloc(15,3,1); }
break;
case 131:
#line 681 "script.y"
{ AddComBloc(16,3,1); }
break;
case 132:
#line 686 "script.y"
{ AddVar(yyvsp[0].str); }
break;
case 133:
#line 688 "script.y"
{ AddConstStr(yyvsp[0].str); }
break;
case 134:
#line 690 "script.y"
{ AddConstStr(yyvsp[0].str); }
break;
case 135:
#line 692 "script.y"
{ AddConstNum(yyvsp[0].number); }
break;
case 136:
#line 694 "script.y"
{ AddConstNum(-1); }
break;
case 137:
#line 696 "script.y"
{ AddConstNum(-2); }
break;
case 138:
#line 698 "script.y"
{ AddLevelBufArg(); }
break;
case 139:
#line 700 "script.y"
{ AddFunct(1,1); }
break;
case 140:
#line 701 "script.y"
{ AddFunct(2,1); }
break;
case 141:
#line 702 "script.y"
{ AddFunct(3,1); }
break;
case 142:
#line 703 "script.y"
{ AddFunct(4,1); }
break;
case 143:
#line 704 "script.y"
{ AddFunct(5,1); }
break;
case 144:
#line 705 "script.y"
{ AddFunct(6,1); }
break;
case 145:
#line 706 "script.y"
{ AddFunct(7,1); }
break;
case 146:
#line 707 "script.y"
{ AddFunct(8,1); }
break;
case 147:
#line 708 "script.y"
{ AddFunct(9,1); }
break;
case 148:
#line 709 "script.y"
{ AddFunct(10,1); }
break;
case 149:
#line 710 "script.y"
{ AddFunct(11,1); }
break;
case 150:
#line 711 "script.y"
{ AddFunct(12,1); }
break;
case 151:
#line 712 "script.y"
{ AddFunct(13,1); }
break;
case 152:
#line 713 "script.y"
{ AddFunct(14,1); }
break;
case 153:
#line 714 "script.y"
{ AddFunct(15,1); }
break;
case 154:
#line 715 "script.y"
{ AddFunct(16,1); }
break;
case 155:
#line 716 "script.y"
{ AddFunct(17,1); }
break;
case 156:
#line 717 "script.y"
{ AddFunct(18,1); }
break;
case 157:
#line 718 "script.y"
{ AddFunct(19,1); }
break;
case 158:
#line 719 "script.y"
{ AddFunct(20,1); }
break;
case 159:
#line 720 "script.y"
{ AddFunct(21,1); }
break;
case 160:
#line 725 "script.y"
{ }
break;
case 187:
#line 771 "script.y"
{ l=1-250000; AddBufArg(&l,1); }
break;
case 188:
#line 772 "script.y"
{ l=2-250000; AddBufArg(&l,1); }
break;
case 189:
#line 773 "script.y"
{ l=3-250000; AddBufArg(&l,1); }
break;
case 190:
#line 774 "script.y"
{ l=4-250000; AddBufArg(&l,1); }
break;
case 191:
#line 775 "script.y"
{ l=5-250000; AddBufArg(&l,1); }
break;
case 192:
#line 776 "script.y"
{ l=6-250000; AddBufArg(&l,1); }
break;
#line 1761 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
