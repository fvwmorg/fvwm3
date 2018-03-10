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
# include "config.h"
# include "script.h"
# include "types.h"
# include "libs/Parse.h"

/* Suppress definition of yyunput() and yyinput(), as we do not use it. */
#define YY_NO_UNPUT
#define YY_NO_INPUT

int numligne=1;
%}


bl		[ \t]*			
nl		\n
commentaire	#.*
nb		[-+]?[0-9]*
str		[^ = \t \n \( \) \< \> \} \{]+
gstr		\{[^\{\}\n]*[\}\n]
inf		<
infeq		<=
equal		==
supeq		>=
sup		>
diff		<>
get		=

%%
{bl}		;
{commentaire}	;

^{bl}+UseGettext	       { return USEGETTEXT; }
^{bl}+WindowTitle	       { return WINDOWTITLE; }
^{bl}+ChangeWindowTitle	       { return CHWINDOWTITLE; }
^{bl}+ChangeWindowTitleFromArg { return CHWINDOWTITLEFARG; }
^{bl}+WindowLocaleTitle	       { return WINDOWLOCALETITLE; }
^{bl}+WindowSize	       { return WINDOWSIZE; }
^{bl}+WindowPosition	       { return WINDOWPOSITION; }
^{bl}+ForeColor		       { return FORECOLOR; }
^{bl}+BackColor		       { return BACKCOLOR; }
^{bl}+ShadowColor	       { return SHADCOLOR; }
^{bl}+HilightColor	       { return LICOLOR; }
^{bl}+Colorset		       { return COLORSET; }
^{bl}+Widget		       { return OBJECT; }
^{bl}+Property		       { return PROP; }
^{bl}+Main		       { return MAIN; }
^{bl}+End		       { return END; }
^{bl}+Font.* {
  char *tmp,*rest;

  rest = GetNextToken(yytext, &tmp); /* skip Font */
  GetNextToken(rest, &yylval.str); /* get the font */
  if (tmp != NULL)
    free(tmp);
  return FONT;
}

^{bl}+Type		{ return TYPE; }
^{bl}+Size		{ return SIZE; }
^{bl}+Position		{ return POSITION; }
^{bl}+Value		{ return VALUE; }
^{bl}+MinValue		{ return VALUEMIN; }
^{bl}+MaxValue		{ return VALUEMAX; }
^{bl}+Title		{ return TITLE; }
^{bl}+LocaleTitle	{ return LOCALETITLE; }
^{bl}+SwallowExec	{ return SWALLOWEXEC; }
^{bl}+Icon		{ return ICON; }
^{bl}+Flags		{ return FLAGS; }
Hidden			{ return HIDDEN; }
NoFocus			{ return NOFOCUS; }
NoReliefString		{ return NORELIEFSTRING; }
Center			{ return CENTER; }
Left			{ return LEFT; }
Right			{ return RIGHT; }

^{bl}+Do		{ return EXEC; }
^{bl}+HideWidget	{ return HIDE; }
^{bl}+ShowWidget	{ return SHOW; }
^{bl}+ChangeValue	{ return CHVALUE; }
^{bl}+ChangeMaxValue	{ return CHVALUEMAX; }
^{bl}+ChangeMinValue	{ return CHVALUEMIN; }
GetTitle		{ return GETTITLE; }
GetValue		{ return GETVALUE; }
GetMinValue		{ return GETMINVALUE; }
GetMaxValue		{ return GETMAXVALUE; }
GetFore			{ return GETFORE; }
GetBack			{ return GETBACK; }
GetHilight		{ return GETHILIGHT; }
GetShadow		{ return GETSHADOW; }
GetOutput		{ return GETOUTPUT; }
NumToHex		{ return NUMTOHEX; }
HexToNum		{ return HEXTONUM; }
Add			{ return ADD;}
Mult			{ return MULT;}
Div			{ return DIV;}
StrCopy			{ return STRCOPY; }
LaunchScript		{ return LAUNCHSCRIPT; }
GetScriptFather		{ return GETSCRIPTFATHER; }
ReceivFromScript	{ return RECEIVFROMSCRIPT; }
RemainderOfDiv		{ return REMAINDEROFDIV; }
GetTime			{ return GETTIME; }
GetScriptArgument	{ return GETSCRIPTARG; }
GetPid			{ return GETPID; }
SendMsgAndGet		{ return SENDMSGANDGET; }
Parse			{ return PARSE; }
LastString		{ return LASTSTRING; }
Gettext                 { return GETTEXT; }
^{bl}+ChangePosition	{ return POSITION; }
^{bl}+ChangeSize	{ return SIZE; }
^{bl}+ChangeTitle	{ return TITLE; }
^{bl}+ChangeLocaleTitle	{ return LOCALETITLE; }
^{bl}+ChangeIcon	{ return ICON; }
^{bl}+ChangeFont	{ return CHFONT; }
^{bl}+ChangeForeColor	{ return CHFORECOLOR; }
^{bl}+ChangeBackColor	{ return CHBACKCOLOR; }
^{bl}+ChangeColorset	{ return CHCOLORSET; }
^{bl}+Set		{ return SET; }
^{bl}+SendSignal	{ return SENDSIGN; }
^{bl}+Quit		{ return QUIT; }
^{bl}+Init		{ return INIT; }
^{bl}+PeriodicTasks	{ return PERIODICTASK; }
^{bl}+QuitFunc		{ return QUITFUNC; }
^{bl}+For		{ return FOR; }
^{bl}+WarpPointer	{ return WARP; }
^{bl}+WriteToFile	{ return WRITETOFILE; }
^{bl}+SendToScript	{ return SENDTOSCRIPT; }
^{bl}+SingleClic	{ return SINGLECLIC; }
^{bl}+DoubleClic	{ return DOUBLECLIC; }
^{bl}+Key		{ return KEY; }
To			{ return TO; }
Do			{ return DO; }
^{bl}+If		{ return IF; }
Then			{ return THEN; }
Else			{ return ELSE; }

{inf}			{ return INF; }
{infeq}			{ return INFEQ; }
{supeq}			{ return SUPEQ; }
{sup}			{ return SUP; }
{diff}			{ return DIFF; }
{equal}			{ return EQUAL; }
{get}			{ return GET; }

^{bl}+While		{ return WHILE; }
^{bl}+Case+[ \t]+message+[ \t]of { return CASE; }
^{bl}+Begin		{ return BEG; }
:			{ return POINT; }
[\(]			{ return BEGF; }
[\)]			{ return ENDF; }


{nb}		{ yylval.number=atoi(yytext);
                  return NUMBER;
                }

[\$]+{str}	{
		  yylval.str=(char*) safestrdup(yytext+1);
		  yylval.str[yyleng-1]='\0';
                  return VAR;
		}

{gstr}		{ /* Str entre "" */
		  yylval.str=(char*) safestrdup(yytext+1);
		  yylval.str[yyleng-2]='\0';
                  return GSTR;
		}

{str}		{ yylval.str=(char*) safestrdup(yytext);
		  yylval.str[yyleng]='\0';
                  return STR;
		}

{nl}		{ numligne++; }
