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

/* ----------------------------------- macros ------------------------------ */

#define trimleft(s) {while(*s==' '||*s=='\t'||*s=='\n')(s)++;}
#define trimright(s) {                                                        \
  char *t=(s);while(*t!=0) t++;                                               \
  if(t>s) while(*(t-1)==' '||*(t-1)=='\t'||*(t-1)=='\n') t--;                 \
  *t=0;}

/**
*** seekright()
***
*** split off the first continous section of the string into a new allocated
*** string and move the old pointer forward. Accepts and strips quoting with
*** '," or `, and the current quote q can be escaped inside the string with \q.
**/
char *seekright(char **s)
{
  char *b,*t,*r,q;
  trimleft(*s);
  q=**s;
  if(q=='"' || q=='`' || q=='\'')
    {
      b=t=(*s)++;
      while(**s && (**s!=q || (*(*s-1)=='\\' && t--)))
	*t++=*((*s)++);
      if(!**s)
	fprintf(stderr,"%s: Unterminated %c quote found\n",MyName,q);
      else (*s)++;
    }
  else
    {
      b=t=*s;
      while(**s && !isspace(**s) && **s!=')' && **s!=',')
	(*s)++;
      t=*s;
    }
  if(t<=b)
    return NULL;
  r=mymalloc(t-b+1);
  strncpy(r,b,t-b);
  r[t-b]=0;
  return r;
}

/**
*** MatchSeveralLines()
***
*** Returns which, if any, of the strings in o[] matches start of s, t is moved
*** after the end of the match.
**/
int MatchSeveralLines(char *s,char **o,char **t)
{
  int i=0,j;
  if(s && o && o[0])
    while(o[i])
      {
	j=0;while(o[i][j] && tolower(s[j])==tolower(o[i][j]))j++;
	if(!o[i][j])
	  {
	    *t=&s[j];
	    return i;
	  }
	i++;
      }
  *t=s;
  return -1;
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
      trimleft(s);
      if(*s==',')
	s++;
      else switch(MatchSeveralLines(s,opts,&s))
	{
	case 0: /* Icon */
	  r=1;
	  fprintf(stderr,"%s: Back(Icon) not supported yet\n",MyName);
	  break;
	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal back option \"%s\"\n",MyName,t);
	  free(t);
	}
    }
  if(*s) s++;
  *ss=s;
  return r;
}

/**
*** ParseTitle()
*** Parses the options possible to Title
**/
void ParseTitle(char **ss,byte *flags,byte *mask)
{
  char *titleopts[]={"left","right","center","side",NULL};
  char *t,*s=*ss;

  while(*s && *s!=')')
    {
      trimleft(s);
      if(*s==',')
	s++;
      else switch(MatchSeveralLines(s,titleopts,&s))
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
	  fprintf(stderr,"%s: Illegal title option \"%s\"\n",MyName,t);
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
      trimleft(s);
      if(*s==',')
	s++;
      else switch(MatchSeveralLines(s,swallowopts,&s))
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
	  fprintf(stderr,"%s: Illegal swallow option \"%s\"\n",MyName,t);
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
		   "padding","title","swallow","nosize","size",NULL};
  char *t,*o,*s=*ss;
  int i,j;

  while(*s && *s!=')')
    {
      trimleft(s);
      if(*s==',')
	s++;
      else switch(MatchSeveralLines(s,conts,&s))
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
	  b->c->font_string=seekright(&s);
	  b->c->flags|=b_Font;
	  break;
	case 3: /* Frame */
	  b->c->framew=strtol(s,&t,10);
	  b->c->flags|=b_Frame;
	  s=t;
	  break;
	case 4: /* Back */
	  trimleft(s);
	  if(*s=='(' && s++)
	    if(ParseBack(&s))
	      b->c->flags|=b_IconBack;
	  b->c->back=seekright(&s);
	  if(b->c->back)
	    b->c->flags|=b_Back;
	  else
	    b->c->flags&=~b_IconBack;
	  break;
	case 5: /* Fore */
	  b->c->fore=seekright(&s);
	  b->c->flags|=b_Fore;
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
	  trimleft(s);
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
	      fprintf(stderr,"%s: Illegal title in container options\n",
		      MyName);
	      free(seekright(&s));
	    }
	  break;
	case 8: /* Swallow - flags */
	  trimleft(s);
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
	      fprintf(stderr,"%s: Illegal swallow in container options\n",
		      MyName);
	      free(seekright(&s));
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

	default:
	  t=seekright(&s);
	  fprintf(stderr,"%s: Illegal container option \"%s\"\n",MyName,t);
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
  trimleft(s);

  if(*s=='(' && s++)
    {
      char *opts[]={"back","fore","font","title","icon","frame","padding",
		      "swallow","action","container","end","nosize","size",
                      "panel",
		      NULL};
      trimleft(s);
      while(*s && *s!=')')
	{
	  if(*s>='0' && *s<='9')
	    {
	      sscanf(s,"%dx%d",&i,&j);
	      if(i>0) b->BWidth=i;
	      if(j>0) b->BHeight=j;
	      while(*s!=' ' && *s!='\t' && *s!=')' && *s!=',') s++;
	      trimleft(s);
	      continue;
	    }
	  if(*s==',' && s++)
	    trimleft(s);
	  switch(MatchSeveralLines(s,opts,&s))
	    {
	    case 0: /* Back */
	      trimleft(s);
	      if(*s=='(' && s++)
		if(ParseBack(&s))
		  b->flags|=b_IconBack;
	      if(b->flags&b_Back) free(b->back);
	      b->back=seekright(&s);
	      if(b->back)
		b->flags|=b_Back;
	      else
		b->flags&=~b_IconBack;
	      break;

	    case 1: /* Fore */
	      if(b->flags&b_Fore) free(b->fore);
	      b->fore=seekright(&s);
	      b->flags|=b_Fore;
	      break;

	    case 2: /* Font */
	      if(b->flags&b_Font) free(b->font_string);
	      b->font_string=seekright(&s);
	      b->flags|=b_Font;
	      break;

	      /* --------------------------- Title ------------------------- */

	    case 3: /* Title */
	      trimleft(s);
	      if(*s=='(' && s++)
		{
		  b->justify=0;
		  b->justify_mask=0;
		  ParseTitle(&s,&b->justify,&b->justify_mask);
		  if(b->justify_mask)
		    b->flags|=b_Justify;
		}
	      t=seekright(&s);
	      if(t && (t[0]!='-' || t[1]!=0))
		{
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
	      if(t && (t[0]!='-' && t[1]!=0))
		{
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
	      trimleft(s);
	      b->swallow=0;
	      b->swallow_mask=0;
	      if(*s=='(' && s++)
		ParseSwallow(&s,&b->swallow,&b->swallow_mask);
	      t=seekright(&s);
	      o=seekright(&s);
	      if(t)
		{
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
	      trimleft(s);
	      i=0;
	      if(*s=='(')
		{
		  s++;
		  if(mystrncasecmp(s,"mouse",5)!=0)
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
	      t=seekright(&s);
	      if(t)
		AddButtonAction(b,i,strdup(t));
	      else
		fprintf(stderr,"%s: Missing action argument\n",MyName);
	      break;

	      /* -------------------------- container ---------------------- */

	    case 9: /* Container */
	      b->flags&=b_Frame|b_Back|b_Fore|b_Padding|b_Action;
	      MakeContainer(b);
	      *uberb=b;
	      trimleft(s);
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
	      trimleft(s);
	      i=0;
	      if(*s=='(')
		{
		  s++;
		  i=strtol(s,&t,10);
		  s=t;
		  while(*s && *s!=')') 
		    s++;
		  if(*s==')')s++;
		}
              if (i == 1)
              { /* horizontal */
                AddButtonAction(b,0,strdup("panel-h"));
                if ((~b->flags)&(b_Title | b_Icon))
		{ b->icon_file = strdup("arrow_left.xpm");
		  b->IconWin   = None;
		  b->flags    |= b_Icon;
                }
              }
	      else
              { /* vertical */
                AddButtonAction(b,0,strdup("panel-v"));
                if ((~b->flags)&(b_Title | b_Icon))
		{ b->icon_file = strdup("arrow_up.xpm");
		  b->IconWin   = None;
		  b->flags    |= b_Icon;
                }
              }
	      t = seekright(&s);
              b->hangon = strdup(t);  /* which panel to popup */
	      break;

	    default:
	      t=seekright(&s);
	      fprintf(stderr,"%s: Illegal button option \"%s\"\n",MyName,t);
	      free(t);
	      break;
	    }
	  trimleft(s);
	}
      s++;
      trimleft(s);
    }

  /* get title and iconname */
  if(!(b->flags&b_Title))
    {
      b->title=seekright(&s);
      if(b->title && ((b->title)[0]!='-'||(b->title)[1]!=0))
	b->flags |= b_Title;
      else
	if(b->title)free(b->title);
    }
  else
    free(seekright(&s));

  if(!(b->flags&b_Icon))
    {
      b->icon_file=seekright(&s);
      if(b->icon_file && ((b->icon_file)[0]!='-'||(b->icon_file)[1]!=0))
	{
	  b->flags|=b_Icon;
	  b->IconWin=None;
	}
      else
	if(b->icon_file)free(b->icon_file);
    }
  else
    free(seekright(&s));

  trimleft(s);

  /* Swallow hangon command */
  if(mystrncasecmp(s,"swallow",7)==0)
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
      if (mystrncasecmp(s,"module",6)==0)
      {
        s+=6;
      }
      b->hangon=seekright(&s);
      b->flags|=(b_Swallow|b_Hangon);
      b->swallow|=1;
      trimleft(s);
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
    AddButtonAction(b,0,strdup(s));
  return;
}

/**
*** ParseConfigLine
**/
void ParseConfigLine(button_info **ubb,char *s)
{
  button_info *ub=*ubb;
  char *opts[]={"geometry","font","padding","columns","rows","back","fore",
		  "frame","file","pixmap","panel",NULL};
  int i,j,k;

  switch(MatchSeveralLines(s,opts,&s))
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
	    if(flags&WidthValue) w=width;
	    if(flags&HeightValue) h=height;
	    if(flags&XValue)    UberButton->x = g_x;
	    if(flags&YValue)    UberButton->y = g_y;
	    if(flags&XNegative) UberButton->w = 1;
	    if(flags&YNegative) UberButton->h = 1;
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
      trimleft(s);
      config_file=seekright(&s);
      break;
    case 9:/* Pixmap */
      trimleft(s);
      if (mystrncasecmp(s,"none",4)==0)
	ub->c->flags|=b_TransBack;
      else
        CopyString(&(ub->c->back_file),s);
      ub->c->flags|=b_IconBack;
      break;
    case 10:/* Panel */
      trimleft(s);
      CurrentPanel->next = (panel_info *) mymalloc(sizeof(panel_info));
      CurrentPanel = CurrentPanel->next;
      CurrentPanel->next = NULL;
      CurrentPanel->uber = UberButton
                         = (button_info *) mymalloc(sizeof(button_info));
      UberButton->title = seekright(&s);
      UberButton->flags = 0;
      UberButton->parent = NULL;
      UberButton->BWidth = 1;
      UberButton->BHeight = 1;
      UberButton->swallow = 0; /* subpanel is hidden initially */
      MakeContainer(UberButton);
      *ubb = UberButton;
      break;
    default:
      trimleft(s);
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

  while(fgets(s,1023,f))
    {
      /* Got to do some preprocessing here... Line continuation: */
      while((l=strlen(s))<sizeof(s) && s[l-1]=='\n' && s[l-2]=='\\')
	fgets(s+l-2,sizeof(s)-l,f);
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
      t=s;trimleft(t);
      if(*t)ParseConfigLine(&ub,t);
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
  int rc;

  items[3]=mymalloc(strlen(MyName)+2);
  sprintf(items[3],"*%s",MyName);

  GetConfigLine(fd,&s);
  while(s && s[0])
    {
      switch(MatchSeveralLines(s,items,&s))
	{
	case -1:
	  break;
	case 0:
	  CopyString(&iconPath,s);
	  break;
	case 1:
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

  return;
}

