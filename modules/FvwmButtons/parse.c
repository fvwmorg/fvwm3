/*
   FvwmButtons v2.0.41-plural-Z-alpha, copyright 1996, Jarl Totland

 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.

*/

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
/*
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
*/

#include "FvwmButtons.h"
#include "button.h"

extern int w,h,x,y,xneg,yneg; /* used in ParseConfigLine */
extern char *config_file;

/* contains the character that terminated the last string from seekright */
static char terminator = '\0';

/* ----------------------------------- macros ------------------------------ */

char *trimleft(char *s)
{
  while (s && isspace(*s))
    s++;
  return s;
}

/**
*** seekright()
***
*** split off the first continous section of the string into a new allocated
*** string and move the old pointer forward. Accepts and strips quoting with
*** '," or `, and the current quote q can be escaped inside the string with \q.
**/
char *seekright(char **s)
{
  char *token = NULL;
  char *line = *s;

  line = DoGetNextToken(line, &token, NULL, "),", &terminator);
  if (*s != NULL && line == NULL)
    line = strchr(*s, '\0');
  *s = line;

  return token;
}

/**
*** ParseBack()
*** Parses the options possible to Back
**/
int ParseBack(char **ss)
{
  char *opts[]={"icon",NULL};
  char *t,*s=*ss;
  int r=0;

  while(*s && *s!=')')
    {
      s = trimleft(s);
      if(*s==',')
	s++;
      else switch(GetTokenIndex(s,opts,-1,&s))
	{
	case 0: /* Icon */
	  r=1;
	  fprintf(stderr,"%s: Back(Icon) not supported yet\n",MyName);
	  break;
	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal back option \"%s\"\n",MyName,(t)?t:"");
	  if (t)
	    free(t);
	}
    }
  if(*s) s++;
  *ss=s;
  return r;
}

/**
*** ParseBoxSize()
*** Parses the options possible to BoxSize
**/
void ParseBoxSize(char **ss, unsigned long *flags)
{
  char *opts[]={"dumb","fixed","smart",NULL};
  char *s = *ss;
  int m;

  if (!s)
    return;
  *ss = GetNextTokenIndex(*ss, opts, 0, &m);
  switch(m)
    {
    case 0:
      *flags &= ~(b_SizeFixed|b_SizeSmart);
      break;
    case 1:
      *flags |= b_SizeFixed;
      *flags &= ~b_SizeSmart;
      break;
    case 2:
      *flags |= b_SizeSmart;
      *flags &= ~b_SizeFixed;
      break;
    default:
      *flags &= ~(b_SizeFixed|b_SizeSmart);
      fprintf(stderr,"%s: Illegal boxsize option \"%s\"\n",MyName, s);
      break;
    }
  return;
}

/**
*** ParseTitle()
*** Parses the options possible to Title
**/
void ParseTitle(char **ss, byte *flags, byte *mask)
{
  char *titleopts[]={"left","right","center","side",NULL};
  char *t,*s=*ss;

  while(*s && *s!=')')
    {
      s = trimleft(s);
      if(*s==',')
	s++;
      else switch(GetTokenIndex(s,titleopts,-1,&s))
	{
	case 0: /* Left */
	  *flags&=~b_TitleHoriz;
	  *mask|=b_TitleHoriz;
	  break;
	case 1: /* Right */
	  *flags&=~b_TitleHoriz;
	  *flags|=2;
	  *mask|=b_TitleHoriz;
	  break;
	case 2: /* Center */
	  *flags&=~b_TitleHoriz;
	  *flags|=1;
	  *mask|=b_TitleHoriz;
	  break;
	case 3: /* Side */
	  *flags|=b_Horizontal;
	  *mask|=b_Horizontal;
	  break;
	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal title option \"%s\"\n",MyName,(t)?t:"");
	  if (t)
	    free(t);
	}
    }
  if(*s) s++;
  *ss=s;
}

/**
*** ParseSwallow()
*** Parses the options possible to Swallow
**/
void ParseSwallow(char **ss,byte *flags,byte *mask)
{
  char *swallowopts[]={"nohints","hints","nokill","kill","noclose","close",
			 "respawn","norespawn","useold",
			 "noold","usetitle","notitle",NULL};
  char *t,*s=*ss;

  while(*s && *s!=')')
    {
      s = trimleft(s);
      if(*s==',')
	s++;
      else switch(GetTokenIndex(s,swallowopts,-1,&s))
	{
	case 0: /* NoHints */
	  *flags|=b_NoHints;
	  *mask|=b_NoHints;
	  break;
	case 1: /* Hints */
	  *flags&=~b_NoHints;
	  *mask|=b_NoHints;
	  break;
	case 2: /* NoKill */
	  *flags&=~b_Kill;
	  *mask|=b_Kill;
	  break;
	case 3: /* Kill */
	  *flags|=b_Kill;
	  *mask|=b_Kill;
	  break;
	case 4: /* NoClose */
	  *flags|=b_NoClose;
	  *mask|=b_NoClose;
	  break;
	case 5: /* Close */
	  *flags&=~b_NoClose;
	  *mask|=b_NoClose;
	  break;
	case 6: /* Respawn */
	  *flags|=b_Respawn;
	  *mask|=b_Respawn;
	  break;
	case 7: /* NoRespawn */
	  *flags&=~b_Respawn;
	  *mask|=b_Respawn;
	  break;
	case 8: /* UseOld */
	  *flags|=b_UseOld;
	  *mask|=b_UseOld;
	  break;
	case 9: /* NoOld */
	  *flags&=~b_UseOld;
	  *mask|=b_UseOld;
	  break;
	case 10: /* UseTitle */
	  *flags|=b_UseTitle;
	  *mask|=b_UseTitle;
	  break;
	case 11: /* NoTitle */
	  *flags&=~b_UseTitle;
	  *mask|=b_UseTitle;
	  break;
	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal swallow option \"%s\"\n",MyName,
		  (t)?t:"");
	  if (t)
	    free(t);
	}
    }
  if(*s) s++;
  *ss=s;
}

/**
*** ParseContainer()
*** Parses the options possible to Container
**/
void ParseContainer(char **ss,button_info *b)
{
  char *conts[]={"columns","rows","font","frame","back","fore",
		   "padding","title","swallow","nosize","size",
		   "boxsize",NULL};
  char *t,*o,*s=*ss;
  int i,j;

  while(*s && *s!=')')
    {
      s = trimleft(s);
      if(*s==',')
	s++;
      else switch(GetTokenIndex(s,conts,-1,&s))
	{
	case 0: /* Columns */
	  b->c->num_columns=max(1,strtol(s,&t,10));
	  s=t;
	  break;
	case 1: /* Rows */
	  b->c->num_rows=max(1,strtol(s,&t,10));
	  s=t;
	  break;
	case 2: /* Font */
	  if (b->c->font_string) free(b->c->font_string);
	  b->c->font_string=seekright(&s);
	  if(b->c->font_string)
	    b->c->flags|=b_Font;
	  else
	    b->c->flags&=~b_Font;
	  break;
	case 3: /* Frame */
	  b->c->framew=strtol(s,&t,10);
	  b->c->flags|=b_Frame;
	  s=t;
	  break;
	case 4: /* Back */
	  s = trimleft(s);
	  if(*s=='(' && s++)
	    if(ParseBack(&s))
	      b->c->flags|=b_IconBack;
	  if (b->c->back) free(b->c->back);
	  b->c->back=seekright(&s);
	  if(b->c->back)
	    b->c->flags|=b_Back;
	  else
	    b->c->flags&=~(b_IconBack|b_Back);
	  break;
	case 5: /* Fore */
	  if (b->c->fore) free(b->c->fore);
	  b->c->fore=seekright(&s);
	  if(b->c->fore)
	    b->c->flags|=b_Fore;
	  else
	    b->c->flags&=~b_Fore;
	  break;
	case 6: /* Padding */
	  i=strtol(s,&t,10);
	  if(t>s)
	    {
	      b->c->xpad=b->c->ypad=i;
	      s=t;
	      i=strtol(s,&t,10);
	      if(t>s)
		{
		  b->c->ypad=i;
		  s=t;
		}
	      b->c->flags|=b_Padding;
	    }
	  else
	    fprintf(stderr,"%s: Illegal padding argument\n",MyName);
	  break;
	case 7: /* Title - flags */
	  s = trimleft(s);
	  if(*s=='(' && s++)
	    {
	      b->c->justify=0;
	      b->c->justify_mask=0;
	      ParseTitle(&s,&b->c->justify,&b->c->justify_mask);
	      if(b->c->justify_mask)
		b->c->flags|=b_Justify;
	    }
	  else
	    {
	      char *temp;
	      fprintf(stderr,"%s: Illegal title in container options\n",
		      MyName);
	      temp = seekright(&s);
	      if (temp)
		free(temp);
	    }
	  break;
	case 8: /* Swallow - flags */
	  s = trimleft(s);
	  if(*s=='(' && s++)
	    {
	      b->c->swallow=0;
	      b->c->swallow_mask=0;
	      ParseSwallow(&s,&b->c->swallow,&b->c->swallow_mask);
	      if(b->c->swallow_mask)
		b->c->flags|=b_Swallow;
	    }
	  else
	    {
	      char *temp;
	      fprintf(stderr,"%s: Illegal swallow in container options\n",
		      MyName);
	      temp = seekright(&s);
	      if (temp)
		free(temp);
	    }
	  break;
	case 9: /* NoSize */
	  b->c->flags|=b_Size;
	  b->c->minx=b->c->miny=0;
	  break;

	case 10: /* Size */
	  i=strtol(s,&t,10);
	  j=strtol(t,&o,10);
	  if(t>s && o>t)
	    {
	      b->c->minx=i;
	      b->c->miny=j;
	      b->c->flags|=b_Size;
	      s=o;
	    }
	  else
	    fprintf(stderr,"%s: Illegal size arguments\n",MyName);
	  break;
	case 11: /* BoxSize */
	  ParseBoxSize(&s, &b->c->flags);
	  break;

	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal container option \"%s\"\n",MyName,
		  (t)?t:"");
	  if (t)
	    free(t);
	}
    }
  if(*s) s++;
  *ss=s;
}

/**
*** match_string()
***
*** parse a buttonconfig string.
*** *FvwmButtons(option[ options]) title iconname command
**/
/*#define DEBUG_PARSER*/
void match_string(button_info **uberb,char *s)
{
  button_info *b,*ub=*uberb;
  int i,j;
  char *t,*o;
  b=alloc_button(ub,(ub->c->num_buttons)++);
  s = trimleft(s);

  if(*s=='(' && s++)
    {
      char *opts[]={"back","fore","font","title","icon","frame","padding",
		    "swallow","action","container","end","nosize","size",
		    "panel", "left", "right", "center",
		    NULL};
      s = trimleft(s);
      while(*s && *s!=')')
	{
	  if((*s>='0' && *s<='9') || *s=='+' || *s=='-')
	    {
	      char *geom;
	      int x,y,w,h,flags;
	      geom=seekright(&s);
	      if (geom)
		{
		  flags=XParseGeometry(geom, &x, &y, &w, &h);
		  if(flags&WidthValue)
		    b->BWidth=w;
		  if(flags&HeightValue)
		    b->BHeight=h;
		  if(flags&XValue)
		    {
		      b->BPosX=x;
		      b->flags|=b_PosFixed;
		    }
		  if(flags&YValue)
		    {
		      b->BPosY=y;
		      b->flags|=b_PosFixed;
		    }
		  if(flags&XNegative)
		    b->BPosX=-1-x;
		  if(flags&YNegative)
		    b->BPosY=-1-y;
		  free(geom);
		}
	      s = trimleft(s);
	      continue;
	    }
	  if(*s==',' && s++)
	    s = trimleft(s);
	  switch(GetTokenIndex(s,opts,-1,&s))
	    {
	    case 0: /* Back */
	      s = trimleft(s);
	      if(*s=='(' && s++)
		if(ParseBack(&s))
		  b->flags|=b_IconBack;
	      if(b->flags&b_Back && b->back) free(b->back);
	      b->back=seekright(&s);
	      if(b->back)
		b->flags|=b_Back;
	      else
		b->flags&=~(b_IconBack|b_Back);
	      break;

	    case 1: /* Fore */
	      if(b->flags&b_Fore && b->fore) free(b->fore);
	      b->fore=seekright(&s);
	      if(b->fore)
		b->flags|=b_Fore;
	      else
		b->flags&=~b_Fore;
	      break;

	    case 2: /* Font */
	      if(b->flags&b_Font && b->font_string) free(b->font_string);
	      b->font_string=seekright(&s);
	      if(b->font_string)
		b->flags|=b_Font;
	      else
		b->flags&=~b_Font;
	      break;

	      /* --------------------------- Title ------------------------- */

	    case 3: /* Title */
	      s = trimleft(s);
	      if(*s=='(' && s++)
		{
		  b->justify=0;
		  b->justify_mask=0;
		  ParseTitle(&s,&b->justify,&b->justify_mask);
		  if(b->justify_mask)
		    b->flags|=b_Justify;
		}
	      t=seekright(&s);
	      if(t && *t && (t[0]!='-' || t[1]!=0))
		{
		  if (b->title)
		    free(b->title);
		  b->title=t;
#ifdef DEBUG_PARSER
		  fprintf(stderr,"PARSE: Title \"%s\"\n",b->title);
#endif
		  b->flags|=b_Title;
		}
	      else
		{
		  fprintf(stderr,"%s: Missing title argument\n",MyName);
		  if(t)free(t);
		}
	      break;

	      /* ---------------------------- icon ------------------------- */

	    case 4: /* Icon */
	      t=seekright(&s);
	      if(t && *t && (t[0] != '-' || t[1] != 0))
		{
		  if (b->icon_file)
		    free(b->icon_file);
		  b->icon_file=t;
		  b->IconWin=None;
		  b->flags|=b_Icon;
		}
	      else
		{
		  fprintf(stderr,"%s: Missing icon argument\n",MyName);
		  if(t)free(t);
		}
	      break;

	      /* --------------------------- frame ------------------------- */

	    case 5: /* Frame */
	      i=strtol(s,&t,10);
	      if(t>s)
		{
		  b->flags|=b_Frame;
		  b->framew=i;
		  s=t;
		}
	      else
		fprintf(stderr,"%s: Illegal frame argument\n",MyName);
	      break;

	      /* -------------------------- padding ------------------------ */

	    case 6: /* Padding */
	      i=strtol(s,&t,10);
	      if(t>s)
		{
		  b->xpad=b->ypad=i;
		  b->flags |= b_Padding;
		  s=t;
		  i=strtol(s,&t,10);
		  if(t>s)
		    {
		      b->ypad=i;
		      s=t;
		    }
		}
	      else
		fprintf(stderr,"%s: Illegal padding argument\n",MyName);
	      break;

	      /* -------------------------- swallow ------------------------ */

	    case 7: /* Swallow */
	      s = trimleft(s);
	      b->swallow=0;
	      b->swallow_mask=0;
	      if(*s=='(' && s++)
		ParseSwallow(&s,&b->swallow,&b->swallow_mask);
	      t=seekright(&s);
	      o=seekright(&s);
	      if(t)
		{
		  if (b->hangon)
		    free(b->hangon);
		  b->hangon=t;
		  b->flags|=b_Hangon;
		  b->flags|=b_Swallow;
		  b->swallow|=1;
		  if(!(b->swallow&b_NoHints))
		    b->hints=(XSizeHints*)mymalloc(sizeof(XSizeHints));
		  if(o)
		    {
		      if(!(buttonSwallow(b)&b_UseOld))
			SendText(fd,o,0);
		      if (b->spawn)
			free(b->spawn);
		      b->spawn=o;  /* Might be needed if respawning sometime */
		    }
      		}
	      else
		{
		  fprintf(stderr,"%s: Missing swallow argument\n",MyName);
		  if(t)free(t);
		  if(o)free(o);
		}
	      break;

	      /* --------------------------- action ------------------------ */

	    case 8: /* Action */
	      s = trimleft(s);
	      i=0;
	      if(*s=='(')
		{
		  s++;
		  if(strncasecmp(s,"mouse",5)!=0)
		    {
		      fprintf(stderr,"%s: Couldn't parse action\n",MyName);
		    }
		  s+=5;
		  i=strtol(s,&t,10);
		  s=t;
		  while(*s && *s!=')')
		    s++;
		  if(*s==')')s++;
		}
	      s = GetQuotedString(s, &t, ",)", NULL, NULL, NULL);
	      if(t)
		{
		  AddButtonAction(b,i,t);
		  free(t);
		}
	      else
		fprintf(stderr,"%s: Missing action argument\n",MyName);
	      break;

	      /* -------------------------- container ---------------------- */

	    case 9: /* Container */
	      b->flags&=b_Frame|b_Back|b_Fore|b_Padding|b_Action;
	      MakeContainer(b);
	      *uberb=b;
	      s = trimleft(s);
	      if(*s=='(' && s++)
		ParseContainer(&s,b);
	      break;

	    case 10: /* End */
	      *uberb=ub->parent;
	      ub->c->buttons[--(ub->c->num_buttons)]=NULL;
	      if(!ub->parent)
		{
		  fprintf(stderr,"%s: Unmatched END in config file\n",MyName);
		  exit(1);
		}
	      break;

	    case 11: /* NoSize */
	      b->flags|=b_Size;
	      b->minx=b->miny=0;
	      break;

	    case 12: /* Size */
	      i=strtol(s,&t,10);
	      j=strtol(t,&o,10);
	      if(t>s && o>t)
		{
		  b->minx=i;
		  b->miny=j;
		  b->flags|=b_Size;
		  s=o;
		}
	      else
		fprintf(stderr,"%s: Illegal size arguments\n",MyName);
	      break;

	      /* --------------------------- panel ------------------------ */

	    case 13: /* Panel */
	      s = trimleft(s);
	      if(*s=='(')
	      {
		s++;
		t = seekright(&s);
		if (terminator != ')')
		  while(*s && *s!=')')
		    s++;
		if(*s==')')
		  s++;
		if      (strncasecmp(t,"right",5)==0)
		  t = "panel-r";
		else if (strncasecmp(t,"left" ,4)==0)
		  t = "panel-l";
		else if (strncasecmp(t,"down" ,4)==0)
		  t = "panel-d";
		else if (strncasecmp(t,"geometry",8)==0)
		  t = "panel-g";
		else
		  t = "panel-u";
	      }
	      else
		t = "panel-u";
	      AddButtonAction(b, 0, t);

	      b->IconWin = None;
	      t = seekright(&s);
	      b->hangon = (t)? t : strdup("");  /* which panel to popup */
	      break;

	    case 14: /* Left */
	      b->flags |= b_Left;
	      b->flags &= ~b_Right;
	      break;

	    case 15: /* Right */
	      b->flags |= b_Right;
	      b->flags &= ~b_Left;
	      break;

	    case 16: /* Center */
	      b->flags &= ~(b_Right|b_Left);
	      break;

	    default:
	      t=seekright(&s);
	      fprintf(stderr,"%s: Illegal button option \"%s\"\n",MyName,
		      (t)?t:"");
	      if (t)
		free(t);
	      break;
	    }
	  s = trimleft(s);
	}
      if (s && *s)
	{
	  s++;
	  s = trimleft(s);
	}
    }

  /* get title and iconname */
  if(!(b->flags&b_Title))
    {
      b->title=seekright(&s);
      if(b->title && *b->title && ((b->title)[0]!='-'||(b->title)[1]!=0))
	b->flags |= b_Title;
      else
	if(b->title)free(b->title);
    }
  else
    {
      char *temp;
      temp = seekright(&s);
      if (temp)
	free(temp);
    }

  if(!(b->flags&b_Icon))
    {
      b->icon_file=seekright(&s);
      if(b->icon_file && b->icon_file &&
	 ((b->icon_file)[0]!='-'||(b->icon_file)[1]!=0))
	{
	  b->flags|=b_Icon;
	  b->IconWin=None;
	}
      else
	if(b->icon_file)free(b->icon_file);
    }
  else
    {
      char *temp;
      temp = seekright(&s);
      if (temp)
	free(temp);
    }

  s = trimleft(s);

  /* Swallow hangon command */
  if(strncasecmp(s,"swallow",7)==0)
    {
      if(b->flags&b_Swallow)
	{
	  fprintf(stderr,"%s: Illegal with both old and new swallow!\n",
		  MyName);
	  exit(1);
	}
      s+=7;
      /*
       * Swallow old 'swallowmodule' command
       */
      if (strncasecmp(s,"module",6)==0)
      {
        s+=6;
      }
      if (b->hangon)
	free(b->hangon);
      b->hangon=seekright(&s);
      if (!b->hangon)
	b->hangon = strdup("");
      b->flags|=(b_Swallow|b_Hangon);
      b->swallow|=1;
      s = trimleft(s);
      if(!(b->swallow&b_NoHints))
	b->hints=(XSizeHints*)mymalloc(sizeof(XSizeHints));
      if(*s)
	{
	  if(!(buttonSwallow(b)&b_UseOld))
	    SendText(fd,s,0);
	  b->spawn=strdup(s);
	}
    }
  else if(*s)
    AddButtonAction(b,0,s);
  return;
}

/**
*** ParseConfigLine
**/
void ParseConfigLine(button_info **ubb,char *s)
{
  button_info *ub=*ubb;
  char *opts[]={"geometry","font","padding","columns","rows","back","fore",
		  "frame","file","pixmap","panel","boxsize",NULL};
  int i,j,k;

  switch(GetTokenIndex(s,opts,-1,&s))
    {
    case 0:/* Geometry */
      {
	char geom[64];
	int flags,g_x,g_y;
        unsigned int width,height;
	i=sscanf(s,"%63s",geom);
	if(i==1)
	  {
	    flags=XParseGeometry(geom,&g_x,&g_y,&width,&height);
	    UberButton->w = 0;
	    UberButton->h = 0;
	    if(flags&WidthValue)
	      w=width;
	    if(flags&HeightValue)
	      h=height;
	    if(flags&XValue)
	      UberButton->x = g_x;
	    if(flags&YValue)
	      UberButton->y = g_y;
	    if(flags&XNegative)
	      UberButton->w = 1;
	    if(flags&YNegative)
	      UberButton->h = 1;
	  }
	break;
      }
    case 1:/* Font */
      CopyString(&ub->c->font_string,s);
      break;
    case 2:/* Padding */
      i=sscanf(s,"%d %d",&j,&k);
      if(i>0) ub->c->xpad=ub->c->ypad=j;
      if(i>1) ub->c->ypad=k;
      break;
    case 3:/* Columns */
      i=sscanf(s,"%d",&j);
      if(i>0) ub->c->num_columns=j;
      break;
    case 4:/* Rows */
      i=sscanf(s,"%d",&j);
      if(i>0) ub->c->num_rows=j;
      break;
    case 5:/* Back */
      CopyString(&(ub->c->back),s);
      break;
    case 6:/* Fore */
      CopyString(&(ub->c->fore),s);
      break;
    case 7:/* Frame */
      i=sscanf(s,"%d",&j);
      if(i>0) ub->c->framew=j;
      break;
    case 8:/* File */
      s = trimleft(s);
      if (config_file)
	free(config_file);
      config_file=seekright(&s);
      break;
    case 9:/* Pixmap */
      s = trimleft(s);
      if (strncasecmp(s,"none",4)==0)
	ub->c->flags|=b_TransBack;
      else
        CopyString(&(ub->c->back_file),s);
      ub->c->flags|=b_IconBack;
      break;
    case 10:/* Panel */
      s = trimleft(s);
      CurrentPanel->next = (panel_info *) mymalloc(sizeof(panel_info));
      CurrentPanel = CurrentPanel->next;
      CurrentPanel->next = NULL;
      CurrentPanel->uber = UberButton
                         = (button_info *) mymalloc(sizeof(button_info));
      if (UberButton->title)
	free(UberButton->title);
      UberButton->title = seekright(&s);
      UberButton->flags = 0;
      UberButton->parent = NULL;
      UberButton->BWidth = 1;
      UberButton->BHeight = 1;
      UberButton->swallow = 0; /* subpanel is hidden initially */
      MakeContainer(UberButton);
      ub = *ubb = UberButton;
      break;
    case 11: /* BoxSize */
      ParseBoxSize(&s, &ub->c->flags);
      break;
    default:
      s = trimleft(s);
      match_string(ubb,s);
      break;
    }
}

/**
*** ParseConfigFile()
*** Parses optional separate configuration file for FvwmButtons
**/
void ParseConfigFile(button_info *ub)
{
  char s[1024],*t;
  FILE *f=fopen(config_file,"r");
  int l;
  if(!f)
    {
      fprintf(stderr,"%s: Couldn't open config file %s\n",MyName,config_file);
      return;
    }

  while (fgets(s, 1023, f))
    {
      /* Allow for line continuation: */
      while ((l=strlen(s)) < sizeof(s)
             && l>=2 && s[l-1]=='\n' && s[l-2]=='\\')
	fgets(s+l-2, sizeof(s)-l, f);

      /* And comments: */
      t=s;
      while(*t)
	{
	  if(*t=='#' && (t==s || *(t-1)!='\\'))
	    {
	      *t=0;
	      break;
	    }
	  t++;
	}
      t = s;
      t = trimleft(t);
      if(*t)
	ParseConfigLine(&ub,t);
    }

  fclose(f);
}

extern int save_color_limit;            /* global for xpm color limiting */
/**
*** ParseOptions()
**/
void ParseOptions(button_info *ub)
{
  char *s;
  char *items[]={"iconpath","pixmappath","colorlimit",NULL,NULL};

  items[3]=mymalloc(strlen(MyName)+2);
  sprintf(items[3],"*%s",MyName);

  GetConfigLine(fd,&s);
  while(s && s[0])
    {
      switch(GetTokenIndex(s,items,-1,&s))
	{
	case -1:
	  break;
	case 0:
	  if (iconPath)
	    free(iconPath);
	  CopyString(&iconPath,s);
	  break;
	case 1:
	  if (pixmapPath)
	    free(pixmapPath);
	  CopyString(&pixmapPath,s);
	  break;
	case 2:                         /* colorlimit */
          sscanf(s,"%d",&save_color_limit);
	  break;
	case 3:
	  if(s && s[0] && !config_file)
	    ParseConfigLine(&ub,s);
	}
      GetConfigLine(fd,&s);
    }

  if(config_file)
    ParseConfigFile(ub);

  free(items[3]);
  return;
}
