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

/* Changed 12/20/98 Dan Espen:

 - Removed form limitations including:
   number of lines in a form.
   number of items in a form.
   number of items in a line.
   number of selections in a choice.
   number of commands on a button.
   Colors can be changed anywhere in a form.
   Fonts can be changed anywhere in a form.

 - Changed the general organization of the module to match FvwmAnimate.
   See comments in FvwmAnimate to see what it mimicks.
   Some parts of this module have comments containing "FvwmAnimate"
   are common module/macro candidates.
   Changed debugging technique to match FvwmAnimate with the
   additional ability to Debug to File.

 - Configurability updates:
   Form appearance can be configured globaly:
   Form defaults are read from .FvwmForm.
   There is a built in Default setting/saving dialogue.
   Forms can be read in directly from a file.
   The file is the alias with a leading dot.
   The file is in $HOME or the system configuration directory.
   Comes with forms installed in the system configuration directory.

 - Operability:
   You can tab to previous input field with ^P, Up arrow, shift tab.

 - This module now has a configuration proceedure:
   AddToMenu "Module-Popup" "FvwmForm Defaults" FvwmForm FormFvwmForm

 - Use FvwmAnimate command parsing.
   The part of the command after the module name is no longer case sensitive.
   Use command tables instead of huge "else if".

 - Misc:
   Avoid core when choice not preceeded by a selection.
   Rename union member "select" so it doesn't conflict with the function.
   You can now control vertical spacing on text.  By default text is spaced
   vertically the way you would want it for buttons.
   This is for compatibility.  Now you can change the spacing to zero as
   you might want for a help panel.
   A button can execute a synchronous shell command.  The first use I
   put this is a form that writes its new definition to a file and
   reinvokes itself.
   Use SendText instead of writes to pipe.
   Changed button press-in effect from 1 sec to .1 sec.  Didn't seem to
   do anything on a slow machine...
   Added preload arg, and Map, Stop and UnMap commands for fast forms.
   (FvwmForm is now parsing commands during form display.)
   Add "Message" command, display "Error" and "String" messages from fvwm.
   Removed CopyNString, strdup replaces it.

   Moved ParseCommand, ReadXServer to separate file.
   Add UseData.
   Remove DefineMe.

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
#ifdef I18N_MB
#include <X11/Xlocale.h>
#endif
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include "libs/Module.h"                /* for headersize, etc. */
#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"

#include <libs/Picture.h>               /* for InitPictureCMap */

#define IamTheMain 1                    /* in FvwmForm.h, chg extern to "" */
#include "FvwmForm.h"                   /* common FvwmForm stuff */
#undef IamTheMain

/* prototypes */
static Pixel MyGetColor(char *color,char *Myname,int bw);
static void nocolor(char *, char *,char *);
static void AssignDrawTable(Item *);
static void AddItem();
static void PutDataInForm(char *);
static void ReadFormData();

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

/* get the font height */
static int FontHeight (XFontStruct *xfs) {
  return (xfs->ascent + xfs->descent);
}

/* get the font width, for fixed-width font only */
int FontWidth (XFontStruct *xfs) {
  return (xfs->per_char[0].width);
}


/* Command parsing section */

/* FvwmAnimate++ type command table */
typedef struct CommandTable {
  char *name;
  void (*function)(char *);
} ct;
static void ct_Back(char *);
static void ct_Button(char *);
static void ct_ButtonFont(char *);
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
static void ct_Position(char *);
static void ct_Read(char *);
static void ct_Selection(char *);
static void ct_Text(char *);
static void ct_UseData(char *);
static void ct_padVText(char *);
static void ct_WarpPointer(char *);

/* Must be in Alphabetic order (caseless) */
static struct CommandTable ct_table[] = {
  {"Back",ct_Back},
  {"Button",ct_Button},
  {"ButtonFont",ct_ButtonFont},
  {"Choice",ct_Choice},
  {"Command",ct_Command},
  {"Font",ct_Font},
  {"Fore",ct_Fore},
  {"GrabServer",ct_GrabServer},
  {"Input",ct_Input},
  {"InputFont",ct_InputFont},
  {"ItemBack",ct_ItemBack},
  {"ItemFore",ct_ItemFore},
  {"Line",ct_Line},
  {"Message",ct_Message},
  {"PadVText",ct_padVText},
  {"Position",ct_Position},
  {"Selection",ct_Selection},
  {"Text",ct_Text},
  {"UseData",ct_UseData},
  {"WarpPointer",ct_WarpPointer}
};
/* These commands are the default setting commands,
   read before the other form defining commands. */
static struct CommandTable def_table[] = {
  {"Back",ct_Back},
  {"ButtonFont",ct_ButtonFont},
  {"Font",ct_Font},
  {"Fore",ct_Fore},
  {"InputFont",ct_InputFont},
  {"ItemBack",ct_ItemBack},
  {"ItemFore",ct_ItemFore},
  {"Read",ct_Read}
};

static void ParseDefaults(char *buf) {
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }
  /* grok the global config lines sent from fvwm */
  if (strncasecmp(buf, DEFGRAPHSTR, DEFGRAPHLEN)==0)
    ParseGraphics(dpy, buf, G);
  /* Accept commands beginning with "*FvwmFormDefault".
     This is to make sure defaults are read first.
     Note the hack w. bg_state. */
  else if (strncasecmp(buf, "*FvwmFormDefault", 16) == 0) {
    p=buf+16;
    e = FindToken(p,def_table,struct CommandTable);/* find cmd in table */
    if (e != 0) {                       /* if its valid */
      p=p+strlen(e->name);              /* skip over name */
      while (isspace((unsigned char)*p)) p++;          /* skip whitespace */
      e->function(p);                   /* call cmd processor */
      bg_state = 'd';                   /* stay in default state */
    }
  }
} /* end function */

static void ParseConfigLine(char *buf) {
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }

  /* This used to be case sensitive */
  if (strncasecmp(buf, MyName, MyNameLen) != 0) {/* If its not for me */
    return;
  } /* Now I know its for me. */
  p = buf+MyNameLen;                  /* jump to end of my name */
  /* at this point we have recognized "*FvwmForm" */
  e = FindToken(p,ct_table,struct CommandTable);/* find cmd in table */
  if (e == 0) {                       /* if no match */
    fprintf(stderr,"%s: unknown command: %s\n",MyName+1,buf);
    return;
  }

  p=p+strlen(e->name);                  /* skip over name */
  while (isspace((unsigned char)*p)) p++;              /* skip whitespace */
  e->function(p);                       /* call cmd processor */
  return;
} /* end function */

/* Expands item array */
static void ExpandArray(Line *this_line) {
  if (this_line->n + 1 >= this_line->item_array_size) { /* no empty space */
    this_line->item_array_size += ITEMS_PER_EXPANSION; /* get bigger */
    this_line->items = realloc(this_line->items,
                               (sizeof(Item *) *
                               this_line->item_array_size));
    if (this_line->items == 0) {
      fprintf(stderr, "%s: For line items couldn't malloc %d bytes\n",
              MyName+1, this_line->item_array_size);
      exit (1);                         /* Give up */
    } /* end malloc failure */
  } /* end array full */
}

/* Function to add an item to the current line */
static void AddToLine(Item *newItem) {
  ExpandArray(cur_line);                /* expand item array if needed */
  cur_line->items[cur_line->n++] = newItem; /* add to lines item array */
  cur_line->size_x += newItem->header.size_x; /* incr lines width */
  if (cur_line->size_y < newItem->header.size_y) { /* new item bigger */
    cur_line->size_y = newItem->header.size_y; /* then line is bigger */
  }
}


/* All the functions starting with "ct_" (command table) are called thru
   their function pointers. Arg 1 is always the rest of the command. */
static void ct_GrabServer(char *cp) {
  CF.grab_server = 1;
}
static void ct_WarpPointer(char *cp) {
  CF.warp_pointer = 1;
}
static void ct_Position(char *cp) {
  CF.have_geom = 1;
  CF.gx = atoi(cp);
  while (!isspace((unsigned char)*cp)) cp++;
  while (isspace((unsigned char)*cp)) cp++;
  CF.gy = atoi(cp);
  myfprintf((stderr, "Position @ (%d, %d)\n", CF.gx, CF.gy));
}
static void ct_Fore(char *cp) {
  color_names[c_fg] = strdup(cp);
  myfprintf((stderr, "ColorFore: %s\n", color_names[c_fg]));
}
static void ct_Back(char *cp) {
  color_names[c_bg] = strdup(cp);
  if (bg_state == 'd') {
    screen_background_color = strdup(color_names[c_bg]);
    bg_state = 's';                     /* indicate set by command */
  }
  myfprintf((stderr, "ColorBack: %s, screen background %s, bg_state %c\n",
          color_names[c_bg],screen_background_color,(int)bg_state));
}
static void ct_ItemFore(char *cp) {
  color_names[c_item_fg] = strdup(cp);
  myfprintf((stderr, "ColorItemFore: %s\n", color_names[c_item_fg]));
}
static void ct_ItemBack(char *cp) {
  color_names[c_item_bg] = strdup(cp);
  myfprintf((stderr, "ColorItemBack: %s\n", color_names[c_item_bg]));
}
static void ct_Font(char *cp) {
  font_names[f_text] = strdup(cp);
  myfprintf((stderr, "Font: %s\n", font_names[f_text]));
}
static void ct_ButtonFont(char *cp) {
  font_names[f_button] = strdup(cp);
  myfprintf((stderr, "ButtonFont: %s\n", font_names[f_button]));
}
static void ct_InputFont(char *cp) {
  font_names[f_input] = strdup(cp);
  myfprintf((stderr, "InputFont: %s\n", font_names[f_input]));
}
static void ct_Line(char *cp) {
  cur_line->next=calloc(sizeof(struct _line),1); /* malloc new line */
  if (cur_line->next == 0) {
    fprintf(stderr, "%s: Malloc for line, (%d bytes) failed. exiting.\n",
            MyName+1, (int)sizeof(struct _line));
    exit (1);
  }
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

static void ct_Message(char *cp) {
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
  item->header.size_x = (XTextWidth(item->header.dt_ptr->dt_font_struct,
                                   item->text.value,
                                   item->text.n/4) * 4) + 2 * TEXT_SPC;
  item->header.size_y = FontHeight(item->header.dt_ptr->dt_font_struct)
    + CF.padVText;

  memset(item->text.value,' ',80);      /* Clear it out */
  myfprintf((stderr, "Message area [%d, %d]\n",
             item->header.size_x, item->header.size_y));
  AddToLine(item);
  CF.last_error = item;                 /* save location of message item */
}
/* allocate colors and fonts needed */
static void CheckAlloc(Item *this_item,DrawTable *dt) {
  static XGCValues xgcv;
  static int xgcv_mask = GCBackground | GCForeground | GCFont;

  if (dt->dt_used == 2) {               /* fonts colors shadows */
    return;
  }
  if (dt->dt_used == 0) {               /* if nothing allocated */
    dt->dt_colors[c_fg] = MyGetColor(dt->dt_color_names[c_fg],MyName+1,0);
    dt->dt_colors[c_bg] = MyGetColor(dt->dt_color_names[c_bg],MyName+1,1);

    xgcv.foreground = dt->dt_colors[c_fg];
    xgcv.background = dt->dt_colors[c_bg];
    xgcv.font = dt->dt_font;
    dt->dt_GC = XCreateGC(dpy, CF.frame, xgcv_mask, &xgcv);

    dt->dt_used = 1;                    /* fore/back font allocated */
  }
  if (this_item->type == I_TEXT) {      /* If no shadows needed */
    return;
  }
  dt->dt_colors[c_item_fg] = MyGetColor(dt->dt_color_names[c_item_fg],
                                        MyName+1,0);
  dt->dt_colors[c_item_bg] = MyGetColor(dt->dt_color_names[c_item_bg],
                                        MyName+1,1);
  xgcv.foreground = dt->dt_colors[c_item_fg];
  xgcv.background = dt->dt_colors[c_item_bg];
  xgcv.font = dt->dt_font;
  dt->dt_item_GC = XCreateGC(dpy, CF.frame, xgcv_mask, &xgcv);
  if (G->depth < 2) {
    dt->dt_colors[c_itemlo] = BlackPixel(dpy, screen);
    dt->dt_colors[c_itemhi] = WhitePixel(dpy, screen);
  } else {
    dt->dt_colors[c_itemlo] = GetShadow(dt->dt_colors[c_item_bg]);
    dt->dt_colors[c_itemhi] = GetHilite(dt->dt_colors[c_item_bg]);
  }
  dt->dt_used = 2;                     /* fully allocated */
}


/* Input is the current item.  Assign a drawTable entry to it. */
static void AssignDrawTable(Item *adt_item) {
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
  new_dt = malloc(sizeof(struct _drawtable)); /* get one */
  if (new_dt == 0) {                /* malloc failed? */
    fprintf(stderr,
            "%s: Malloc for DrawTable, (%d bytes) failed. exiting.\n",
            MyName+1, (int)sizeof(struct _drawtable));
    exit (1);                           /* give up */
  }
  new_dt->dt_next = 0;                  /* new end of list */
  if (CF.roots_dt == 0) {                  /* If first entry in list */
    CF.roots_dt = new_dt;                  /* set root pointer */
  } else {                              /* not first entry */
    last_dt->dt_next = new_dt;          /* link old to new */
  }

  new_dt->dt_font_name = strdup(match_font);
  new_dt->dt_color_names[c_fg]      = strdup(match_text_fore);
  new_dt->dt_color_names[c_bg]      = strdup(match_text_back);
  new_dt->dt_color_names[c_item_fg] = strdup(match_item_fore);
  new_dt->dt_color_names[c_item_bg] = strdup(match_item_back);
  new_dt->dt_used = 0;                  /* show nothing allocated */
  new_dt->dt_font_struct = GetFontOrFixed(dpy, new_dt->dt_font_name);
  new_dt->dt_font = new_dt->dt_font_struct->fid;
  myfprintf((stderr,"Created drawtable with %s %s %s %s %s\n",
             new_dt->dt_color_names[c_fg], new_dt->dt_color_names[c_bg],
             new_dt->dt_color_names[c_item_fg],
             new_dt->dt_color_names[c_item_bg],
             new_dt->dt_font_name));
  adt_item->header.dt_ptr = new_dt;            /* point item to new drawtable */
}

/* input/output is global "item" - currently allocated last item */
static void AddItem() {
  Item *save_item;
  save_item = (Item *)item;             /* save current item */
  item = calloc(sizeof(Item),1);        /* get a new item */
  if (item == 0) {                      /* if out of mem */
    fprintf(stderr, "%s: Malloc for item, (%d bytes) failed. exiting.\n",
            MyName+1, (int)sizeof(Item));
    exit (1);                           /* give up */
  }
  if (save_item == 0) {                 /* if first item */
    root_item_ptr = item;               /* save root item */
  } else {                              /* else not first item */
    save_item->header.next = item;      /* link prior to new */
  }
}

static void ct_Text(char *cp) {
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

  item->header.size_x = XTextWidth(item->header.dt_ptr->dt_font_struct,
                                   item->text.value,
                                   item->text.n) + 2 * TEXT_SPC;
  item->header.size_y = FontHeight(item->header.dt_ptr->dt_font_struct)
    + CF.padVText;

  myfprintf((stderr, "Text \"%s\" [%d, %d]\n", item->text.value,
             item->header.size_x, item->header.size_y));
  AddToLine(item);
}
static void ct_padVText(char *cp) {
  /* syntax: *FFText "<padVText pixels>" */
  CF.padVText = atoi(cp);
  myfprintf((stderr, "Text Vertical Padding %d\n", CF.padVText));
}
static void ct_Input(char *cp) {
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
  item->input.init_value = "";          /* init */
  if (*cp == '\"') {
    item->input.init_value = CopyQuotedString(++cp);
#if 0
    /* does this have any value? dje */
  } else if (*cp == '$') {              /* if environment var */
    item->input.init_value = getenv(++cp); /* get its value */
    if (item->input.init_value == NULL) { /* if env var not set */
      myfprintf((stderr,"Bad Envir var = %s\n",cp));
      item->input.init_value = "";      /* its blank */
    } /* end var not set */
#endif
  } /* end env var */
  item->input.blanks = (char *)safemalloc(item->input.size);
  for (j = 0; j < item->input.size; j++)
    item->input.blanks[j] = ' ';
  item->input.buf = strlen(item->input.init_value) + 1;
  item->input.value = (char *)safemalloc(item->input.buf);

  item->header.size_x = FontWidth(item->header.dt_ptr->dt_font_struct)
    * item->input.size + 2 * TEXT_SPC + 2 * BOX_SPC;
  item->header.size_y = FontHeight(item->header.dt_ptr->dt_font_struct)
    + 3 * TEXT_SPC + 2 * BOX_SPC;

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
static void ct_Read(char *cp) {
  /* syntax: *FFRead 0 | 1 */
  myfprintf((stderr,"Got read command, char is %c\n",(int)*cp));
  endDefaultsRead = *cp;                /* copy whatever it is */
}
/* read and save vars from a file for later use in form
   painting.
*/
static void ct_UseData(char *cp) {
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
static void ReadFormData() {
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
*/
static void PutDataInForm(char *cp) {
  char *var_name;
  char *var_value;
  int var_len;
  Item *item;

  if (CF.cur_input == 0) return;        /* if no input, leave */
  var_name = CopySolidString(cp);       /* var */
  if (*var_name == 0) {
    return;
  }
  cp += strlen(var_name);
  while (isspace((unsigned char)*cp)) cp++;
  var_value = cp;
  item = CF.cur_input;
  do {
    if (strcasecmp(var_name,item->header.name) == 0) {
      var_len = strlen(cp);
      item->input.init_value = safemalloc(var_len+1); /* leak! */
      strcpy(item->input.init_value,cp); /* new initial value in field */
      item->input.value = safemalloc(var_len+1); /* leak! */
      strcpy(item->input.value,cp);     /* new value in field */
      /* New value, but don't change length */
    }
    item=item->input.next_input;        /* next input field */
  } while (item != CF.cur_input);       /* while not end of ring */
  free(var_name);                       /* not needed now */
}
static void ct_Selection(char *cp) {
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
static void ct_Choice(char *cp) {
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
      (Item **)realloc(cur_sel->selection.choices,
                       sizeof(Item *) *
                       cur_sel->selection.choices_array_count); /* expand */
  }

  cur_sel->selection.choices[cur_sel->selection.n++] = item;
  item->header.size_y = FontHeight(item->header.dt_ptr->dt_font_struct)
    + 2 * TEXT_SPC;
  item->header.size_x = FontHeight(item->header.dt_ptr->dt_font_struct)
    + 4 * TEXT_SPC +
    XTextWidth(item->header.dt_ptr->dt_font_struct,
               item->choice.text, item->choice.n);

  myfprintf((stderr, "Choice %s, \"%s\", [%d, %d]\n", item->header.name,
          item->choice.text, item->header.size_x, item->header.size_y));
  AddToLine(item);
}
static void ct_Button(char *cp) {
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
  item->header.size_y = FontHeight(item->header.dt_ptr->dt_font_struct)
    + 2 * TEXT_SPC + 2 * BOX_SPC;
  item->header.size_x = 2 * TEXT_SPC + 2 * BOX_SPC
    + XTextWidth(item->header.dt_ptr->dt_font_struct, item->button.text,
                 item->button.len);
  AddToLine(item);
  cur_button = item;
  myfprintf((stderr,"Created button, fore %s, bg %s, text %s\n",
             item->header.dt_ptr->dt_color_names[c_item_fg],
             item->header.dt_ptr->dt_color_names[c_item_bg],
             item->button.text));
}
static void ct_Command(char *cp) {
  /* syntax: *FFCommand <command> */
  if (cur_button->button.button_array_size <= cur_button->button.n) {
    cur_button->button.button_array_size += BUTTON_COMMAND_EXPANSION;
    cur_button->button.commands =
      (char **)realloc(cur_button->button.commands,
                       sizeof(char *) *
                       cur_button->button.button_array_size);
    if (cur_button->button.commands == 0) {
      fprintf(stderr,"%s: realloc button array size %d failed\n",
              MyName+1, cur_button->button.button_array_size *
              (int)sizeof(char *));
      exit (1);
    }
  }
  cur_button->button.commands[cur_button->button.n++] =
    strdup(cp);
}

/* End of ct_ routines */



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
    * queue in fvwm2 that indicates whether the file has to be read.
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
static void MassageConfig() {
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
void RedrawFrame () {
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
        item->header.dt_ptr->dt_font_struct->ascent;
      XDrawImageString(dpy, CF.frame, item->header.dt_ptr->dt_GC,
                       x, y, item->choice.text,
                       item->choice.n);
      break;
    }
  }
}

void RedrawText(Item *item) {
  int x, y;
  CheckAlloc(item,item->header.dt_ptr); /* alloc colors and fonts needed */
  x = item->header.pos_x + TEXT_SPC;
  y = item->header.pos_y + ( CF.padVText / 2 ) +
    item->header.dt_ptr->dt_font_struct->ascent;
  XDrawImageString(dpy, CF.frame, item->header.dt_ptr->dt_GC,
                   x, y, item->text.value,
                   item->text.n);
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
      x = BOX_SPC + TEXT_SPC + FontWidth(item->header.dt_ptr->dt_font_struct)
        * CF.abs_cursor - 1;
      XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
                     item->header.dt_ptr->dt_colors[c_item_bg]);
      XDrawLine(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		x, BOX_SPC, x, dy - BOX_SPC);
    }
    len = item->input.n - item->input.left;
    XSetForeground(dpy, item->header.dt_ptr->dt_item_GC,
                   item->header.dt_ptr->dt_colors[c_item_fg]);
    if (len > item->input.size)
      len = item->input.size;
    else
      XDrawImageString(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		       BOX_SPC + TEXT_SPC
                       + FontWidth(item->header.dt_ptr->dt_font_struct) * len,
		       BOX_SPC + TEXT_SPC
                       + item->header.dt_ptr->dt_font_struct->ascent,
		       item->input.blanks, item->input.size - len);
    XDrawImageString(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		     BOX_SPC + TEXT_SPC,
		     BOX_SPC + TEXT_SPC +
                     item->header.dt_ptr->dt_font_struct->ascent,
		     item->input.value + item->input.left, len);
    if (item == CF.cur_input && !click) {
      x = BOX_SPC + TEXT_SPC
        + FontWidth(item->header.dt_ptr->dt_font_struct) * CF.abs_cursor - 1;
      XDrawLine(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		x, BOX_SPC, x, dy - BOX_SPC);
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
    XDrawImageString(dpy, item->header.win, item->header.dt_ptr->dt_item_GC,
		     BOX_SPC + TEXT_SPC,
		     BOX_SPC + TEXT_SPC +
                     item->header.dt_ptr->dt_font_struct->ascent,
		     item->button.text, item->button.len);
    myfprintf((stderr,"Just put %s into a button\n",
               item->button.text));
    break;
  }
  XFlush(dpy);
}

/* execute a command */
void DoCommand (Item *cmd)
{
  int k, dn;
  char *sp;
  Item *item;

  /* pre-command */
  if (cmd->button.key == IB_QUIT)
    XWithdrawWindow(dpy, CF.frame, screen);

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
    RedrawText(CF.last_error);          /* update display */
  } /* end form has last_error field */
  if (cmd->button.key == IB_QUIT) {
    if (CF.grab_server)
      XUngrabServer(dpy);
    /* This is a temporary bug workaround for the pipe drainage problem */
#if 1
    SendText(Channel,"KillMe", ref);    /* let commands complete */
    /* Note how the window is withdrawn, but execution continues until
       the KillMe command catches up with this module...
       Do something useful... */
    sleep(1);                           /* don't use cpu */
#else
    exit(0);
#endif
  }
  if (cmd->button.key == IB_RESTART) {
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
  Item *item;
  static XColor xcf, xcb;
  static XSetWindowAttributes xswa;
  static XWMHints wmh = { InputHint, True };
  static XSizeHints sh = { PPosition | PSize | USPosition | USSize };

  xc_ibeam = XCreateFontCursor(dpy, XC_xterm);
  xc_hand = XCreateFontCursor(dpy, XC_hand2);
  xcf.pixel = WhitePixel(dpy, screen);
  XQueryColor(dpy, PictureCMap, &xcf);
  xcb.pixel = CF.screen_background =
    MyGetColor(screen_background_color,MyName+1,0);
  XQueryColor(dpy, PictureCMap, &xcb);
  XRecolorCursor(dpy, xc_ibeam, &xcf, &xcb);

  /* the frame window first */
  if (CF.have_geom) {
    if (CF.gx >= 0)
      x = CF.gx;
    else
      x = DisplayWidth(dpy, screen) - CF.max_width + CF.gx;
    if (CF.gy >= 0)
      y = CF.gy;
    else
      y = DisplayHeight(dpy, screen) - CF.total_height + CF.gy;
  } else {
    x = (DisplayWidth(dpy, screen) - CF.max_width) / 2;
    y = (DisplayHeight(dpy, screen) - CF.total_height) / 2;
  }
  myfprintf((stderr,"going to create window w. bg %s\n",
             screen_background_color));
  xswa.background_pixel = CF.screen_background;
  xswa.border_pixel = 0;
  xswa.colormap = G->cmap;
  CF.frame = XCreateWindow(dpy, root, x, y, CF.max_width, CF.total_height, 0,
			   G->depth, InputOutput, G->viz,
			   CWColormap | CWBackPixel | CWBorderPixel, &xswa);
  XSelectInput(dpy, CF.frame,
               KeyPressMask | ExposureMask | StructureNotifyMask);
  XStoreName(dpy, CF.frame, MyName+1);
  XSetWMHints(dpy, CF.frame, &wmh);
  sh.x = x, sh.y = y;
  sh.width = CF.max_width, sh.height = CF.total_height;
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
      xswa.cursor = xc_ibeam;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
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
      xswa.cursor = xc_hand;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
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
      xswa.cursor = xc_hand;
      XChangeWindowAttributes(dpy, item->header.win, CWCursor, &xswa);
      break;
    }
  }
  Restart();
  if (preload_yorn == 'n') {            /* if not a preload */
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
static void ReadFvwm () {

#if 1
    FvwmPacket* packet = ReadFvwmPacket(Channel[1]);
    if ( packet == NULL )
	exit(0);
    else
	process_message( packet->type, packet->body );
#else
  int n;
  static char buffer[32];
  n = read(Channel[1], buffer, 32);
  if (n == 0) {                         /* came here on select, 0 is end */
    if (CF.grab_server)
      XUngrabServer(dpy);
    exit(0);
  }
#endif
}
static void process_message(unsigned long type, unsigned long *body) {
  switch (type) {
  case M_CONFIG_INFO:                   /* any module config command */
    myfprintf((stderr,"process_message: Got command: %s\n", (char *)&body[3]));
    ParseActiveMessage((char *)&body[3]);
    break;
  case M_ERROR:
  case M_STRING:
    if (CF.last_error) {                /* if form has message area */
      /* ignore form size, its OK to write outside the window boundary */
      int msg_len;
      char *msg_ptr;
      msg_ptr = (char *)&body[3];
      msg_len = strlen(msg_ptr);
      if (msg_ptr[msg_len-1] == '\n') { /* line ends w newline */
        msg_ptr[msg_len-1] = '\0'; /* strip off \n */
      }
      if (CF.last_error->text.n <= msg_len) { /* if message wont fit */
        CF.last_error->text.value = realloc(CF.last_error->text.value,
                                            msg_len * 2);
        CF.last_error->text.n = msg_len * 2;
      }
      strncpy(CF.last_error->text.value,msg_ptr,
              CF.last_error->text.n);
      CF.last_error->text.value[CF.last_error->text.n] = 0;
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
static struct CommandTable am_table[] = {
  {"Map",am_Map},
  {"Stop",am_Stop},
  {"UnMap",am_UnMap}
};

/* This is similar to the other 2 "Parse" functions. */
static void ParseActiveMessage(char *buf) {
  char *p;
  struct CommandTable *e;
  if (buf[strlen(buf)-1] == '\n') {     /* if line ends with newline */
    buf[strlen(buf)-1] = '\0';	/* strip off \n */
  }

  if (strncasecmp(buf, MyName, MyNameLen) != 0) {/* If its not for me */
    return;
  } /* Now I know its for me. */
  p = buf+MyNameLen;                  /* jump to end of my name */
  /* at this point we have recognized "*FvwmForm" */
  e = FindToken(p,am_table,struct CommandTable);/* find cmd in table */
  if (e == 0) {                       /* if no match */
    fprintf(stderr,"%s: Active command unknown: %s\n",MyName+1,buf);
    return;                             /* ignore it */
  }

  p=p+strlen(e->name);                  /* skip over name */
  while (isspace((unsigned char)*p)) p++;              /* skip whitespace */
  e->function(p);                       /* call cmd processor */
  return;
} /* end function */

static void am_Map(char *cp) {
  XMapRaised(dpy, CF.frame);
  XMapSubwindows(dpy, CF.frame);
  if (CF.warp_pointer) {
    XWarpPointer(dpy, None, CF.frame, 0, 0, 0, 0,
                 CF.max_width / 2, CF.total_height - 1);
  }
  myfprintf((stderr, "Map: got it\n"));
}
static void am_UnMap(char *cp) {
  XUnmapWindow(dpy, CF.frame);
  myfprintf((stderr, "UnMap: got it\n"));
}
static void am_Stop(char *cp) {
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
  FILE *fdopen();
  char *s;                              /* FvwmAnimate */
  char mask_mesg[20];
  char cmd[200];

#ifdef DEBUGTOFILE
  freopen(".FvwmFormDebug","w",stderr);
#endif

#ifdef I18N_MB
  setlocale(LC_CTYPE, "");
#endif

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

  if ((argc < 6)||(argc > 9)) {	/* Now MyName is defined */
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

  if (argc >= 8) {                      /* if have arg 7 */
    if (strcasecmp(argv[7],"preload") == 0) { /* if its preload */
      preload_yorn = 'y';               /* remember that. */
    }
  }
  ref = strtol(argv[4], NULL, 16);      /* capture reference window */
  if (ref == 0) ref = None;
  myfprintf((stderr, "ref == %d\n", (int)ref));

  G = CreateGraphics(dpy);

  fd_x = XConnectionNumber(dpy);

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);

  ReadDefaults();                       /* get config from fvwm */

  if (strcasecmp(MyName+1,"FvwmForm") != 0) { /* if not already read */
    sprintf(cmd,"read %s Quiet",MyName+1); /* read quiet modules config */
    SendText(Channel,cmd,0);
  }

  ReadConfig();                         /* get config from fvwm */

  SavePictureCMap(dpy, G->viz, G->cmap, G->depth); /* for shadow routines */

  MassageConfig();                      /* add data, calc window x/y */

  /* Now tell fvwm we want *Alias commands in real time */
  sprintf(mask_mesg,"SET_MASK %lu\n",(unsigned long)
          (M_SENDCONFIG|M_CONFIG_INFO|M_ERROR|M_STRING));
  SendInfo(Channel, mask_mesg, 0);      /* tell fvwm about our mask */
  OpenWindows();                        /* create initial window */
  MainLoop();                           /* start */

  return 0;                             /* */
}


void DeadPipe(int nonsense) {
  exit(0);
}

/*
 * *************************************************************************
 * Returns color Pixel value for a named color.  Similar to all the other
 * color allocation  subroutines, except it handles  black  and white and
 * gets the module name as input for error messages.
 *
 * Would make a generic subroutine if dpy,  screen, scr_depth are handled
 * somehow.
 * *************************************************************************
 */
static Pixel MyGetColor(char *name, char *ModName, int bw)
{
  XColor color;

  if (G->depth < 2) {
    return (bw ? WhitePixel(dpy, screen) : BlackPixel(dpy, screen));
  }
  color.pixel = 0;
  if (!XParseColor (dpy, G->cmap, name, &color))
    nocolor("parse",name,ModName);
  else if(!XAllocColor (dpy, G->cmap, &color))
    nocolor("alloc",name,ModName);
  return color.pixel;
}

static void nocolor(char *a, char *b, char *ModName) {
 fprintf(stderr,"%s: can't %s %s\n", ModName, a,b);
}

