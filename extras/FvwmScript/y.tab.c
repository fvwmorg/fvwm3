#ifndef lint
/*static char yysccsid[] = "from: @(#)yaccpar	1.9 (Berkeley) 02/21/93";*/
static char yyrcsid[] = "$Id$";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "Compiler/bisonin"
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


#line 346 "Compiler/bisonin"
typedef union {  char *str;
          int number;
       } YYSTYPE;
#line 357 "y.tab.c"
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
#define OBJECT 269
#define INIT 270
#define PERIODICTASK 271
#define MAIN 272
#define END 273
#define PROP 274
#define TYPE 275
#define SIZE 276
#define POSITION 277
#define VALUE 278
#define VALUEMIN 279
#define VALUEMAX 280
#define TITLE 281
#define SWALLOWEXEC 282
#define ICON 283
#define FLAGS 284
#define WARP 285
#define WRITETOFILE 286
#define HIDDEN 287
#define CANBESELECTED 288
#define NORELIEFSTRING 289
#define CASE 290
#define SINGLECLIC 291
#define DOUBLECLIC 292
#define BEG 293
#define POINT 294
#define EXEC 295
#define HIDE 296
#define SHOW 297
#define CHFORECOLOR 298
#define CHBACKCOLOR 299
#define GETVALUE 300
#define CHVALUE 301
#define CHVALUEMAX 302
#define CHVALUEMIN 303
#define ADD 304
#define DIV 305
#define MULT 306
#define GETTITLE 307
#define GETOUTPUT 308
#define STRCOPY 309
#define NUMTOHEX 310
#define HEXTONUM 311
#define QUIT 312
#define LAUNCHSCRIPT 313
#define GETSCRIPTFATHER 314
#define SENDTOSCRIPT 315
#define RECEIVFROMSCRIPT 316
#define GET 317
#define SET 318
#define SENDSIGN 319
#define REMAINDEROFDIV 320
#define GETTIME 321
#define GETSCRIPTARG 322
#define IF 323
#define THEN 324
#define ELSE 325
#define FOR 326
#define TO 327
#define DO 328
#define WHILE 329
#define BEGF 330
#define ENDF 331
#define EQUAL 332
#define INFEQ 333
#define SUPEQ 334
#define INF 335
#define SUP 336
#define DIFF 337
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    1,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    3,    3,    4,    4,    5,    5,    8,    9,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,   12,   12,   12,   12,   10,
   11,   11,   13,   14,   14,   14,   15,   15,   17,   16,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,   40,   40,   40,   40,   40,   40,   40,
   40,   40,   40,   40,   40,   40,   40,   40,   40,   40,
   40,   40,   40,   40,   18,   21,   22,   23,   24,   25,
   26,   27,   29,   28,   30,   31,   32,   33,   34,   35,
   19,   36,   20,   37,   38,   39,   47,   49,   49,    6,
   48,   48,   51,   51,   50,   52,   55,   56,   57,   58,
   59,   60,   41,   61,   61,   61,   61,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   61,   61,   42,   42,
   42,   42,   42,   42,   42,   42,   53,   53,   53,   53,
   53,   53,   53,   43,   43,   43,   43,   43,   44,   44,
   44,   45,   45,   45,   46,   54,   54,   54,   54,   54,
   54,
};
short yylen[] = {                                         2,
    5,    0,    0,    3,    3,    4,    4,    3,    3,    3,
    3,    3,    0,    5,    0,    5,    0,    7,    1,    0,
    3,    4,    4,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    0,    2,    2,    2,    0,
    1,    5,    0,    0,    4,    4,    1,    1,    1,    3,
    0,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    4,    4,    4,
    6,    6,    4,    4,    4,    4,    4,    5,    4,    0,
    2,    4,    4,    4,    3,    3,    7,    0,    3,    0,
    3,    1,    3,    1,    9,    7,    1,    1,    1,    1,
    1,    1,    0,    2,    2,    4,    3,    2,    3,    3,
    3,    4,    2,    1,    2,    3,    1,    2,    0,    2,
    2,    2,    2,    2,    2,    5,    1,    1,    1,    1,
    1,    1,    4,    1,    1,    1,    1,    4,    1,    1,
    4,    1,    1,    4,    1,    1,    1,    1,    1,    1,
    1,
};
short yydefred[] = {                                      2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  120,    0,    4,    0,    0,   12,    9,    8,   10,
   11,    5,    0,  120,    0,    7,    6,    0,    0,    0,
    1,  133,  133,  133,  133,  133,  133,  133,  133,  133,
  133,  133,  133,  133,  133,  133,  110,  133,  133,  133,
  133,  133,  133,    0,    0,   19,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  120,    0,    0,  120,
    0,    0,  120,   14,    0,    0,   64,  127,  130,  131,
  132,  133,  133,  167,  166,  164,  165,   61,  133,   60,
  133,   62,  133,   63,  133,   53,  111,   54,  128,  133,
  133,  169,  170,   52,  129,  133,   95,    0,    0,    0,
    0,    0,    0,   55,   96,   56,   97,   65,  133,   66,
  133,   57,  133,   58,  133,   59,  133,   69,   70,  133,
   67,    0,  175,   68,  133,   71,  133,  133,  157,  161,
  160,  162,  158,  159,    0,   72,    0,    0,   73,  133,
    0,   16,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   40,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  152,  154,  153,
  155,  150,  151,    0,    0,    0,    0,    0,    0,  133,
    0,    0,    0,  133,  133,  133,  133,  133,  133,  133,
    0,  133,  133,  133,  133,  133,  133,  133,  133,  110,
  133,  133,  133,  133,  133,  122,    0,  133,    0,  124,
  115,    0,  116,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  144,    0,    0,  147,    0,    0,  105,  133,
  133,  133,  104,  172,  173,  103,    0,  113,    0,  106,
  107,   98,   99,  100,  112,    0,  109,    0,  178,  177,
  179,  176,  180,  181,  133,   86,   83,   82,   84,   85,
   75,   76,    0,   74,   77,   78,   87,   88,   79,   80,
   81,   91,   92,   89,   90,   93,   94,  120,  114,    0,
    0,  133,   34,   31,   30,   32,   33,   21,    0,    0,
   24,   25,   26,   27,   28,   29,   37,   39,   38,   35,
   43,   41,    0,  134,    0,    0,    0,  135,    0,    0,
    0,  138,  143,  145,    0,  148,  168,    0,    0,    0,
  171,    0,  108,  163,    0,  121,    0,    0,  123,    0,
   22,   23,    0,   18,  139,  141,  140,    0,    0,  137,
  146,  102,  101,    0,  156,    0,  119,  133,    0,    0,
  136,  142,  174,  117,    0,  126,   49,   47,   48,    0,
    0,    0,    0,   42,    0,    0,  125,    0,    0,    0,
    0,   45,   46,   50,
};
short yydgoto[] = {                                       1,
    2,   12,   23,   35,   41,   33,   64,   67,  208,  282,
  373,  281,  403,  430,  431,  439,  432,   82,   78,   80,
   84,   86,   92,   94,   96,   72,   70,   74,   76,   68,
   88,   90,  101,  103,   98,   99,  105,  108,  111,  260,
   69,  147,  123,  141,  303,  172,  107,  257,  349,  110,
  261,  113,  178,  325,  124,  149,  150,  125,  126,  127,
  298,
};
short yysindex[] = {                                      0,
    0,  309, -231, -221, -217, -212, -203, -201, -199, -195,
 -196, -202,  309, -190, -182,  309,  309,  309,  309,  309,
  309,    0, -180,    0,  309,  309,    0,    0,    0,    0,
    0,    0, -204,    0, -174,    0,    0,  417, -194, -173,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -168,  417,    0, -159,  417, -114,  417,
 -114,  417, -114,  417, -114,  417, -114,  417, -114,  417,
 -234,  417,  -18,  417, -114,  417, -114,  417, -114,  417,
 -114,  417, -114,  417, -114,  417, -114,  417,  417, -114,
  417, -153,  417, -114,  417,   10,    0,  417, -153,    0,
  417,   10,    0,    0, -154,  607,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -18,  -18,  -18,
  -18,  -18,  -18,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -191,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  473,    0, -188,  520,    0,    0,
  520,    0, -134, -128, -125, -122, -117, -107, -108, -104,
 -101,  -98,  -97,  -94,  -93,  -91, -206,    0,  243, -234,
 -114, -114, -186, -234,  243,  -18,  243,    0,    0,    0,
    0,    0,    0, -186, -186, -114, -114, -114,  -18,    0,
 -114,  243,   73,    0,    0,    0,    0,    0,    0,    0,
  417,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -152,    0,  417,    0,
    0,   73,    0,  607,  607,  607,  607,  607,  607,  -86,
  -85,  607,  607,  607,  607,  607,  607, -206, -206, -206,
  607, -240, -114, -114, -114, -114, -114, -186, -186, -114,
 -186, -186,    0, -114, -114,    0, -114, -146,    0,    0,
    0,    0,    0,    0,    0,    0, -145,    0, -141,    0,
    0,    0,    0,    0,    0,  -18,    0, -133,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  -82,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   10,
  -60,    0,    0,    0,    0,    0,    0,    0,  607,  607,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -174,    0, -114, -114, -114,    0, -114, -114,
 -114,    0,    0,    0, -114,    0,    0, -114, -114,  243,
    0,  -18,    0,    0,   10,    0,  520, -139,    0,   10,
    0,    0,  -76,    0,    0,    0,    0, -114, -114,    0,
    0,    0,    0, -112,    0, -102,    0,    0, -105, -143,
    0,    0,    0,    0,   10,    0,    0,    0,    0,  -47,
  -66,  -63,  -92,    0,  -56,  -56,    0,  417, -143, -143,
  -39,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,   14,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   29,   14,    0,    0,   14,   14,   14,   14,   14,
   14,    0,   21,    0,   14,   14,    0,    0,    0,    0,
    0,    0,    0,    0,  238,    0,    0,  -28,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -28,    0,    0,  -28,    0,  -28,
    0,  -28,    0,  -28,    0,  -28,    0,  -28,    0,  -28,
    0,  -28,  189,  -28,    0,  -28,    0,  -28,    0,  -28,
    0,  -28,    0,  -28,    0,  -28,    0,  -28,  -28,    0,
  -28,    0,  -28,    0,  -28,    0,    0,  -28,    0,    0,
  -28,    0,    0,    0,    0, -197,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  189,  189,  189,
  189,  189,  189,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  586,    0,    0,    0,
    0,    0,    0,    0,    0,  189,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  189,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -28,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,  324,    0,  -28,    0,
    0,    0,    0, -197, -197, -197, -197, -197, -197,    0,
    0, -197, -197, -197, -197, -197, -197,  586,  586,  586,
 -197,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  189,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -197, -197,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,  238,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  189,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -27,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -28,  -27,  -27,
    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
    0,  638,    0,    0, -121,   -6,  -34,    0,  173,    0,
    0, -140,    0, -355,    0, -185,    0,   11,   15,   22,
   12,   17,   16,   18,   28,   35,   30,   27,   42,   47,
   41,   43,   36,   34,   49,   52,    0,   50,   51,  120,
  -43,  -40,  299, -184, -131,  204,    0,    0,    0,    0,
 -167,    0,  -75,   59,   19,  -59,  -71,   31,  144,  166,
 -166,
};
#define YYTABLESIZE 891
short yytable[] = {                                      71,
   73,   75,   77,   79,   81,   83,   85,   87,   89,   91,
   93,   95,   97,    3,  100,  102,  104,  106,  109,  112,
   15,  143,  139,  263,  118,  299,   13,   39,   13,  306,
  115,  371,  372,  117,  181,  128,  190,  130,   14,  132,
  181,  134,   15,  136,   16,  138,  180,  144,  307,  154,
  309,  156,  180,  158,   17,  160,   18,  162,   19,  164,
   21,  166,   20,  168,  169,  318,  171,   22,  174,   25,
  176,  145,  118,  186,   20,   20,  189,   26,  209,  210,
  278,  279,  280,  442,  443,  211,   66,  212,   38,  213,
   34,  214,  310,  311,   40,  140,  215,  216,   65,  142,
  185,  148,  217,  188,  114,  118,  191,  218,  219,  220,
  221,  222,  223,  151,  116,  224,  427,  225,  192,  226,
  173,  227,  264,  228,  179,  230,  229,  173,  258,  265,
  179,  231,  266,  232,  233,  267,  182,  367,  368,  369,
  268,  305,  182,  302,  118,  119,  262,  428,  429,  269,
  143,  270,  305,  305,  143,  271,  379,  380,  272,  382,
  383,  273,  274,  275,  276,  277,  148,  148,  148,  148,
  148,  148,  348,  359,  360,  308,  120,  121,  151,  151,
  151,  151,  151,  151,  387,  391,  316,  418,  315,  392,
  396,   71,   73,   75,   77,   79,   81,  394,   83,   85,
   87,   89,   91,   93,   95,   97,  333,  100,  102,  104,
  109,  112,  399,  420,  350,  122,  305,  305,  423,  305,
  305,  424,  426,  414,  351,  434,  152,  435,  142,  417,
  436,  304,  142,  444,  148,  437,  438,   17,  139,  145,
  118,  119,  304,  304,   51,   44,  151,  148,  153,  183,
  440,  404,  334,  331,  335,  183,  388,  389,  390,  151,
  336,  332,  339,  329,  327,  340,  139,  145,  118,  119,
  328,  184,  120,  121,  398,  393,  341,  184,  181,  330,
  326,  395,    3,    3,    3,  337,  345,  344,  338,   15,
  180,  152,  152,  152,  152,  152,  152,   13,  342,   13,
  120,  121,  343,  346,  256,  347,  304,  304,  400,  304,
  304,  146,  187,  153,  153,  153,  153,  153,  153,  416,
  352,    0,    0,  181,  419,    0,    0,    0,  181,    0,
    0,    0,    0,    0,  148,  180,    0,    0,    0,  177,
  180,  397,    0,    0,    0,    0,  151,    0,    0,  433,
    0,  415,    0,  181,    0,    0,    0,    0,    0,  152,
    0,    0,    0,    0,    0,  180,    0,    0,  179,  129,
    0,  131,  152,  133,  425,  135,    0,  137,    0,    0,
  182,  153,    0,  155,    0,  157,    0,  159,    0,  161,
    0,  163,    0,  165,  153,  167,    0,    0,  170,    0,
    0,    0,  175,  441,  319,  320,  321,  322,  323,  324,
  148,    0,    0,  179,    0,    0,    0,    0,  179,    0,
    0,    0,  151,    0,    0,  182,    0,    0,    0,    0,
  182,    0,    0,    0,    0,    0,  353,  354,  355,  356,
  357,  358,    0,  179,  361,  362,  363,  364,  365,  366,
    0,    0,  149,  370,    0,  182,    0,    0,    0,  152,
    0,  149,    0,    0,  149,  149,    0,    0,    0,  149,
    0,  149,    0,  149,  149,    0,    0,    0,    0,    0,
    0,  153,    0,  149,  149,  149,  149,  149,    0,  149,
  149,  149,    0,  183,    0,    0,    0,    0,    0,    0,
  149,    0,    0,  149,    0,    0,  149,  149,    0,  300,
  301,  149,    0,  149,  149,  184,    0,  149,    0,    0,
    0,    0,    0,    0,  312,  313,  314,    0,    0,  317,
    0,  401,  402,    0,    0,  152,    0,    0,  183,    0,
    0,    0,  283,  183,    0,    0,  284,  285,  286,  287,
  288,  289,  290,  291,    0,  292,  293,  153,  294,    0,
  184,    0,  295,  296,  297,  184,    0,    0,  183,    3,
    4,    5,    6,    7,    8,    9,   10,    0,    0,    0,
    0,  374,  375,  376,  377,  378,    0,  118,  381,    0,
  184,   11,  384,  385,    0,  386,  118,    0,    0,  118,
  118,    0,    0,    0,  118,    0,  118,    0,  118,  118,
    0,    0,    0,    0,    0,    0,    0,    0,  118,  118,
  118,  118,  118,    0,  118,  118,  118,    0,    0,    0,
    0,    0,    0,    0,    0,  118,    0,    0,  118,    0,
    0,  118,  118,    0,    0,    0,  118,    0,    0,  118,
   24,    0,  118,   27,   28,   29,   30,   31,   32,    0,
    0,    0,   36,   37,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  405,  406,  407,    0,  408,  409,  410,
   42,    0,    0,  411,    0,    0,  412,  413,    0,    0,
    0,    0,   43,   44,    0,    0,    0,   45,    0,   46,
    0,   47,   48,    0,    0,    0,  421,  422,    0,    0,
    0,   49,   50,   51,   52,   53,    0,   54,   55,   56,
    0,    0,    0,    0,    0,    0,    0,    0,   57,    0,
    0,   58,    0,    0,   59,   60,  234,    0,    0,   61,
    0,    0,   62,    0,    0,   63,    0,    0,  235,  236,
    0,    0,    0,  237,    0,  238,    0,  239,  240,    0,
    0,    0,    0,    0,    0,  241,    0,  242,  243,  244,
  245,  246,    0,  247,  248,  249,    0,    0,    0,    0,
    0,    0,    0,  234,  250,    0,    0,  251,    0,    0,
  252,  253,    0,    0,    0,  235,  236,    0,  254,    0,
  237,  255,  238,    0,  239,  240,    0,    0,    0,    0,
    0,    0,  259,    0,  242,  243,  244,  245,  246,    0,
  247,  248,  249,    0,    0,    0,    0,    0,    0,    0,
    0,  250,    0,    0,  251,    0,    0,  252,  253,    0,
    0,    0,    0,    0,    0,  254,    0,    0,  255,   36,
   36,   36,   36,   36,    0,    0,    0,   36,   36,    0,
   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,
  193,  194,  195,  196,  197,    0,    0,    0,    0,    0,
    0,  198,  199,  200,  201,  202,  203,  204,  205,  206,
  207,
};
short yycheck[] = {                                      43,
   44,   45,   46,   47,   48,   49,   50,   51,   52,   53,
   54,   55,   56,    0,   58,   59,   60,   61,   62,   63,
    0,   81,  257,  191,  259,  210,  258,   34,    0,  214,
   65,  272,  273,   68,  106,   70,  112,   72,  260,   74,
  112,   76,  260,   78,  257,   80,  106,   82,  215,   84,
  217,   86,  112,   88,  258,   90,  258,   92,  258,   94,
  257,   96,  258,   98,   99,  232,  101,  270,  103,  260,
  105,  258,  259,  108,  272,  273,  111,  260,  122,  123,
  287,  288,  289,  439,  440,  129,  260,  131,  293,  133,
  271,  135,  224,  225,  269,  330,  140,  141,  293,   81,
  107,   83,  146,  110,  273,  259,  113,  148,  149,  150,
  151,  152,  153,   83,  274,  159,  260,  161,  273,  163,
  102,  165,  257,  167,  106,  317,  170,  109,  317,  258,
  112,  175,  258,  177,  178,  258,  106,  278,  279,  280,
  258,  213,  112,  330,  259,  260,  190,  291,  292,  257,
  210,  260,  224,  225,  214,  260,  288,  289,  260,  291,
  292,  260,  260,  258,  258,  257,  148,  149,  150,  151,
  152,  153,  325,  260,  260,  216,  291,  292,  148,  149,
  150,  151,  152,  153,  331,  331,  230,  327,  229,  331,
  273,  235,  236,  237,  238,  239,  240,  331,  242,  243,
  244,  245,  246,  247,  248,  249,  241,  251,  252,  253,
  254,  255,  273,  290,  258,  330,  288,  289,  331,  291,
  292,  324,  328,  390,  259,  273,   83,  294,  210,  397,
  294,  213,  214,  273,  216,  328,  293,    0,  257,  258,
  259,  260,  224,  225,  273,  273,  216,  229,   83,  106,
  436,  373,  242,  239,  243,  112,  300,  301,  302,  229,
  244,  240,  247,  237,  235,  248,  257,  258,  259,  260,
  236,  106,  291,  292,  350,  316,  249,  112,  350,  238,
  234,  325,  269,  270,  271,  245,  253,  252,  246,  269,
  350,  148,  149,  150,  151,  152,  153,  269,  250,  271,
  291,  292,  251,  254,  185,  255,  288,  289,  352,  291,
  292,  330,  109,  148,  149,  150,  151,  152,  153,  395,
  262,   -1,   -1,  395,  400,   -1,   -1,   -1,  400,   -1,
   -1,   -1,   -1,   -1,  316,  395,   -1,   -1,   -1,  330,
  400,  348,   -1,   -1,   -1,   -1,  316,   -1,   -1,  425,
   -1,  392,   -1,  425,   -1,   -1,   -1,   -1,   -1,  216,
   -1,   -1,   -1,   -1,   -1,  425,   -1,   -1,  350,   71,
   -1,   73,  229,   75,  418,   77,   -1,   79,   -1,   -1,
  350,  216,   -1,   85,   -1,   87,   -1,   89,   -1,   91,
   -1,   93,   -1,   95,  229,   97,   -1,   -1,  100,   -1,
   -1,   -1,  104,  438,  332,  333,  334,  335,  336,  337,
  392,   -1,   -1,  395,   -1,   -1,   -1,   -1,  400,   -1,
   -1,   -1,  392,   -1,   -1,  395,   -1,   -1,   -1,   -1,
  400,   -1,   -1,   -1,   -1,   -1,  264,  265,  266,  267,
  268,  269,   -1,  425,  272,  273,  274,  275,  276,  277,
   -1,   -1,  264,  281,   -1,  425,   -1,   -1,   -1,  316,
   -1,  273,   -1,   -1,  276,  277,   -1,   -1,   -1,  281,
   -1,  283,   -1,  285,  286,   -1,   -1,   -1,   -1,   -1,
   -1,  316,   -1,  295,  296,  297,  298,  299,   -1,  301,
  302,  303,   -1,  350,   -1,   -1,   -1,   -1,   -1,   -1,
  312,   -1,   -1,  315,   -1,   -1,  318,  319,   -1,  211,
  212,  323,   -1,  325,  326,  350,   -1,  329,   -1,   -1,
   -1,   -1,   -1,   -1,  226,  227,  228,   -1,   -1,  231,
   -1,  359,  360,   -1,   -1,  392,   -1,   -1,  395,   -1,
   -1,   -1,  300,  400,   -1,   -1,  304,  305,  306,  307,
  308,  309,  310,  311,   -1,  313,  314,  392,  316,   -1,
  395,   -1,  320,  321,  322,  400,   -1,   -1,  425,  261,
  262,  263,  264,  265,  266,  267,  268,   -1,   -1,   -1,
   -1,  283,  284,  285,  286,  287,   -1,  264,  290,   -1,
  425,  283,  294,  295,   -1,  297,  273,   -1,   -1,  276,
  277,   -1,   -1,   -1,  281,   -1,  283,   -1,  285,  286,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  295,  296,
  297,  298,  299,   -1,  301,  302,  303,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,  312,   -1,   -1,  315,   -1,
   -1,  318,  319,   -1,   -1,   -1,  323,   -1,   -1,  326,
   13,   -1,  329,   16,   17,   18,   19,   20,   21,   -1,
   -1,   -1,   25,   26,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  375,  376,  377,   -1,  379,  380,  381,
  264,   -1,   -1,  385,   -1,   -1,  388,  389,   -1,   -1,
   -1,   -1,  276,  277,   -1,   -1,   -1,  281,   -1,  283,
   -1,  285,  286,   -1,   -1,   -1,  408,  409,   -1,   -1,
   -1,  295,  296,  297,  298,  299,   -1,  301,  302,  303,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  312,   -1,
   -1,  315,   -1,   -1,  318,  319,  264,   -1,   -1,  323,
   -1,   -1,  326,   -1,   -1,  329,   -1,   -1,  276,  277,
   -1,   -1,   -1,  281,   -1,  283,   -1,  285,  286,   -1,
   -1,   -1,   -1,   -1,   -1,  293,   -1,  295,  296,  297,
  298,  299,   -1,  301,  302,  303,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  264,  312,   -1,   -1,  315,   -1,   -1,
  318,  319,   -1,   -1,   -1,  276,  277,   -1,  326,   -1,
  281,  329,  283,   -1,  285,  286,   -1,   -1,   -1,   -1,
   -1,   -1,  293,   -1,  295,  296,  297,  298,  299,   -1,
  301,  302,  303,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  312,   -1,   -1,  315,   -1,   -1,  318,  319,   -1,
   -1,   -1,   -1,   -1,   -1,  326,   -1,   -1,  329,  264,
  265,  266,  267,  268,   -1,   -1,   -1,  272,  273,   -1,
  275,  276,  277,  278,  279,  280,  281,  282,  283,  284,
  264,  265,  266,  267,  268,   -1,   -1,   -1,   -1,   -1,
   -1,  275,  276,  277,  278,  279,  280,  281,  282,  283,
  284,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 337
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
"SHADCOLOR","LICOLOR","OBJECT","INIT","PERIODICTASK","MAIN","END","PROP","TYPE",
"SIZE","POSITION","VALUE","VALUEMIN","VALUEMAX","TITLE","SWALLOWEXEC","ICON",
"FLAGS","WARP","WRITETOFILE","HIDDEN","CANBESELECTED","NORELIEFSTRING","CASE",
"SINGLECLIC","DOUBLECLIC","BEG","POINT","EXEC","HIDE","SHOW","CHFORECOLOR",
"CHBACKCOLOR","GETVALUE","CHVALUE","CHVALUEMAX","CHVALUEMIN","ADD","DIV","MULT",
"GETTITLE","GETOUTPUT","STRCOPY","NUMTOHEX","HEXTONUM","QUIT","LAUNCHSCRIPT",
"GETSCRIPTFATHER","SENDTOSCRIPT","RECEIVFROMSCRIPT","GET","SET","SENDSIGN",
"REMAINDEROFDIV","GETTIME","GETSCRIPTARG","IF","THEN","ELSE","FOR","TO","DO",
"WHILE","BEGF","ENDF","EQUAL","INFEQ","SUPEQ","INF","SUP","DIFF",
};
char *yyrule[] = {
"$accept : script",
"script : initvar head initbloc periodictask object",
"initvar :",
"head :",
"head : WINDOWTITLE GSTR head",
"head : ICON STR head",
"head : WINDOWPOSITION NUMBER NUMBER head",
"head : WINDOWSIZE NUMBER NUMBER head",
"head : BACKCOLOR GSTR head",
"head : FORECOLOR GSTR head",
"head : SHADCOLOR GSTR head",
"head : LICOLOR GSTR head",
"head : FONT STR head",
"initbloc :",
"initbloc : INIT creerbloc BEG instr END",
"periodictask :",
"periodictask : PERIODICTASK creerbloc BEG instr END",
"object :",
"object : OBJECT id PROP init verify mainloop object",
"id : NUMBER",
"init :",
"init : TYPE STR init",
"init : SIZE NUMBER NUMBER init",
"init : POSITION NUMBER NUMBER init",
"init : VALUE NUMBER init",
"init : VALUEMIN NUMBER init",
"init : VALUEMAX NUMBER init",
"init : TITLE GSTR init",
"init : SWALLOWEXEC GSTR init",
"init : ICON STR init",
"init : BACKCOLOR GSTR init",
"init : FORECOLOR GSTR init",
"init : SHADCOLOR GSTR init",
"init : LICOLOR GSTR init",
"init : FONT STR init",
"init : FLAGS flags init",
"flags :",
"flags : HIDDEN flags",
"flags : NORELIEFSTRING flags",
"flags : CANBESELECTED flags",
"verify :",
"mainloop : END",
"mainloop : MAIN addtabcase CASE case END",
"addtabcase :",
"case :",
"case : clic POINT bloc case",
"case : number POINT bloc case",
"clic : SINGLECLIC",
"clic : DOUBLECLIC",
"number : NUMBER",
"bloc : BEG instr END",
"instr :",
"instr : EXEC exec instr",
"instr : WARP warp instr",
"instr : WRITETOFILE writetofile instr",
"instr : HIDE hide instr",
"instr : SHOW show instr",
"instr : CHVALUE chvalue instr",
"instr : CHVALUEMAX chvaluemax instr",
"instr : CHVALUEMIN chvaluemin instr",
"instr : POSITION position instr",
"instr : SIZE size instr",
"instr : TITLE title instr",
"instr : ICON icon instr",
"instr : FONT font instr",
"instr : CHFORECOLOR chforecolor instr",
"instr : CHBACKCOLOR chbackcolor instr",
"instr : SET set instr",
"instr : SENDSIGN sendsign instr",
"instr : QUIT quit instr",
"instr : SENDTOSCRIPT sendtoscript instr",
"instr : IF ifthenelse instr",
"instr : FOR loop instr",
"instr : WHILE while instr",
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
#line 750 "Compiler/bisonin"














#line 1073 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
#if defined(__STDC__)
yyparse(void)
#else
yyparse()
#endif
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
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
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
#line 372 "Compiler/bisonin"
{ InitVarGlob(); }
break;
case 4:
#line 377 "Compiler/bisonin"
{		/* Titre de la fenetre */
				 scriptprop->titlewin=yyvsp[-1].str;
				}
break;
case 5:
#line 380 "Compiler/bisonin"
{
				 scriptprop->icon=yyvsp[-1].str;
				}
break;
case 6:
#line 384 "Compiler/bisonin"
{		/* Position et taille de la fenetre */
				 scriptprop->x=yyvsp[-2].number;
				 scriptprop->y=yyvsp[-1].number;
				}
break;
case 7:
#line 389 "Compiler/bisonin"
{		/* Position et taille de la fenetre */
				 scriptprop->width=yyvsp[-2].number;
				 scriptprop->height=yyvsp[-1].number;
				}
break;
case 8:
#line 393 "Compiler/bisonin"
{ 		/* Couleur de fond */
				 scriptprop->backcolor=yyvsp[-1].str;
				}
break;
case 9:
#line 396 "Compiler/bisonin"
{ 		/* Couleur des lignes */
				 scriptprop->forecolor=yyvsp[-1].str;
				}
break;
case 10:
#line 399 "Compiler/bisonin"
{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=yyvsp[-1].str;
				}
break;
case 11:
#line 402 "Compiler/bisonin"
{ 		/* Couleur des lignes */
				 scriptprop->licolor=yyvsp[-1].str;
				}
break;
case 12:
#line 405 "Compiler/bisonin"
{
				 scriptprop->font=yyvsp[-1].str;
				}
break;
case 14:
#line 412 "Compiler/bisonin"
{
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				}
break;
case 16:
#line 418 "Compiler/bisonin"
{
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				}
break;
case 19:
#line 431 "Compiler/bisonin"
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
				}
break;
case 21:
#line 447 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].type=yyvsp[-1].str;
				 HasType=1;
				}
break;
case 22:
#line 451 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].width=yyvsp[-2].number;
				 (*tabobj)[nbobj].height=yyvsp[-1].number;
				}
break;
case 23:
#line 455 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].x=yyvsp[-2].number;
				 (*tabobj)[nbobj].y=yyvsp[-1].number;
				 HasPosition=1;
				}
break;
case 24:
#line 460 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].value=yyvsp[-1].number;
				}
break;
case 25:
#line 463 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].value2=yyvsp[-1].number;
				}
break;
case 26:
#line 466 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].value3=yyvsp[-1].number;
				}
break;
case 27:
#line 469 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].title=yyvsp[-1].str;
				}
break;
case 28:
#line 472 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].swallow=yyvsp[-1].str;
				}
break;
case 29:
#line 475 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].icon=yyvsp[-1].str;
				}
break;
case 30:
#line 478 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].backcolor=yyvsp[-1].str;
				}
break;
case 31:
#line 481 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].forecolor=yyvsp[-1].str;
				}
break;
case 32:
#line 484 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].shadcolor=yyvsp[-1].str;
				}
break;
case 33:
#line 487 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].licolor=yyvsp[-1].str;
				}
break;
case 34:
#line 490 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].font=yyvsp[-1].str;
				}
break;
case 37:
#line 496 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].flags[0]=True;
				}
break;
case 38:
#line 499 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].flags[1]=True;
				}
break;
case 39:
#line 502 "Compiler/bisonin"
{
				 (*tabobj)[nbobj].flags[2]=True;
				}
break;
case 40:
#line 508 "Compiler/bisonin"
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
case 41:
#line 519 "Compiler/bisonin"
{ InitObjTabCase(0); }
break;
case 43:
#line 523 "Compiler/bisonin"
{ InitObjTabCase(1); }
break;
case 47:
#line 530 "Compiler/bisonin"
{ InitCase(-1); }
break;
case 48:
#line 531 "Compiler/bisonin"
{ InitCase(-2); }
break;
case 49:
#line 534 "Compiler/bisonin"
{ InitCase(yyvsp[0].number); }
break;
case 95:
#line 591 "Compiler/bisonin"
{ AddCom(1,1); }
break;
case 96:
#line 593 "Compiler/bisonin"
{ AddCom(2,1);}
break;
case 97:
#line 595 "Compiler/bisonin"
{ AddCom(3,1);}
break;
case 98:
#line 597 "Compiler/bisonin"
{ AddCom(4,2);}
break;
case 99:
#line 599 "Compiler/bisonin"
{ AddCom(21,2);}
break;
case 100:
#line 601 "Compiler/bisonin"
{ AddCom(22,2);}
break;
case 101:
#line 603 "Compiler/bisonin"
{ AddCom(5,3);}
break;
case 102:
#line 605 "Compiler/bisonin"
{ AddCom(6,3);}
break;
case 103:
#line 607 "Compiler/bisonin"
{ AddCom(7,2);}
break;
case 104:
#line 609 "Compiler/bisonin"
{ AddCom(8,2);}
break;
case 105:
#line 611 "Compiler/bisonin"
{ AddCom(9,2);}
break;
case 106:
#line 613 "Compiler/bisonin"
{ AddCom(10,2);}
break;
case 107:
#line 615 "Compiler/bisonin"
{ AddCom(19,2);}
break;
case 108:
#line 617 "Compiler/bisonin"
{ AddCom(11,2);}
break;
case 109:
#line 619 "Compiler/bisonin"
{ AddCom(12,2);}
break;
case 110:
#line 621 "Compiler/bisonin"
{ AddCom(13,0);}
break;
case 111:
#line 623 "Compiler/bisonin"
{ AddCom(17,1);}
break;
case 112:
#line 625 "Compiler/bisonin"
{ AddCom(23,2);}
break;
case 113:
#line 627 "Compiler/bisonin"
{ AddCom(18,2);}
break;
case 117:
#line 637 "Compiler/bisonin"
{ AddComBloc(14,3,2); }
break;
case 120:
#line 642 "Compiler/bisonin"
{ EmpilerBloc(); }
break;
case 121:
#line 644 "Compiler/bisonin"
{ DepilerBloc(2); }
break;
case 122:
#line 645 "Compiler/bisonin"
{ DepilerBloc(2); }
break;
case 123:
#line 648 "Compiler/bisonin"
{ DepilerBloc(1); }
break;
case 124:
#line 649 "Compiler/bisonin"
{ DepilerBloc(1); }
break;
case 125:
#line 653 "Compiler/bisonin"
{ AddComBloc(15,3,1); }
break;
case 126:
#line 657 "Compiler/bisonin"
{ AddComBloc(16,3,1); }
break;
case 127:
#line 662 "Compiler/bisonin"
{ AddVar(yyvsp[0].str); }
break;
case 128:
#line 664 "Compiler/bisonin"
{ AddConstStr(yyvsp[0].str); }
break;
case 129:
#line 666 "Compiler/bisonin"
{ AddConstStr(yyvsp[0].str); }
break;
case 130:
#line 668 "Compiler/bisonin"
{ AddConstNum(yyvsp[0].number); }
break;
case 131:
#line 670 "Compiler/bisonin"
{ AddConstNum(-1); }
break;
case 132:
#line 672 "Compiler/bisonin"
{ AddConstNum(-2); }
break;
case 133:
#line 674 "Compiler/bisonin"
{ AddLevelBufArg(); }
break;
case 134:
#line 676 "Compiler/bisonin"
{ AddFunct(1,1); }
break;
case 135:
#line 677 "Compiler/bisonin"
{ AddFunct(2,1); }
break;
case 136:
#line 678 "Compiler/bisonin"
{ AddFunct(3,1); }
break;
case 137:
#line 679 "Compiler/bisonin"
{ AddFunct(4,1); }
break;
case 138:
#line 680 "Compiler/bisonin"
{ AddFunct(5,1); }
break;
case 139:
#line 681 "Compiler/bisonin"
{ AddFunct(6,1); }
break;
case 140:
#line 682 "Compiler/bisonin"
{ AddFunct(7,1); }
break;
case 141:
#line 683 "Compiler/bisonin"
{ AddFunct(8,1); }
break;
case 142:
#line 684 "Compiler/bisonin"
{ AddFunct(9,1); }
break;
case 143:
#line 685 "Compiler/bisonin"
{ AddFunct(10,1); }
break;
case 144:
#line 686 "Compiler/bisonin"
{ AddFunct(11,1); }
break;
case 145:
#line 687 "Compiler/bisonin"
{ AddFunct(12,1); }
break;
case 146:
#line 688 "Compiler/bisonin"
{ AddFunct(13,1); }
break;
case 147:
#line 689 "Compiler/bisonin"
{ AddFunct(14,1); }
break;
case 148:
#line 690 "Compiler/bisonin"
{ AddFunct(15,1); }
break;
case 149:
#line 695 "Compiler/bisonin"
{ }
break;
case 176:
#line 741 "Compiler/bisonin"
{ l=1-250000; AddBufArg(&l,1); }
break;
case 177:
#line 742 "Compiler/bisonin"
{ l=2-250000; AddBufArg(&l,1); }
break;
case 178:
#line 743 "Compiler/bisonin"
{ l=3-250000; AddBufArg(&l,1); }
break;
case 179:
#line 744 "Compiler/bisonin"
{ l=4-250000; AddBufArg(&l,1); }
break;
case 180:
#line 745 "Compiler/bisonin"
{ l=5-250000; AddBufArg(&l,1); }
break;
case 181:
#line 746 "Compiler/bisonin"
{ l=6-250000; AddBufArg(&l,1); }
break;
#line 1667 "y.tab.c"
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
