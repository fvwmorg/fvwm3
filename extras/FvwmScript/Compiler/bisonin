%{
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


%}

/* Declaration des types des tokens, tous les types sont assemblés dans union */ 
/* Le type est celui de yyval, yyval est utilisé dans lex explicitement */
/* Dans bison, il est utlise implicitement avec $1, $2... */
%union {  char *str;
          int number;
       }

/* Declaration des symboles terminaux */
%token <str> STR GSTR VAR
%token <number> NUMBER	/* Nombre pour communiquer les dimensions */

%token WINDOWTITLE WINDOWSIZE WINDOWPOSITION FONT
%token FORECOLOR BACKCOLOR SHADCOLOR LICOLOR
%token OBJECT INIT PERIODICTASK MAIN END PROP
%token TYPE SIZE POSITION VALUE VALUEMIN VALUEMAX TITLE SWALLOWEXEC ICON FLAGS WARP WRITETOFILE
%token HIDDEN CANBESELECTED NORELIEFSTRING
%token CASE SINGLECLIC DOUBLECLIC BEG POINT
%token EXEC HIDE SHOW FONT CHFORECOLOR CHBACKCOLOR GETVALUE CHVALUE CHVALUEMAX CHVALUEMIN
%token ADD DIV MULT GETTITLE GETOUTPUT STRCOPY NUMTOHEX HEXTONUM QUIT
%token LAUNCHSCRIPT GETSCRIPTFATHER SENDTOSCRIPT RECEIVFROMSCRIPT
%token GET SET SENDSIGN REMAINDEROFDIV GETTIME GETSCRIPTARG
%token IF THEN ELSE FOR TO DO WHILE
%token BEGF ENDF
%token EQUAL INFEQ SUPEQ INF SUP DIFF

%%
script: initvar head initbloc periodictask object ;

/* Initialisation des variables */
initvar: 			{ InitVarGlob(); }
       ;

/* Entete du scripte decrivant les options par defaut */
head:				/* vide: dans ce cas on utilise les valeurs par défaut */	
    | WINDOWTITLE GSTR head	{		/* Titre de la fenetre */
				 scriptprop->titlewin=$2;
				}
    | ICON STR head		{
				 scriptprop->icon=$2;
				}
    | WINDOWPOSITION NUMBER NUMBER head
				{		/* Position et taille de la fenetre */
				 scriptprop->x=$2;
				 scriptprop->y=$3;
				}
    | WINDOWSIZE NUMBER NUMBER head
				{		/* Position et taille de la fenetre */
				 scriptprop->width=$2;
				 scriptprop->height=$3;
				}
    | BACKCOLOR GSTR head	{ 		/* Couleur de fond */
				 scriptprop->backcolor=$2;
				}
    | FORECOLOR GSTR head	{ 		/* Couleur des lignes */
				 scriptprop->forecolor=$2;
				}
    | SHADCOLOR GSTR head	{ 		/* Couleur des lignes */
				 scriptprop->shadcolor=$2;
				}
    | LICOLOR GSTR head	{ 		/* Couleur des lignes */
				 scriptprop->licolor=$2;
				}
    | FONT STR head		{
				 scriptprop->font=$2;
				}
   ;

/* Bloc d'initialisation du script */
initbloc:		/* cas ou il n'y pas de bloc d'initialisation du script */
	| INIT creerbloc BEG instr END {
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--; 
				}

periodictask:		/* cas ou il n'y a pas de tache periodique */
	    | PERIODICTASK creerbloc BEG instr END {
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--; 
				}

	    ;


/* Desciption d'un objet */
object :			/* Vide */
    | OBJECT id PROP init verify mainloop object
    ;

id: NUMBER			{ nbobj++;
				  if (nbobj>100)
				  { yyerror("Too many items\n");
				    exit(1);}
				  if (($1<1)||($1>100))
				  { yyerror("Choose item id between 1 and 100\n");
				    exit(1);} 
				  if (TabIdObj[$1]!=-1) 
				  { i=$1; fprintf(stderr,"Line %d: item id %d already used:\n",numligne,$1);
				    exit(1);}
			          TabIdObj[$1]=nbobj;
				  (*tabobj)[nbobj].id=$1;
				}
  ;

init:				/* vide */
    | TYPE STR init		{
				 (*tabobj)[nbobj].type=$2;
				 HasType=1;
				}
    | SIZE NUMBER NUMBER init	{
				 (*tabobj)[nbobj].width=$2;
				 (*tabobj)[nbobj].height=$3;
				}
    | POSITION NUMBER NUMBER init {
				 (*tabobj)[nbobj].x=$2;
				 (*tabobj)[nbobj].y=$3;
				 HasPosition=1;
				}
    | VALUE NUMBER init		{
				 (*tabobj)[nbobj].value=$2;
				}
    | VALUEMIN NUMBER init		{
				 (*tabobj)[nbobj].value2=$2;
				}
    | VALUEMAX NUMBER init		{
				 (*tabobj)[nbobj].value3=$2;
				}
    | TITLE GSTR init		{
				 (*tabobj)[nbobj].title=$2;
				}
    | SWALLOWEXEC GSTR init	{
				 (*tabobj)[nbobj].swallow=$2;
				}
    | ICON STR init		{
				 (*tabobj)[nbobj].icon=$2;
				}
    | BACKCOLOR GSTR init	{
				 (*tabobj)[nbobj].backcolor=$2;
				}
    | FORECOLOR GSTR init	{
				 (*tabobj)[nbobj].forecolor=$2;
				}
    | SHADCOLOR GSTR init	{
				 (*tabobj)[nbobj].shadcolor=$2;
				}
    | LICOLOR GSTR init	{
				 (*tabobj)[nbobj].licolor=$2;
				}
    | FONT STR init		{
				 (*tabobj)[nbobj].font=$2;
				}
    | FLAGS flags init		
    ;
flags:
     | HIDDEN flags		{
				 (*tabobj)[nbobj].flags[0]=True;
				}
     | NORELIEFSTRING flags	{
				 (*tabobj)[nbobj].flags[1]=True;
				}
     | CANBESELECTED flags	{
				 (*tabobj)[nbobj].flags[2]=True;
				}
    ; 


verify:				 { 
				  if (!HasPosition)
				   { yyerror("No position for object");
				     exit(1);}
				  if (!HasType)
				   { yyerror("No type for object");
				     exit(1);}
				  HasPosition=0;
				  HasType=0;
				 }

mainloop: END			{ InitObjTabCase(0); }
	| MAIN addtabcase CASE case END
	;

addtabcase:			{ InitObjTabCase(1); }

case:
    | clic POINT bloc case
    | number POINT bloc case
    ;

clic :  SINGLECLIC	{ InitCase(-1); }
     |  DOUBLECLIC	{ InitCase(-2); }
     ;

number :  NUMBER		{ InitCase($1); }
       ;

bloc: BEG instr END
    ;

				
/* ensemble d'instructions */
instr:
    |	EXEC exec instr
    |   WARP warp instr
    |	WRITETOFILE writetofile instr
    |	HIDE hide instr
    |	SHOW show instr
    |	CHVALUE chvalue instr
    |	CHVALUEMAX chvaluemax instr
    |	CHVALUEMIN chvaluemin instr
    |	POSITION position instr
    |	SIZE size instr
    |	TITLE title instr
    |	ICON icon instr
    |	FONT font instr
    |	CHFORECOLOR chforecolor instr
    |	CHBACKCOLOR chbackcolor instr
    |   SET set instr
    |   SENDSIGN sendsign instr
    |	QUIT quit instr
    |	SENDTOSCRIPT sendtoscript instr
    |	IF ifthenelse instr
    |	FOR loop instr
    |	WHILE while instr
    ;

/* une seule instruction */
oneinstr: EXEC exec
    	| WARP warp
	| WRITETOFILE writetofile
	| HIDE hide
	| SHOW show
	| CHVALUE chvalue
	| CHVALUEMAX chvaluemax
	| CHVALUEMIN chvaluemin
	| POSITION position
	| SIZE size
	| TITLE title
	| ICON icon
	| FONT font
	| CHFORECOLOR chforecolor
	| CHBACKCOLOR chbackcolor
	| SET set
	| SENDSIGN sendsign
	| QUIT quit
	| SENDTOSCRIPT sendtoscript
	| FOR loop
	| WHILE while
	;

exec: addlbuff args			{ AddCom(1,1); }
    ;
hide: addlbuff numarg			{ AddCom(2,1);}
    ;
show: addlbuff numarg			{ AddCom(3,1);}
    ;
chvalue: addlbuff numarg addlbuff numarg	{ AddCom(4,2);}
       ;
chvaluemax: addlbuff numarg addlbuff numarg	{ AddCom(21,2);}
       ;
chvaluemin: addlbuff numarg addlbuff numarg	{ AddCom(22,2);}
       ;
position: addlbuff numarg addlbuff numarg addlbuff numarg	{ AddCom(5,3);}
        ;
size: addlbuff numarg addlbuff numarg addlbuff numarg		{ AddCom(6,3);}
    ;
icon: addlbuff numarg addlbuff strarg	{ AddCom(7,2);}
    ;
title: addlbuff numarg addlbuff gstrarg	{ AddCom(8,2);}
     ;
font: addlbuff numarg addlbuff strarg	{ AddCom(9,2);}
    ;
chforecolor: addlbuff numarg addlbuff gstrarg	{ AddCom(10,2);}
           ;
chbackcolor: addlbuff numarg addlbuff gstrarg	{ AddCom(19,2);}
           ;
set: addlbuff vararg GET addlbuff args	{ AddCom(11,2);}
   ;
sendsign: addlbuff numarg addlbuff numarg{ AddCom(12,2);}
	;
quit: 					{ AddCom(13,0);}
    ;
warp: addlbuff numarg			{ AddCom(17,1);}
    ;
sendtoscript: addlbuff numarg addlbuff args 	{ AddCom(23,2);}
	    ;
writetofile: addlbuff strarg addlbuff args	{ AddCom(18,2);}
	   ;
ifthenelse: headif creerbloc bloc1 else
          ;
loop: headloop creerbloc bloc2
    ;
while: headwhile creerbloc bloc2
    ;

/* Boucle conditionnelle: compare n'importe quel type d'argument */
headif: addlbuff arg addlbuff compare addlbuff arg THEN 	{ AddComBloc(14,3,2); }
      ;
else: 				/* Le else est optionnel */
    | ELSE creerbloc bloc2
    ;
creerbloc:			{ EmpilerBloc(); }
         ;
bloc1: BEG instr END		{ DepilerBloc(2); }
     | oneinstr			{ DepilerBloc(2); }
     ;

bloc2: BEG instr END		{ DepilerBloc(1); }
     | oneinstr			{ DepilerBloc(1); }
     ;

/* Boucle sur une variable */
headloop: addlbuff vararg GET addlbuff arg TO addlbuff arg DO	{ AddComBloc(15,3,1); }
        ;

/* Boucle conditionnelle while */
headwhile: addlbuff arg addlbuff compare addlbuff arg DO	{ AddComBloc(16,3,1); }
        ;

/* Argument de commandes */
/* Argument elementaire */
var	: VAR			{ AddVar($1); }
	;
str	: STR			{ AddConstStr($1); }
	;
gstr	: GSTR			{ AddConstStr($1); }
	;
num	: NUMBER		{ AddConstNum($1); }
	;
singleclic2: SINGLECLIC		{ AddConstNum(-1); }
	   ;
doubleclic2: DOUBLECLIC		{ AddConstNum(-2); }
	   ;
addlbuff:			{ AddLevelBufArg(); }
	;
function: GETVALUE numarg	{ AddFunct(1,1); }
	| GETTITLE numarg	{ AddFunct(2,1); }
	| GETOUTPUT gstrarg numarg numarg { AddFunct(3,1); }
	| NUMTOHEX numarg numarg { AddFunct(4,1); }
	| HEXTONUM gstrarg	{ AddFunct(5,1); }
	| ADD numarg numarg { AddFunct(6,1); }
	| MULT numarg numarg { AddFunct(7,1); }
	| DIV numarg numarg { AddFunct(8,1); }
	| STRCOPY gstrarg numarg numarg { AddFunct(9,1); }
	| LAUNCHSCRIPT gstrarg { AddFunct(10,1); }
	| GETSCRIPTFATHER { AddFunct(11,1); }
	| RECEIVFROMSCRIPT numarg { AddFunct(12,1); }
	| REMAINDEROFDIV numarg numarg { AddFunct(13,1); }
	| GETTIME { AddFunct(14,1); }
	| GETSCRIPTARG numarg { AddFunct(15,1); }
	;


/* Plusieurs arguments de type differents */
args	:			{ }
	| singleclic2 args
	| doubleclic2 args
	| var args
	| gstr args
	| str args
	| num args
	| BEGF addlbuff function ENDF args
	;


/* Argument unique de n'importe quel type */
arg	: var
	| singleclic2
	| doubleclic2
	| gstr
	| str
	| num
	| BEGF addlbuff function ENDF
	;

/* Argument unique de type numerique */
numarg	: singleclic2
	| doubleclic2
	| num
	| var
	| BEGF addlbuff function ENDF
	;

/* Argument unique de type str */
strarg	: var
	| str
	| BEGF addlbuff function ENDF
	;

/* Argument unique de type gstr */
gstrarg	: var
	| gstr
	| BEGF addlbuff function ENDF
	;

/* Argument unique de type var, pas de fonction */
vararg	: var
	;

/* element de comparaison entre deux variables numerique */
compare	: INF			 { l=1-250000; AddBufArg(&l,1); }
	| INFEQ			 { l=2-250000; AddBufArg(&l,1); }
	| EQUAL			 { l=3-250000; AddBufArg(&l,1); }
	| SUPEQ			 { l=4-250000; AddBufArg(&l,1); }
	| SUP			 { l=5-250000; AddBufArg(&l,1); }
	| DIFF			 { l=6-250000; AddBufArg(&l,1); }
	;

%%














