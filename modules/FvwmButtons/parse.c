/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1993, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact.
 *
 */

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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "libs/Module.h"
#include "libs/Colorset.h"
#include "FvwmButtons.h"
#include "button.h"
#include "parse.h"

extern int w,h,x,y,xneg,yneg; /* used in ParseConfigLine */
extern char *config_file;
extern int button_width;
extern int button_height;
extern int has_button_geometry;

/* contains the character that terminated the last string from seekright */
static char terminator = '\0';

/* ----------------------------------- macros ------------------------------ */

static char *trimleft(char *s)
{
  while (s && isspace((unsigned char)*s))
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
static char *seekright(char **s)
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
static int ParseBack(char **ss)
{
  char *opts[] =
  {
    "icon",
    NULL
  };
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
  if(*s)
    s++;
  *ss=s;

  return r;
}

/**
*** ParseBoxSize()
*** Parses the options possible to BoxSize
**/
static void ParseBoxSize(char **ss, unsigned long *flags)
{
  char *opts[] =
  {
    "dumb",
    "fixed",
    "smart",
    NULL
  };
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
static void ParseTitle(char **ss, byte *flags, byte *mask)
{
  char *titleopts[] =
  {
    "left",
    "right",
    "center",
    "side",
    NULL
  };
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
static void ParseSwallow(char **ss,byte *flags,byte *mask)
{
  char *swallowopts[] =
  {
    "nohints", "hints",
    "nokill","kill",
    "noclose", "close",
    "respawn","norespawn",
    "useold", "noold",
    "usetitle", "notitle",
    NULL
  };
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
      fprintf(stderr,"%s: Illegal Swallow option \"%s\"\n",MyName,
	      (t)?t:"");
      if (t)
	free(t);
    }
  }
  if(*s) s++;
  *ss=s;
}

/**
*** ParsePanel()
*** Parses the options possible to Panel
**/
static void ParsePanel(char **ss, byte *flags, byte *mask, char *direction,
		       int *steps, int *delay)
{
  char *swallowopts[] =
  {
    "nohints", "hints",
    "nokill","kill",
    "noclose", "close",
    "respawn","norespawn",
    "useold", "noold",
    "usetitle", "notitle",
    "up", "down", "left", "right",
    "steps",
    "delay",
    NULL
  };
  char *t,*s=*ss;
  int n;

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
    case 12: /* up */
      *direction = SLIDE_UP;
      break;
    case 13: /* down */
      *direction = SLIDE_DOWN;
      break;
    case 14: /* left */
      *direction = SLIDE_LEFT;
      break;
    case 15: /* right */
      *direction = SLIDE_RIGHT;
      break;
    case 16: /* steps */
      sscanf(s, "%d%n", steps, &n);
      s += n;
      break;
    case 17: /* delay */
      sscanf(s, "%d%n", delay, &n);
      s += n;
      break;
    default:
      t=seekright(&s);
      fprintf(stderr,"%s: Illegal Panel option \"%s\"\n",MyName,
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
static void ParseContainer(char **ss,button_info *b)
{
  char *conts[] =
  {
    "columns",
    "rows",
    "font",
    "frame",
    "back",
    "fore",
    "padding",
    "title",
    "swallow",
    "nosize",
    "size",
    "boxsize",
    "colorset",
    NULL
  };
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
      {
	b->c->flags|=b_Font;
      } else
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
      {
	b->c->flags|=b_Back;
      }
      else
	b->c->flags&=~(b_IconBack|b_Back);
      break;
    case 5: /* Fore */
      if (b->c->fore) free(b->c->fore);
      b->c->fore=seekright(&s);
      if(b->c->fore)
      {
	b->c->flags|=b_Fore;
      }
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
      {
	Bool failed = False;

	s = trimleft(s);
	if (b->c->flags & (b_Swallow | b_Panel))
	{
	  fprintf(stderr, "%s: Multiple Swallow or Panel options are not"
		" allowed in a signle button", MyName);
	  failed = True;
	}
	else if(*s=='(' && s++)
	{
	  b->c->swallow=0;
	  b->c->swallow_mask=0;
	  ParseSwallow(&s, &b->c->swallow, &b->c->swallow_mask);
	  if(b->c->swallow_mask)
	  {
	    b->c->flags |= b_Swallow;
	  }
	}
	else
	{
	  fprintf(stderr,
		  "%s: Illegal swallow or panel in container options\n",
		  MyName);
	  failed = True;
	}
	if (failed)
	{
	  char *temp;

	  temp = seekright(&s);
	  if (temp)
	    free(temp);
	}
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

    case 12: /* Colorset */
      i = strtol(s, &t, 10);
      if(t > s)
      {
	b->c->colorset = i;
	b->c->flags |= b_Colorset;
      }
      else
      {
	b->c->flags &= ~b_Colorset;
      }
      break;

    default:
      t=seekright(&s);
      fprintf(stderr,"%s: Illegal container option \"%s\"\n",MyName,
	      (t)?t:"");
      if (t)
	free(t);
    }
  }
  if(*s)
    s++;
  *ss=s;
}

/**
*** ParseButton()
***
*** parse a buttonconfig string.
*** *FvwmButtons(option[ options]) title iconname command
**/
/*#define DEBUG_PARSER*/
static void ParseButton(button_info **uberb,char *s)
{
  button_info *b,*ub=*uberb;
  int i,j;
  char *t,*o;
  b=alloc_button(ub,(ub->c->num_buttons)++);
  s = trimleft(s);

  if(*s=='(' && s++)
  {
    char *opts[] =
    {
      "back",
      "fore",
      "font",
      "title",
      "icon",
      "frame",
      "padding",
      "swallow",
      "panel",
      "action",
      "container",
      "end",
      "nosize",
      "size",
      "left",
      "right",
      "center",
      "colorset",
      NULL
    };
    s = trimleft(s);
    while(*s && *s!=')')
    {
      Bool is_swallow = False;

      if((*s>='0' && *s<='9') || *s=='+' || *s=='-')
      {
	char *geom;
	int x,y,flags;
	unsigned int w,h;
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
	if(b->flags&b_Back && b->back)
	  free(b->back);
	b->back=seekright(&s);
	if(b->back)
	{
	  b->flags|=b_Back;
	}
	else
	  b->flags&=~(b_IconBack|b_Back);
	break;

      case 1: /* Fore */
	if(b->flags&b_Fore && b->fore)
	  free(b->fore);
	b->fore=seekright(&s);
	if(b->fore)
	{
	  b->flags|=b_Fore;
	}
	else
	  b->flags&=~b_Fore;
	break;

      case 2: /* Font */
	if(b->flags&b_Font && b->font_string)
	  free(b->font_string);
	b->font_string=seekright(&s);
	if(b->font_string)
	{
	  b->flags|=b_Font;
	}
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
	is_swallow = True;
	/* fall through */
      case 8: /* Panel */
	s = trimleft(s);
	if (is_swallow)
	{
	  b->swallow=0;
	  b->swallow_mask=0;
	}
	else
	{
	  /* set defaults */
	  b->swallow = b_Respawn;
	  b->swallow_mask = b_Respawn;
	  b->slide_direction = SLIDE_UP;
	  b->slide_steps = 12;
	  b->slide_delay_ms = 5;
	}
	if(*s=='(' && s++)
	{
	  if (is_swallow)
	    ParseSwallow(&s, &b->swallow,&b->swallow_mask);
	  else
	    ParsePanel(&s, &b->swallow, &b->swallow_mask, &b->slide_direction,
		       &b->slide_steps, &b->slide_delay_ms);
	}
	t=seekright(&s);
	o=seekright(&s);
	if(t)
	{
	  if (b->hangon)
	    free(b->hangon);
	  b->hangon=t;
	  if (is_swallow)
	    b->flags |= (b_Swallow | b_Hangon);
	  else
	  {
	    b->flags |= (b_Panel | b_Hangon);
	    b->newflags.is_panel = 1;
	    b->newflags.panel_mapped = 0;
	  }
	  b->swallow|=1;
	  if(!(b->swallow&b_NoHints))
	    b->hints=(XSizeHints*)mymalloc(sizeof(XSizeHints));
	  if(o)
	  {
	    char *p;

	    p = expand_action(o, NULL);
	    if (p)
	    {
	      if(!(buttonSwallow(b)&b_UseOld))
		SendText(fd,p,0);
	      if (b->spawn)
		free(b->spawn);
	      b->spawn=o;  /* Might be needed if respawning sometime */
	      free(p);
	    }
	  }
	}
	else
	{
	  fprintf(stderr,"%s: Missing swallow argument\n",MyName);
	  if(t)
	    free(t);
	  if(o)
	    free(o);
	}
	break;

	/* --------------------------- action ------------------------ */

      case 9: /* Action */
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

      case 10: /* Container */
	b->flags&=b_Frame|b_Back|b_Fore|b_Padding|b_Action;
	MakeContainer(b);
	*uberb=b;
	s = trimleft(s);
	if(*s=='(' && s++)
	  ParseContainer(&s,b);
	break;

      case 11: /* End */
	*uberb=ub->parent;
	ub->c->buttons[--(ub->c->num_buttons)]=NULL;
	if(!ub->parent)
	{
	  fprintf(stderr,"%s: Unmatched END in config file\n",MyName);
	  exit(1);
	}
	break;

      case 12: /* NoSize */
	b->flags|=b_Size;
	b->minx=b->miny=0;
	break;

      case 13: /* Size */
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

      case 17: /* Colorset */
	i = strtol(s, &t, 10);
	if(t > s)
	{
	  b->colorset = i;
	  b->flags |= b_Colorset;
	  s=t;
	}
	else
	{
	  b->flags &= ~b_Colorset;
	}
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
  if (strncasecmp(s,"swallow",7)==0 || strncasecmp(s,"panel",7)==0)
  {
    if(b->flags & (b_Swallow | b_Panel))
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
    if (tolower(*s) == 's')
      b->flags |= b_Swallow | b_Hangon;
    else
      b->flags |= b_Panel | b_Hangon;
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
static void ParseConfigLine(button_info **ubb,char *s)
{
  button_info *ub=*ubb;
  char *opts[] =
  {
    "geometry",
    "buttongeometry",
    "font",
    "padding",
    "columns",
    "rows",
    "back",
    "fore",
    "frame",
    "file",
    "pixmap",
    "boxsize",
    "colorset",
    NULL
  };
  int i,j,k;

  switch(GetTokenIndex(s,opts,-1,&s))
  {
  case 0:/* Geometry */
  {
    char geom[64];

    i=sscanf(s,"%63s",geom);
    if(i==1)
    {
      parse_window_geometry(geom);
    }
    break;
  }
  case 1:/* ButtonGeometry */
  {
    char geom[64];

    i=sscanf(s,"%63s",geom);
    if(i==1)
    {
      int flags;
      int g_x;
      int g_y;
      unsigned int width;
      unsigned int height;

      flags = XParseGeometry(geom,&g_x,&g_y,&width,&height);
      if (!flags)
	break;
      UberButton->w = 0;
      UberButton->h = 0;
      if (flags&WidthValue)
	button_width = width;
      if (flags&HeightValue)
	button_height = height;
      if (flags&XValue)
	UberButton->x = g_x;
      if (flags&YValue)
	UberButton->y = g_y;
      if (flags&XNegative)
	UberButton->w = 1;
      if (flags&YNegative)
	UberButton->h = 1;
      has_button_geometry = 1;
    }
    break;
  }
  case 2:/* Font */
    CopyString(&ub->c->font_string,s);
    break;
  case 3:/* Padding */
    i=sscanf(s,"%d %d",&j,&k);
    if(i>0)
      ub->c->xpad=ub->c->ypad=j;
    if(i>1)
      ub->c->ypad=k;
    break;
  case 4:/* Columns */
    i=sscanf(s,"%d",&j);
    if(i>0)
      ub->c->num_columns=j;
    break;
  case 5:/* Rows */
    i=sscanf(s,"%d",&j);
    if(i>0)
      ub->c->num_rows=j;
    break;
  case 6:/* Back */
    CopyString(&(ub->c->back),s);
    break;
  case 7:/* Fore */
    CopyString(&(ub->c->fore),s);
    break;
  case 8:/* Frame */
    i=sscanf(s,"%d",&j);
    if(i>0) ub->c->framew=j;
    break;
  case 9:/* File */
    s = trimleft(s);
    if (config_file)
      free(config_file);
    config_file=seekright(&s);
    break;
  case 10:/* Pixmap */
    s = trimleft(s);
    if (strncasecmp(s,"none",4)==0)
      ub->c->flags|=b_TransBack;
    else
      CopyString(&(ub->c->back_file),s);
    ub->c->flags|=b_IconBack;
    break;
  case 11: /* BoxSize */
    ParseBoxSize(&s, &ub->c->flags);
    break;
  case 12: /* Colorset */
    i = sscanf(s, "%d", &j);
    if (i > 0)
    {
      ub->c->colorset = j;
      ub->c->flags |= b_Colorset;
    }
    else
    {
      ub->c->flags &= ~b_Colorset;
    }
    break;
  default:
    s = trimleft(s);
    ParseButton(ubb,s);
    break;
  }
}

/**
*** ParseConfigFile()
*** Parses optional separate configuration file for FvwmButtons
**/
static void ParseConfigFile(button_info *ub)
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

char *expand_action(char *in_action, button_info *b)
{
  char *variables[] =
  {
    "$",
    "fg",
    "bg",
    "left",
    "-left",
    "right",
    "-right",
    "top",
    "-top",
    "bottom",
    "-bottom",
    "width",
    "height",
    NULL
  };
  char *action = NULL;
  char *src;
  char *dest;
  char *string = NULL;
  char *rest;
  int px;
  int py;
  int val = 0;
  int offset;
  int x;
  int y;
  int f;
  int i;
  unsigned int w = 0;
  unsigned int h = 0;
  Window win;
  extern int dpw;
  extern int dph;


  /* create a temporary storage for expanding */
  action = (char *)malloc(MAX_MODULE_INPUT_TEXT_LEN);
  if (!action)
  {
    /* could not alloc memory */
    return NULL;
  }

  /* calculate geometry */
  if (b)
  {
    w = buttonWidth(b);
    h = buttonHeight(b);
    buttonInfo(b, &x, &y, &px, &py, &f);
    XTranslateCoordinates(Dpy, MyWindow, Root, x, y, &x, &y, &win);
  }

  for (src = in_action, dest = action; *src != 0; src++)
  {
    if (*src != '$')
    {
      *(dest++) = *src;
    }
    else
    {
      char *dest_org = dest;
      Bool is_string = False;
      Bool is_value = False;

      *(dest++) = *(src++);
      i = GetTokenIndex(src, variables, -1, &rest);
      if (i == -1)
      {
	src--;
	continue;
      }
      switch (i)
      {
      case 0: /* $ */
	continue;
      case 1: /* fg */
	string = UberButton->c->fore;
	is_string = True;
	break;
      case 2: /* bg */
	string = UberButton->c->back;
	is_string = True;
	break;
      case 3: /* left */
	val = x;
	is_value = True;
	break;
      case 4: /* -left */
	val = dpw - x - 1;
	is_value = True;
	break;
      case 5: /* right */
	val = x + w;
	is_value = True;
	break;
      case 6: /* -right */
	val = dpw - x - w - 1;
	is_value = True;
	break;
      case 7: /* top */
	val = y;
	is_value = True;
	break;
      case 8: /* -top */
	val = dph - y - 1;
	is_value = True;
	break;
      case 9: /* bottom */
	val = y + h;
	is_value = True;
	break;
      case 10: /* -bottom */
	val = dph - y - h - 1;
	is_value = True;
	break;
      case 11: /* width */
	val = w;
	is_value = True;
	break;
      case 12: /* height */
	val = h;
	is_value = True;
	break;
      default: /* unknown */
	src--;
	continue;
      } /* switch */
      dest = dest_org;
      src = --rest;
      if (is_value)
      {
	if (MAX_MODULE_INPUT_TEXT_LEN - (dest - action) <= 16)
	{
	  /* out of space */
	  free(action);
	  return NULL;
	}
	/* print the number into the string */
	sprintf(dest, "%d%n", val, &offset);
	dest += offset;
      }
      else if (is_string)
      {
	if (MAX_MODULE_INPUT_TEXT_LEN - (dest - action) <= strlen(string))
	{
	  /* out of space */
	  free(action);
	  return NULL;
	}
	/* print the colour name into the string */
	if (string)
	{
	  sprintf(dest, "%s%n", string, &offset);
	  dest += offset;
	}
      }
    } /* if */
  } /* for */
  *dest = 0;
  return action;
}

void parse_window_geometry(char *geom)
{
  int flags;
  int g_x;
  int g_y;
  unsigned int width;
  unsigned int height;

  flags = XParseGeometry(geom,&g_x,&g_y,&width,&height);
  UberButton->w = 0;
  UberButton->h = 0;
  if (flags&WidthValue)
    w = width;
  if (flags&HeightValue)
    h = height;
  if (flags&XValue)
    UberButton->x = g_x;
  if (flags&YValue)
    UberButton->y = g_y;
  if (flags&XNegative)
    UberButton->w = 1;
  if (flags&YNegative)
    UberButton->h = 1;
  has_button_geometry = 0;

  return;
}

/**
*** ParseOptions()
**/
extern int save_color_limit;            /* global for xpm color limiting */
void ParseConfiguration(button_info *ub)
{
  char *s;
  char *items[] =
  {
    NULL,
    "imagepath",
    "colorlimit",
    "colorset",
    NULL
  };

  items[0]=mymalloc(strlen(MyName)+2);
  sprintf(items[0],"*%s",MyName);

  InitGetConfigLine(fd,items[0]);       /* send config lines with MyName */
  GetConfigLine(fd,&s);
  while(s && s[0])
  {
    char *rest;
    switch(GetTokenIndex(s,items,-1,&rest))
    {
    case -1:
      break;
    case 0:
      if(rest && rest[0] && !config_file)
	ParseConfigLine(&ub,rest);
      break;
    case 1:
      if (imagePath)
	free(imagePath);
      CopyString(&imagePath,rest);
      break;
    case 2:                         /* colorlimit */
      sscanf(rest, "%d", &save_color_limit);
      break;
    case 3:
      /* store colorset sent by fvwm */
      LoadColorset(rest);
      break;
    }
    GetConfigLine(fd,&s);
  }

  if(config_file)
    ParseConfigFile(ub);

  free(items[0]);
  return;
}
