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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>                     /* for getenv */

#include <sys/time.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include "libs/Module.h"                /* for headersize, etc. */
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"

#include <libs/Picture.h>               /* for InitPictureCMap */
#include "libs/Colorset.h"               /* for InitPictureCMap */
#include "libs/FScreen.h"
#include "libs/FShape.h"

#include "FvwmForm.h"                   /* common FvwmForm stuff */

/* globals that are exported, keep in sync with externs in FvwmForm.h */
Form cur_form;                   /* current form */

/* Link list roots */
Item *root_item_ptr;             /* pointer to root of item list */
Line root_line = {&root_line,    /* ->next */
                  0,             /* number of items */
                  L_CENTER,0,0,  /* justify, size x/y */
                  0,             /* current array size */
                  0};            /* items ptr */
Line *cur_line = &root_line;            /* curr line in parse rtns */
char preload_yorn='n';           /* init to non-preload */
Item *item;                             /* current during parse */
Item *cur_sel, *cur_button;             /* current during parse */
Display *dpy;
int fd_x;                  /* fd for X connection */
Window root, ref;
int screen;
char *color_names[4];
char bg_state = 'd';                    /* in default state */
  /* d = default state */
  /* s = set by command (must be in "d" state for first "back" cmd to set it) */
  /* u = used (color allocated, too late to accept "back") */
char endDefaultsRead = 'n';
char *font_names[3];
char *screen_background_color;
char *MyName;
int MyNameLen;
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
static void AssignDrawTable(Item *);
static void AddItem();
static void PutDataInForm(char *);
static void ReadFormData();
static void FormVarsCheck(char **);

/* copy a string until '"', or '\n', or '\0' */
static char *CopyQuotedString (char *cp)
{
  char *dp, *bp, c;
  bp = dp = (char *)safemalloc(strlen(cp) + 1);
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
  bp = dp = (char *)safemalloc(strlen(cp) + 1);
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
static void ct_Text(char *);
static void ct_InputPointer(char *);
static void ct_InputPointerBack(char *);
static void ct_InputPointerFore(char *);
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
  {"Text",ct_Text},
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
  {"Read",ct_Read}
};

/* If there were vars on the command line, do env var sustitution on
   all input. */
static void FormVarsCheck(char **p)
{
  if (CF.have_env_var) {                /* if cmd line vars */
    if (strlen(*p) + 200 > CF.expand_buffer_size) { /* fast and loose */
      CF.expand_buffer_size = strlen(*p) + 2000; /* new size */
      if (CF.expand_buffer) {           /* already have one */
        CF.expand_buffer = saferealloc(CF.expand_buffer, CF.expand_buffer_size);
      } else {                          /* first time */
        CF.expand_buffer = safemalloc(CF.expand_buffer_size);
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
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }
  /* Accept commands beginning with "*FvwmFormDefault".
     This is to make sure defaults are read first.
     Note the hack w. bg_state. */
  if (strncasecmp(buf, "*FvwmFormDefault", 16) == 0) {
    p=buf+16;
    FormVarsCheck(&p);                   /* do var substitution if called for */
    e = FindToken(p,def_table,struct CommandTable);/* find cmd in table */
    if (e != 0) {                       /* if its valid */
      p=p+strlen(e->name);              /* skip over name */
      while (isspace((unsigned char)*p)) p++;          /* skip whitespace */
      e->function(p);                   /* call cmd processor */
      bg_state = 'd';                   /* stay in default state */
    }
  }
} /* end function */


static void ParseConfigLine(char *buf)
{
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
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
  if (strncasecmp(buf, MyName, MyNameLen) != 0) {/* If its not for me */
    return;
  } /* Now I know its for me. */
  p = buf+MyNameLen;                  /* jump to end of my name */
  /* at this point we have recognized "*FvwmForm" */
  FormVarsCheck(&p);
  e = FindToken(p,ct_table,struct CommandTable);/* find cmd in table */
  if (e == 0) {                       /* if no match */
    fprintf(stderr,"%s: unknown command: %s\n",MyName+1,buf);
    return;
  }

  p=p+strlen(e->name);                  /* skip over name */
  while (isspace((unsigned char)*p)) p++;              /* skip whitespace */

  FormVarsCheck(&p);                     /* do var substitution if called for */

  e->function(p);                       /* call cmd processor */
  return;
} /* end function */

/* Expands item array */
static void ExpandArray(Line *this_line)
{
  if (this_line->n + 1 >= this_line->item_array_size) { /* no empty space */
    this_line->item_array_size += ITEMS_PER_EXPANSION; /* get bigger */
    this_line->items =
      (Item **)saferealloc((void *)this_line->items,
			  (sizeof(Item *) * this_line->item_array_size));
  } /* end array full */
}

/* Function to add an item to the current line */
static void AddToLine(Item *newItem)
{
  ExpandArray(cur_line);                /* expand item array if needed */
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
            MyName+1,cp);
    return;
  }
  for (i=0,j=0;i<strlen(cp);i++) {
    if (cp[i] != ' ') {                   /* remove any spaces */
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
            MyName+1,option,cp);
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
  color_names[c_fg] = safestrdup(cp);
  colorset = -1;
  myfprintf((stderr, "ColorFore: %s\n", color_names[c_fg]));
}
static void ct_Back(char *cp)
{
  if (color_names[c_bg])
    free(color_names[c_bg]);
  color_names[c_bg] = safestrdup(cp);
  if (bg_state == 'd')
  {
    if (screen_background_color)
      free(screen_background_color);
    screen_background_color = safestrdup(color_names[c_bg]);
    bg_state = 's';                     /* indicate set by command */
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
  color_names[c_item_fg] = safestrdup(cp);
  itemcolorset = -1;
  myfprintf((stderr, "ColorItemFore: %s\n", color_names[c_item_fg]));
}
static void ct_ItemBack(char *cp)
{
  if (color_names[c_item_bg])
    free(color_names[c_item_bg]);
  color_names[c_item_bg] = safestrdup(cp);
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
  font_names[f_text] = safestrdup(cp);
  myfprintf((stderr, "Font: %s\n", font_names[f_text]));
}
static void ct_ButtonFont(char *cp)
{
  if (font_names[f_button])
    free(font_names[f_button]);
  font_names[f_button] = safestrdup(cp);
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
  font_names[f_input] = safestrdup(cp);
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
  cur_line->next = (struct _line *)safemalloc(sizeof(struct _line));
  memset(cur_line->next, 0, sizeof(struct _line));
  cur_line = cur_line->next;            /* new current line */
  cur_line->next = &root_line;          /* new next ptr, (actually root) */

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
  bg_state = 'u';                       /* indicate b/g color now used. */
  item->type = I_TEXT;
  /* Item now added to list of items, now it needs a pointer
     to the correct DrawTable. */
  AssignDrawTable(item);
  item->header.name = "FvwmMessage";    /* No purpose to this? dje */
  item->text.value = safemalloc(80);    /* point at last error recvd */
  item->text.n = 80;
  strcpy(item->text.value,"A mix of chars. MM20"); /* 20 mixed width chars */
  item->header.size_x = (FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,
                                          item->text.value,
                                          item->text.n/4) * 4) + 2 * TEXT_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
    + CF.padVText;
  memset(item->text.value,' ',80);      /* Clear it out */
  myfprintf((stderr, "Message area [%d, %d]\n",
             item->header.size_x, item->header.size_y));
  AddToLine(item);
  CF.last_error = item;                 /* save location of message item */
}
/* allocate colors and fonts needed */
static void CheckAlloc(Item *this_item,DrawTable *dt)
{
  static XGCValues xgcv;
  static int xgcv_mask = GCBackground | GCForeground | GCFont;

  if (dt->dt_used == 2) {               /* fonts colors shadows */
    return;
  }
  if (dt->dt_used == 0) {               /* if nothing allocated */
    dt->dt_colors[c_fg] = (colorset < 0)
      ? GetColor(dt->dt_color_names[c_fg])
      : Colorset[colorset].fg;
    dt->dt_colors[c_bg] = (colorset < 0)
      ? GetColor(dt->dt_color_names[c_bg])
      : Colorset[colorset].bg;

    xgcv.foreground = dt->dt_colors[c_fg];
    xgcv.background = dt->dt_colors[c_bg];
    xgcv.font = dt->dt_Ffont->font->fid;
    dt->dt_GC = fvwmlib_XCreateGC(dpy, CF.frame, xgcv_mask, &xgcv);

    dt->dt_used = 1;                    /* fore/back font allocated */
  }
  if (this_item->type == I_TEXT) {      /* If no shadows needed */
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
  xgcv.font = dt->dt_Ffont->font->fid;
  dt->dt_item_GC = fvwmlib_XCreateGC(dpy, CF.frame, GCForeground | GCFont, &xgcv);
  if (Pdepth < 2) {
    dt->dt_colors[c_itemlo] = BlackPixel(dpy, screen);
    dt->dt_colors[c_itemhi] = WhitePixel(dpy, screen);
  } else {
    dt->dt_colors[c_itemlo] = (itemcolorset < 0)
      ? GetShadow(dt->dt_colors[c_item_bg])
      : Colorset[itemcolorset].shadow;
    dt->dt_colors[c_itemhi] = (itemcolorset < 0)
      ? GetHilite(dt->dt_colors[c_item_bg])
      : Colorset[itemcolorset].hilite;
  }
  dt->dt_used = 2;                     /* fully allocated */
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
  DrawTable *new_dt;                    /* pointer to a new one */

  match_text_fore = color_names[c_fg];
  match_text_back = color_names[c_bg];
  match_item_fore = color_names[c_item_fg];
  match_item_back = color_names[c_item_bg];
  if (adt_item->type == I_TEXT) {
    match_font = font_names[f_text];
  } else if (adt_item->type == I_INPUT) {
    match_font = font_names[f_input];
  } else {
    match_font = font_names[f_button]; /* choices same as buttons */
  }
  last_dt = 0;
  for (find_dt = CF.roots_dt;              /* start at front */
       find_dt != 0;                    /* until end */
       find_dt = find_dt->dt_next) {    /* follow next pointer */
    last_dt = find_dt;
    if ((strcasecmp(match_text_fore,find_dt->dt_color_names[c_fg]) == 0) &&
        (strcasecmp(match_text_back,find_dt->dt_color_names[c_bg]) == 0) &&
        (strcasecmp(match_item_fore,find_dt->dt_color_names[c_item_fg]) == 0) &&
        (strcasecmp(match_item_back,find_dt->dt_color_names[c_item_bg]) == 0) &&
        (strcasecmp(match_font,find_dt->dt_font_name) == 0)) { /* Match */
      adt_item->header.dt_ptr = find_dt;       /* point item to drawtable */
      return;                           /* done */
    } /* end match */
  } /* end all drawtables checked, no match */

  /* Time to add a DrawTable */
  /* get one */
  new_dt = (struct _drawtable *)safemalloc(sizeof(struct _drawtable));
  memset(new_dt, 0, sizeof(struct _drawtable));
  new_dt->dt_next = 0;                  /* new end of list */
  if (CF.roots_dt == 0) {                  /* If first entry in list */
    CF.roots_dt = new_dt;                  /* set root pointer */
  } else {                              /* not first entry */
    last_dt->dt_next = new_dt;          /* link old to new */
  }

  new_dt->dt_font_name = safestrdup(match_font);
  new_dt->dt_color_names[c_fg]      = safestrdup(match_text_fore);
  new_dt->dt_color_names[c_bg]      = safestrdup(match_text_back);
  new_dt->dt_color_names[c_item_fg] = safestrdup(match_item_fore);
  new_dt->dt_color_names[c_item_bg] = safestrdup(match_item_back);
  new_dt->dt_used = 0;                  /* show nothing allocated */
  new_dt->dt_Ffont = FlocaleLoadFont(dpy, new_dt->dt_font_name, MyName+1);
  myfprintf((stderr,"Created drawtable with %s %s %s %s %s\n",
             new_dt->dt_color_names[c_fg], new_dt->dt_color_names[c_bg],
             new_dt->dt_color_names[c_item_fg],
             new_dt->dt_color_names[c_item_bg],
             new_dt->dt_font_name));
  adt_item->header.dt_ptr = new_dt;            /* point item to new drawtable */
}

/* input/output is global "item" - currently allocated last item */
static void AddItem()
{
  Item *save_item;
  save_item = (Item *)item;             /* save current item */
  item = (Item *)safecalloc(sizeof(Item),1); /* get a new item */
  if (save_item == 0) {                 /* if first item */
    root_item_ptr = item;               /* save root item */
  } else {                              /* else not first item */
    save_item->header.next = item;      /* link prior to new */
  }
}

static void ct_Text(char *cp)
{
  /* syntax: *FFText "<text>" */
  AddItem();
  bg_state = 'u';                       /* indicate b/g color now used. */
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
  item->input.init_value = safestrdup("");          /* init */
  if (*cp == '\"') {
    free(item->input.init_value);
    item->input.init_value = CopyQuotedString(++cp);
  }
  item->input.blanks = (char *)safemalloc(item->input.size);
  for (j = 0; j < item->input.size; j++)
    item->input.blanks[j] = ' ';
  item->input.buf = strlen(item->input.init_value) + 1;
  item->input.value = (char *)safemalloc(item->input.buf);
  item->input.value[0] = 0;             /* avoid reading unitialized data */

  item->header.size_x = FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,"W",1)
    * item->input.size + 2 * TEXT_SPC + 2 * BOX_SPC;
  item->header.size_y = item->header.dt_ptr->dt_Ffont->height
    + 3 * TEXT_SPC + 2 * BOX_SPC;
  myfprintf((stderr,"Input size_y is %d\n",item->header.size_y));

  if (CF.cur_input == 0) {                 /* first input field */
    item->input.next_input = item;      /* ring, next field is first field */
    item->input.prev_input = item;      /* ring, prev field is first field */
    CF.first_input = item;                 /* save loc of first item */
  } else {                              /* not first field */
    CF.cur_input->input.next_input = item; /* old next ptr point to this item */
    item->input.prev_input = CF.cur_input; /* current items prev is old item */
    item->input.next_input = CF.first_input; /* next is first item */
    CF.first_input->input.prev_input = item; /* prev in first is this item */
  }
  CF.cur_input = item;                     /* new current input item */
  myfprintf((stderr, "Input, %s, [%d], \"%s\"\n", item->header.name,
          item->input.size, item->input.init_value));
  AddToLine(item);
}
static void ct_Read(char *cp)
{
  /* syntax: *FFRead 0 | 1 */
  myfprintf((stderr,"Got read command, char is %c\n",(int)*cp));
  endDefaultsRead = *cp;                /* copy whatever it is */
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
    CF.file_to_read = 0;                   /* stop read */
    return;
  }
  /* Cant do the actual reading of the data file here,
     we are already in a readconfig loop. */
}
static void ReadFormData()
{
  int leading_len;
  char *line_buf;                       /* ptr to curr config line */
  char cmd_buffer[200];
  sprintf(cmd_buffer,"read %s Quiet",CF.file_to_read);
  SendText(Channel,cmd_buffer,0);       /* read data */
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
  free(CF.file_to_read);                /* dont need it anymore */
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

  var_name = CopySolidString(cp);       /* var */
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
        item->input.init_value = safemalloc(var_len+1);
        strcpy(item->input.init_value,cp); /* new initial value in field */
	if (item->input.value)
	  free(item->input.value);
        item->input.value = safemalloc(var_len+1);
        strcpy(item->input.value,cp);     /* new value in field */
        /* New value, but don't change length */
        free(var_name);                 /* goto's have their uses */
        return;
      }
      item=item->input.next_input;        /* next input field */
    } while (item != CF.cur_input);       /* while not end of ring */
  }
  /* You have a matching line, but it doesn't match an input
     field.  What to do?  I know, try a choice. */
  line = &root_line;                     /* start at first line */
  do {                                  /* for all lines */
    for (i = 0; i < line->n; i++) {     /* all items on line */
      item = line->items[i];
      if (item->type == I_CHOICE) {     /* choice is good */
        if (strcasecmp(var_name,item->header.name) == 0) { /* match */
          item->choice.init_on = 0;
          if (strncasecmp(cp, "on", 2) == 0) {
            item->choice.init_on = 1;   /* set default state */
            free(var_name);             /* goto's have their uses */
            return;
          }
        }
      }
    } /* end all items in line */
    line = line->next;                  /* go to next line */
  } while (line != &root_line);         /* do all lines */
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
  free(var_name);                       /* not needed now */
}
static void ct_Selection(char *cp)
{
  /* syntax: *FFSelection <name> single | multiple */
  AddItem();
  cur_sel = item;                       /* save ptr as cur_sel */
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
  if (cur_sel == 0) {                   /* need selection for a choice */
    fprintf(stderr,"%s: Need selection for choice %s\n",
            MyName+1, cp);
    return;
  }
  bg_state = 'u';                       /* indicate b/g color now used. */
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
      <= cur_sel->selection.n) {           /* no room */
    cur_sel->selection.choices_array_count += CHOICES_PER_SEL_EXPANSION;
    cur_sel->selection.choices =
      (Item **)saferealloc((void *)cur_sel->selection.choices,
			   sizeof(Item *) *
			   cur_sel->selection.choices_array_count); /* expand */
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
      (char **)saferealloc((void *)cur_button->button.commands,
			   sizeof(char *) *
			   cur_button->button.button_array_size);
  }
  cur_button->button.commands[cur_button->button.n++] = safestrdup(cp);
}

/* End of ct_ routines */

/* Init constants with values that can be freed later. */
static void InitConstants () {
  color_names[0]=safestrdup("Light Gray");
  color_names[1]=safestrdup("Black");
  color_names[2]=safestrdup("Gray50");
  color_names[3]=safestrdup("Wheat");
  font_names[0]=safestrdup("8x13bold");
  font_names[1]=safestrdup("8x13bold");
  font_names[2]=safestrdup("8x13bold");
  screen_background_color=safestrdup("Light Gray");
  CF.p_c[input_fore].pointer_color.pixel = WhitePixel(dpy, screen);
  CF.p_c[input_back].pointer_color.pixel = BlackPixel(dpy, screen);
  CF.p_c[button_fore].pointer_color.pixel = BlackPixel(dpy, screen);
  CF.p_c[button_back].pointer_color.pixel = WhitePixel(dpy, screen);
  /* The in pointer is the reverse of the hover pointer. */
  CF.p_c[button_in_fore].pointer_color.pixel = WhitePixel(dpy, screen);
  CF.p_c[button_in_back].pointer_color.pixel = BlackPixel(dpy, screen);
}

/* read the configuration file */
static void ReadDefaults ()
{
  char *line_buf;                       /* ptr to curr config line */

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
    ParseDefaults(line_buf);             /* process default config lines 1st */
  }
  if (endDefaultsRead == 'y') {         /* defaults read already */
    myfprintf((stderr,"Defaults read, no need to read file.\n"));
    return;
  } /* end defaults read already */
  SendText(Channel,"read .FvwmForm Quiet",0); /* read default config */
  SendText(Channel,"*FvwmFormDefaultRead y",0); /* remember you read it */

  InitGetConfigLine(Channel,"*FvwmFormDefault");
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* get config from fvwm */
    ParseDefaults(line_buf);             /* process default config lines 1st */
  }
} /* done */

static void ReadConfig ()
{
  char *line_buf;                       /* ptr to curr config line */

  InitGetConfigLine(Channel,MyName);
  while (GetConfigLine(Channel,&line_buf),line_buf) { /* get config from fvwm */
    ParseConfigLine(line_buf);          /* process config lines */
  }
} /* done */

/* After this config is read, figure it out */
static void MassageConfig()
{
  int i, extra;
  Line *line;                           /* for scanning form lines */

  if (CF.file_to_read) {                /* if theres a data file to read */
    ReadFormData();                     /* go read it */
  }
  /* get the geometry right */
  CF.max_width = 0;
  CF.total_height = ITEM_VSPC;
  line = &root_line;                     /* start at first line */
  do {                                  /* for all lines */
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
    line = line->next;                  /* go to next line */
  } while (line != &root_line);         /* do all lines */
  do {                                  /* note, already back at root_line */
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
    line = line->next;                  /* go to next line */
  } while (line != &root_line);          /* do all lines */
}

/* reset all the values (also done on first display) */
static void Restart ()
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
	    (char **)safecalloc(sizeof(char *), 50);
          item->input.value_history_ptr[0] = safestrdup(item->input.value);
          item->input.value_history_count = 1; /* next insertion point */
          myfprintf((stderr,"Initial save of %s in slot 0\n",
                     item->input.value_history_ptr[0]));
        } else {                        /* we have a history */
          int prior;
          prior = item->input.value_history_count - 1;
          if (prior < 0) {
            for (prior = VH_SIZE - 1;
                 CF.cur_input->input.value_history_ptr[prior] == 0;
                 --prior);              /* find last used slot */
          }
          myfprintf((stderr,"Prior is %d, compare %s to %s\n",
                     prior, item->input.value,
                     item->input.value_history_ptr[prior]));

          if ( strcmp(item->input.value, item->input.value_history_ptr[prior])
               != 0) {                  /* different value */
            if (item->input.value_history_ptr[item->input.value_history_count]) {
              free(item->input.value_history_ptr[item->input.value_history_count]);
              myfprintf((stderr,"Freeing old item in slot %d\n",
                         item->input.value_history_count));
            }
            item->input.value_history_ptr[item->input.value_history_count] =
              safestrdup(item->input.value); /* save value ptr in array */
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
      item->input.o_cursor = 0;
      break;
    case I_CHOICE:
      item->choice.on = item->choice.init_on;
      break;
    }
  }
}

/* redraw the frame */
void RedrawFrame ()
{
  int x, y;
  Item *item;

  for (item = root_item_ptr; item != 0;
       item = item->header.next) {      /* all items */
    switch (item->type) {
    case I_TEXT:
      RedrawText(item);
      break;
    case I_CHOICE:
      x = item->header.pos_x + TEXT_SPC + item->header.size_y;
      y = item->header.pos_y + TEXT_SPC +
        item->header.dt_ptr->dt_Ffont->ascent;
      XDrawString(dpy, CF.frame, item->header.dt_ptr->dt_GC,
                       x, y, item->choice.text,
                       item->choice.n);
      break;
    }
  }
}

void RedrawText(Item *item)
{
  int x, y;
  int len;
  char *p;

  CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
  x = item->header.pos_x + TEXT_SPC;
  y = item->header.pos_y + ( CF.padVText / 2 ) +
    item->header.dt_ptr->dt_Ffont->ascent;
  len = item->text.n;
  if ((p = memchr(item->text.value, '\0', len)) != NULL)
    len = p - item->text.value;
  XDrawString(dpy, CF.frame, item->header.dt_ptr->dt_GC,
	      x, y, item->text.value, len);

  return;
}

/* redraw an item */
void RedrawItem (Item *item, int click)
{
  int dx, dy, len, x;
  static XSegment xsegs[4];

  switch (item->type) {
  case I_INPUT:
    /* Create frame (pressed in): */
    dx = item->header.size_x - 1;
    dy = item->header.size_y - 1;
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
                   item->header.dt_ptr->dt_colors[c_itemlo]);

    /* around 12/26/99, added XClearArea to this function.
       this was done to deal with display corruption during
       multifield paste.  dje */
    XClearArea(dpy, item->header.win,
               BOX_SPC + TEXT_SPC - 1, BOX_SPC,
               item->header.size_x
               - (2 * BOX_SPC) - 2 - TEXT_SPC,
               (item->header.size_y - 1)
               - 2 * BOX_SPC + 1, False);

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
        FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,"W",1)
        * CF.abs_cursor - 1;
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
    if (len > item->input.size)
      len = item->input.size;
    else
      XDrawString(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
                  BOX_SPC + TEXT_SPC
                  + FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,"W",1)
                  * len,
                  BOX_SPC + TEXT_SPC
                  + item->header.dt_ptr->dt_Ffont->ascent,
                  item->input.blanks, item->input.size - len);
    XDrawString(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
                BOX_SPC + TEXT_SPC,
                BOX_SPC + TEXT_SPC +
                item->header.dt_ptr->dt_Ffont->ascent,
                item->input.value + item->input.left, len);
    if (item == CF.cur_input && !click) {
      x = BOX_SPC + TEXT_SPC +
        FlocaleTextWidth(item->header.dt_ptr->dt_Ffont,"W",1)
        * CF.abs_cursor - 1;
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
    XDrawString(
      dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
      BOX_SPC + TEXT_SPC,
      BOX_SPC + TEXT_SPC +
      item->header.dt_ptr->dt_Ffont->ascent,
      item->button.text, item->button.len);
    myfprintf((stderr,"Just put %s into a button\n",
               item->button.text));
    break;
  }
  XFlush(dpy);
}

/* update transparency if backgroude colorset is transparent */
void UpdateRootTransapency(void)
{
  Item *item;

  if (colorset > -1 && Colorset[colorset].pixmap == ParentRelative)
  {
    /* window has moved redraw the background if it is transparent */
    XClearArea(dpy, CF.frame, 0,0,0,0, True);
    if (itemcolorset > -1 &&
	Colorset[itemcolorset].pixmap == ParentRelative)
    {
      for (item = root_item_ptr; item != 0; item = item->header.next)
      {
	if (item->header.win != None)
	  XClearArea(dpy, item->header.win, 0,0,0,0, True);
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
    if ( parsed_command[0] == '!') {    /* If command starts with ! */
      system(parsed_command+1);         /* Need synchronous execution */
    } else {
      SendText(Channel,parsed_command, ref);
    }
  }

  /* post-command */
  if (CF.last_error) {                  /* if form has last_error field */
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
         item = item->header.next) {    /* all items */
      if (item->type == I_INPUT) {
	XClearWindow(dpy, item->header.win);
	RedrawItem(item, 0);
      }
      if (item->type == I_CHOICE)
	RedrawItem(item, 0);
    }
  }
}

/* open the windows */
static void OpenWindows ()
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
  xswa.background_pixel = CF.screen_background;
  xswa.border_pixel = 0;
  xswa.colormap = Pcmap;
  myfprintf((stderr,
          "going to create window w. bg %s, b/g pixel %X, black pixel %X\n",
          screen_background_color,
          (int)xswa.background_pixel,
          BlackPixel(dpy, screen)));
  CF.frame = XCreateWindow(dpy, root, x, y, CF.max_width, CF.total_height, 0,
			   Pdepth, InputOutput, Pvisual,
			   CWColormap | CWBackPixel | CWBorderPixel, &xswa);
  XSelectInput(dpy, CF.frame,
               KeyPressMask | ExposureMask | StructureNotifyMask |
	       VisibilityChangeMask);
  if (!CF.title) {
    CF.title = MyName+1;
  }
  XStoreName(dpy, CF.frame, CF.title);
  XSetWMHints(dpy, CF.frame, &wmh);
  myclasshints.res_name = MyName+1;
  myclasshints.res_class = "FvwmForm";
  XSetClassHint(dpy,CF.frame,&myclasshints);
  sh.x = x;
  sh.y = y;
  sh.width = CF.max_width;
  sh.height = CF.total_height;
  sh.win_gravity = gravity;
  XSetWMNormalHints(dpy, CF.frame, &sh);

  for (item = root_item_ptr; item != 0;
       item = item->header.next) {      /* all items */
    switch (item->type) {
    case I_INPUT:
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
    }
  }
  Restart();
  if (colorset >= 0)
  {
    SetWindowBackground(dpy, CF.frame, CF.max_width, CF.total_height,
                        &Colorset[(colorset)], Pdepth,
                        root_item_ptr->header.dt_ptr->dt_GC, True);
  }
  if (preload_yorn != 'y') {            /* if not a preload */
    XMapRaised(dpy, CF.frame);
    XMapSubwindows(dpy, CF.frame);
    if (CF.warp_pointer) {
      XWarpPointer(dpy, None, CF.frame, 0, 0, 0, 0,
                   CF.max_width / 2, CF.total_height - 1);
    }
  }
  DoCommand(&CF.def_button);
}

static void process_message(unsigned long, unsigned long *); /* proto */
static void ParseActiveMessage(char *); /* proto */

/* read something from Fvwm */
static void ReadFvwm ()
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
  case M_CONFIG_INFO:                   /* any module config command */
    myfprintf((stderr,"process_message: Got command: %s\n", (char *)&body[3]));
    ParseActiveMessage((char *)&body[3]);
    break;
  case MX_PROPERTY_CHANGE:
    if (body[0] == MX_PROPERTY_CHANGE_BACKGROUND &&
	((!Swallowed && body[2] == 0) || (Swallowed && body[2] == CF.frame)))
    {
      UpdateRootTransapency();
    }
    else if  (body[0] == MX_PROPERTY_CHANGE_SWALLOW && body[2] == CF.frame)
    {
	Swallowed = body[1];
    }
    break;
  case M_ERROR:
  case M_STRING:
    if (CF.last_error) {                /* if form has message area */
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
        CF.last_error->text.value = saferealloc(CF.last_error->text.value,
						msg_len * 2);
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
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }

  if (strncasecmp(buf, "Colorset", 8) == 0) {
    Item *item;
    int n = LoadColorset(&buf[8]);
    if(n == colorset || n == itemcolorset) {
      for (item = root_item_ptr; item != 0;
           item = item->header.next) {
        if (item->header.dt_ptr) {      /* if item has a drawtable */
          item->header.dt_ptr->dt_used = 0;
          if(item->header.dt_ptr->dt_GC) {
            XFreeGC(dpy,item->header.dt_ptr->dt_GC);
            item->header.dt_ptr->dt_GC = NULL;
          }
          if(item->header.dt_ptr->dt_item_GC) {
            XFreeGC(dpy,item->header.dt_ptr->dt_item_GC);
            item->header.dt_ptr->dt_item_GC = NULL;
          }
          CheckAlloc(item,item->header.dt_ptr); /* alloc colors/fonts needed */
          RedrawItem(item, 0);
          if (itemcolorset >= 0 && item->header.win != 0) {
            SetWindowBackground(dpy, item->header.win,
                                item->header.size_x, item->header.size_y,
                                &Colorset[(itemcolorset)], Pdepth,
                                item->header.dt_ptr->dt_GC, True);
          }
        } /* end item has a drawtable */
        if (colorset >= 0)
          {
            SetWindowBackground(dpy, CF.frame, CF.max_width, CF.total_height,
                                &Colorset[(colorset)], Pdepth,
                                root_item_ptr->header.dt_ptr->dt_GC, True);
          }
      } /* end all items */
    }
    return;
  } /* end colorset command */
  if (strncasecmp(buf, XINERAMA_CONFIG_STRING, sizeof(XINERAMA_CONFIG_STRING)-1)
      == 0) {
    FScreenConfigureModule(buf + sizeof(XINERAMA_CONFIG_STRING)-1);
    return;
  }
  if (strncasecmp(buf, MyName, MyNameLen) != 0) {/* If its not for me */
    return;
  } /* Now I know its for me. */
  p = buf+MyNameLen;                  /* jump to end of my name */
  /* at this point we have recognized "*FvwmForm" */
  e = FindToken(p,am_table,struct CommandTable);/* find cmd in table */
  if (e == 0) {                       /* if no match */
    /* this may be a configuration command of another same form */
    if (FindToken(p, ct_table, struct CommandTable) == 0)
      fprintf(stderr,"%s: Active command unknown: %s\n",MyName+1,buf);
    return;                             /* ignore it */
  }

  p=p+strlen(e->name);                  /* skip over name */
  while (isspace((unsigned char)*p)) p++;              /* skip whitespace */

  FormVarsCheck(&p);
  e->function(p);                       /* call cmd processor */
  return;
} /* end function */

static void am_Map(char *cp)
{
  XMapRaised(dpy, CF.frame);
  XMapSubwindows(dpy, CF.frame);
  if (CF.warp_pointer) {
    XWarpPointer(dpy, None, CF.frame, 0, 0, 0, 0,
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
  exit (0);                             /* why bother, just exit. */
}

/* main event loop */
static void MainLoop (void)
{
  fd_set fds;

  while ( !isTerminated ) {
    FD_ZERO(&fds);
    FD_SET(Channel[1], &fds);
    FD_SET(fd_x, &fds);

    XFlush(dpy);
    if (fvwmSelect(32, &fds, NULL, NULL, NULL) > 0) {
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
}


/* main procedure */
int main (int argc, char **argv)
{
  int i;
  FILE *fdopen();
  char *s;
  char cmd[200];

#ifdef DEBUGTOFILE
  freopen(".FvwmFormDebug","w",stderr);
#endif

  FInitLocale(LC_CTYPE, getenv("LC_CTYPE"), "", "FvwmForm");

  /* From FvwmAnimate start */
  /* Save our program  name - for error events */
  if ((s=strrchr(argv[0], '/')))	/* strip path */
    s++;
  else				/* no slash */
    s = argv[0];
  if(argc>=7)                         /* if override name */
    s = argv[6];                      /* use arg as name */
  MyNameLen=strlen(s)+1;		/* account for '*' */
  MyName = safemalloc(MyNameLen+1);	/* account for \0 */
  *MyName='*';
  strcpy(MyName+1, s);		/* append name */

  myfprintf((stderr,"%s: Starting, argv[0] is %s, len %d\n",MyName+1,
             argv[0],MyNameLen));

  if (argc < 6) {                       /* Now MyName is defined */
    fprintf(stderr,"%s Version "VERSION" should only be executed by fvwm!\n",
            MyName+1);
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

  Channel[0] = atoi(argv[1]);
  Channel[1] = atoi(argv[2]);

  dpy = XOpenDisplay("");
  if (dpy==NULL) {
    fprintf(stderr,"%s: could not open display\n",MyName+1);
    exit(1);
  }
  /* From FvwmAnimate end */

  i = 7;
  if (argc >= 8) {                      /* if have arg 7 */
    if (strcasecmp(argv[7],"preload") == 0) { /* if its preload */
      preload_yorn = 'y';               /* remember that. */
      i = 8;
    }
  }
  for (;i<argc;i++) {                   /* look at remaining args */
    if (strchr(argv[i],'=')) {          /* if its a candidate */
      putenv(argv[i]);                  /* save it away */
      CF.have_env_var = 'y';            /* remember we have at least one */
    }
  }
  ref = strtol(argv[4], NULL, 16);      /* capture reference window */
  if (ref == 0) ref = None;
  myfprintf((stderr, "ref == %d\n", (int)ref));

  InitPictureCMap(dpy);
  FScreenInit(dpy);
  /* prevent core dumps if fvwm doesn't provide any colorsets */
  AllocColorset(0);
  FShapeInit(dpy);

  fd_x = XConnectionNumber(dpy);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  InitConstants();
  ReadDefaults();                       /* get config from fvwm */

  if (strcasecmp(MyName+1,"FvwmForm") != 0) { /* if not already read */
    sprintf(cmd,"read %s Quiet",MyName+1); /* read quiet modules config */
    SendText(Channel,cmd,0);
  }

  ReadConfig();                         /* get config from fvwm */

  MassageConfig();                      /* add data, calc window x/y */

  /* tell fvwm about our mask */
  SetMessageMask(Channel, M_SENDCONFIG|M_CONFIG_INFO|M_ERROR|M_STRING);
  SetMessageMask(Channel, MX_PROPERTY_CHANGE);
  OpenWindows();                        /* create initial window */
  SendFinishedStartupNotification(Channel);/* tell fvwm we're running */
  MainLoop();                           /* start */

  return 0;                             /* */
}


void DeadPipe(int nonsense)
{
  exit(0);
}

