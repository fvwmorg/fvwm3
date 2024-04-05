%{
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */
#include "config.h"
#include "types.h"
#include "libs/FGettext.h"
#include "libs/log.h"

#define MAX_VARS 5120
extern int numligne;
ScriptProp *scriptprop;
int nbobj=-1;			/* Number of objects */
int HasPosition,HasType=0;
TabObj *tabobj;		/* Array of objects, limit=1000 */
int TabIdObj[1001]; 	/* Array of object IDs */
Bloc **TabIObj;		/* TabIObj[Obj][Case] -> block linked to case */
Bloc *PileBloc[10];	/* 10 imbrications max of conditional loops */
int TopPileB=0;		/* Top of block stack */
CaseObj *TabCObj;	/* Structure to store cases values and their ID */
int CurrCase;
int i;
char **TabNVar;		/* Array of variable names */
char **TabVVar;		/* Array of variable values */
int NbVar;
long BuffArg[6][20];	/* Arguments are added by layers for each function */
int NbArg[6];		/* Array: number of arguments for each layer */
int SPileArg;		/* Size of argument stack */
long l;
extern char* ScriptName;

/* Global Initialization */
void InitVarGlob(void)
{
 scriptprop=fxcalloc(1, sizeof(ScriptProp));
 scriptprop->x=-1;
 scriptprop->y=-1;
 scriptprop->colorset = -1;
 scriptprop->initbloc=NULL;

 tabobj=fxcalloc(1, sizeof(TabObj));
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

/* Object Initialization */
void InitObjTabCase(int HasMainLoop)
{
 if (nbobj==0)
 {
  TabIObj=fxcalloc(1, sizeof(long));
  TabCObj=fxcalloc(1, sizeof(CaseObj));
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

/* Add a case into TabCase array */
/* Initialization of a case: increase array size */
void InitCase(int cond)
{
 CurrCase++;

 /* We store the case condition */
 TabCObj[nbobj].NbCase++;
 if (TabCObj[nbobj].NbCase==0)
  TabCObj[nbobj].LstCase=fxcalloc(1, sizeof(int));
 else
  TabCObj[nbobj].LstCase=(int*)realloc(TabCObj[nbobj].LstCase,sizeof(int)*(CurrCase+1));
 TabCObj[nbobj].LstCase[CurrCase]=cond;

 if (CurrCase==0)
  TabIObj[nbobj]=fxcalloc(1, sizeof(Bloc));
 else
  TabIObj[nbobj]=(Bloc*)realloc(TabIObj[nbobj],sizeof(Bloc)*(CurrCase+1));

 TabIObj[nbobj][CurrCase].NbInstr=-1;
 TabIObj[nbobj][CurrCase].TabInstr=NULL;

 /* This case is for the current instruction block: we stack it */
 PileBloc[0]=&TabIObj[nbobj][CurrCase];
 TopPileB=0;
}

/* Remove a level of args in BuffArg stack */
void RmLevelBufArg(void)
{
  SPileArg--;
}

/* Function to concatenate the n latest levels of stack */
/* Returns the sorted and unstacked elements and size */
long *Depile(int NbLevelArg, int *s)
{
 long *Temp;
 int j;
 int i;
 int size;

 if (NbLevelArg>0)
 {
  Temp=fxcalloc(1, sizeof(long));
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
  for (i=0;i<NbLevelArg;i++)	/* Unstack the argument layers */
   RmLevelBufArg();
  return Temp;
 }
 else
 {
  *s=0;
  return NULL;
 }
}

/* Add a command */
void AddCom(int Type, int NbLevelArg)
{
 int CurrInstr;


 PileBloc[TopPileB]->NbInstr++;
 CurrInstr=PileBloc[TopPileB]->NbInstr;

 if (CurrInstr==0)
  PileBloc[TopPileB]->TabInstr=fxcalloc(1, sizeof(Instr) * (CurrInstr + 1));
 else
  PileBloc[TopPileB]->TabInstr=(Instr*)realloc(PileBloc[TopPileB]->TabInstr,
				sizeof(Instr)*(CurrInstr+1));
 /* Put instructions into block */
 PileBloc[TopPileB]->TabInstr[CurrInstr].Type=Type;
 /* We remove the last arguments layer and put it in command */

 PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg=Depile(NbLevelArg,
		&PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg);
}

/* Initialization of the buffer containing the current command arguments */
/* Add a layer of arguments into stack */
void AddLevelBufArg(void)
{
 /* Increase stack size */
 SPileArg++;
 NbArg[SPileArg]=-1;
}

/* Add an argument into the argument layer on top of TabArg stack */
void AddBufArg(long *TabLong,int NbLong)
{
 int i;

 for (i=0;i<NbLong;i++)
 {
  BuffArg[SPileArg][i+NbArg[SPileArg]+1]=TabLong[i];
 }
 NbArg[SPileArg]=NbArg[SPileArg]+NbLong;
}

/* Search for a variable name in TabVar, create it if nonexistant */
/* Returns an ID */
void AddVar(char *Name)		/* add a variable at the end of last pointed command */
{
 int i;

 /* Comparison with already existing variables */
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

 /* Variable not found: create it */
 NbVar++;

 if (NbVar==0)
 {
  TabNVar=fxcalloc(1, sizeof(long));
  TabVVar=fxcalloc(1, sizeof(long));
 }
 else
 {
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=fxstrdup(Name);
 TabVVar[NbVar]=fxcalloc(1, sizeof(char));
 TabVVar[NbVar][0]='\0';


 /* Add variable into argument buffer */
 l=(long)NbVar;
 AddBufArg(&l,1);
 return ;
}

/* Add a string constant as argument */
void AddConstStr(char *Name)
{
 /* We create a new variable and put constant in it */
 NbVar++;
 if (NbVar==0)
 {
  TabVVar=fxcalloc(1, sizeof(long));
  TabNVar=fxcalloc(1, sizeof(long));
 }
 else
 {
  TabVVar=(char**)realloc(TabVVar,sizeof(long)*(NbVar+1));
  TabNVar=(char**)realloc(TabNVar,sizeof(long)*(NbVar+1));
 }

 TabNVar[NbVar]=fxcalloc(1, sizeof(char));
 TabNVar[NbVar][0]='\0';
 TabVVar[NbVar]=fxstrdup(Name);

 /* Add the constant ID into the current arguments list */
 l=(long)NbVar;
 AddBufArg(&l,1);
}

/* Add a number constant as argument */
void AddConstNum(long num)
{

 /* We don't create a new variable */
 /* We code the number value to form an ID */
 l=num+200000;
 /* Add the constant into the current arguments list */
 AddBufArg(&l,1);
}

/* Add a functon as argument */
/* Remove function arguments from stack, */
/* concatene them, and put them into stack */
void AddFunct(int code,int NbLevelArg)
{
 int size;
 long *l;
 int i;

 /* Method: unstack BuffArg and complete the bottom level of BuffArg */
 l=Depile(NbLevelArg, &size);

 size++;
 if (size==1)
  l=fxcalloc(1, sizeof(long));
 else
 {
  l=(long*)realloc(l,sizeof(long)*(size));
  for (i=size-2;i>-1;i--)	/* Move arguments */
  {
   l[i+1]=l[i];
  }
 }
 l[0]=(long)code-150000;

 AddBufArg(l,size);
}

/* Add a test instruction to execute one block or more */
/* store the instruction, and these blocks field becomes NULL */
void AddComBloc(int TypeCond, int NbLevelArg, int NbBloc)
{
 int i;
 int OldNA;
 int CurrInstr;

 /* Add the test instruction as a command */
 AddCom(TypeCond, NbLevelArg);

 /* We then initialize both reserved fields bloc1 and bloc2 */
 CurrInstr=PileBloc[TopPileB]->NbInstr;
 /* Caution: NbArg can change if we use a function as argument */
 OldNA=PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg;

 PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg=(long*)realloc(
		PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg,sizeof(long)*(OldNA+NbBloc));
 for (i=0;i<NbBloc;i++)
 {
  PileBloc[TopPileB]->TabInstr[CurrInstr].TabArg[OldNA+i]=0;
 }
 PileBloc[TopPileB]->TabInstr[CurrInstr].NbArg=OldNA+NbBloc;
}

/* Create a new block, and stack it: it's becoming the current block */
void EmpilerBloc(void)
{
 Bloc *TmpBloc;

 TmpBloc=fxcalloc(1, sizeof(Bloc));
 TmpBloc->NbInstr=-1;
 TmpBloc->TabInstr=NULL;
 TopPileB++;
 PileBloc[TopPileB]=TmpBloc;

}

/* Unstack the script initialization block and put it into its special location */
void DepilerBloc(int IdBloc)
{
 Bloc *Bloc1;
 Instr *IfInstr;

 Bloc1=PileBloc[TopPileB];
 TopPileB--;
 IfInstr=&PileBloc[TopPileB]->TabInstr[PileBloc[TopPileB]->NbInstr];
 IfInstr->TabArg[IfInstr->NbArg-IdBloc]=(long)Bloc1;
}

/* Syntax error management */
int yyerror(char *errmsg)
{
 fprintf(stderr,"[%s] Line %d: %s\n",ScriptName,numligne,errmsg);
 return 0;
}


%}

/* Declaration of token types, all types are in union */
/* Type is the one of yyval, yyval being used in lex */
/* In bison, it's implicitly used with $1, $2... */
%union {  char *str;
          int number;
       }

/* Declaration of terminal symbols */
%token <str> STR GSTR VAR FONT
%token <number> NUMBER	/* Number to give dimensions */

%token WINDOWTITLE WINDOWLOCALETITLE WINDOWSIZE WINDOWPOSITION USEGETTEXT
%token FORECOLOR BACKCOLOR SHADCOLOR LICOLOR COLORSET
%token OBJECT INIT PERIODICTASK QUITFUNC MAIN END PROP
%token TYPE SIZE POSITION VALUE VALUEMIN VALUEMAX TITLE SWALLOWEXEC ICON FLAGS WARP WRITETOFILE LOCALETITLE
%token HIDDEN NOFOCUS NORELIEFSTRING CENTER LEFT RIGHT
%token CASE SINGLECLIC DOUBLECLIC BEG POINT
%token EXEC HIDE SHOW CHFONT CHFORECOLOR CHBACKCOLOR CHCOLORSET CHWINDOWTITLE CHWINDOWTITLEFARG KEY
%token GETVALUE GETMINVALUE GETMAXVALUE GETFORE GETBACK GETHILIGHT GETSHADOW CHVALUE CHVALUEMAX CHVALUEMIN
%token ADD DIV MULT GETTITLE GETOUTPUT STRCOPY NUMTOHEX HEXTONUM QUIT
%token LAUNCHSCRIPT GETSCRIPTFATHER SENDTOSCRIPT RECEIVFROMSCRIPT
%token GET SET SENDSIGN REMAINDEROFDIV GETTIME GETSCRIPTARG
%token GETPID SENDMSGANDGET PARSE LASTSTRING GETTEXT
%token IF THEN ELSE FOR TO DO WHILE
%token BEGF ENDF
%token EQUAL INFEQ SUPEQ INF SUP DIFF

%%
script: initvar head initbloc periodictask quitfunc object ;

/* Variables Initialization */
initvar: 			{ InitVarGlob(); }
       ;

/* Script header for default options */
head:
| head USEGETTEXT GSTR
{
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
	FGettextSetLocalePath($3);
}
| head USEGETTEXT
{
	fprintf(stderr,"UseGettext!\n");
	FGettextInit("FvwmScript", LOCALEDIR, "FvwmScript");
}
/* empty: we use default values */
| head WINDOWTITLE GSTR
{
	/* Window Title */
	scriptprop->titlewin=$3;
}
| head WINDOWLOCALETITLE GSTR
{
	/* Window Title */
	scriptprop->titlewin=(char *)FGettext($3);
}
| head ICON STR
{
	scriptprop->icon=$3;
}
| head WINDOWPOSITION NUMBER NUMBER
{
	/* Window Position and Size */
	scriptprop->x=$3;
	scriptprop->y=$4;
}
| head WINDOWSIZE NUMBER NUMBER
{
	/* Window Width and Height */
	scriptprop->width=$3;
	scriptprop->height=$4;
}
| head BACKCOLOR GSTR
{
	/* Background Color */
	scriptprop->backcolor=$3;
	scriptprop->colorset = -1;
}
| head FORECOLOR GSTR
{
	/* Foreground Color */
	scriptprop->forecolor=$3;
	scriptprop->colorset = -1;
}
| head SHADCOLOR GSTR
{
	/* Shaded Color */
	scriptprop->shadcolor=$3;
	scriptprop->colorset = -1;
}
| head LICOLOR GSTR
{
	/* Highlighted Color */
	scriptprop->hilicolor=$3;
	scriptprop->colorset = -1;
}
| head COLORSET NUMBER
{
	scriptprop->colorset = $3;
	AllocColorset($3);
}
| head FONT
{
	scriptprop->font=$2;
}

/* Script initialization block */
initbloc:		/* case where there's no script initialization block */
	| INIT creerbloc BEG instr END {
				 scriptprop->initbloc=PileBloc[TopPileB];
				 TopPileB--;
				}

periodictask:		/* case where there's no periodic task */
	    | PERIODICTASK creerbloc BEG instr END {
				 scriptprop->periodictasks=PileBloc[TopPileB];
				 TopPileB--;
				}

quitfunc:		/* case where there are no QuitFunc */
	    | QUITFUNC creerbloc BEG instr END {
				 scriptprop->quitfunc=PileBloc[TopPileB];
				 TopPileB--;
				}

	    ;


/* Object Desciption */
object :			/* Empty */
    | object OBJECT id PROP init verify mainloop
    ;

id: NUMBER			{ nbobj++;
				  if (nbobj>1000)
				  { yyerror("Too many items\n");
				    exit(1);}
				  if (($1<1)||($1>1000))
				  { yyerror("Choose item id between 1 and 1000\n");
				    exit(1);}
				  if (TabIdObj[$1]!=-1)
				  { i=$1; fprintf(stderr,"Line %d: item id %d already used:\n",numligne,$1);
				    exit(1);}
			          TabIdObj[$1]=nbobj;
				  (*tabobj)[nbobj].id=$1;
				  (*tabobj)[nbobj].colorset = -1;
				}
  ;

init:				/* vide */
    | init TYPE STR		{
				 (*tabobj)[nbobj].type=$3;
				 HasType=1;
				}
    | init SIZE NUMBER NUMBER	{
				 (*tabobj)[nbobj].width=$3;
				 (*tabobj)[nbobj].height=$4;
				}
    | init POSITION NUMBER NUMBER {
				 (*tabobj)[nbobj].x=$3;
				 (*tabobj)[nbobj].y=$4;
				 HasPosition=1;
				}
    | init VALUE NUMBER		{
				 (*tabobj)[nbobj].value=$3;
				}
    | init VALUEMIN NUMBER		{
				 (*tabobj)[nbobj].value2=$3;
				}
    | init VALUEMAX NUMBER		{
				 (*tabobj)[nbobj].value3=$3;
				}
    | init TITLE GSTR		{
				 (*tabobj)[nbobj].title= $3;
				}
    | init LOCALETITLE GSTR     {
				 (*tabobj)[nbobj].title= FGettextCopy($3);
				}
    | init SWALLOWEXEC GSTR	{
				 (*tabobj)[nbobj].swallow=$3;
				}
    | init ICON STR		{
				 (*tabobj)[nbobj].icon=$3;
				}
    | init BACKCOLOR GSTR	{
				 (*tabobj)[nbobj].backcolor=$3;
				 (*tabobj)[nbobj].colorset = -1;
				}
    | init FORECOLOR GSTR	{
				 (*tabobj)[nbobj].forecolor=$3;
				 (*tabobj)[nbobj].colorset = -1;
				}
    | init SHADCOLOR GSTR	{
				 (*tabobj)[nbobj].shadcolor=$3;
				 (*tabobj)[nbobj].colorset = -1;
				}
    | init LICOLOR GSTR		{
				 (*tabobj)[nbobj].hilicolor=$3;
				 (*tabobj)[nbobj].colorset = -1;
				}
    | init COLORSET NUMBER	{
				 (*tabobj)[nbobj].colorset = $3;
				 AllocColorset($3);
				}
    | init FONT			{
				 (*tabobj)[nbobj].font=$2;
				}
    | init FLAGS flags
    ;
flags:
     | flags HIDDEN		{
				 (*tabobj)[nbobj].flags[0]=True;
				}
     | flags NORELIEFSTRING	{
				 (*tabobj)[nbobj].flags[1]=True;
				}
     | flags NOFOCUS		{
				 (*tabobj)[nbobj].flags[2]=True;
				}
     | flags CENTER		{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_CENTER;
				}
     | flags LEFT		{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_LEFT;
				}
     | flags RIGHT		{
				 (*tabobj)[nbobj].flags[3]=TEXT_POS_RIGHT;
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
    | case clic POINT bloc
    | case number POINT bloc
    ;

clic :  SINGLECLIC	{ InitCase(-1); }
     |  DOUBLECLIC	{ InitCase(-2); }
     ;

number :  NUMBER		{ InitCase($1); }
       ;

bloc: BEG instr END
    ;


/* instructions */
instr:
    | instr EXEC exec
    | instr WARP warp
    | instr WRITETOFILE writetofile
    | instr HIDE hide
    | instr SHOW show
    | instr CHVALUE chvalue
    | instr CHVALUEMAX chvaluemax
    | instr CHVALUEMIN chvaluemin
    | instr CHWINDOWTITLE addlbuff gstrarg {AddCom(27,1);}
    | instr CHWINDOWTITLEFARG numarg {AddCom(28,1);}
    | instr POSITION position
    | instr SIZE size
    | instr TITLE title
    | instr LOCALETITLE localetitle
    | instr ICON icon
    | instr CHFONT font
    | instr CHFORECOLOR chforecolor
    | instr CHBACKCOLOR chbackcolor
    | instr CHCOLORSET chcolorset
    | instr SET set
    | instr SENDSIGN sendsign
    | instr QUIT quit
    | instr SENDTOSCRIPT sendtoscript
    | instr IF ifthenelse
    | instr FOR loop
    | instr WHILE while
    | instr KEY key
    ;

/* One single instruction */
oneinstr: EXEC exec
    	| WARP warp
	| WRITETOFILE writetofile
	| HIDE hide
	| SHOW show
	| CHVALUE chvalue
	| CHVALUEMAX chvaluemax
	| CHVALUEMIN chvaluemin
	| CHWINDOWTITLE addlbuff gstrarg {AddCom(27,1);}
	| CHWINDOWTITLEFARG numarg {AddCom(28,1);}
	| POSITION position
	| SIZE size
	| TITLE title
	| LOCALETITLE localetitle
	| ICON icon
	| CHFONT font
	| CHFORECOLOR chforecolor
	| CHBACKCOLOR chbackcolor
	| CHCOLORSET chcolorset
	| SET set
	| SENDSIGN sendsign
	| QUIT quit
	| SENDTOSCRIPT sendtoscript
	| FOR loop
	| WHILE while
    	| KEY key
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
font: addlbuff numarg addlbuff args { AddCom(9,2);}
    ;
chforecolor: addlbuff numarg addlbuff gstrarg	{ AddCom(10,2);}
           ;
chbackcolor: addlbuff numarg addlbuff gstrarg	{ AddCom(19,2);}
           ;
chcolorset : addlbuff numarg addlbuff numarg	{ AddCom(24,2);}
           ;
set: addlbuff vararg GET addlbuff args	{ AddCom(11,2);}
   ;
sendsign: addlbuff numarg addlbuff numarg { AddCom(12,2);}
	;
quit: 					{ AddCom(13,0);}
    ;
warp: addlbuff numarg			{ AddCom(17,1);}
    ;
sendtoscript: addlbuff numarg addlbuff args 	{ AddCom(23,2);}
	    ;
writetofile: addlbuff strarg addlbuff args	{ AddCom(18,2);}
	   ;
key: addlbuff strarg addlbuff strarg addlbuff numarg addlbuff numarg addlbuff args { AddCom(25,5);}
	   ;
localetitle: addlbuff numarg addlbuff gstrarg	{ AddCom(26,2);}
     ;
ifthenelse: headif creerbloc bloc1 else
          ;
loop: headloop creerbloc bloc2
    ;
while: headwhile creerbloc bloc2
    ;

/* Conditional loop: compare whatever argument type */
headif: addlbuff arg addlbuff compare addlbuff arg THEN 	{ AddComBloc(14,3,2); }
      ;
else: 				/* else is optional */
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

/* Loop on a variable */
headloop: addlbuff vararg GET addlbuff arg TO addlbuff arg DO	{ AddComBloc(15,3,1); }
        ;

/* While conditional loop */
headwhile: addlbuff arg addlbuff compare addlbuff arg DO	{ AddComBloc(16,3,1); }
        ;

/* Command Argument */
/* Elementary Argument */
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
	| GETFORE numarg { AddFunct(16,1); }
	| GETBACK numarg { AddFunct(17,1); }
	| GETHILIGHT numarg { AddFunct(18,1); }
	| GETSHADOW numarg { AddFunct(19,1); }
	| GETMINVALUE numarg { AddFunct(20,1); }
	| GETMAXVALUE numarg { AddFunct(21,1); }
	| GETPID { AddFunct(22,1); }
	| SENDMSGANDGET gstrarg gstrarg numarg { AddFunct(23,1); }
	| PARSE gstrarg numarg { AddFunct(24,1); }
	| LASTSTRING { AddFunct(25,1); }
	| GETTEXT gstrarg { AddFunct(26,1); }
	;


/* Unique argument of several types */
args	:			{ }
	| singleclic2 args
	| doubleclic2 args
	| var args
	| gstr args
	| str args
	| num args
	| BEGF addlbuff function ENDF args
	;

/* Unique argument of whatever type */
arg	: var
	| singleclic2
	| doubleclic2
	| gstr
	| str
	| num
	| BEGF addlbuff function ENDF
	;

/* Unique argument of number type */
numarg	: singleclic2
	| doubleclic2
	| num
	| var
	| BEGF addlbuff function ENDF
	;

/* Unique argument of str type */
strarg	: var
	| str
	| BEGF addlbuff function ENDF
	;

/* Unique argument of gstr type */
gstrarg	: var
	| gstr
	| BEGF addlbuff function ENDF
	;

/* Unique argument of variable type, no function */
vararg	: var
	;

/* comparison element between 2 numbers */
compare	: INF			 { l=1-250000; AddBufArg(&l,1); }
	| INFEQ			 { l=2-250000; AddBufArg(&l,1); }
	| EQUAL			 { l=3-250000; AddBufArg(&l,1); }
	| SUPEQ			 { l=4-250000; AddBufArg(&l,1); }
	| SUP			 { l=5-250000; AddBufArg(&l,1); }
	| DIFF			 { l=6-250000; AddBufArg(&l,1); }
	;

%%
