/****************************************************************************
 * Changed 10/06/97 by dje:
 * Change single IconBox into chain of IconBoxes.
 * Allow IconBox to be specified using X Geometry string.
 * Parse optional IconGrid.
 * Parse optional IconFill.
 * Use macros to make parsing more uniform, and easier to read.
 * Rewrote AddToList without tons of arg passing and merging.
 * Added a few comments.
 *
 * This module was all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for parsing the fvwm style command
 *
 ***********************************************************************/
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "style.h"
#include "fvwm.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

static window_style *all_styles = NULL; /* list of window names with attributes */

static int Get_TBLR(char *, unsigned char *); /* prototype */
static void add_style_to_list(window_style **style_list,
                              window_style *new_style); /* prototype */

/* A macro for skipping over white space */
#define SKIPSPACE \
   while(isspace(*restofline))restofline++;

/* A macro for checking the command with a caseless compare */
#define ITIS(THIS) \
  strncasecmp(restofline,THIS,sizeof(THIS)-1)==0

/* A macro for skipping over the command without counting it's size */
#define SKIP(THIS) \
  restofline += sizeof(THIS)-1

/* A macro for getting a non-quoted operand */
#define GETWORD \
  SKIPSPACE; \
  tmp = restofline; \
  len = 0; \
  while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&& \
        (*tmp != '\n')&&(!isspace(*tmp))) { \
    tmp++; \
    len++; \
  }

/* A macro for getting a quoted operand */
#define GETQUOTEDWORD \
  is_quoted = 0; \
  SKIPSPACE; \
  if (*restofline == '"') { \
    is_quoted = 1; \
    ++restofline; \
  } \
  tmp = restofline; \
  len = 0; \
  while (tmp && *tmp && \
         ((!is_quoted&&(*tmp != ',')&&(*tmp != '\n')&&(!isspace(*tmp))) \
          || (is_quoted&&(*tmp != '\n')&&(*tmp != '"')))) \
    { \
      tmp++; \
      len++; \
    } \
    if (tmp && (*tmp == '"')) ++tmp;

/* Process a style command.  First built up in a temp area.
   If valid, added to the list in a malloced area. */
void ProcessNewStyle(XEvent *eventp, Window w, FvwmWindow *tmp_win,
		     unsigned long context, char *text, int *Module)
{
  char *line;
  char *restofline;
  char *tmp;
  window_style *add_style;
  int butt;                             /* work area for button number */
  int num,i;
  /*  RBW - 11/02/1998  */
  int tmpno1 = -1, tmpno2 = -1, tmpno3 = -1, spargs = 0;
  /**/

  window_style tmpstyle;                      /* temp area to build name list */
  int len = 0;
  icon_boxes *which = 0;                /* which current boxes to chain to */
  int is_quoted;                        /* for parsing args with quotes */

  memset(&tmpstyle, 0, sizeof(window_style)); /* init temp window_style area */

  restofline = GetNextToken(text, &tmpstyle.name); /* parse style name */
  /* in case there was no argument! */
  if((tmpstyle.name == NULL)||(restofline == NULL))/* If no name, or blank cmd */
    {
      if (tmpstyle.name)
        free(tmpstyle.name);
      return;                             /* drop it. */
    }

  SKIPSPACE;                            /* skip over white space */
  line = restofline;

  if(restofline == NULL)
    {
      free(tmpstyle.name);
      return;
    }
  while((*restofline != 0)&&(*restofline != '\n'))
  {
    SKIPSPACE;                          /* skip white space */
    /* It might make more sense to capture the whole word, fix its
       case, and use strcmp, but there aren't many caseless compares
       because of this "switch" on the first letter. */
    switch (tolower(restofline[0]))
    {
      case 'a':
        if(ITIS("ACTIVEPLACEMENT"))
        {
          SKIP("ACTIVEPLACEMENT");
          tmpstyle.flags.do_place_random = 0;
          tmpstyle.flag_mask.do_place_random = 1;
          //          tmpstyle.off_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 'b':
        if(ITIS("BACKCOLOR"))
        {
          SKIP("BACKCOLOR");
          GETWORD;
          if(len > 0)
          {
            tmpstyle.back_color_name = safemalloc(len+1);
            //            tmpstyle.BackColor = safemalloc(len+1);
            strncpy(tmpstyle.back_color_name, restofline, len);
            //            strncpy(tmpstyle.BackColor,restofline,len);
            tmpstyle.back_color_name[len] = 0;
            //            tmpstyle.BackColor[len] = 0;
            tmpstyle.flags.has_color_back = 1;
            tmpstyle.flag_mask.has_color_back = 1;
            //            tmpstyle.on_flags |= BACK_COLOR_FLAG;
          }
          restofline = tmp;
        }
        else if (ITIS("BUTTON"))
        {
          SKIP("BUTTON");
          butt = -1; /* just in case sscanf fails */
          sscanf(restofline,"%d",&butt);
          GETWORD;
          restofline = tmp;
          SKIPSPACE;
          if (butt == 0) butt = 10;
          if (butt > 0 && butt <= 10)
            tmpstyle.off_buttons |= (1<<(butt-1));
        }
        else if(ITIS("BorderWidth"))
        {
          SKIP("BorderWidth");
          tmpstyle.flags.has_border_width = 1;
          tmpstyle.flag_mask.has_border_width = 1;
          //          tmpstyle.on_flags |= BW_FLAG;
          sscanf(restofline, "%d", &tmpstyle.border_width);
          //          sscanf(restofline,"%d",&tmpstyle.border_width);
          GETWORD;
          restofline = tmp;
          SKIPSPACE;
        }
        break;
      case 'c':
        if(ITIS("COLOR"))
        {
          SKIP("COLOR");
          SKIPSPACE;
          tmp = restofline;
          len = 0;
          while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                (*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
          {
            tmp++;
            len++;
          }
          if(len > 0)
          {
            tmpstyle.fore_color_name = safemalloc(len+1);
            strncpy(tmpstyle.fore_color_name, restofline, len);
            tmpstyle.fore_color_name[len] = 0;
            tmpstyle.flags.has_color_fore = 1;
            tmpstyle.flag_mask.has_color_fore = 1;
            //            tmpstyle.ForeColor = safemalloc(len+1);
            //            strncpy(tmpstyle.ForeColor,restofline,len);
            //            tmpstyle.ForeColor[len] = 0;
            //            tmpstyle.on_flags |= FORE_COLOR_FLAG;
          }

          while(isspace(*tmp))
            tmp++;
          if(*tmp == '/')
          {
            tmp++;
            while(isspace(*tmp))
              tmp++;
            restofline = tmp;
            len = 0;
            while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
                  (*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
            {
              tmp++;
              len++;
            }
            if(len > 0)
            {
              tmpstyle.back_color_name = safemalloc(len+1);
              strncpy(tmpstyle.back_color_name, restofline, len);
              tmpstyle.back_color_name[len] = 0;
              tmpstyle.flags.has_color_back = 1;
              tmpstyle.flag_mask.has_color_back = 1;
              //              tmpstyle.BackColor = safemalloc(len+1);
              //              strncpy(tmpstyle.BackColor,restofline,len);
              //              tmpstyle.BackColor[len] = 0;
              //              tmpstyle.on_flags |= BACK_COLOR_FLAG;
            }
          }
          restofline = tmp;
        }
        else if(ITIS("CirculateSkipIcon"))
        {
          SKIP("CirculateSkipIcon");
          tmpstyle.flags.common.do_circulate_skip_icon = 1;
          tmpstyle.flag_mask.common.do_circulate_skip_icon = 1;
          //      tmpstyle.on_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(ITIS("CirculateHitIcon"))
        {
          SKIP("CirculateHitIcon");
          tmpstyle.flags.common.do_circulate_skip_icon = 0;
          tmpstyle.flag_mask.common.do_circulate_skip_icon = 1;
          //          tmpstyle.off_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(ITIS("CLICKTOFOCUS"))
        {
          SKIP("CLICKTOFOCUS");
          tmpstyle.flags.common.focus_mode = FOCUS_CLICK;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 1;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.on_flags |= CLICK_FOCUS_FLAG;
          //          tmpstyle.off_flags |= SLOPPY_FOCUS_FLAG;
          //          tmpstyle.on_flags |= GRAB_FOCUS;
        }
        else if(ITIS("CirculateSkip"))
        {
          SKIP("CirculateSkip");
          tmpstyle.flags.common.do_circulate_skip = 1;
          tmpstyle.flag_mask.common.do_circulate_skip = 1;
          //          tmpstyle.on_flags |= CIRCULATESKIP_FLAG;
        }
        else if(ITIS("CirculateHit"))
        {
          SKIP("CirculateHit");
          tmpstyle.flags.common.do_circulate_skip = 0;
          tmpstyle.flag_mask.common.do_circulate_skip = 1;
          //          tmpstyle.off_flags |= CIRCULATESKIP_FLAG;
        }
        break;
      case 'd':
        if(ITIS("DecorateTransient"))
        {
          SKIP("DecorateTransient");
          tmpstyle.flags.do_decorate_transient = 1;
          tmpstyle.flag_mask.do_decorate_transient = 1;
          //          tmpstyle.on_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(ITIS("DUMBPLACEMENT"))
        {
          SKIP("DUMBPLACEMENT");
          tmpstyle.flags.do_place_smart = 0;
          tmpstyle.flag_mask.do_place_smart = 1;
          //          tmpstyle.off_flags |= SMART_PLACE_FLAG;
        }
        break;
      case 'e':
        break;
      case 'f':
        if(ITIS("FORECOLOR"))
        {
          SKIP("FORECOLOR");
          GETWORD;
          if(len > 0)
          {
            tmpstyle.fore_color_name = safemalloc(len+1);
            strncpy(tmpstyle.fore_color_name, restofline, len);
            tmpstyle.fore_color_name[len] = 0;
            tmpstyle.flags.has_color_fore = 1;
            tmpstyle.flag_mask.has_color_fore = 1;
            //            tmpstyle.ForeColor = safemalloc(len+1);
            //            strncpy(tmpstyle.ForeColor,restofline,len);
            //            tmpstyle.ForeColor[len] = 0;
            //            tmpstyle.on_flags |= FORE_COLOR_FLAG;
          }
          restofline = tmp;
        }
        else if(ITIS("FVWMBUTTONS"))
        {
          SKIP("FVWMBUTTONS");
          tmpstyle.flags.common.has_mwm_buttons = 0;
          tmpstyle.flag_mask.common.has_mwm_buttons = 1;
          //      tmpstyle.off_flags |= MWM_BUTTON_FLAG;
        }
        else if(ITIS("FVWMBORDER"))
        {
          SKIP("FVWMBORDER");
          tmpstyle.flags.common.has_mwm_border = 0;
          tmpstyle.flag_mask.common.has_mwm_border = 1;
          //          tmpstyle.off_flags |= MWM_BORDER_FLAG;
        }
        else if(ITIS("FocusFollowsMouse"))
        {
          SKIP("FocusFollowsMouse");
          tmpstyle.flags.common.focus_mode = FOCUS_MOUSE;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.off_flags |= CLICK_FOCUS_FLAG;
          //          tmpstyle.off_flags |= SLOPPY_FOCUS_FLAG;
        }
        break;
      case 'g':
        if(ITIS("GrabFocusOff"))
        {
          SKIP("GRABFOCUSOFF");
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.off_flags |= GRAB_FOCUS;
        }
        else if(ITIS("GrabFocus"))
        {
          SKIP("GRABFOCUS");
          tmpstyle.flags.common.do_grab_focus_when_created = 1;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.on_flags |= GRAB_FOCUS;
        }
        break;
      case 'h':
        if(ITIS("HINTOVERRIDE"))
        {
          SKIP("HINTOVERRIDE");
          tmpstyle.flags.common.has_mwm_override = 1;
          tmpstyle.flag_mask.common.has_mwm_override = 1;
          //          tmpstyle.on_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(ITIS("HANDLES"))
        {
          SKIP("HANDLES");
          tmpstyle.flags.has_no_border = 0;
          tmpstyle.flag_mask.has_no_border = 1;
          //          tmpstyle.off_flags |= NOBORDER_FLAG;
        }
        else if(ITIS("HandleWidth"))
        {
          SKIP("HandleWidth");
          tmpstyle.flags.has_handle_width = 1;
          tmpstyle.flag_mask.has_handle_width = 1;
          //          tmpstyle.on_flags |= NOBW_FLAG;
          sscanf(restofline, "%d", &tmpstyle.handle_width);
          //          sscanf(restofline,"%d",&tmpstyle.resize_width);
          GETWORD;
          restofline = tmp;
          SKIPSPACE;
        }
        break;
      case 'i':
        if(ITIS("IconTitle"))
        {
          SKIP("IconTitle");
          tmpstyle.flags.common.has_no_icon_title = 0;
          tmpstyle.flag_mask.common.has_no_icon_title = 1;
          //          tmpstyle.off_flags |= NOICON_TITLE_FLAG;
        }
        else if(ITIS("IconBox"))
        {
          icon_boxes *IconBoxes = 0;
          SKIP("IconBox");              /* Skip over word "IconBox" */
          IconBoxes = (icon_boxes *)safemalloc(sizeof(icon_boxes));
          memset(IconBoxes, 0, sizeof(icon_boxes)); /* clear it */
          IconBoxes->IconGrid[0] = 3;   /* init grid x */
          IconBoxes->IconGrid[1] = 3;   /* init grid y */
          /* try for 4 numbers x y x y */
          num = sscanf(restofline,"%d%d%d%d",
                       &IconBoxes->IconBox[0],
                       &IconBoxes->IconBox[1],
                       &IconBoxes->IconBox[2],
                       &IconBoxes->IconBox[3]);
          if (num == 4) {               /* if 4 numbers */
            for(i=0;i<num;i++) {
              SKIPSPACE;
              if (*restofline == '-') { /* If leading minus sign */
                if (i == 0 || i == 2) { /* if a width */
                  IconBoxes->IconBox[i] += Scr.MyDisplayWidth;
                } else {                  /* it must be a height */
                  IconBoxes->IconBox[i] += Scr.MyDisplayHeight;
                } /* end width/height */
              } /* end leading minus sign */
              while((!isspace(*restofline))&&(*restofline!= 0)&&
                    (*restofline != ',')&&(*restofline != '\n'))
                restofline++;
            }
            /* Note: here there is no test for valid co-ords, use geom */
          } else {                   /* Not 4 numeric args dje */
            char geom_string[25];    /* bigger than =32767x32767+32767+32767 */
            int geom_flags;
            GETWORD;                    /* read in 1 word w/o advancing */
            if(len > 0 && len < 24) {   /* if word found, not too long */
              strncpy(geom_string,restofline,len); /* copy and null term */
              geom_string[len] = 0;     /* null terminate it */
              geom_flags=XParseGeometry(geom_string,
                                        &IconBoxes->IconBox[0],
                                        &IconBoxes->IconBox[1], /* x/y */
                                        &IconBoxes->IconBox[2],
                                        &IconBoxes->IconBox[3]); /* width/ht */
              if (IconBoxes->IconBox[2] == 0) { /* zero width ind invalid */
                fvwm_msg(ERR,"ProcessNewStyle",
                "IconBox requires 4 numbers or geometry! Invalid string <%s>.",
                         geom_string);
                free(IconBoxes);        /* Drop the box */
                IconBoxes = 0;          /* forget about it */
              } else {                  /* got valid iconbox geom */
                if (geom_flags&XNegative) {
                  IconBoxes->IconBox[0] = Scr.MyDisplayWidth /* screen width */
                    + IconBoxes->IconBox[0] /* neg x coord */
                    - IconBoxes->IconBox[2] -2; /* width - 2 */
                }
                if (geom_flags&YNegative) {
                  IconBoxes->IconBox[1] = Scr.MyDisplayHeight /* scr height */
                    + IconBoxes->IconBox[1] /* neg y coord */
                    - IconBoxes->IconBox[3] -2; /* height - 2 */
                }
                IconBoxes->IconBox[2] +=
                  IconBoxes->IconBox[0]; /* x + wid = right x */
                IconBoxes->IconBox[3] +=
                  IconBoxes->IconBox[1]; /* y + height = bottom y */
              } /* end icon geom worked */
            } else {                    /* no word or too long */
              fvwm_msg(ERR,"ProcessNewStyle",
                       "IconBox requires 4 numbers or geometry! Too long (%d).",
                       len);
              free(IconBoxes);          /* Drop the box */
              IconBoxes = 0;            /* forget about it */
            } /* end word found, not too long */
            restofline = tmp;           /* got word, move past it */
          } /* end not 4 args */
          /* If we created an IconBox, put it in the chain. */
          if (IconBoxes != 0) {         /* If no error */
            if (tmpstyle.IconBoxes == 0) { /* If first one */
              tmpstyle.IconBoxes = IconBoxes; /* chain to root */
            } else {                    /* else not first one */
              which->next = IconBoxes;  /* add to end of chain */
            } /* end not first one */
            which = IconBoxes;          /* new current box. save for grid */
          } /* end no error */
        } /* end iconbox parameter */
        else if(ITIS("ICONGRID")) {
          SKIP("ICONGRID");
          SKIPSPACE;                    /* skip whitespace after keyword */
          /* The grid always affects the prior iconbox */
          if (which == 0) {             /* If no current box */
            fvwm_msg(ERR,"ProcessNewStyle",
                     "IconGrid must follow an IconBox in same Style command");
          } else {                      /* have a place to grid */
            num = sscanf(restofline,"%hd%hd", /* 2 shorts */
                         &which->IconGrid[0],
                         &which->IconGrid[1]);
            if (num != 2
                || which->IconGrid[0] < 1
                || which->IconGrid[1] < 1) {
              fvwm_msg(ERR,"ProcessNewStyle",
                "IconGrid needs 2 numbers > 0. Got %d numbers. x=%d y=%d!",
                       num, (int)which->IconGrid[0], (int)which->IconGrid[1]);
              which->IconGrid[0] = 3;   /* reset grid x */
              which->IconGrid[1] = 3;   /* reset grid y */
            } else {                    /* it worked */
              GETWORD;                  /* swallow word */
              restofline = tmp;
              GETWORD;                  /* swallow word */
              restofline = tmp;
            } /* end bad grid */
          } /* end place to grid */
        } else if(ITIS("ICONFILL")) {   /* direction to fill iconbox */
          SKIP("ICONFILL");
          SKIPSPACE;                    /* skip whitespace after keyword */
          /* The fill always affects the prior iconbox */
          if (which == 0) {             /* If no current box */
            fvwm_msg(ERR,"ProcessNewStyle",
                     "IconFill must follow an IconBox in same Style command");
          } else {                      /* have a place to fill */
            unsigned char IconFill_1;   /* first  type direction parsed */
            unsigned char IconFill_2;   /* second type direction parsed */
            GETWORD;                    /* read in word for length */
            if (Get_TBLR(restofline,&IconFill_1) == 0) { /* top/bot/lft/rgt */
              fvwm_msg(ERR,"ProcessNewStyle",
                "IconFill must be followed by T|B|R|L, found %.*s.",
                       len, restofline); /* its wrong */
            } else {                    /* first word valid */
              restofline = tmp;         /* swallow it */
              SKIPSPACE;                /* skip space between words */
              GETWORD;                  /* read in second word */
              if (Get_TBLR(restofline,&IconFill_2) == 0) {/* top/bot/lft/rgt */
                fvwm_msg(ERR,"ProcessNewStyle",
                         "IconFill must be followed by T|B|R|L, found %.*s.",
                         len, restofline); /* its wrong */
              } else if ((IconFill_1&ICONFILLHRZ)==(IconFill_2&ICONFILLHRZ)) {
                fvwm_msg(ERR,"ProcessNewStyle",
                 "IconFill must specify a horizontal and vertical direction.");
              } else {                  /* Its valid! */
                which->IconFlags |= IconFill_1; /* merge in flags */
                IconFill_2 &= ~ICONFILLHRZ; /* ignore horiz in 2nd arg */
                which->IconFlags |= IconFill_2; /* merge in flags */
              } /* end second word valid */
            } /* end first word valid */
            restofline = tmp;           /* swallow first or second word */
          } /* end have a place to fill */
        } /* end iconfill */
        else if(ITIS("ICON"))
        {
          SKIP("ICON");
          GETWORD;
          if(len > 0)
          {
            tmpstyle.icon_name = safemalloc(len+1);
            strncpy(tmpstyle.icon_name, restofline, len);
            tmpstyle.icon_name[len] = 0;
            tmpstyle.flags.has_icon = 1;
            tmpstyle.flag_mask.has_icon = 1;
            tmpstyle.flags.common.is_icon_suppressed = 0;
            tmpstyle.flag_mask.common.is_icon_suppressed = 1;
            //            tmpstyle.value = safemalloc(len+1);
            //            strncpy(tmpstyle.value,restofline,len);
            //            tmpstyle.value[len] = 0;
            //            tmpstyle.on_flags |= ICON_FLAG;
            //            tmpstyle.off_flags |= SUPPRESSICON_FLAG;
          }
          else
            {
              tmpstyle.flags.common.is_icon_suppressed = 0;
              tmpstyle.flag_mask.common.is_icon_suppressed = 1;
              //              tmpstyle.off_flags |= SUPPRESSICON_FLAG;
            }
          restofline = tmp;
        }
        break;
      case 'j':
        break;
      case 'k':
        break;
      case 'l':
        if(ITIS("LENIENCE"))
        {
          SKIP("LENIENCE");
          tmpstyle.flags.common.is_lenient = 1;
          tmpstyle.flag_mask.common.is_lenient = 1;
          //          tmpstyle.on_flags |= LENIENCE_FLAG;
        }
        else if (ITIS("LAYER"))
          {
            SKIP("LAYER");
            sscanf(restofline, "%d", &tmpstyle.layer);
            if(tmpstyle.layer < 0)
              {
                fvwm_msg(ERR, "ProcessNewStyle",
                         "Layer must be non-negative." );
                tmpstyle.flag_mask.use_layer = 0;
              }
            else
              {
                tmpstyle.flags.use_layer = 1;
                tmpstyle.flag_mask.use_layer = 1;
              }
            GETWORD;
            restofline = tmp;
            SKIPSPACE;
          }
        break;
      case 'm':
        if(ITIS("MWMBUTTONS"))
        {
          SKIP("MWMBUTTONS");
          tmpstyle.flags.common.has_mwm_buttons = 1;
          tmpstyle.flag_mask.common.has_mwm_buttons = 1;
          //          tmpstyle.on_flags |= MWM_BUTTON_FLAG;
        }
#ifdef MINI_ICONS
        else if (ITIS("MINIICON"))
        {
          SKIP("MINIICON");
          GETWORD;
          if(len > 0)
          {
            tmpstyle.mini_icon_name = safemalloc(len+1);
            strncpy(tmpstyle.mini_icon_name, restofline, len);
            tmpstyle.mini_icon_name[len] = 0;
            tmpstyle.flags.has_mini_icon = 1;
            tmpstyle.flag_mask.has_mini_icon = 1;
            //            tmpstyle.mini_value = safemalloc(len+1);
            //            strncpy(tmpstyle.mini_value,restofline,len);
            //            tmpstyle.mini_value[len] = 0;
            //            tmpstyle.on_flags |= MINIICON_FLAG;
          }
          restofline = tmp;
        }
#endif
        else if(ITIS("MWMBORDER"))
        {
          SKIP("MWMBORDER");
          tmpstyle.flags.common.has_mwm_border = 1;
          tmpstyle.flag_mask.common.has_mwm_border = 1;
          //          tmpstyle.on_flags |= MWM_BORDER_FLAG;
        }
        else if(ITIS("MWMDECOR"))
        {
          SKIP("MWMDECOR");
          tmpstyle.flags.has_mwm_decor = 1;
          tmpstyle.flag_mask.has_mwm_decor = 1;
          //          tmpstyle.on_flags |= MWM_DECOR_FLAG;
        }
        else if(ITIS("MWMFUNCTIONS"))
        {
          SKIP("MWMFUNCTIONS");
          tmpstyle.flags.has_mwm_functions = 1;
          tmpstyle.flag_mask.has_mwm_functions = 1;
          //          tmpstyle.on_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(ITIS("MOUSEFOCUS"))
        {
          SKIP("MOUSEFOCUS");
          tmpstyle.flags.common.focus_mode = FOCUS_MOUSE;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.off_flags |= CLICK_FOCUS_FLAG;
          //          tmpstyle.off_flags |= SLOPPY_FOCUS_FLAG;
          //          tmpstyle.off_flags |= GRAB_FOCUS;
        }
        break;
      case 'n':
        if(ITIS("NoIconTitle"))
        {
          SKIP("NoIconTitle");
          tmpstyle.flags.common.has_no_icon_title = 1;
          tmpstyle.flag_mask.common.has_no_icon_title = 1;
          //          tmpstyle.on_flags |= NOICON_TITLE_FLAG;
        }
        else if(ITIS("NOICON"))
        {
          SKIP("NOICON");
          tmpstyle.flags.common.is_icon_suppressed = 1;
          tmpstyle.flag_mask.common.is_icon_suppressed = 1;
          //          tmpstyle.on_flags |= SUPPRESSICON_FLAG;
        }
        else if(ITIS("NOTITLE"))
        {
          SKIP("NOTITLE");
          tmpstyle.flags.has_no_title = 1;
          tmpstyle.flag_mask.has_no_title = 1;
          //          tmpstyle.on_flags |= NOTITLE_FLAG;
        }
        else if(ITIS("NoPPosition"))
        {
          SKIP("NoPPosition");
          tmpstyle.flags.use_no_pposition = 1;
          tmpstyle.flag_mask.use_no_pposition = 1;
          //          tmpstyle.on_flags |= NO_PPOSITION_FLAG;
        }
        else if(ITIS("NakedTransient"))
        {
          SKIP("NakedTransient");
          tmpstyle.flags.do_decorate_transient = 0;
          tmpstyle.flag_mask.do_decorate_transient = 1;
          //          tmpstyle.off_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(ITIS("NODECORHINT"))
        {
          SKIP("NODECORHINT");
          tmpstyle.flags.has_mwm_decor = 0;
          tmpstyle.flag_mask.has_mwm_decor = 1;
          //          tmpstyle.off_flags |= MWM_DECOR_FLAG;
        }
        else if(ITIS("NOFUNCHINT"))
        {
          SKIP("NOFUNCHINT");
          tmpstyle.flags.has_mwm_functions = 0;
          tmpstyle.flag_mask.has_mwm_functions = 1;
          //          tmpstyle.off_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(ITIS("NOOVERRIDE"))
        {
          SKIP("NOOVERRIDE");
          tmpstyle.flags.common.has_mwm_override = 0;
          tmpstyle.flag_mask.common.has_mwm_override = 1;
          //          tmpstyle.off_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(ITIS("NOHANDLES"))
        {
          SKIP("NOHANDLES");
          tmpstyle.flags.has_no_border = 1;
          tmpstyle.flag_mask.has_no_border = 1;
          //          tmpstyle.on_flags |= NOBORDER_FLAG;
        }
        else if(ITIS("NOLENIENCE"))
        {
          SKIP("NOLENIENCE");
          tmpstyle.flags.common.is_lenient = 0;
          tmpstyle.flag_mask.common.is_lenient = 1;
          //          tmpstyle.off_flags |= LENIENCE_FLAG;
        }
        else if (ITIS("NOBUTTON"))
        {
          SKIP("NOBUTTON");

          butt = -1; /* just in case sscanf fails */
          sscanf(restofline, "%d", &butt);
          GETWORD;
          SKIPSPACE;

          if (butt == 0)
            butt = 10;
          if (butt > 0 && butt <= 10)
            tmpstyle.on_buttons |= (1<<(butt-1));
          restofline = tmp;
        }
        else if(ITIS("NOOLDECOR"))
        {
          SKIP("NOOLDECOR");
          tmpstyle.flags.has_ol_decor = 0;
          tmpstyle.flag_mask.has_ol_decor = 1;
          //          tmpstyle.off_flags |= OL_DECOR_FLAG;
        }
        break;
      case 'o':
        if(ITIS("OLDECOR"))
        {
          SKIP("OLDECOR");
          tmpstyle.flags.has_ol_decor = 1;
          tmpstyle.flag_mask.has_ol_decor = 1;
          //          tmpstyle.on_flags |= OL_DECOR_FLAG;
        }
        break;
      case 'p':
        break;
      case 'q':
        break;
      case 'r':
        if(ITIS("RANDOMPLACEMENT"))
        {
          SKIP("RANDOMPLACEMENT");
          tmpstyle.flags.do_place_random = 1;
          tmpstyle.flag_mask.do_place_random = 1;
          //          tmpstyle.on_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 's':
        if(ITIS("SMARTPLACEMENT"))
        {
          SKIP("SMARTPLACEMENT");
          tmpstyle.flags.do_place_smart = 1;
          tmpstyle.flag_mask.do_place_smart = 1;
          //          tmpstyle.on_flags |= SMART_PLACE_FLAG;
        }
        else if(ITIS("SkipMapping"))
        {
          SKIP("SkipMapping");
          tmpstyle.flags.common.do_show_on_map = 0;
          tmpstyle.flag_mask.common.do_show_on_map = 1;
          //          tmpstyle.on_flags |= SHOW_MAPPING;
        }
        else if(ITIS("ShowMapping"))
        {
          SKIP("ShowMapping");
          tmpstyle.flags.common.do_show_on_map = 1;
          tmpstyle.flag_mask.common.do_show_on_map = 1;
          //          tmpstyle.off_flags |= SHOW_MAPPING;
        }
        else if(ITIS("StickyIcon"))
        {
          SKIP("StickyIcon");
          tmpstyle.flags.common.is_icon_sticky = 1;
          tmpstyle.flag_mask.common.is_icon_sticky = 1;
          //          tmpstyle.on_flags |= STICKY_ICON_FLAG;
        }
        else if(ITIS("SlipperyIcon"))
        {
          SKIP("SlipperyIcon");
          tmpstyle.flags.common.is_icon_sticky = 0;
          tmpstyle.flag_mask.common.is_icon_sticky = 1;
          //          tmpstyle.off_flags |= STICKY_ICON_FLAG;
        }
        else if(ITIS("SLOPPYFOCUS"))
        {
          SKIP("SLOPPYFOCUS");
          tmpstyle.flags.common.focus_mode = FOCUS_SLOPPY;
          tmpstyle.flag_mask.common.focus_mode = FOCUS_MASK;
          tmpstyle.flags.common.do_grab_focus_when_created = 0;
          tmpstyle.flag_mask.common.do_grab_focus_when_created = 1;
          //          tmpstyle.off_flags |= CLICK_FOCUS_FLAG;
          //          tmpstyle.on_flags |= SLOPPY_FOCUS_FLAG;
          //          tmpstyle.off_flags |= GRAB_FOCUS;
        }
        else if(ITIS("StartIconic"))
        {
          SKIP("StartIconic");
          tmpstyle.flags.common.do_start_iconic = 1;
          tmpstyle.flag_mask.common.do_start_iconic = 1;
          //          tmpstyle.on_flags |= START_ICONIC_FLAG;
        }
        else if(ITIS("StartNormal"))
        {
          SKIP("StartNormal");
          tmpstyle.flags.common.do_start_iconic = 0;
          tmpstyle.flag_mask.common.do_start_iconic = 1;
          //          tmpstyle.off_flags |= START_ICONIC_FLAG;
        }
        else if(ITIS("StaysOnTop"))
        {
          SKIP("StaysOnTop");
          tmpstyle.flags.use_layer = 1;
          tmpstyle.flag_mask.use_layer = 1;
          tmpstyle.layer = Scr.OnTopLayer;
        }
        else if(ITIS("StaysPut"))
        {
          SKIP("StaysPut");
          tmpstyle.flags.use_layer = 1;
          tmpstyle.flag_mask.use_layer = 1;
          tmpstyle.layer = Scr.StaysPutLayer;
        }
        else if(ITIS("Sticky"))
        {
          SKIP("Sticky");
          tmpstyle.flags.common.is_sticky = 1;
          tmpstyle.flag_mask.common.is_sticky = 1;
          //          tmpstyle.on_flags |= STICKY_FLAG;
        }
        else if(ITIS("Slippery"))
        {
          SKIP("Slippery");
          tmpstyle.flags.common.is_sticky = 0;
          tmpstyle.flag_mask.common.is_sticky = 1;
          //          tmpstyle.off_flags |= STICKY_FLAG;
        }
        else if(ITIS("STARTSONDESK"))
        {
          SKIP("STARTSONDESK");
          //          tmpstyle.on_flags |= STARTSONDESK_FLAG;
          /*  RBW - 11/02/1998  */
          spargs = sscanf(restofline, "%d", &tmpno1);
          if (spargs == 1)
            {
              tmpstyle.flags.use_start_on_desk = 1;
              tmpstyle.flag_mask.use_start_on_desk = 1;
              /*  RBW - 11/20/1998 - allow for the special case of -1  */
              tmpstyle.start_desk = (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
            }
          else
            {
              //              tmpstyle.on_flags &= ~STARTSONDESK_FLAG;
              fvwm_msg(ERR,"ProcessNewStyle",
                       "bad StartsOnDesk arg: %s", restofline);
            }
          /**/
          GETWORD;
          restofline = tmp;
          SKIPSPACE;
        }
       /*  RBW - 11/02/1998
           StartsOnPage is like StartsOnDesk-Plus
       */
       else if(ITIS("STARTSONPAGE"))
         {
           SKIP("STARTSONPAGE");
           //           tmpstyle.on_flags |= STARTSONDESK_FLAG;
           spargs = sscanf(restofline,"%d %d %d", &tmpno1, &tmpno2, &tmpno3);

          if (spargs == 1 || spargs == 3)
            {
            /*  We have a desk no., with or without page.  */
              /*  RBW - 11/20/1998 - allow for the special case of -1  */
              tmpstyle.start_desk = (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;  /*  Desk is now actual + 1  */
              /*  Bump past desk no.    */
              GETWORD;
              restofline = tmp;
              SKIPSPACE;
            }

          if (spargs == 2 || spargs == 3)
            {
              if (spargs == 3)
                {
                  /*  RBW - 11/20/1998 - allow for the special case of -1  */
                  tmpstyle.start_page_x = (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
                  tmpstyle.start_page_y = (tmpno3 > -1) ? tmpno3 + 1 : tmpno3;
                }
              else
                {
                  tmpstyle.start_page_x = (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
                  tmpstyle.start_page_y = (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
                }
              /*  Bump past next 2 args.    */
              GETWORD;
              restofline = tmp;
              SKIPSPACE;
              GETWORD;
              restofline = tmp;
              SKIPSPACE;

            }
          if (spargs < 1 || spargs > 3)
            {
              //              tmpstyle.on_flags &= ~STARTSONDESK_FLAG;
              fvwm_msg(ERR,"ProcessNewStyle",
                       "bad StartsOnPage args: %s", restofline);
            }
          else
            {
              tmpstyle.flags.use_start_on_desk = 1;
              tmpstyle.flag_mask.use_start_on_desk = 1;
            }

         }
       /**/
        else if(ITIS("STARTSANYWHERE"))
        {
          SKIP("STARTSANYWHERE");
          tmpstyle.flags.use_start_on_desk = 0;
          tmpstyle.flag_mask.use_start_on_desk = 1;
          //          tmpstyle.off_flags |= STARTSONDESK_FLAG;
        }
        else if (ITIS("STARTSLOWERED"))
        {
          SKIP("STARTSLOWERED");
          tmpstyle.flags.use_start_raised_lowered = 1;
          tmpstyle.flag_mask.use_start_raised_lowered = 1;
          tmpstyle.flags.do_start_lowered = 1;
          tmpstyle.flag_mask.do_start_lowered = 1;
          //      tmpstyle.tmpflags.has_starts_lowered = 1;
          //      tmpstyle.tmpflags.starts_lowered = 1;
        }
        else if (ITIS("STARTSRAISED"))
        {
          SKIP("STARTSRAISED");
          tmpstyle.flags.use_start_raised_lowered = 1;
          tmpstyle.flag_mask.use_start_raised_lowered = 1;
          tmpstyle.flags.do_start_lowered = 0;
          tmpstyle.flag_mask.do_start_lowered = 1;
          //      tmpstyle.tmpflags.has_starts_lowered = 1;
          //      tmpstyle.tmpflags.starts_lowered = 0;
        }
        break;
      case 't':
        if(ITIS("TITLE"))
        {
          SKIP("TITLE");
          tmpstyle.flags.has_no_title = 0;
          tmpstyle.flag_mask.has_no_title = 1;
          //          tmpstyle.off_flags |= NOTITLE_FLAG;
        }
        break;
      case 'u':
        if(ITIS("UsePPosition"))
        {
          SKIP("UsePPosition");
          tmpstyle.flags.use_no_pposition = 0;
          tmpstyle.flag_mask.use_no_pposition = 1;
          //          tmpstyle.off_flags |= NO_PPOSITION_FLAG;
        }
#ifdef USEDECOR
        if(ITIS("UseDecor"))
        {
          SKIP("UseDecor");
          GETQUOTEDWORD;
          if (len > 0)
          {
            tmpstyle.decor_name = safemalloc(len+1);
            strncpy(tmpstyle.decor_name, restofline, len);
            tmpstyle.decor_name[len] = 0;
          }
          restofline = tmp;
        }
#endif
        else if(ITIS("UseStyle"))
        {
          SKIP("UseStyle");
          GETQUOTEDWORD;
          if (len > 0) {
            int hit = 0;
            /* changed to accum multiple Style definitions (veliaa@rpi.edu) */
            for ( add_style = all_styles; add_style;
		  add_style = add_style->next ) {
              if (!strncasecmp(restofline,add_style->name,len)) { /* match style */
                if (!hit)
                {             /* first match */
                  char *save_name;
                  save_name = tmpstyle.name;
                  memcpy((void*)&tmpstyle, (const void*)add_style, sizeof(window_style)); /* copy everything */
                  tmpstyle.next = NULL;      /* except the next pointer */
                  tmpstyle.name = save_name; /* and the name */
                  hit = 1;                   /* set not first match */
                }
                else
                {
		  merge_styles(&tmpstyle, add_style);
                } /* end hit/not hit */
              } /* end found matching style */
            } /* end looking at all styles */

            restofline = tmp;           /* move forward one word */
            if (!hit) {
              tmp=safemalloc(500);
              strcat(tmp,"UseStyle: ");
              strncat(tmp,restofline-len,len);
              strcat(tmp," style not found!");
              fvwm_msg(ERR,"ProcessNewStyle",tmp);
              free(tmp);
            }
          } /* if (len > 0) */
          while(isspace(*restofline)) restofline++;
        }
        break;
      case 'v':
        break;
      case 'w':
        if(ITIS("WindowListSkip"))
        {
          SKIP("WindowListSkip");
	  tmpstyle.flags.common.do_window_list_skip = 1;
	  tmpstyle.flag_mask.common.do_window_list_skip = 1;
	  //          tmpstyle.on_flags |= LISTSKIP_FLAG;
        }
        else if(ITIS("WindowListHit"))
        {
          SKIP("WindowListHit");
	  tmpstyle.flags.common.do_window_list_skip = 0;
	  tmpstyle.flag_mask.common.do_window_list_skip = 1;
	  //          tmpstyle.off_flags |= LISTSKIP_FLAG;
        }
        break;
      case 'x':
        break;
      case 'y':
        break;
      case 'z':
        break;
      default:
        break;
    }

    SKIPSPACE;
    if(*restofline == ',')
      restofline++;
    else if((*restofline != 0)&&(*restofline != '\n'))
    {
      fvwm_msg(ERR,"ProcessNewStyle",
               "bad style command: %s", restofline);
      /* Can't return here since all malloced memory will be lost. Ignore rest
       * of line instead. */
      break;
    }
  } /* end while still stuff on command */

  /* capture default icons */
  if(strcmp(tmpstyle.name, "*") == 0)
  {
    if(tmpstyle.flags.has_icon == 1)
      Scr.DefaultIcon = tmpstyle.icon_name;
    tmpstyle.flags.has_icon = 0;
    tmpstyle.flag_mask.has_icon = 1;
    //    tmpstyle.on_flags &= ~ICON_FLAG;
    tmpstyle.icon_name = NULL;
    //    tmpstyle.value = NULL;
  }
  add_style_to_list(&all_styles, &tmpstyle);                /* add temp name list to list */
}

/* Check word after IconFill to see if its "Top,Bottom,Left,Right" */
static int Get_TBLR(char *restofline,unsigned char *IconFill) {
  *IconFill = 0;                        /* init */
  if (ITIS("B") || ITIS("BOT")|| ITIS("BOTTOM")) {
    *IconFill |= ICONFILLBOT; /* turn on bottom bit */
    *IconFill |= ICONFILLHRZ; /* turn on vertical */
  } else if (ITIS("T") || ITIS("TOP")) { /* else if its "top" */
    *IconFill |= ICONFILLHRZ; /* turn on vertical */
  } else if (ITIS("R") || ITIS("RGT") || ITIS("RIGHT")) {
    *IconFill |= ICONFILLRGT; /* turn on right bit */
  } else if (!(ITIS("L") || ITIS("LFT") || ITIS("LEFT"))) { /* "left" */
    return 0;                           /* anything else is bad */
  }
  return 1;                             /* return OK */
}

/***********************************************************************
 *
 *  Procedure:
 * merge_styles - For a matching style, merge window_style to window_style
 *
 *  Returned Value:
 *      merged matching styles in callers window_style.
 *
 *  Inputs:
 *      styles - callers return area
 *      nptr - matching window_style
 *
 *  Note:
 *      The only trick here is that on and off flags/buttons are
 *      combined into the on flag/button.
 *
 ***********************************************************************/

void merge_styles(window_style *merged_style, window_style *add_style)
{
  int i;
  char *merge_flags;
  char *add_flags;
  char *merge_mask;
  char *add_mask;

  if(add_style->icon_name != NULL)
    merged_style->icon_name = add_style->icon_name;
#ifdef MINI_ICONS
  if(add_style->mini_icon_name != NULL)
    merged_style->mini_icon_name = add_style->mini_icon_name;
#endif
#ifdef USEDECOR
  if (add_style->decor_name != NULL)
    merged_style->decor_name = add_style->decor_name;
#endif
  if(add_style->flags.use_start_on_desk)
    /*  RBW - 11/02/1998  */
    {
      merged_style->start_desk = add_style->start_desk;
      merged_style->start_page_x = add_style->start_page_x;
      merged_style->start_page_y = add_style->start_page_y;
    }
  if(add_style->flags.has_border_width)
    merged_style->border_width = add_style->border_width;
  if(add_style->flags.has_color_fore)
    merged_style->fore_color_name = add_style->fore_color_name;
  if(add_style->flags.has_color_back)
    merged_style->back_color_name = add_style->back_color_name;
  if(add_style->flags.has_handle_width)
    merged_style->handle_width = add_style->handle_width;

  /* merge the style flags */
  merge_flags = (char *)&(merged_style->flags);
  add_flags = (char *)&(add_style->flags);
  merge_mask = (char *)&(merged_style->flag_mask);
  add_mask = (char *)&(add_style->flag_mask);
  for (i = 0; i < sizeof(style_flags); i++)
    {
      merge_flags[i] |= (add_flags[i] & add_mask[i]);
      merge_flags[i] &= (add_flags[i] | ~add_mask[i]);
      merge_mask[i] |= add_mask[i];
    }
  merged_style->on_buttons |= add_style->on_buttons; /* combine buttons */
  merged_style->on_buttons &= ~(add_style->off_buttons);

  /* Note, only one style cmd can define a windows iconboxes,
   * the last one encountered. */
  if(add_style->IconBoxes != NULL) {         /* If style has iconboxes */
    merged_style->IconBoxes = add_style->IconBoxes; /* copy it */
  }
  if (add_style->flags.use_layer) {
    merged_style->layer = add_style->layer;
  }
  if (add_style->flags.use_start_raised_lowered) {
    merged_style->flags.do_start_lowered = add_style->flags.do_start_lowered;
  }
  return;                               /* return */
}

static void add_style_to_list(window_style **style_list,
                              window_style *new_style)
{
  window_style *nptr;
  window_style *lastptr = NULL;

  /* This used to contain logic that returned if the style didn't contain
     anything.  I don't see why we should bother. dje. */

  /* used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */

  /* seems like a pretty inefficient way to keep track of the end
     of the list, but how long can the style list be? dje */
  for (nptr = *style_list; nptr != NULL; nptr = nptr->next) {
    lastptr=nptr;                       /* find end of style list */
  }

  nptr = (window_style *)safemalloc(sizeof(window_style)); /* malloc area */
  memcpy((void*)nptr, (const void*)new_style, sizeof(window_style)); /* copy term area into list */
  if(lastptr != NULL)                   /* If not first entry in list */
    lastptr->next = nptr;               /* chain this entry to the list */
  else                                  /* else first entry in list */
    *style_list = nptr;                /* set the list root pointer. */
} /* end function */

/***********************************************************************
 *
 *  Procedure:
 *      lookup_style - look through a list for a window name, or class
 *
 *  Returned Value:
 *      merged matching styles in callers window_style.
 *
 *  Inputs:
 *      tmp_win - FvwWindow structure to match against
 *      styles - callers return area
 *
 *  Changes:
 *      dje 10/06/97 test for NULL class removed, can't happen.
 *      use merge subroutine instead of coding merges 3 times.
 *      Use structure to return values, not many, many args
 *      and return value.
 *      Point at iconboxes chain, not single iconboxes elements.
 *
 ***********************************************************************/
void lookup_style(FvwmWindow *tmp_win, window_style *styles)
{
  window_style *nptr;

  memset(styles, 0, sizeof(window_style)); /* clear callers return area */
  styles->layer = Scr.StaysPutLayer; /* initialize to default layer */
  /* look thru all styles in order defined. */
  for (nptr = all_styles; nptr != NULL; nptr = nptr->next) {
    /* If name/res_class/res_name match, merge */
    if (matchWildcards(nptr->name,tmp_win->class.res_class) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->class.res_name) == TRUE) {
      merge_styles(styles, nptr);
    } else if (matchWildcards(nptr->name,tmp_win->name) == TRUE) {
      merge_styles(styles, nptr);
    }
  }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *      cmp_masked_flags - compare two flag structures passed as byte
 *                         arrays. Only compare bits set in the mask.
 *
 *  Returned Value:
 *      zero if the flags are the same
 *      non-zero otherwise
 *
 *  Inputs:
 *      flags1 - first byte array of flags to compare
 *      flags2 - second byte array of flags to compare
 *      mask   - byte array of flags to be considered for the comparison
 *      len    - number of bytes to compare
 *
 ***********************************************************************/
int cmp_masked_flags(void *flags1, void *flags2, void *mask, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      if ( (((char *)flags1)[i] & ((char *)mask)[i]) !=
	   (((char *)flags2)[i] & ((char *)mask)[i]) )
	/* flags are not the same, return 1 */
	return 1;
    }
  return 0;
}
