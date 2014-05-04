/* -*-c-*- */
/* FvwmForm is original work of Thomas Zuwei Feng.
 *
 * Copyright Feb 1995, Thomas Zuwei Feng.  No guarantees or warantees are
 * provided or implied in any way whatsoever.  Use this program at your own
 * risk.  Permission to use, modify, and redistribute this program is hereby
 * given, provided that this copyright is kept intact. */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#include "libs/ftime.h"
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include "libs/Module.h"		/* for headersize, etc. */
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/ColorUtils.h"
#include "libs/Cursor.h"
#include "libs/envvar.h"
#include "libs/Graphics.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "libs/System.h"
#include "libs/XError.h"

#include "libs/PictureBase.h"		 /* for PictureInitCMap */
#include "libs/Colorset.h"
#include "libs/FScreen.h"
#include "libs/FShape.h"
#include "libs/FRenderInit.h"
#include "libs/Rectangles.h"

#include "FvwmForm.h"			/* common FvwmForm stuff */

/* globals that are exported, keep in sync with externs in FvwmForm.h */
Form cur_form;			 /* current form */

/* Link list roots */
Item *root_item_ptr;		 /* pointer to root of item list */
Line root_line = {&root_line,	 /* ->next */
		  0,		 /* number of items */
		  L_CENTER,0,0,	 /* justify, size x/y */
		  0,		 /* current array size */
		  0};		 /* items ptr */
Line *cur_line = &root_line;		/* curr line in parse rtns */
char preload_yorn='n';		 /* init to non-preload */
Item *item;				/* current during parse */
Item *cur_sel, *cur_button;		/* current during parse */
Item *timer = NULL;			/* timeout tracking */
Display *dpy;
Atom wm_del_win;
int fd_x;		   /* fd for X connection */
Window root, ref;
int screen;
char *color_names[4];
char bg_state = 'd';			/* in default state */
  /* d = default state */
  /* s = set by command (must be in "d" state for first "back" cmd to set it) */
  /* u = used (color allocated, too late to accept "back") */
char endDefaultsRead = 'n';
char *font_names[4];
char *screen_background_color;
static ModuleArgs *module;
int Channel[2];
Bool Swallowed = False;

/* Font/color stuff
   The colors are:
   . defaults are changed by commands during parse
   . copied into items during parse
   . displayed in the customization dialog
   */
int colorset = -1;
int itemcolorset = 0;

/* prototypes */
static void RedrawSeparator(Item *item);
static void AssignDrawTable(Item *);
static void AddItem(void);
static void PutDataInForm(char *);
static void ReadFormData(void);
static void FormVarsCheck(char **);
static RETSIGTYPE TimerHandler(int);
static int ErrorHandler(Display *dpy, XErrorEvent *error_event);
static void SetupTimer(void)
{
#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGALRM);
    sigact.sa_handler = TimerHandler;

    sigaction(SIGALRM, &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGALRM) );
#endif
  signal(SIGALRM, TimerHandler);  /* Dead pipe == Fvwm died */
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGALRM, 1);
#endif
#endif

  alarm(1);
}

/* copy a string until '"', or '\n', or '\0' */
static char *CopyQuotedString (char *cp)
{
  char *dp, *bp, c;
  bp = dp = xmalloc(strlen(cp) + 1);
  while (1) {
    switch (c = *(cp++)) {
    case '\\':
      *(dp++) = *(cp++);
      break;
    case '\"':
    case '\n':
    case '\0':
      *dp = '\0';
      return bp;
      break;
    default:
      *(dp++) = c;
      break;
    }
  }
}

/* copy a string until the first space */
static char *CopySolidString (char *cp)
{
  char *dp, *bp, c;
  bp = dp = xmalloc(strlen(cp) + 1);
  while (1) {
    c = *(cp++);
    if (c == '\\') {
      *(dp++) = '\\';
      *(dp++) = *(cp++);
    } else if (isspace((unsigned char)c) || c == '\0') {
      *dp = '\0';
      return bp;
    } else
      *(dp++) = c;
  }
}

/* Command parsing section */

/* FvwmAnimate++ type command table */
typedef struct CommandTable
{
  char *name;
  void (*function)(char *);
} ct;
static void ct_ActivateOnPress(char *);
static void ct_Back(char *);
static void ct_Button(char *);
static void ct_ButtonFont(char *);
static void ct_ButtonInPointer(char *);
static void ct_ButtonInPointerBack(char *);
static void ct_ButtonInPointerFore(char *);
static void ct_ButtonPointer(char *);
static void ct_ButtonPointerBack(char *);
static void ct_ButtonPointerFore(char *);
static void ct_Choice(char *);
static void ct_Command(char *);
static void ct_Font(char *);
static void ct_Fore(char *);
static void ct_GrabServer(char *);
static void ct_Input(char *);
static void ct_InputFont(char *);
static void ct_ItemBack(char *);
static void ct_ItemFore(char *);
static void ct_Line(char *);
static void ct_Message(char *);
static void ct_Geometry(char *);
static void ct_Position(char *);
static void ct_Read(char *);
static void ct_Selection(char *);
static void ct_Separator(char *);
static void ct_Text(char *);
static void ct_InputPointer(char *);
static void ct_InputPointerBack(char *);
static void ct_InputPointerFore(char *);
static void ct_Timeout(char *);
static void ct_TimeoutFont(char *);
static void ct_Title(char *);
static void ct_UseData(char *);
static void ct_padVText(char *);
static void ct_WarpPointer(char *);
static void ct_Colorset(char *);
static void ct_ItemColorset(char *);

/* Must be in Alphabetic order (caseless) */
static struct CommandTable ct_table[] =
{
  {"ActivateOnPress",ct_ActivateOnPress},
  {"Back",ct_Back},
  {"Button",ct_Button},
  {"ButtonFont",ct_ButtonFont},
  {"ButtonInPointer",ct_ButtonInPointer},
  {"ButtonInPointerBack",ct_ButtonInPointerBack},
  {"ButtonInPointerFore",ct_ButtonInPointerFore},
  {"ButtonPointer",ct_ButtonPointer},
  {"ButtonPointerBack",ct_ButtonPointerBack},
  {"ButtonPointerFore",ct_ButtonPointerFore},
  {"Choice",ct_Choice},
  {"Colorset",ct_Colorset},
  {"Command",ct_Command},
  {"Font",ct_Font},
  {"Fore",ct_Fore},
  {"Geometry",ct_Geometry},
  {"GrabServer",ct_GrabServer},
  {"Input",ct_Input},
  {"InputFont",ct_InputFont},
  {"InputPointer",ct_InputPointer},
  {"InputPointerBack",ct_InputPointerBack},
  {"InputPointerFore",ct_InputPointerFore},
  {"ItemBack",ct_ItemBack},
  {"ItemColorset",ct_ItemColorset},
  {"ItemFore",ct_ItemFore},
  {"Line",ct_Line},
  {"Message",ct_Message},
  {"PadVText",ct_padVText},
  {"Position",ct_Position},
  {"Selection",ct_Selection},
  {"Separator",ct_Separator},
  {"Text",ct_Text},
  {"Timeout",ct_Timeout},
  {"TimeoutFont",ct_TimeoutFont},
  {"Title",ct_Title},
  {"UseData",ct_UseData},
  {"WarpPointer",ct_WarpPointer}
};
/* These commands are the default setting commands,
   read before the other form defining commands. */
static struct CommandTable def_table[] =
{
  {"ActivateOnPress",ct_ActivateOnPress},
  {"Back",ct_Back},
  {"ButtonFont",ct_ButtonFont},
  {"ButtonInPointer",ct_ButtonInPointer},
  {"ButtonInPointerBack",ct_ButtonInPointerBack},
  {"ButtonInPointerFore",ct_ButtonInPointerFore},
  {"ButtonPointer",ct_ButtonPointer},
  {"ButtonPointerBack",ct_ButtonPointerBack},
  {"ButtonPointerFore",ct_ButtonPointerFore},
  {"Colorset",ct_Colorset},
  {"Font",ct_Font},
  {"Fore",ct_Fore},
  {"InputFont",ct_InputFont},
  {"InputPointer",ct_InputPointer},
  {"InputPointerBack",ct_InputPointerBack},
  {"InputPointerFore",ct_InputPointerFore},
  {"ItemBack",ct_ItemBack},
  {"ItemColorset",ct_ItemColorset},
  {"ItemFore",ct_ItemFore},
  {"Read",ct_Read},
  {"TimeoutFont",ct_TimeoutFont}
};

/* If there were vars on the command line, do env var sustitution on
   all input. */
static void FormVarsCheck(char **p)
{
  if (CF.have_env_var) {		/* if cmd line vars */
    if (strlen(*p) + 200 > CF.expand_buffer_size) { /* fast and loose */
      CF.expand_buffer_size = strlen(*p) + 2000; /* new size */
      if (CF.expand_buffer) {		/* already have one */
	CF.expand_buffer = xrealloc(CF.expand_buffer, CF.expand_buffer_size,
			sizeof(CF.expand_buffer));
      } else {				/* first time */
	CF.expand_buffer = xmalloc(CF.expand_buffer_size);
      }
    }
    strcpy(CF.expand_buffer,*p);
    *p = CF.expand_buffer;
    envExpand(*p,CF.expand_buffer_size); /* expand the command in place */
  }
}

static void ParseDefaults(char *buf)
{
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {	/* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }
  /* Accept commands beginning with "*FvwmFormDefault".
     This is to make sure defaults are read first.
     Note the hack w. bg_state. */
  if (strncasecmp(buf, "*FvwmFormDefault", 16) == 0) {
    p=buf+16;
    FormVarsCheck(&p);			 /* do var substitution if called for */
    e = FindToken(p,def_table,struct CommandTable);/* find cmd in table */
    if (e != 0) {			/* if its valid */
      p=p+strlen(e->name);		/* skip over name */
      while (isspace((unsigned char)*p)) p++;	       /* skip whitespace */
      e->function(p);			/* call cmd processor */
      bg_state = 'd';			/* stay in default state */
    }
  }
} /* end function */


static void ParseConfigLine(char *buf)
{
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {	/* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }

  if (strncasecmp(buf, XINERAMA_CONFIG_STRING,
		  sizeof(XINERAMA_CONFIG_STRING)-1) == 0)
  {
    FScreenConfigureModule(buf + sizeof(XINERAMA_CONFIG_STRING)-1);
    return;
  }
  if (strncasecmp(buf, "Colorset", 8) == 0) {
    LoadColorset(&buf[8]);
    return;
  }
  if (strncasecmp(buf, CatString3("*",module->name,0), module->namelen+1) != 0) {/* If its not for me */
    return;
  } /* Now I know its for me. */
  p = buf+module->namelen+1;		      /* jump to end of my name */
  /* at this point we have recognized "*FvwmForm" */
  FormVarsCheck(&p);
  e = FindToken(p,ct_table,struct CommandTable);/* find cmd in table */
  if (e == 0) {			      /* if no match */
    fprintf(stderr,"%s: unknown command: %s\n",module->name,buf);
    return;
  }

  p=p+strlen(e->name);			/* skip over name */
  while (isspace((unsigned char)*p)) p++;	       /* skip whitespace */

  FormVarsCheck(&p);			 /* do var substitution if called for */

  e->function(p);			/* call cmd processor */
  return;
} /* end function */

/* Expands item array */
static void ExpandArray(Line *this_line)
{
  if (this_line->n + 1 >= this_line->item_array_size) { /* no empty space */
    this_line->item_array_size += ITEMS_PER_EXPANSION; /* get bigger */
    this_line->items =
      xrealloc((void *)this_line->items,
               (sizeof(Item *) * this_line->item_array_size),
	       sizeof(this_line->items));
  } /* end array full */
}

/* Function to add an item to the current line */
static void AddToLine(Item *newItem)
{
  ExpandArray(cur_line);		/* expand item array if needed */
  cur_line->items[cur_line->n++] = newItem; /* add to lines item array */
  cur_line->size_x += newItem->header.size_x; /* incr lines width */
  if (cur_line->size_y < newItem->header.size_y) { /* new item bigger */
    cur_line->size_y = newItem->header.size_y; /* then line is bigger */
  }
}


/* All the functions starting with "ct_" (command table) are called thru
   their function pointers. Arg 1 is always the rest of the command. */
static void ct_ActivateOnPress(char *cp)
{
  /* arg can be "on", "off", "0", 1", "true", "false", "" */
  char option[6];
  int i,j;
  if (strlen(cp) > 5) {
    fprintf(stderr,"%s: arg for ActivateOnPress (%s) too long\n",
	    module->name,cp);
    return;
  }
  for (i=0,j=0;i<strlen(cp);i++) {
    if (cp[i] != ' ') {			  /* remove any spaces */
      option[j++]=tolower(cp[i]);
    }
  }
  option[j]=0;
  if (strcmp(option,"on") == 0
      || strcmp(option,"true") == 0
      || strcmp(option,"1") == 0
      || (cp == 0 || strcmp(option,"")) == 0) {
    CF.activate_on_press = 1;
  } else if (strcmp(option,"off") == 0
	     || strcmp(option,"false") == 0
	     || strcmp(option,"0") == 0) {
    CF.activate_on_press = 0;
  } else {
    fprintf(stderr,"%s: arg for ActivateOnPress (%s/%s) invalid\n",
	    module->name,option,cp);
  }
}
static void ct_GrabServer(char *cp)
{
  CF.grab_server = 1;
}
static void ct_WarpPointer(char *cp)
{
  CF.warp_pointer = 1;
}
static void ct_Position(char *cp)
{
  CF.have_geom = 1;
  CF.gx = atoi(cp);
  CF.xneg = 0;
  if (CF.gx < 0)
  {
    CF.gx = -1 - CF.gx;
    CF.xneg = 1;
  }
  while (!isspace((unsigned char)*cp)) cp++;
  while (isspace((unsigned char)*cp)) cp++;
  CF.gy = atoi(cp);
  CF.yneg = 0;
  if (CF.gy < 0)
  {
    CF.gy = -1 - CF.gy;
    CF.yneg = 1;
  }
  myfprintf((stderr, "Position @ (%c%d%c%d)\n",
	     (CF.xneg)?'-':'+',CF.gx, (CF.yneg)?'-':'+',CF.gy));
}
static void ct_Geometry(char *cp)
{
  int x = 0;
  int y = 0;
  int flags;
  unsigned int dummy;

  CF.gx = 0;
  CF.gy = 0;
  CF.xneg = 0;
  CF.yneg = 0;
  while (isspace(*cp))
    cp++;
  flags = FScreenParseGeometry(cp, &x, &y, &dummy, &dummy);
  if (flags&XValue)
  {
    CF.have_geom = 1;
    CF.gx = x;
    CF.xneg = (flags&XNegative);
  }
  if (flags&YValue)
  {
    CF.have_geom = 1;
    CF.gy = y;
    CF.yneg = (flags&YNegative);
  }
  myfprintf((stderr, "Geometry @ (%c%d%c%d)\n",
	     (CF.xneg)?'-':'+',CF.gx, (CF.yneg)?'-':'+',CF.gy));
}
static void ct_Fore(char *cp)
{
  if (color_names[c_fg])
    free(color_names[c_fg]);
  color_names[c_fg] = xstrdup(cp);
  colorset = -1;
  myfprintf((stderr, "ColorFore: %s\n", color_names[c_fg]));
}
static void ct_Back(char *cp)
{
  if (color_names[c_bg])
    free(color_names[c_bg]);
  color_names[c_bg] = xstrdup(cp);
  if (bg_state == 'd')
  {
    if (screen_background_color)
      free(screen_background_color);
    screen_background_color = xstrdup(color_names[c_bg]);
    bg_state = 's';			/* indicate set by command */
  }
  colorset = -1;
  myfprintf((stderr, "ColorBack: %s, screen background %s, bg_state %c\n",
	     color_names[c_bg],screen_background_color,(int)bg_state));
}
static void ct_Colorset(char *cp)
{
  sscanf(cp, "%d", &colorset);
  AllocColorset(colorset);
}
static void ct_ItemFore(char *cp)
{
  if (color_names[c_item_fg])
    free(color_names[c_item_fg]);
  color_names[c_item_fg] = xstrdup(cp);
  itemcolorset = -1;
  myfprintf((stderr, "ColorItemFore: %s\n", color_names[c_item_fg]));
}
static void ct_ItemBack(char *cp)
{
  if (color_names[c_item_bg])
    free(color_names[c_item_bg]);
  color_names[c_item_bg] = xstrdup(cp);
  itemcolorset = -1;
  myfprintf((stderr, "ColorItemBack: %s\n", color_names[c_item_bg]));
}
static void ct_ItemColorset(char *cp)
{
  sscanf(cp, "%d", &itemcolorset);
  AllocColorset(itemcolorset);
}
static void ct_Font(char *cp)
{
  if (font_names[f_text])
    free(font_names[f_text]);
  CopyStringWithQuotes(&font_names[f_text], cp);
  myfprintf((stderr, "Font: %s\n", font_names[f_text]));
}
static void ct_TimeoutFont(char *cp)
{
  if (font_names[f_timeout])
    free(font_names[f_timeout]);
  CopyStringWithQuotes(&font_names[f_timeout], cp);
  myfprintf((stderr, "TimeoutFont: %s\n", font_names[f_timeout]));
}
static void ct_ButtonFont(char *cp)
{
  if (font_names[f_button])
    free(font_names[f_button]);
  CopyStringWithQuotes(&font_names[f_button], cp);
  myfprintf((stderr, "ButtonFont: %s\n", font_names[f_button]));
}
static void ct_ButtonInPointer(char *cp)
{
  int cursor;
  cursor=fvwmCursorNameToIndex(cp);
  if (cursor == -1) {
    fprintf(stderr,"ButtonInPointer: invalid cursor name %s\n",cp);
  } else {
    CF.pointer[button_in_pointer] = XCreateFontCursor(dpy,cursor);
  }
}
static void ct_ButtonPointer(char *cp)
{
  int cursor;
  cursor=fvwmCursorNameToIndex(cp);
  if (cursor == -1) {
    fprintf(stderr,"ButtonPointer: invalid cursor name %s\n",cp);
  } else {
    CF.pointer[button_pointer] = XCreateFontCursor(dpy,cursor);
  }
}
static void ct_InputFont(char *cp)
{
  if (font_names[f_input])
    free(font_names[f_input]);
  CopyStringWithQuotes(&font_names[f_input], cp);
  myfprintf((stderr, "InputFont: %s\n", font_names[f_input]));
}
static void ct_InputPointer(char *cp)
{
  int cursor;
  cursor=fvwmCursorNameToIndex(cp);
  if (cursor == -1) {
    fprintf(stderr,"InputPointer: invalid cursor name %s\n",cp);
  } else {
    CF.pointer[input_pointer] = XCreateFontCursor(dpy,cursor);
  }
}
/* process buttons using an index to the ___ array */
static void color_arg(Ptr_color *color_cell, char *color_name);
static void color_arg(Ptr_color *color_cell, char *color_name) {
  if (color_name && *color_name) {
    color_cell->pointer_color.pixel = GetColor(color_name);
    color_cell->used=1;
  }
}

/* arg is a color name, alloc the color */
static void ct_ButtonInPointerBack(char *cp) {
  color_arg(&CF.p_c[button_in_back],cp);
}
static void ct_ButtonInPointerFore(char *cp) {
  color_arg(&CF.p_c[button_in_fore],cp);
}
static void ct_ButtonPointerBack(char *cp) {
  color_arg(&CF.p_c[button_back],cp);
}
static void ct_ButtonPointerFore(char *cp) {
  color_arg(&CF.p_c[button_fore],cp);
}
static void ct_InputPointerBack(char *cp) {
  color_arg(&CF.p_c[input_back],cp);
}
static void ct_InputPointerFore(char *cp) {
  color_arg(&CF.p_c[input_fore],cp);
}

static void ct_Line(char *cp)
{
  /* malloc new line */
  cur_line->next = xmalloc(sizeof(struct _line));
  memset(cur_line->next, 0, sizeof(struct _line));
  cur_line = cur_line->next;		/* new current line */
  cur_line->next = &root_line;		/* new next ptr, (actually root) */

  if (strncasecmp(cp, "left", 4) == 0)
    cur_line->justify = L_LEFT;
  else if (strncasecmp(cp, "right", 5) == 0)
    cur_line->justify = L_RIGHT;
  else if (strncasecmp(cp, "center", 6) == 0)
    cur_line->justify = L_CENTER;
  else
    cur_line->justify = L_LEFTRIGHT;
}
/* Define an area on the form that contains the current
   fvwm error message.
   syntax: *FFMessage Length nn, Font fn, Fore color, Back color
   len default is 70
   font,fg.bg default to text.
  */

static void ct_Message(char *cp)
{
  AddItem();
  bg_state = 'u';			/* indicate b/g color now used. */
  item->type = I_TEXT;
  /* Item now added to list of items, now it needs a pointer
     to the correct DrawTable. */
  AssignDrawTable(item);
  item->header.name = "FvwmMessage";	/* No purpose to this? dje */
  item->text.value = xmalloc(80);	/* point at last error recvd */
  item->text.n = 80;
  strcpy(item->text.value,"A mix of chars. MM20"); /* 20 mixed width chars */
  item->header.size_x = (FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
					  item->text.value,
					  item->text.n/4) * 4) + 2 * TEXT_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
    + CF.padVText;
  memset(item->text.value,' ',80);	/* Clear it out */
  myfprintf((stderr, "Message area [%d, %d]\n",
	     item->header.size_x, item->header.size_y));
  AddToLine(item);
  CF.last_error = item;			/* save location of message item */
}

/* allocate colors and fonts needed */
static void CheckAlloc(Item *this_item,DrawTable *dt)
{
  XGCValues xgcv;
  int xgcv_mask = GCForeground;

  if (dt->dt_used == 2) {		/* fonts colors shadows */
    return;
  }
  if (dt->dt_used == 0) {		/* if nothing allocated */
    dt->dt_colors[c_fg] = (colorset < 0)
      ? GetColor(dt->dt_color_names[c_fg])
      : Colorset[colorset].fg;
    dt->dt_colors[c_bg] = (colorset < 0)
      ? GetColor(dt->dt_color_names[c_bg])
      : Colorset[colorset].bg;
    xgcv.foreground = dt->dt_colors[c_fg];
    xgcv.background = dt->dt_colors[c_bg];
    if (dt->dt_Ffont->font != NULL)
    {
      xgcv_mask |= GCFont;
      xgcv.font = dt->dt_Ffont->font->fid;
    }
    xgcv_mask |= GCBackground;
    dt->dt_GC = fvwmlib_XCreateGC(dpy, CF.frame, xgcv_mask, &xgcv);

    dt->dt_used = 1;			/* fore/back font allocated */
  }
  if (this_item->type == I_TEXT
      || this_item->type == I_TIMEOUT) {      /* If no shadows needed */
    return;
  }
  dt->dt_colors[c_item_fg] = (itemcolorset < 0)
    ? GetColor(dt->dt_color_names[c_item_fg])
    : Colorset[itemcolorset].fg;
  dt->dt_colors[c_item_bg] = (itemcolorset < 0)
    ? GetColor(dt->dt_color_names[c_item_bg])
    : Colorset[itemcolorset].bg;
  xgcv.foreground = dt->dt_colors[c_item_fg];
  xgcv.background = dt->dt_colors[c_item_bg];
  xgcv_mask = GCForeground;
  if (dt->dt_Ffont->font != NULL)
  {
    xgcv_mask |= GCFont;
    xgcv.font = dt->dt_Ffont->font->fid;
  }
  dt->dt_item_GC = fvwmlib_XCreateGC(dpy, CF.frame, xgcv_mask, &xgcv);
  if (Pdepth < 2) {
    dt->dt_colors[c_itemlo] = PictureBlackPixel();
    dt->dt_colors[c_itemhi] = PictureWhitePixel();
  } else {
    dt->dt_colors[c_itemlo] = (itemcolorset < 0)
      ? GetShadow(dt->dt_colors[c_item_bg])
      : Colorset[itemcolorset].shadow;
    dt->dt_colors[c_itemhi] = (itemcolorset < 0)
      ? GetHilite(dt->dt_colors[c_item_bg])
      : Colorset[itemcolorset].hilite;
  }
  dt->dt_used = 2;		       /* fully allocated */
}


/* Input is the current item.  Assign a drawTable entry to it. */
static void AssignDrawTable(Item *adt_item)
{
  DrawTable *find_dt, *last_dt;
  char *match_text_fore;
  char *match_text_back;
  char *match_item_fore;
  char *match_item_back;
  char *match_font;
  DrawTable *new_dt;			/* pointer to a new one */

  match_text_fore = color_names[c_fg];
  match_text_back = color_names[c_bg];
  match_item_fore = color_names[c_item_fg];
  match_item_back = color_names[c_item_bg];

  switch(adt_item->type) {
  case I_TEXT:
  case I_SEPARATOR:			/* hack */
    match_font = font_names[f_text];
    break;
  case	I_TIMEOUT:
    match_font = font_names[f_timeout];
    break;
  case I_INPUT:
    match_font = font_names[f_input];
    break;
  default:
    match_font = font_names[f_button]; /* choices same as buttons */
    break;
  }
  last_dt = 0;
  for (find_dt = CF.roots_dt;		   /* start at front */
       find_dt != 0;			/* until end */
       find_dt = find_dt->dt_next) {	/* follow next pointer */
    last_dt = find_dt;
    if ((strcasecmp(match_text_fore,find_dt->dt_color_names[c_fg]) == 0) &&
	(strcasecmp(match_text_back,find_dt->dt_color_names[c_bg]) == 0) &&
	(strcasecmp(match_item_fore,find_dt->dt_color_names[c_item_fg]) == 0) &&
	(strcasecmp(match_item_back,find_dt->dt_color_names[c_item_bg]) == 0) &&
	(strcasecmp(match_font,find_dt->dt_font_name) == 0)) { /* Match */
      adt_item->header.dt_ptr = find_dt;       /* point item to drawtable */
      return;				/* done */
    } /* end match */
  } /* end all drawtables checked, no match */

  /* Time to add a DrawTable */
  /* get one */
  new_dt = xmalloc(sizeof(struct _drawtable));
  memset(new_dt, 0, sizeof(struct _drawtable));
  new_dt->dt_next = 0;			/* new end of list */
  if (CF.roots_dt == 0) {		   /* If first entry in list */
    CF.roots_dt = new_dt;		   /* set root pointer */
  } else {				/* not first entry */
    last_dt->dt_next = new_dt;		/* link old to new */
  }

  new_dt->dt_font_name = xstrdup(match_font);
  new_dt->dt_color_names[c_fg]	    = xstrdup(match_text_fore);
  new_dt->dt_color_names[c_bg]	    = xstrdup(match_text_back);
  new_dt->dt_color_names[c_item_fg] = xstrdup(match_item_fore);
  new_dt->dt_color_names[c_item_bg] = xstrdup(match_item_back);
  new_dt->dt_used = 0;			/* show nothing allocated */
  new_dt->dt_Ffont = FlocaleLoadFont(dpy, new_dt->dt_font_name, module->name);
  FlocaleAllocateWinString(&new_dt->dt_Fstr);

  myfprintf((stderr,"Created drawtable with %s %s %s %s %s\n",
	     new_dt->dt_color_names[c_fg], new_dt->dt_color_names[c_bg],
	     new_dt->dt_color_names[c_item_fg],
	     new_dt->dt_color_names[c_item_bg],
	     new_dt->dt_font_name));
  adt_item->header.dt_ptr = new_dt;	       /* point item to new drawtable */
}

/* input/output is global "item" - currently allocated last item */
static void AddItem(void)
{
  Item *save_item;
  save_item = (Item *)item;		/* save current item */
  item = xcalloc(sizeof(Item), 1); /* get a new item */
  if (save_item == 0) {			/* if first item */
    root_item_ptr = item;		/* save root item */
  } else {				/* else not first item */
    save_item->header.next = item;	/* link prior to new */
  }
}

static void ct_Separator(char *cp)
{
  AddItem();
  item->header.name = "";		/* null = create no variables in form */
  item->type = I_SEPARATOR;
  item->header.size_y = 2;		/* 2 pixels high */
  AssignDrawTable(item);
  AddToLine(item);
  myfprintf((stderr, "ct_separator command set max width %d\n",(int)item->header.size_x));
}

static void ct_Text(char *cp)
{
  /* syntax: *FFText "<text>" */
  AddItem();
  bg_state = 'u';			/* indicate b/g color now used. */
  item->type = I_TEXT;
  /* Item now added to list of items, now it needs a pointer
     to the correct DrawTable. */
  AssignDrawTable(item);
  item->header.name = "";
  if (*cp == '\"')
    item->text.value = CopyQuotedString(++cp);
  else
    item->text.value = "";
  item->text.n = strlen(item->text.value);

  item->header.size_x = FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
				   item->text.value,
				   item->text.n) + 2 * TEXT_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
     + CF.padVText;
  myfprintf((stderr, "Text \"%s\" [%d, %d]\n", item->text.value,
	     item->header.size_x, item->header.size_y));
  AddToLine(item);
}
/* Set the form's title.
   The default is the aliasname.
   If there is no quoted string, create a blank title. */
static void ct_Title(char *cp)
{
  /* syntax: *FFTitle "<text>" */
  if (*cp == '\"')
    CF.title = CopyQuotedString(++cp);
  else
    CF.title = "";
  myfprintf((stderr, "Title \"%s\"\n", CF.title));
}
static void ct_Timeout(char *cp)
{
  /* syntax: *FFTimeout seconds <Command> "Text" */
  char *tmpcp, *tmpbuf;

  if (timer != NULL) {
    fprintf(stderr,"Only one timeout per form allowed, skipped %s.\n",cp);
    return;
  }
  AddItem();
  bg_state = 'u';			/* indicate b/g color now used. */
  item->type = I_TIMEOUT;
  /* Item now added to list of items, now it needs a pointer
     to the correct DrawTable. */
  AssignDrawTable(item);
  item->header.name = "";

  item->timeout.timeleft = atoi(cp);
  if (item->timeout.timeleft < 0)
  {
    item->timeout.timeleft = 0;
  }
  else if (item->timeout.timeleft > 99999)
  {
    item->timeout.timeleft = 99999;
  }
  timer = item;

  while (*cp && *cp != '\n' && !isspace((unsigned char)*cp)) cp++;
							/* skip timeout */
  while (*cp && *cp != '\n' && isspace((unsigned char)*cp)) cp++;
							/* move up to command */

  if (!*cp || *cp == '\n') {
    fprintf(stderr,"Improper arguments specified for FvwmForm Timeout.\n");
    return;
  }

  if (*cp == '\"') {
    item->timeout.command = CopyQuotedString(++cp);
    /* skip over the whole quoted string to continue parsing */
    cp += strlen(item->timeout.command) + 1;
  }
  else {
    tmpbuf = xstrdup(cp);
    tmpcp = tmpbuf;
    while (!isspace((unsigned char)*tmpcp)) tmpcp++;
    *tmpcp = '\0';			  /* cutoff command at first word */
    item->timeout.command = xstrdup(tmpbuf);
    free(tmpbuf);
    while (!isspace((unsigned char)*cp)) cp++; /* move past command again */
  }

  while (*cp && *cp != '\n' && isspace((unsigned char)*cp)) cp++;
						/* move up to next arg */

  if (!*cp || *cp == '\n') {
    fprintf(stderr,"Improper arguments specified for FvwmForm Timeout.\n");
    return;
  }

  if (*cp == '\"') {
    item->timeout.text = CopyQuotedString(++cp);
  } else
    item->timeout.text = "";
  item->timeout.len = strlen(item->timeout.text);

  item->header.size_x = FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
				   item->timeout.text,
				   item->timeout.len) + 2 * TEXT_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
     + CF.padVText;
  myfprintf((stderr, "Timeout %d \"%s\" [%d, %d]\n", item->timeout.timeleft,
		item->timeout.text, item->header.size_x, item->header.size_y));
  AddToLine(item);
}
static void ct_padVText(char *cp)
{
  /* syntax: *FFText "<padVText pixels>" */
  CF.padVText = atoi(cp);
  myfprintf((stderr, "Text Vertical Padding %d\n", CF.padVText));
}
static void ct_Input(char *cp)
{
  int j;
  /* syntax: *FFInput <name> <size> "<init_value>" */
  AddItem();
  item->type = I_INPUT;
  AssignDrawTable(item);
  item->header.name = CopySolidString(cp);
  cp += strlen(item->header.name);
  while (isspace((unsigned char)*cp)) cp++;
  item->input.size = atoi(cp);
  while (!isspace((unsigned char)*cp)) cp++;
  while (isspace((unsigned char)*cp)) cp++;
  item->input.init_value = xstrdup("");	    /* init */
  if (*cp == '\"') {
    free(item->input.init_value);
    item->input.init_value = CopyQuotedString(++cp);
  }
  item->input.blanks = xmalloc(item->input.size);
  for (j = 0; j < item->input.size; j++)
    item->input.blanks[j] = ' ';
  item->input.buf = strlen(item->input.init_value) + 1;
  item->input.value = xmalloc(item->input.buf);
  item->input.value[0] = 0;		/* avoid reading unitialized data */

  item->header.size_x = item->header.dt_ptr->dt_Ffont->max_char_width
    * item->input.size + 2 * TEXT_SPC + 2 * BOX_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
    + 3 * TEXT_SPC + 2 * BOX_SPC;
  myfprintf((stderr,"Input size_y is %d\n",item->header.size_y));

  if (CF.cur_input == 0) {		   /* first input field */
    item->input.next_input = item;	/* ring, next field is first field */
    item->input.prev_input = item;	/* ring, prev field is first field */
    CF.first_input = item;		   /* save loc of first item */
  } else {				/* not first field */
    CF.cur_input->input.next_input = item; /* old next ptr point to this item */
    item->input.prev_input = CF.cur_input; /* current items prev is old item */
    item->input.next_input = CF.first_input; /* next is first item */
    CF.first_input->input.prev_input = item; /* prev in first is this item */
  }
  CF.cur_input = item;			   /* new current input item */
  myfprintf((stderr, "Input, %s, [%d], \"%s\"\n", item->header.name,
	  item->input.size, item->input.init_value));
  AddToLine(item);
}
static void ct_Read(char *cp)
{
  /* syntax: *FFRead 0 | 1 */
  myfprintf((stderr,"Got read command, char is %c\n",(int)*cp));
  endDefaultsRead = *cp;		/* copy whatever it is */
}
/* read and save vars from a file for later use in form
   painting.
*/
static void ct_UseData(char *cp)
{
  /* syntax: *FFUseData filename cmd_prefix */
  CF.file_to_read = CopySolidString(cp);
  if (*CF.file_to_read == 0) {
    fprintf(stderr,"UseData command missing first arg, File to read\n");
    return;
  }
  cp += strlen(CF.file_to_read);
  while (isspace((unsigned char)*cp)) cp++;
  CF.leading = CopySolidString(cp);
  if (*CF.leading == 0) {
    fprintf(stderr,"UseData command missing second arg, Leading\n");
    CF.file_to_read = 0;		   /* stop read */
    return;
  }
  /* Cant do the actual reading of the data file here,
     we are already in a readconfig loop. */
}
static void ReadFormData(void)
{
  int leading_len;
  char *line_buf;			/* ptr to curr config line */
  char cmd_buffer[200];
  sprintf(cmd_buffer,"read %s Quiet",CF.file_to_read);
  SendText(Channel,cmd_buffer,0);	/* read data */
  leading_len = strlen(CF.leading);
  InitGetConfigLine(Channel, CF.leading); /* ask for certain lines */
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* while there is some */
    if (strncasecmp(line_buf, CF.leading, leading_len) == 0) { /* leading = */
      if (line_buf[strlen(line_buf)-1] == '\n') { /* if line ends with nl */
	line_buf[strlen(line_buf)-1] = '\0'; /* strip off \n */
      }
      PutDataInForm(line_buf+leading_len); /* pass arg, space, value */
    } /* end match on arg 2, "leading" */
  } /* end while there is config data */
  free(CF.file_to_read);		/* dont need it anymore */
  free(CF.leading);
  CF.file_to_read = 0;
  CF.leading = 0;
}
/*
  Input is a line with varname space value.
  Search form for matching input fields and set values.
  If you don't get a match on an input field, try a choice.
*/
static void PutDataInForm(char *cp)
{
  char *var_name;
  char *var_value;
  int var_len, i;
  Item *item;
  Line *line;

  var_name = CopySolidString(cp);	/* var */
  if (*var_name == 0) {
    return;
  }
  cp += strlen(var_name);
  while (isspace((unsigned char)*cp)) cp++;
  var_value = cp;
  if (CF.cur_input != 0) {
    item = CF.cur_input;
    do {
      if (strcasecmp(var_name,item->header.name) == 0) {
	var_len = strlen(cp);
	if (item->input.init_value)
	  free(item->input.init_value);
	item->input.init_value = xmalloc(var_len + 1);
	strcpy(item->input.init_value,cp); /* new initial value in field */
	if (item->input.value)
	  free(item->input.value);
	item->input.buf = var_len+1;
	item->input.value = xmalloc(item->input.buf);
	strcpy(item->input.value,cp);	  /* new value in field */
	free(var_name);			/* goto's have their uses */
	return;
      }
      item=item->input.next_input;	  /* next input field */
    } while (item != CF.cur_input);	  /* while not end of ring */
  }
  /* You have a matching line, but it doesn't match an input
     field.  What to do?  I know, try a choice. */
  line = &root_line;			 /* start at first line */
  do {					/* for all lines */
    for (i = 0; i < line->n; i++) {	/* all items on line */
      item = line->items[i];
      if (item->type == I_CHOICE) {	/* choice is good */
	if (strcasecmp(var_name,item->header.name) == 0) { /* match */
	  item->choice.init_on = 0;
	  if (strncasecmp(cp, "on", 2) == 0) {
	    item->choice.init_on = 1;	/* set default state */
	    free(var_name);		/* goto's have their uses */
	    return;
	  }
	}
      }
    } /* end all items in line */
    line = line->next;			/* go to next line */
  } while (line != &root_line);		/* do all lines */
  /* You have a matching line, it didn't match an input field,
     and it didn't match a choice.  I've got it, it may match a
     selection, in which case we should use the value to
     set the choices! */
  for (item = root_item_ptr; item != 0;
       item = item->header.next) {/* all items */
    if (item->type == I_SELECT) {     /* selection is good */
      if (strcasecmp(var_name,item->header.name) == 0) { /* match */
	/* Now find the choices for this selection... */
	Item *choice_ptr;
	for (i = 0;
	     i<item->selection.n;
	     i++) {
	  choice_ptr=item->selection.choices[i];
	  /* if the choice value matches the selection */
	  if (strcmp(choice_ptr->choice.value,var_value) == 0) {
	    choice_ptr->choice.init_on = 1;
	  } else {
	    choice_ptr->choice.init_on = 0;
	  }
	} /* end all choices */
      } /* end match */
    } /* end selection */
  } /* end all items */
  free(var_name);			/* not needed now */
}
static void ct_Selection(char *cp)
{
  /* syntax: *FFSelection <name> single | multiple */
  AddItem();
  cur_sel = item;			/* save ptr as cur_sel */
  cur_sel->type = I_SELECT;
  cur_sel->header.name = CopySolidString(cp);
  cp += strlen(cur_sel->header.name);
  while (isspace((unsigned char)*cp)) cp++;
  if (strncasecmp(cp, "multiple", 8) == 0)
    cur_sel->selection.key = IS_MULTIPLE;
  else
    cur_sel->selection.key = IS_SINGLE;
}
static void ct_Choice(char *cp)
{
  /* syntax: *FFChoice <name> <value> [on | _off_] ["<text>"] */
  /* This next edit is a liitle weak, the selection should be right
     before the choice. At least a core dump is avoided. */
  if (cur_sel == 0) {			/* need selection for a choice */
    fprintf(stderr,"%s: Need selection for choice %s\n",
	    module->name, cp);
    return;
  }
  bg_state = 'u';			/* indicate b/g color now used. */
  AddItem();
  item->type = I_CHOICE;
  AssignDrawTable(item);
  item->header.name = CopySolidString(cp);
  cp += strlen(item->header.name);
  while (isspace((unsigned char)*cp)) cp++;
  item->choice.value = CopySolidString(cp);
  cp += strlen(item->choice.value);
  while (isspace((unsigned char)*cp)) cp++;
  if (strncasecmp(cp, "on", 2) == 0)
    item->choice.init_on = 1;
  else
    item->choice.init_on = 0;
  while (!isspace((unsigned char)*cp)) cp++;
  while (isspace((unsigned char)*cp)) cp++;
  if (*cp == '\"')
    item->choice.text = CopyQuotedString(++cp);
  else
    item->choice.text = "";
  item->choice.n = strlen(item->choice.text);
  item->choice.sel = cur_sel;

  if (cur_sel->selection.choices_array_count
      <= cur_sel->selection.n) {	   /* no room */
    cur_sel->selection.choices_array_count += CHOICES_PER_SEL_EXPANSION;
    cur_sel->selection.choices =
      xrealloc((void *)cur_sel->selection.choices,
               sizeof(Item *) * cur_sel->selection.choices_array_count,
	       sizeof(cur_sel->selection.choices)); /* expand */
  }

  cur_sel->selection.choices[cur_sel->selection.n++] = item;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
     + 2 * TEXT_SPC;
  /* this is weird, the x dimension is the sum of the height and width? */
  item->header.size_x = item->header.dt_ptr->dt_Ffont->height
     + 4 * TEXT_SPC +
     FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
       item->choice.text, item->choice.n);
  myfprintf((stderr, "Choice %s, \"%s\", [%d, %d]\n", item->header.name,
	  item->choice.text, item->header.size_x, item->header.size_y));
  AddToLine(item);
}
static void ct_Button(char *cp)
{
  /* syntax: *FFButton continue | restart | quit "<text>" */
  AddItem();
  item->type = I_BUTTON;
  AssignDrawTable(item);
  item->header.name = "";
  if (strncasecmp(cp, "restart", 7) == 0)
    item->button.key = IB_RESTART;
  else if (strncasecmp(cp, "quit", 4) == 0)
    item->button.key = IB_QUIT;
  else
    item->button.key = IB_CONTINUE;
  while (!isspace((unsigned char)*cp)) cp++;
  while (isspace((unsigned char)*cp)) cp++;
  if (*cp == '\"') {
    item->button.text = CopyQuotedString(++cp);
    cp += strlen(item->button.text) + 1;
    while (isspace((unsigned char)*cp)) cp++;
  } else
    item->button.text = "";
  if (*cp == '^')
    item->button.keypress = *(++cp) - '@';
  else if (*cp == 'F')
    item->button.keypress = 256 + atoi(++cp);
  else
    item->button.keypress = -1;
  item->button.len = strlen(item->button.text);
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
     + 2 * TEXT_SPC + 2 * BOX_SPC;
  item->header.size_x = 2 * TEXT_SPC + 2 * BOX_SPC
  + FlocaleTextWidth(item->header.dt_ptr->dt_Ffont, item->button.text,
    item->button.len);
  AddToLine(item);
  cur_button = item;
  myfprintf((stderr,"Created button, fore %s, bg %s, text %s\n",
	     item->header.dt_ptr->dt_color_names[c_item_fg],
	     item->header.dt_ptr->dt_color_names[c_item_bg],
	     item->button.text));
}
static void ct_Command(char *cp)
{
  /* syntax: *FFCommand <command> */
  if (cur_button->button.button_array_size <= cur_button->button.n)
  {
    cur_button->button.button_array_size += BUTTON_COMMAND_EXPANSION;
    cur_button->button.commands =
      xrealloc((void *)cur_button->button.commands,
               sizeof(char *) * cur_button->button.button_array_size,
	       sizeof(cur_button->button.commands));
  }
  cur_button->button.commands[cur_button->button.n++] = xstrdup(cp);
}

/* End of ct_ routines */

/* Init constants with values that can be freed later. */
static void InitConstants(void) {
  color_names[0]=xstrdup("Light Gray");
  color_names[1]=xstrdup("Black");
  color_names[2]=xstrdup("Gray50");
  color_names[3]=xstrdup("Wheat");
  font_names[0]=xstrdup("8x13bold");
  font_names[1]=xstrdup("8x13bold");
  font_names[2]=xstrdup("8x13bold");
  font_names[3]=xstrdup("8x13bold");
  screen_background_color=xstrdup("Light Gray");
  CF.p_c[input_fore].pointer_color.pixel = PictureWhitePixel();
  CF.p_c[input_back].pointer_color.pixel = PictureBlackPixel();
  CF.p_c[button_fore].pointer_color.pixel = PictureBlackPixel();
  CF.p_c[button_back].pointer_color.pixel = PictureWhitePixel();
  /* The in pointer is the reverse of the hover pointer. */
  CF.p_c[button_in_fore].pointer_color.pixel = PictureWhitePixel();
  CF.p_c[button_in_back].pointer_color.pixel = PictureBlackPixel();
}

/* read the configuration file */
static void ReadDefaults(void)
{
  char *line_buf;			/* ptr to curr config line */

  /* default button is for initial functions */
  cur_button = &CF.def_button;
  CF.def_button.button.key = IB_CONTINUE;

   /*
    * Reading .FvwmFormDefault for every form seems slow.
    *
    * This next bit puts a command at the end of the module command
    * queue in fvwm that indicates whether the file has to be read.
    *
    * Read defaults looking for "*FvwmFormDefaultRead"
    * if not there, send read,
    * then "*FvwmFormDefaultRead y",
    * then look thru defaults again.
    * The customization dialog sends "*FvwmFormDefaultRead n"
    * to start over.
    */
  InitGetConfigLine(Channel,"*FvwmFormDefault");
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* get config from fvwm */
    ParseDefaults(line_buf);		 /* process default config lines 1st */
  }
  if (endDefaultsRead == 'y') {		/* defaults read already */
    myfprintf((stderr,"Defaults read, no need to read file.\n"));
    return;
  } /* end defaults read already */
  SendText(Channel,"read .FvwmForm Quiet",0); /* read default config */
  SendText(Channel,"*FvwmFormDefaultRead y",0); /* remember you read it */

  InitGetConfigLine(Channel,"*FvwmFormDefault");
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* get config from fvwm */
    ParseDefaults(line_buf);		 /* process default config lines 1st */
  }
} /* done */

static void ReadConfig(void)
{
  char *line_buf;			/* ptr to curr config line */

  InitGetConfigLine(Channel,CatString3("*",module->name,0));
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* get config from fvwm */
    ParseConfigLine(line_buf);		/* process config lines */
  }
} /* done */

/* After this config is read, figure it out */
static void MassageConfig(void)
{
  int i, extra;
  Line *line;				/* for scanning form lines */

  if (CF.file_to_read) {		/* if theres a data file to read */
    ReadFormData();			/* go read it */
  }
  /* get the geometry right */
  CF.max_width = 0;
  CF.total_height = ITEM_VSPC;
  line = &root_line;			 /* start at first line */
  do {					/* for all lines */
    for (i = 0; i < line->n; i++) {
      line->items[i]->header.pos_y = CF.total_height;
      if (line->items[i]->header.size_y < line->size_y)
	line->items[i]->header.pos_y +=
	  (line->size_y - line->items[i]->header.size_y) / 2 + 1 ;
    } /* end all items in line */
    CF.total_height += ITEM_VSPC + line->size_y;
    line->size_x += (line->n + 1) * ITEM_HSPC;
    if (line->size_x > CF.max_width)
      CF.max_width = line->size_x;
    line = line->next;			/* go to next line */
  } while (line != &root_line);		/* do all lines */
  do {					/* note, already back at root_line */
    int width;
    switch (line->justify) {
    case L_LEFT:
      width = ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_RIGHT:
      width = CF.max_width - line->size_x + ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_CENTER:
      width = (CF.max_width - line->size_x) / 2 + ITEM_HSPC;
      for (i = 0; i < line->n; i++) {
	line->items[i]->header.pos_x = width;
	myfprintf((stderr, "Line Item[%d] @ (%d, %d)\n", i,
	       line->items[i]->header.pos_x, line->items[i]->header.pos_y));
	width += ITEM_HSPC + line->items[i]->header.size_x;
      }
      break;
    case L_LEFTRIGHT:
    /* count the number of inputs on the line - the extra space will be
     * shared amongst these if there are any, otherwise it will be added
     * as space in between the elements
     */
      extra = 0 ;
      for (i = 0 ; i < line->n ; i++) {
	if (line->items[i]->type == I_INPUT)
	  extra++ ;
      }
      if (extra == 0) {
	if (line->n < 2) {  /* same as L_CENTER */
	  width = (CF.max_width - line->size_x) / 2 + ITEM_HSPC;
	  for (i = 0; i < line->n; i++) {
	    line->items[i]->header.pos_x = width;
	    width += ITEM_HSPC + line->items[i]->header.size_x;
	  }
	} else {
	  extra = (CF.max_width - line->size_x) / (line->n - 1);
	  width = ITEM_HSPC;
	  for (i = 0; i < line->n; i++) {
	    line->items[i]->header.pos_x = width;
	    width += ITEM_HSPC + line->items[i]->header.size_x + extra;
	  }
	}
      } else {
	extra = (CF.max_width - line->size_x) / extra ;
	width = ITEM_HSPC ;
	for (i = 0 ; i < line->n ; i++) {
	  line->items[i]->header.pos_x = width ;
	  if (line->items[i]->type == I_INPUT)
	    line->items[i]->header.size_x += extra ;
	  width += ITEM_HSPC + line->items[i]->header.size_x ;
	}
      }
      break;
    }
    line = line->next;			/* go to next line */
  } while (line != &root_line);		 /* do all lines */
}

/* reset all the values (also done on first display) */
static void Restart(void)
{
  Item *item;

  CF.cur_input = NULL;
  CF.abs_cursor = CF.rel_cursor = 0;
  for (item = root_item_ptr; item != 0;
       item = item->header.next) {/* all items */
    switch (item->type) {
    case I_INPUT:
      if (!CF.cur_input)
	CF.cur_input = item;

      /* save old input values in a recall ring. */
      if (item->input.value && item->input.value[0] != 0) { /* ? to save */
	if (item->input.value_history_ptr == 0) {  /* no history yet */
	  item->input.value_history_ptr =
	    xcalloc(sizeof(char *), 50);
	  item->input.value_history_ptr[0] = xstrdup(item->input.value);
	  item->input.value_history_count = 1; /* next insertion point */
	  myfprintf((stderr,"Initial save of %s in slot 0\n",
		     item->input.value_history_ptr[0]));
	} else {			/* we have a history */
	  int prior;
	  prior = item->input.value_history_count - 1;
	  if (prior < 0) {
	    for (prior = VH_SIZE - 1;
		 CF.cur_input->input.value_history_ptr[prior] == 0;
		 --prior);		/* find last used slot */
	  }
	  myfprintf((stderr,"Prior is %d, compare %s to %s\n",
		     prior, item->input.value,
		     item->input.value_history_ptr[prior]));

	  if ( strcmp(item->input.value, item->input.value_history_ptr[prior])
	       != 0) {			/* different value */
	    if (item->input.value_history_ptr[item->input.value_history_count])
	    {
	      free(item->input.value_history_ptr[
			   item->input.value_history_count]);
	      myfprintf((stderr,"Freeing old item in slot %d\n",
			 item->input.value_history_count));
	    }
	    item->input.value_history_ptr[item->input.value_history_count] =
	      xstrdup(item->input.value); /* save value ptr in array */
	    myfprintf((stderr,"Save of %s in slot %d\n",
		       item->input.value,
		       item->input.value_history_count));

	    /* leave count pointing at the next insertion point. */
	    if (item->input.value_history_count < VH_SIZE - 1) { /* not full */
	      ++item->input.value_history_count;    /* next slot */
	    } else {
	      item->input.value_history_count = 0;  /* wrap around */
	    }
	  } /* end something different */
	} /* end have a history */
	myfprintf((stderr,"New history yankat %d\n",
		   item->input.value_history_yankat));
      } /* end something to save */
      item->input.value_history_yankat = item->input.value_history_count;
      item->input.n = strlen(item->input.init_value);
      strcpy(item->input.value, item->input.init_value);
      item->input.left = 0;
      break;
    case I_CHOICE:
      item->choice.on = item->choice.init_on;
      break;
    }
  }
}

/* redraw the frame */
void RedrawFrame(XEvent *pev)
{
	Item *item;
	Region region = None;

	Bool clear = False;
	if (FftSupport)
	{
		item = root_item_ptr;
		while(item != 0 && !clear)
		{
			if ((item->type == I_TEXT || item->type == I_CHOICE) &&
			    item->header.dt_ptr->dt_Ffont->fftf.fftfont != NULL)
				clear = True;
			item = item->header.next;
		}
		if (clear && pev)
		{
			XClearArea(
				dpy, CF.frame,
				pev->xexpose.x, pev->xexpose.y,
				pev->xexpose.width, pev->xexpose.height,
				False);
		}
		else
		{
			XClearWindow(dpy, CF.frame);
		}
	}
	if (pev)
	{
		XRectangle r;

		r.x = pev->xexpose.x;
		r.y =  pev->xexpose.y;
		r.width = pev->xexpose.width;
		r.height = pev->xexpose.height;
		region = XCreateRegion();
		XUnionRectWithRegion (&r, region, region);
	}
	for (item = root_item_ptr; item != 0;
	     item = item->header.next) {      /* all items */
		DrawTable *dt_ptr = item->header.dt_ptr;

		if (dt_ptr && dt_ptr->dt_Fstr)
		{
			if (region)
			{
				dt_ptr->dt_Fstr->flags.has_clip_region = True;
				dt_ptr->dt_Fstr->clip_region = region;
			}
			else
			{
				dt_ptr->dt_Fstr->flags.has_clip_region = False;
			}
		}
		switch (item->type) {
		case I_TEXT:
			RedrawText(item);
			break;
		case I_TIMEOUT:
			RedrawTimeout(item);
			break;
		case I_CHOICE:
			dt_ptr->dt_Fstr->win = CF.frame;
			dt_ptr->dt_Fstr->gc  = dt_ptr->dt_GC;
			dt_ptr->dt_Fstr->str = item->choice.text;
			dt_ptr->dt_Fstr->x   = item->header.pos_x
				+ TEXT_SPC + item->header.size_y;
			dt_ptr->dt_Fstr->y   = item->header.pos_y
				+ TEXT_SPC + dt_ptr->dt_Ffont->ascent;
			dt_ptr->dt_Fstr->len = item->choice.n;
			dt_ptr->dt_Fstr->flags.has_colorset = False;
			if (itemcolorset >= 0)
			{
				dt_ptr->dt_Fstr->colorset =
					&Colorset[itemcolorset];
				dt_ptr->dt_Fstr->flags.has_colorset
					= True;
			}
			FlocaleDrawString(dpy,
					  dt_ptr->dt_Ffont,
					  dt_ptr->dt_Fstr,
					  FWS_HAVE_LENGTH);
			break;
		case I_SEPARATOR:
			myfprintf((stderr, "redraw_frame calling RedrawSeparator\n"));
			RedrawSeparator(item);
			break;
		}
		if (dt_ptr && dt_ptr->dt_Fstr)
		{
			dt_ptr->dt_Fstr->flags.has_clip_region = False;
		}
	}
	if (region)
	{
		XDestroyRegion(region);
	}
}

void RedrawText(Item *item)
{
  char *p;

  CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
  item->header.dt_ptr->dt_Fstr->len = item->text.n;
  if ((p = memchr(item->text.value, '\0', item->header.dt_ptr->dt_Fstr->len))
      != NULL)
    item->header.dt_ptr->dt_Fstr->len = p - item->text.value;
  item->header.dt_ptr->dt_Fstr->win = CF.frame;
  item->header.dt_ptr->dt_Fstr->gc  = item->header.dt_ptr->dt_GC;
  item->header.dt_ptr->dt_Fstr->str = item->text.value;
  item->header.dt_ptr->dt_Fstr->x   = item->header.pos_x + TEXT_SPC;
  item->header.dt_ptr->dt_Fstr->y   = item->header.pos_y + ( CF.padVText / 2 ) +
    item->header.dt_ptr->dt_Ffont->ascent;
  item->header.dt_ptr->dt_Fstr->flags.has_colorset = False;
  if (colorset >= 0)
  {
    item->header.dt_ptr->dt_Fstr->colorset = &Colorset[colorset];
    item->header.dt_ptr->dt_Fstr->flags.has_colorset = True;
  }
  FlocaleDrawString(dpy,
		    item->header.dt_ptr->dt_Ffont,
		    item->header.dt_ptr->dt_Fstr, FWS_HAVE_LENGTH);
  return;
}

/*
 *  Draw horizontal lines to form a separator
 *
 */
static void RedrawSeparator(Item *item)
{

  item->header.size_x = CF.max_width - 6;
  CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
  if ( item->header.dt_ptr && item->header.dt_ptr->dt_colors[c_itemlo] ) { /* safety */
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,item->header.dt_ptr->dt_colors[c_itemlo]);
    XDrawLine(dpy, item->header.win,item->header.dt_ptr->dt_item_GC, 0, 0, item->header.size_x, 0);
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,item->header.dt_ptr->dt_colors[c_itemhi]);
    XDrawLine(dpy, item->header.win,item->header.dt_ptr->dt_item_GC, 0, 1, item->header.size_x, 1);

  } else {
    fprintf(stderr,"%s: Separators, no colors %p\n",module->name,item->header.dt_ptr);
  }
  return;
}

void RedrawTimeout(Item *item)
{
  char *p;
  char *tmpbuf, *tmpptr, *tmpbptr;
  int reallen;

  XClearArea(dpy, CF.frame,
	       item->header.pos_x, item->header.pos_y,
	       item->header.size_x, item->header.size_y,
	       False);

  tmpbuf = xmalloc(item->timeout.len + 6);
  tmpbptr = tmpbuf;
  for (tmpptr = item->timeout.text; *tmpptr != '\0' &&
		!(tmpptr[0] == '%' && tmpptr[1] == '%'); tmpptr++) {
    *tmpbptr = *tmpptr;
    tmpbptr++;
  }
  if (tmpptr[0] == '%') {
    tmpptr++; tmpptr++;
    sprintf(tmpbptr, "%d", item->timeout.timeleft);
    tmpbptr += strlen(tmpbptr);
  }
  for (; *tmpptr != '\0'; tmpptr++) {
    *tmpbptr = *tmpptr;
    tmpbptr++;
  }
  *tmpbptr = '\0';

  reallen = strlen(tmpbuf);
  item->header.size_x = FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
				   tmpbuf, reallen) + 2 * TEXT_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height + CF.padVText;

  CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
  item->header.dt_ptr->dt_Fstr->len = reallen;
  if ((p = memchr(item->timeout.text, '\0', item->header.dt_ptr->dt_Fstr->len))
      != NULL)
    item->header.dt_ptr->dt_Fstr->len = p - tmpbuf;
  item->header.dt_ptr->dt_Fstr->win = CF.frame;
  item->header.dt_ptr->dt_Fstr->gc  = item->header.dt_ptr->dt_GC;
  item->header.dt_ptr->dt_Fstr->flags.has_colorset = False;
  if (colorset >= 0)
  {
    item->header.dt_ptr->dt_Fstr->colorset = &Colorset[colorset];
    item->header.dt_ptr->dt_Fstr->flags.has_colorset = True;
  }
  if (item->header.dt_ptr->dt_Fstr->str != NULL)
    free(item->header.dt_ptr->dt_Fstr->str);
  item->header.dt_ptr->dt_Fstr->str = xstrdup(tmpbuf);
  item->header.dt_ptr->dt_Fstr->x   = item->header.pos_x + TEXT_SPC;
  item->header.dt_ptr->dt_Fstr->y   = item->header.pos_y + ( CF.padVText / 2 ) +
    item->header.dt_ptr->dt_Ffont->ascent;
  FlocaleDrawString(dpy,
		    item->header.dt_ptr->dt_Ffont,
		    item->header.dt_ptr->dt_Fstr, FWS_HAVE_LENGTH);
  free(tmpbuf);
  return;
}

/* redraw an item */
void RedrawItem (Item *item, int click, XEvent *pev)
{
  int dx, dy, len, x;
  static XSegment xsegs[4];
  XRectangle r,inter;
  Region region = None;
  Bool text_inter = True;
  DrawTable *dt_ptr = item->header.dt_ptr;

  /* Init intersection to size of the item. */
  inter.x = BOX_SPC + TEXT_SPC - 1;
  inter.y = BOX_SPC;
  inter.width = item->header.size_x - (2 * BOX_SPC) - 2 - TEXT_SPC;
  inter.height = (item->header.size_y - 1) - 2 * BOX_SPC + 1;

  myfprintf((stderr,"%s: RedrawItem expose x=%d/y=%d h=%d/w=%d\n",module->name,
	     (int)pev->xexpose.x,
	     (int)pev->xexpose.y,
	     (int)pev->xexpose.width,
	     (int)pev->xexpose.height));

  /* This is a slightly altered expose event from ReadXServer. */
  if (pev)
  {
	  r.x = pev->xexpose.x;
	  r.y =	 pev->xexpose.y;
	  r.width = pev->xexpose.width;
	  r.height = pev->xexpose.height;
	  text_inter = frect_get_intersection(
		  r.x, r.y, r.width, r.height,
		  inter.x,inter.y,inter.width,inter.height,
		  &inter);
  }
  else
  {
	  /* If its not an expose event, the area assume the
	     whole item is intersected. */
	  r.x = inter.x;
	  r.y = inter.y;
	  r.width = inter.width;
	  r.height = inter.height;
  }
  if (pev && text_inter && dt_ptr && dt_ptr->dt_Fstr)
  {
	  region = XCreateRegion();
	  XUnionRectWithRegion (&r, region, region);
	  if (region)
	  {
		  dt_ptr->dt_Fstr->flags.has_clip_region = True;
		  dt_ptr->dt_Fstr->clip_region = region;
	  }
	  else
	  {
		  dt_ptr->dt_Fstr->flags.has_clip_region = False;
	  }
  }

  switch (item->type) {
  case I_INPUT:
    if (!text_inter)
    {
	    return;
    }
    /* Create frame (pressed in): */
    dx = item->header.size_x - 1;
    dy = item->header.size_y - 1;
    /*fprintf(stderr,"GC: %lu\n", item->header.dt_ptr->dt_item_GC);*/
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		   item->header.dt_ptr->dt_colors[c_itemlo]);

    /* around 12/26/99, added XClearArea to this function.
       this was done to deal with display corruption during
       multifield paste.  dje */
    if (frect_get_intersection(
	    r.x, r.y, r.width, r.height,
	    inter.x, inter.y, inter.width, inter.height,
	    &inter))
    {
	    XClearArea(
		       dpy, item->header.win,
		       inter.x, inter.y, inter.width, inter.height, False);
    }

    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		   item->header.dt_ptr->dt_colors[c_itemhi]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);

    if (click) {
      x = BOX_SPC + TEXT_SPC +
	      FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
			       item->input.value, CF.abs_cursor) - 1;
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_item_bg]);
      XDrawLine(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		x, BOX_SPC, x, dy - BOX_SPC);
      myfprintf((stderr,"Line %d/%d - %d/%d (first)\n",
		     x, BOX_SPC, x, dy - BOX_SPC));
    }
    len = item->input.n - item->input.left;
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		   item->header.dt_ptr->dt_colors[c_item_fg]);
    item->header.dt_ptr->dt_Fstr->win = item->header.win;
    item->header.dt_ptr->dt_Fstr->gc  = item->header.dt_ptr->dt_item_GC;
    item->header.dt_ptr->dt_Fstr->flags.has_colorset = False;
    if (itemcolorset >= 0)
    {
      item->header.dt_ptr->dt_Fstr->colorset = &Colorset[itemcolorset];
      item->header.dt_ptr->dt_Fstr->flags.has_colorset = True;
    }
    if (len > item->input.size)
      len = item->input.size;
    else
    {
      item->header.dt_ptr->dt_Fstr->str = item->input.blanks;
      item->header.dt_ptr->dt_Fstr->x  = BOX_SPC + TEXT_SPC +
	      FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
			       item->input.blanks, len);
      item->header.dt_ptr->dt_Fstr->y	= BOX_SPC + TEXT_SPC
		  + item->header.dt_ptr->dt_Ffont->ascent;
      item->header.dt_ptr->dt_Fstr->len = item->input.size - len;
      FlocaleDrawString(dpy,
			item->header.dt_ptr->dt_Ffont,
			item->header.dt_ptr->dt_Fstr, FWS_HAVE_LENGTH);
    }
    item->header.dt_ptr->dt_Fstr->str = item->input.value;
    item->header.dt_ptr->dt_Fstr->x   = BOX_SPC + TEXT_SPC;
    item->header.dt_ptr->dt_Fstr->y   = BOX_SPC + TEXT_SPC
      + item->header.dt_ptr->dt_Ffont->ascent;
    item->header.dt_ptr->dt_Fstr->len = len;
    FlocaleDrawString(dpy,
		      item->header.dt_ptr->dt_Ffont,
		      item->header.dt_ptr->dt_Fstr, FWS_HAVE_LENGTH);
    if (item == CF.cur_input && !click) {
      x = BOX_SPC + TEXT_SPC +
	FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
			 item->input.value,CF.abs_cursor)
	- 1;
      XDrawLine(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		x, BOX_SPC, x, dy - BOX_SPC);
      myfprintf((stderr,"Line %d/%d - %d/%d\n",
		 x, BOX_SPC, x, dy - BOX_SPC));
    }
    myfprintf((stderr,"Just drew input field. click %d\n",(int)click));
    break;
  case I_CHOICE:
    dx = dy = item->header.size_y - 1;
    if (item->choice.on) {
	XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		       item->header.dt_ptr->dt_colors[c_item_fg]);
      if (item->choice.sel->selection.key == IS_MULTIPLE) {
	xsegs[0].x1 = 5, xsegs[0].y1 = 5;
	xsegs[0].x2 = dx - 5, xsegs[0].y2 = dy - 5;
	xsegs[1].x1 = 5, xsegs[1].y1 = dy - 5;
	xsegs[1].x2 = dx - 5, xsegs[1].y2 = 5;
	XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		      xsegs, 2);
      } else {
	XDrawArc(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		 5, 5, dx - 10, dy - 10, 0, 360 * 64);
      }
    } else
      XClearWindow(dpy, item->header.win);
    if (item->choice.on)
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemlo]);
    else
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemhi]);
    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);
    if (item->choice.on)
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemhi]);
    else
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemlo]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);
    break;
  case I_BUTTON:
    if (!text_inter)
    {
	    return;
    }
    dx = item->header.size_x - 1;
    dy = item->header.size_y - 1;
    if (click)
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemlo]);
    else
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemhi]);
    xsegs[0].x1 = 0, xsegs[0].y1 = 0;
    xsegs[0].x2 = 0, xsegs[0].y2 = dy;
    xsegs[1].x1 = 0, xsegs[1].y1 = 0;
    xsegs[1].x2 = dx, xsegs[1].y2 = 0;
    xsegs[2].x1 = 1, xsegs[2].y1 = 1;
    xsegs[2].x2 = 1, xsegs[2].y2 = dy - 1;
    xsegs[3].x1 = 1, xsegs[3].y1 = 1;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = 1;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);
    if (click)
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemhi]);
    else
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		     item->header.dt_ptr->dt_colors[c_itemlo]);
    xsegs[0].x1 = 1, xsegs[0].y1 = dy;
    xsegs[0].x2 = dx, xsegs[0].y2 = dy;
    xsegs[1].x1 = 2, xsegs[1].y1 = dy - 1;
    xsegs[1].x2 = dx, xsegs[1].y2 = dy - 1;
    xsegs[2].x1 = dx, xsegs[2].y1 = 1;
    xsegs[2].x2 = dx, xsegs[2].y2 = dy;
    xsegs[3].x1 = dx - 1, xsegs[3].y1 = 2;
    xsegs[3].x2 = dx - 1, xsegs[3].y2 = dy;
    XDrawSegments(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		  xsegs, 4);
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
		   item->header.dt_ptr->dt_colors[c_item_fg]);
    item->header.dt_ptr->dt_Fstr->win = item->header.win;
    item->header.dt_ptr->dt_Fstr->gc  = item->header.dt_ptr->dt_item_GC;
    item->header.dt_ptr->dt_Fstr->flags.has_colorset = False;
    if (itemcolorset >= 0)
    {
      item->header.dt_ptr->dt_Fstr->colorset = &Colorset[itemcolorset];
      item->header.dt_ptr->dt_Fstr->flags.has_colorset = True;
    }
    item->header.dt_ptr->dt_Fstr->str = item->button.text;
    item->header.dt_ptr->dt_Fstr->x   = BOX_SPC + TEXT_SPC;
    item->header.dt_ptr->dt_Fstr->y   = BOX_SPC + TEXT_SPC
      + item->header.dt_ptr->dt_Ffont->ascent;
    item->header.dt_ptr->dt_Fstr->len = item->button.len;
    if (FftSupport)
    {
      if (item->header.dt_ptr->dt_Ffont->fftf.fftfont != NULL)
	XClearArea(dpy, item->header.win,
		   inter.x, inter.y, inter.width, inter.height, False);
    }
    FlocaleDrawString(dpy,
		      item->header.dt_ptr->dt_Ffont,
		      item->header.dt_ptr->dt_Fstr, FWS_HAVE_LENGTH);
    myfprintf((stderr,"Just put %s into a button\n",
	       item->button.text));
    break;
  case I_SEPARATOR:
    RedrawSeparator(item);
    break;
  }
  if (dt_ptr && dt_ptr->dt_Fstr)
  {
	  dt_ptr->dt_Fstr->flags.has_clip_region = False;
  }
  if (region)
  {
	  XDestroyRegion(region);
  }
  XFlush(dpy);
}

/* update transparency if background colorset is transparent */
/* window has moved redraw the background if it is transparent */
void UpdateRootTransapency(Bool pr_only, Bool do_draw)
{
	Item *item;

	if (CSET_IS_TRANSPARENT(colorset))
	{
		if (CSET_IS_TRANSPARENT_PR_PURE(colorset))
		{
			XClearArea(dpy, CF.frame, 0,0,0,0, False);
			if (do_draw)
			{
				RedrawFrame(NULL);
			}
		}
		else if (!pr_only || CSET_IS_TRANSPARENT_PR(colorset))
		{
			SetWindowBackground(
				dpy, CF.frame, CF.max_width, CF.total_height,
				&Colorset[(colorset)], Pdepth,
				root_item_ptr->header.dt_ptr->dt_GC, False);
			if (do_draw)
			{
				XClearArea(dpy, CF.frame, 0,0,0,0, False);
				RedrawFrame(NULL);
			}
		}
	}
	if (!root_item_ptr->header.next || !CSET_IS_TRANSPARENT(itemcolorset))
	{
		return;
	}
	for (item = root_item_ptr->header.next; item != NULL;
	     item = item->header.next)
	{
		if (item->header.win != None)
		{
			if (CSET_IS_TRANSPARENT_PR(itemcolorset) &&
			    !CSET_IS_TRANSPARENT(colorset))
			{
				continue;
			}
			if (CSET_IS_TRANSPARENT_PR_PURE(itemcolorset))
			{
				XClearArea(
					dpy, item->header.win, 0,0,0,0, False);
				if (do_draw)
				{
					RedrawItem(item, 0, NULL);
				}
			}
			else if (!pr_only ||
				 CSET_IS_TRANSPARENT_PR(itemcolorset))
			{
				SetWindowBackground(
					dpy, item->header.win,
					item->header.size_x, item->header.size_y,
					&Colorset[(itemcolorset)], Pdepth,
					item->header.dt_ptr->dt_GC, False);
				XClearArea(
					dpy, item->header.win, 0,0,0,0, False);
				if (do_draw)
				{
					RedrawItem(item, 0, NULL);
				}
			}
		}
	}
}

/* execute a command */
void DoCommand (Item *cmd)
{
  int k, dn;
  char *sp;
  Item *item;

  /* pre-command */
  if (cmd->button.key == IB_QUIT)
  {
    if (!XWithdrawWindow(dpy, CF.frame, screen))
    {
      /* hm, what can we do now? just ignore this situation. */
    }
  }

  for (k = 0; k < cmd->button.n; k++) {
    char *parsed_command;
    /* construct command */
    parsed_command = ParseCommand(0, cmd->button.commands[k], '\0', &dn, &sp);
    myfprintf((stderr, "Final command[%d]: [%s]\n", k, parsed_command));

    /* send command */
    if ( parsed_command[0] == '!') {	/* If command starts with ! */
      int n;

      n = system(parsed_command+1);		/* Need synchronous execution */
      (void)n;
    } else {
      SendText(Channel,parsed_command, ref);
    }
  }

  /* post-command */
  if (CF.last_error) {			/* if form has last_error field */
    memset(CF.last_error->text.value, ' ', CF.last_error->text.n); /* clear */
    /* To do this more elegantly, the window resize logic should recalculate
       size_x for the Message as the window resizes.  Right now, just clear
       a nice wide area. dje */
    XClearArea(dpy,CF.frame,
	       CF.last_error->header.pos_x,
	       CF.last_error->header.pos_y,
	       /* CF.last_error->header.size_x, */
	       2000,
	       CF.last_error->header.size_y, False);
  } /* end form has last_error field */
  if (cmd->button.key == IB_QUIT) {
    if (CF.grab_server)
      XUngrabServer(dpy);
    /* This is a temporary bug workaround for the pipe drainage problem */
    SendQuitNotification(Channel);    /* let commands complete */
    /* Note how the window is withdrawn, but execution continues until
       the quit notifcation catches up with this module...
       Should not be a problem, there shouldn't be any more commands
       coming into FvwmForm.  dje */
  }
  else if (cmd->button.key == IB_RESTART) {
    Restart();
    for (item = root_item_ptr; item != 0;
	 item = item->header.next) {	/* all items */
      if (item->type == I_INPUT) {
	XClearWindow(dpy, item->header.win);
	RedrawItem(item, 0, NULL);
      }
      if (item->type == I_CHOICE)
	RedrawItem(item, 0, NULL);
    }
  }
}

/* open the windows */
static void OpenWindows(void)
{
  int x, y;
  int gravity = NorthWestGravity;
  Item *item;
  static XSetWindowAttributes xswa;
  static XWMHints wmh = { InputHint, True };
  static XSizeHints sh =
    { PPosition | PSize | USPosition | USSize | PWinGravity};
  XClassHint myclasshints;

  if (!CF.pointer[input_pointer]) {
    CF.pointer[input_pointer] = XCreateFontCursor(dpy, XC_xterm);
  }
  if (!CF.pointer[button_in_pointer]) {
    CF.pointer[button_in_pointer] = XCreateFontCursor(dpy, XC_hand2);
  }
  if (!CF.pointer[button_pointer]) {
    CF.pointer[button_pointer] = XCreateFontCursor(dpy,XC_hand2);
  }
  CF.screen_background = (colorset < 0)
    ? GetColor(screen_background_color)
    : Colorset[colorset].bg;

  if (!CF.p_c[input_back].used) {  /* if not set, use screen b/g */
    CF.p_c[input_back].pointer_color.pixel = CF.screen_background;
  }
  myfprintf((stderr,
	     "screen bg %X, getcolor bg %X, colorset bg %X colorset %d\n",
	     (int)CF.screen_background,
	     (int)GetColor(screen_background_color),
	     (int)Colorset[colorset].bg,
	     (int)colorset));
  XQueryColor(dpy, Pcmap, &CF.p_c[input_fore].pointer_color);
  XQueryColor(dpy, Pcmap, &CF.p_c[input_back].pointer_color);
  XRecolorCursor(dpy, CF.pointer[input_pointer],
		 &CF.p_c[input_fore].pointer_color,
		 &CF.p_c[input_back].pointer_color);
  myfprintf((stderr,"input fore %X, back %X\n",
	  (int)CF.p_c[input_fore].pointer_color.pixel,
	  (int)CF.p_c[input_back].pointer_color.pixel));
  /* The input cursor is handled differently than the 2 button cursors. */
  XQueryColor(dpy, Pcmap, &CF.p_c[button_fore].pointer_color);
  XQueryColor(dpy, Pcmap, &CF.p_c[button_back].pointer_color);
  XRecolorCursor(dpy, CF.pointer[button_pointer],
		 &CF.p_c[button_fore].pointer_color,
		 &CF.p_c[button_back].pointer_color);
  myfprintf((stderr,"button fore %X, back %X\n",
	  (int)CF.p_c[button_fore].pointer_color.pixel,
	  (int)CF.p_c[button_back].pointer_color.pixel));
  XQueryColor(dpy, Pcmap, &CF.p_c[button_in_fore].pointer_color);
  XQueryColor(dpy, Pcmap, &CF.p_c[button_in_back].pointer_color);
  XRecolorCursor(dpy, CF.pointer[button_in_pointer],
		 &CF.p_c[button_in_fore].pointer_color,
		 &CF.p_c[button_in_back].pointer_color);
  myfprintf((stderr,"button in fore %X, back %X\n",
	  (int)CF.p_c[button_in_fore].pointer_color.pixel,
	  (int)CF.p_c[button_in_back].pointer_color.pixel));
  /* the frame window first */
  if (CF.have_geom)
  {
    if (CF.xneg)
    {
      x = DisplayWidth(dpy, screen) - CF.max_width + CF.gx;
      gravity = NorthEastGravity;
    }
    else
    {
      x = CF.gx;
    }
    if (CF.yneg)
    {
      y = DisplayHeight(dpy, screen) - CF.total_height + CF.gy;
      gravity = SouthWestGravity;
    }
    else
    {
      y = CF.gy;
    }
    if (CF.xneg && CF.yneg)
    {
      gravity = SouthEastGravity;
    }
  } else {
    FScreenCenterOnScreen(
      NULL, FSCREEN_CURRENT, &x, &y, CF.max_width, CF.total_height);
  }
  /* hack to prevent mapping on wrong screen with StartsOnScreen */
  FScreenMangleScreenIntoUSPosHints(FSCREEN_CURRENT, &sh);
  xswa.background_pixel = CF.screen_background;
  xswa.border_pixel = 0;
  xswa.colormap = Pcmap;
  myfprintf((stderr,
	  "going to create window w. bg %s, b/g pixel %X, black pixel %X\n",
	     screen_background_color,
	     (int)xswa.background_pixel,
	     (int)BlackPixel(dpy, screen)));
  CF.frame = XCreateWindow(dpy, root, x, y, CF.max_width, CF.total_height, 0,
			   Pdepth, InputOutput, Pvisual,
			   CWColormap | CWBackPixel | CWBorderPixel, &xswa);
  wm_del_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(dpy, CF.frame, &wm_del_win, 1);
  XSelectInput(dpy, CF.frame,
	       KeyPressMask | ExposureMask | StructureNotifyMask |
	       VisibilityChangeMask);
  if (!CF.title) {
    CF.title = module->name;
  }
  XStoreName(dpy, CF.frame, CF.title);
  XSetWMHints(dpy, CF.frame, &wmh);
  myclasshints.res_name = module->name;
  myclasshints.res_class = "FvwmForm";
  XSetClassHint(dpy,CF.frame,&myclasshints);
  sh.width = CF.max_width;
  sh.height = CF.total_height;
  sh.win_gravity = gravity;
  XSetWMNormalHints(dpy, CF.frame, &sh);

  for (item = root_item_ptr; item != 0;
       item = item->header.next) {	/* all items */
    switch (item->type) {
    case I_INPUT:
      myfprintf((stderr,"Checking alloc during OpenWindow on input\n"));
      CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
      item->header.win =
	XCreateSimpleWindow(dpy, CF.frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_x, item->header.size_y,
			    0, CF.screen_background,
			    item->header.dt_ptr->dt_colors[c_item_bg]);
      XSelectInput(dpy, item->header.win, ButtonPressMask | ExposureMask);
      xswa.cursor = CF.pointer[input_pointer];
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      if (itemcolorset >= 0)
      {
	SetWindowBackground(dpy, item->header.win,
			    item->header.size_x, item->header.size_y,
			    &Colorset[(itemcolorset)], Pdepth,
			    item->header.dt_ptr->dt_GC, True);
      }
      break;
    case I_CHOICE:
      myfprintf((stderr,"Checking alloc during Openwindow on choice\n"));
      CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
      item->header.win =
	XCreateSimpleWindow(dpy, CF.frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_y, item->header.size_y,
			    0, CF.screen_background,
			    item->header.dt_ptr->dt_colors[c_item_bg]);
      XSelectInput(dpy, item->header.win, ButtonPressMask | ExposureMask);
      xswa.cursor = CF.pointer[button_pointer];
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      if (itemcolorset >= 0)
      {
	SetWindowBackground(dpy, item->header.win,
			    item->header.size_x, item->header.size_y,
			    &Colorset[(itemcolorset)], Pdepth,
			    item->header.dt_ptr->dt_GC, True);
      }
      break;
    case I_BUTTON:
      myfprintf((stderr,"Checking alloc during Openwindow on button\n"));
      CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
      item->header.win =
	XCreateSimpleWindow(dpy, CF.frame,
			    item->header.pos_x, item->header.pos_y,
			    item->header.size_x, item->header.size_y,
			    0, CF.screen_background,
			    item->header.dt_ptr->dt_colors[c_item_bg]);
      XSelectInput(dpy, item->header.win,
		   ButtonPressMask | ExposureMask);
      xswa.cursor = CF.pointer[button_pointer];
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      if (itemcolorset >= 0)
      {
	SetWindowBackground(dpy, item->header.win,
			    item->header.size_x, item->header.size_y,
			    &Colorset[(itemcolorset)], Pdepth,
			    item->header.dt_ptr->dt_GC, True);
      }
      break;
    case I_SEPARATOR:
      myfprintf((stderr,"Checking alloc during OpenWindow on separator\n"));
      CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
      item->header.size_x = CF.max_width - 6;
      myfprintf((stderr,"Create win %d/%d %d/%d\n",3,item->header.pos_y,
			    item->header.size_x, 2));
      item->header.win =
	XCreateSimpleWindow(dpy, CF.frame,
			    3, item->header.pos_y,
			    item->header.size_x, 2,
			    0, CF.screen_background,
			    item->header.dt_ptr->dt_colors[c_bg]);
      XSelectInput(dpy, item->header.win, ExposureMask);
      if (itemcolorset >= 0)
      {
	SetWindowBackground(dpy, item->header.win,
			    item->header.size_x, 2,
			    &Colorset[(itemcolorset)], Pdepth,
			    item->header.dt_ptr->dt_GC, True);
      }
      break;
    }
  }
  Restart();
  if (colorset >= 0)
  {
    CheckAlloc(root_item_ptr,root_item_ptr->header.dt_ptr);
    SetWindowBackground(dpy, CF.frame, CF.max_width, CF.total_height,
			&Colorset[(colorset)], Pdepth,
			root_item_ptr->header.dt_ptr->dt_GC, True);
  }
  if (preload_yorn != 'y') {		/* if not a preload */
    XMapRaised(dpy, CF.frame);
    XMapSubwindows(dpy, CF.frame);
    if (CF.warp_pointer) {
      FWarpPointer(dpy, None, CF.frame, 0, 0, 0, 0,
		   CF.max_width / 2, CF.total_height - 1);
    }
  }
  DoCommand(&CF.def_button);
}

static void process_message(unsigned long, unsigned long *); /* proto */
static void ParseActiveMessage(char *); /* proto */

/* read something from Fvwm */
static void ReadFvwm(void)
{

    FvwmPacket* packet = ReadFvwmPacket(Channel[1]);
    if ( packet == NULL )
	exit(0);
    else
	process_message( packet->type, packet->body );
}
static void process_message(unsigned long type, unsigned long *body)
{
  switch (type) {
  case M_CONFIG_INFO:			/* any module config command */
    myfprintf((stderr,"process_message: Got command: %s\n", (char *)&body[3]));
    ParseActiveMessage((char *)&body[3]);
    break;
  case MX_PROPERTY_CHANGE:
    if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
	((!Swallowed && body[2] == 0) || (Swallowed && body[2] == CF.frame)))
    {
      UpdateRootTransapency(True, True);
    }
    else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW && body[2] == CF.frame)
    {
	Swallowed = body[1];
    }
    break;
  case M_ERROR:
  case M_STRING:
    if (CF.last_error) {		/* if form has message area */
      /* ignore form size, its OK to write outside the window boundary */
      int msg_len;
      char *msg_ptr;
      /* Clear old message first */
      memset(CF.last_error->text.value, ' ', CF.last_error->text.n); /* clear */
      XClearArea(dpy,CF.frame,
		 CF.last_error->header.pos_x,
		 CF.last_error->header.pos_y,
		 2000,
		 CF.last_error->header.size_y, False);
      msg_ptr = (char *)&body[3];
      msg_len = strlen(msg_ptr);
      if (msg_ptr[msg_len-1] == '\n') { /* line ends w newline */
	msg_ptr[msg_len-1] = '\0'; /* strip off \n */
      }
      if (CF.last_error->text.n <= msg_len) { /* if message wont fit */
	CF.last_error->text.value = xrealloc(CF.last_error->text.value,
                                             msg_len * 2,
					     sizeof(CF.last_error->text.value));
	CF.last_error->text.n = msg_len * 2;
      }
      strncpy(CF.last_error->text.value,msg_ptr,
	      CF.last_error->text.n);
      RedrawText(CF.last_error);
      break;
    } /* module has last_error field */
  } /* end switch header */
}

/* These are the message from fvwm FvwmForm understands after form is
   active. */
static void am_Map(char *);
static void am_UnMap(char *);
static void am_Stop(char *);
static struct CommandTable am_table[] =
{
  {"Map",am_Map},
  {"Stop",am_Stop},
  {"UnMap",am_UnMap}
};

/* This is similar to the other 2 "Parse" functions. */
static void ParseActiveMessage(char *buf)
{
	char *p;
	struct CommandTable *e;
	if (buf[strlen(buf)-1] == '\n')
	{     /* if line ends with newline */
		buf[strlen(buf)-1] = '\0';  /* strip off \n */
	}

	if (strncasecmp(buf, "Colorset", 8) == 0)
	{
		Item *item;
		int n = LoadColorset(&buf[8]);
		if(n == colorset || n == itemcolorset)
		{
			for (item = root_item_ptr; item != 0;
			     item = item->header.next)
			{
				DrawTable *dt_ptr = item->header.dt_ptr;
				if (dt_ptr)
				{
					dt_ptr->dt_used = 0;
					if(dt_ptr->dt_GC)
					{
						XFreeGC(dpy, dt_ptr->dt_GC);
						dt_ptr->dt_GC = None;
					}
					if(dt_ptr->dt_item_GC)
					{
						XFreeGC(
							dpy,
							dt_ptr->dt_item_GC);
						dt_ptr->dt_item_GC = None;
					}
				}
			}
			for (item = root_item_ptr; item != 0;
			     item = item->header.next)
			{
				DrawTable *dt_ptr = item->header.dt_ptr;
				if (dt_ptr)
				{
					CheckAlloc(item, dt_ptr);
				}
			}
			if (colorset >= 0)
			{
				SetWindowBackground(
					dpy, CF.frame, CF.max_width,
					CF.total_height,
					&Colorset[(colorset)], Pdepth,
					root_item_ptr->header.dt_ptr->dt_GC,
					False);
				RedrawFrame(NULL);
			}
			for (item = root_item_ptr->header.next; item != 0;
			     item = item->header.next)
			{
				DrawTable *dt_ptr = item->header.dt_ptr;
				if (dt_ptr && itemcolorset >= 0 &&
				    item->header.win != 0)
				{
					SetWindowBackground(
						dpy, item->header.win,
						item->header.size_x,
						item->header.size_y,
						&Colorset[(itemcolorset)],
						Pdepth, dt_ptr->dt_GC, True);
					RedrawItem(item, 0, NULL);
				} /* end item has a drawtable */
			} /* end all items */
		}
		return;
	} /* end colorset command */
	if (strncasecmp(
		buf, XINERAMA_CONFIG_STRING, sizeof(XINERAMA_CONFIG_STRING)-1)
	    == 0)
	{
		FScreenConfigureModule(buf + sizeof(XINERAMA_CONFIG_STRING)-1);
		return;
	}
	if (
		strncasecmp(
			buf, CatString3("*",module->name,0),
			module->namelen+1) != 0)
	{
		/* If its not for me */
		return;
	} /* Now I know its for me. */
	p = buf+module->namelen+1;		    /* jump to end of my name */
	/* at this point we have recognized "*FvwmForm" */
	e = FindToken(p,am_table,struct CommandTable);/* find cmd in table */
	if (e == 0)
	{
		/* if no match */
		/* this may be a configuration command of another same form */
		if (FindToken(p, ct_table, struct CommandTable) == 0)
			fprintf(
				stderr,"%s: Active command unknown: %s\n",
				module->name,buf);
		return;				    /* ignore it */
	}

	p=p+strlen(e->name);			/* skip over name */
	while (isspace((unsigned char)*p)) p++; /* skip whitespace */

	FormVarsCheck(&p);
	e->function(p);			      /* call cmd processor */
	return;
} /* end function */

static void am_Map(char *cp)
{
  XMapRaised(dpy, CF.frame);
  XMapSubwindows(dpy, CF.frame);
  if (CF.warp_pointer) {
    FWarpPointer(dpy, None, CF.frame, 0, 0, 0, 0,
		 CF.max_width / 2, CF.total_height - 1);
  }
  myfprintf((stderr, "Map: got it\n"));
}
static void am_UnMap(char *cp)
{
  XUnmapWindow(dpy, CF.frame);
  myfprintf((stderr, "UnMap: got it\n"));
}
static void am_Stop(char *cp)
{
  /* syntax: *FFStop */
  myfprintf((stderr,"Got stop command.\n"));
  exit (0);				/* why bother, just exit. */
}

/* main event loop */
static void MainLoop(void)
{
  fd_set fds;
  fd_set_size_t fd_width = GetFdWidth();

  while ( !isTerminated ) {
    /* TA:  20091219:  Automatically flush the buffer from the XServer and
     * process each request as we receive them.
     */
    while(FPending(dpy))
	    ReadXServer();

    FD_ZERO(&fds);
    FD_SET(Channel[1], &fds);
    FD_SET(fd_x, &fds);

    /* TA:  20091219:  Using XFlush() here was always a nasty hack!  See
     * comments above.
     */
    /*XFlush(dpy);*/
    if (fvwmSelect(fd_width, &fds, NULL, NULL, NULL) > 0) {
      if (FD_ISSET(Channel[1], &fds))
	ReadFvwm();
      if (FD_ISSET(fd_x, &fds))
	ReadXServer();
    }
  }
}


/* signal-handler to make the application quit */
static RETSIGTYPE
TerminateHandler(int sig)
{
  fvwmSetTerminate(sig);
  SIGNAL_RETURN;
}

/* signal-handler to make the timer work */
static RETSIGTYPE
TimerHandler(int sig)
{
  int dn;
  char *sp;
  char *parsed_command;

  timer->timeout.timeleft--;
  if (timer->timeout.timeleft <= 0) {
    /* pre-command */
    if (!XWithdrawWindow(dpy, CF.frame, screen))
    {
      /* hm, what can we do now? just ignore this situation. */
    }

    /* construct command */
    parsed_command = ParseCommand(0, timer->timeout.command, '\0', &dn, &sp);
    myfprintf((stderr, "Final command: %s\n", parsed_command));

    /* send command */
    if ( parsed_command[0] == '!') {	/* If command starts with ! */
      int n;

      n = system(parsed_command+1);		/* Need synchronous execution */
      (void)n;
    } else {
      SendText(Channel,parsed_command, ref);
    }

    /* post-command */
    if (CF.last_error) {		  /* if form has last_error field */
      memset(CF.last_error->text.value, ' ', CF.last_error->text.n); /* clear */
      /* To do this more elegantly, the window resize logic should recalculate
	 size_x for the Message as the window resizes.	Right now, just clear
	 a nice wide area. dje */
      XClearArea(dpy,CF.frame,
		 CF.last_error->header.pos_x,
		 CF.last_error->header.pos_y,
		 /* CF.last_error->header.size_x, */
		 2000,
		 CF.last_error->header.size_y, False);
    } /* end form has last_error field */
    if (CF.grab_server)
      XUngrabServer(dpy);
    /* This is a temporary bug workaround for the pipe drainage problem */
    SendQuitNotification(Channel);    /* let commands complete */
    /* Note how the window is withdrawn, but execution continues until
       the quit notifcation catches up with this module...
       Should not be a problem, there shouldn't be any more commands
       coming into FvwmForm.  dje */
  }
  else {
    RedrawTimeout(timer);
    alarm(1);
  }

  SIGNAL_RETURN;
}


/* main procedure */
int main (int argc, char **argv)
{
  int i;
  char cmd[200];

#ifdef DEBUGTOFILE
  freopen(".FvwmFormDebug","w",stderr);
#endif

  FlocaleInit(LC_CTYPE, "", "", "FvwmForm");

  module = ParseModuleArgs(argc,argv,1); /* allow an alias */
  if (module == NULL)
  {
    fprintf(
	    stderr,
	    "FvwmForm Version "VERSION" should only be executed by fvwm!\n");
    exit(1);
  }

#ifdef HAVE_SIGACTION
  {
    struct sigaction  sigact;

#ifdef SA_INTERRUPT
    sigact.sa_flags = SA_INTERRUPT;
#else
    sigact.sa_flags = 0;
#endif
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGTERM);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigact.sa_handler = TerminateHandler;

    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);
  }
#else
#ifdef USE_BSD_SIGNALS
  fvwmSetSignalMask( sigmask(SIGPIPE) | sigmask(SIGTERM) | sigmask(SIGINT) );
#endif
  signal(SIGPIPE, TerminateHandler);  /* Dead pipe == Fvwm died */
  signal(SIGTERM, TerminateHandler);
  signal(SIGINT, TerminateHandler);
#ifdef HAVE_SIGINTERRUPT
  siginterrupt(SIGPIPE, 1);
  siginterrupt(SIGTERM, 1);
  siginterrupt(SIGINT, 1);
#endif
#endif

  Channel[0] = module->to_fvwm;
  Channel[1] = module->from_fvwm;

  dpy = XOpenDisplay("");
  if (dpy==NULL) {
    fprintf(stderr,"%s: could not open display\n",module->name);
    exit(1);
  }
  /* From FvwmAnimate end */

  i = 7;
  if (argc >= 8) {			/* if have arg 7 */
    if (strcasecmp(argv[7],"preload") == 0) { /* if its preload */
      preload_yorn = 'y';		/* remember that. */
      i = 8;
    }
  }
  for (;i<argc;i++) {			/* look at remaining args */
    if (strchr(argv[i],'=')) {		/* if its a candidate */
      putenv(argv[i]);			/* save it away */
      CF.have_env_var = 'y';		/* remember we have at least one */
    }
  }
  ref = strtol(argv[4], NULL, 16);	/* capture reference window */
  if (ref == 0) ref = None;
  myfprintf((stderr, "ref == %d\n", (int)ref));

  flib_init_graphics(dpy);

  fd_x = XConnectionNumber(dpy);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  InitConstants();
  ReadDefaults();			/* get config from fvwm */

  if (strcasecmp(module->name,"FvwmForm") != 0) { /* if not already read */
    sprintf(cmd,"read %s Quiet",module->name); /* read quiet modules config */
    SendText(Channel,cmd,0);
  }

  ReadConfig();				/* get config from fvwm */

  MassageConfig();			/* add data, calc window x/y */

  /* tell fvwm about our mask */
  SetMessageMask(Channel, M_SENDCONFIG|M_CONFIG_INFO|M_ERROR|M_STRING);
  SetMessageMask(Channel, MX_PROPERTY_CHANGE);
  XSetErrorHandler(ErrorHandler);
  OpenWindows();			/* create initial window */
  SendFinishedStartupNotification(Channel);/* tell fvwm we're running */
  if (timer != NULL) {
     SetupTimer();
  }
  MainLoop();				/* start */

  return 0;				/* */
}


RETSIGTYPE DeadPipe(int nonsense)
{
  exit(0);
  SIGNAL_RETURN;
}

/*
  X Error Handler
*/
static int
ErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* some errors are OK=ish */
  if (event->error_code == BadPixmap)
    return 0;
  if (event->error_code == BadDrawable)
    return 0;
  if (FRenderGetErrorCodeBase() + FRenderBadPicture == event->error_code)
    return 0;

  PrintXErrorAndCoredump(dpy, event, module->name);
  return 0;
}
/*  Local Variables: */
/*  c-basic-offset: 8 */
/*  indent-tabs-mode: t */
/*  End: */
