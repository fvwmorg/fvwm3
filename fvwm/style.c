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

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

static int Get_TBLR(char *, unsigned char *); /* prototype */
static void AddToList(name_list *);     /* prototype */

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
void ProcessNewStyle(XEvent *eventp,
                     Window w,
                     FvwmWindow *tmp_win,
                     unsigned long context,
                     char *text,
                     int *Module)
{
  char *line;
  char *restofline,*tmp;
  name_list *nptr;
  int butt;                             /* work area for button number */
  int num,i;
  /*  RBW - 11/02/1998  */
  int tmpno1 = -1, tmpno2 = -1, tmpno3 = -1, spargs = 0;
  /**/

  name_list tname;                      /* temp area to build name list */
  int len = 0;
  icon_boxes *which = 0;                /* which current boxes to chain to */
  int is_quoted;                        /* for parsing args with quotes */

  memset(&tname, 0, sizeof(name_list)); /* init temp name_list area */

  restofline = GetNextToken(text,&tname.name); /* parse style name */
  /* in case there was no argument! */
  if((tname.name == NULL)||(restofline == NULL))/* If no name, or blank cmd */
    {
      if (tname.name)
	free(tname.name);
      return;                             /* drop it. */
    }

  SKIPSPACE;                            /* skip over white space */
  line = restofline;

  if(restofline == NULL)
    {
      free(tname.name);
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
          tname.on_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 'b':
        if(ITIS("BACKCOLOR"))
        {
          SKIP("BACKCOLOR");
          GETWORD;
          if(len > 0)
          {
            tname.BackColor = safemalloc(len+1);
            strncpy(tname.BackColor,restofline,len);
            tname.BackColor[len] = 0;
            tname.off_flags |= BACK_COLOR_FLAG;
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
            tname.on_buttons |= (1<<(butt-1));
        }
        else if(ITIS("BorderWidth"))
        {
          SKIP("BorderWidth");
          tname.off_flags |= BW_FLAG;
          sscanf(restofline,"%d",&tname.border_width);
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
            tname.ForeColor = safemalloc(len+1);
            strncpy(tname.ForeColor,restofline,len);
            tname.ForeColor[len] = 0;
            tname.off_flags |= FORE_COLOR_FLAG;
          }

          while(isspace(*tmp))tmp++;
          if(*tmp == '/')
          {
            tmp++;
            while(isspace(*tmp))tmp++;
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
              tname.BackColor = safemalloc(len+1);
              strncpy(tname.BackColor,restofline,len);
              tname.BackColor[len] = 0;
              tname.off_flags |= BACK_COLOR_FLAG;
            }
          }
          restofline = tmp;
        }
        else if(ITIS("CirculateSkipIcon"))
        {
          SKIP("CirculateSkipIcon");
          tname.off_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(ITIS("CirculateHitIcon"))
        {
          SKIP("CirculateHitIcon");
          tname.on_flags |= CIRCULATE_SKIP_ICON_FLAG;
        }
        else if(ITIS("CLICKTOFOCUS"))
        {
          SKIP("CLICKTOFOCUS");
          tname.off_flags |= CLICK_FOCUS_FLAG;
          tname.on_flags |= SLOPPY_FOCUS_FLAG;
        }
        else if(ITIS("CirculateSkip"))
        {
          SKIP("CirculateSkip");
          tname.off_flags |= CIRCULATESKIP_FLAG;
        }
        else if(ITIS("CirculateHit"))
        {
          SKIP("CirculateHit");
          tname.on_flags |= CIRCULATESKIP_FLAG;
        }
        break;
      case 'd':
        if(ITIS("DecorateTransient"))
        {
          SKIP("DecorateTransient");
          tname.off_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(ITIS("DUMBPLACEMENT"))
        {
          SKIP("DUMBPLACEMENT");
          tname.on_flags |= SMART_PLACE_FLAG;
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
            tname.ForeColor = safemalloc(len+1);
            strncpy(tname.ForeColor,restofline,len);
            tname.ForeColor[len] = 0;
            tname.off_flags |= FORE_COLOR_FLAG;
          }
          restofline = tmp;
        }
        else if(ITIS("FVWMBUTTONS"))
        {
          SKIP("FVWMBUTTONS");
          tname.on_flags |= MWM_BUTTON_FLAG;
        }
        else if(ITIS("FVWMBORDER"))
        {
          SKIP("FVWMBORDER");
          tname.on_flags |= MWM_BORDER_FLAG;
        }
        else if(ITIS("FocusFollowsMouse"))
        {
          SKIP("FocusFollowsMouse");
          tname.on_flags |= CLICK_FOCUS_FLAG;
          tname.on_flags |= SLOPPY_FOCUS_FLAG;
        }
        break;
      case 'g':
        break;
      case 'h':
        if(ITIS("HINTOVERRIDE"))
        {
          SKIP("HINTOVERRIDE");
          tname.off_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(ITIS("HANDLES"))
        {
          SKIP("HANDLES");
          tname.on_flags |= NOBORDER_FLAG;
        }
        else if(ITIS("HandleWidth"))
        {
          SKIP("HandleWidth");
          tname.off_flags |= NOBW_FLAG;
          sscanf(restofline,"%d",&tname.resize_width);
          GETWORD;
          restofline = tmp;
          SKIPSPACE;
        }
        break;
      case 'i':
        if(ITIS("IconTitle"))
        {
          SKIP("IconTitle");
          tname.on_flags |= NOICON_TITLE_FLAG;
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
            if (tname.IconBoxes == 0) { /* If first one */
              tname.IconBoxes = IconBoxes; /* chain to root */
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
            tname.value = safemalloc(len+1);
            strncpy(tname.value,restofline,len);
            tname.value[len] = 0;
            tname.off_flags |= ICON_FLAG;
            tname.on_flags |= SUPPRESSICON_FLAG;
          }
          else
            tname.on_flags |= SUPPRESSICON_FLAG;
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
          tname.off_flags |= LENIENCE_FLAG;
        }
        break;
      case 'm':
        if(ITIS("MWMBUTTONS"))
        {
          SKIP("MWMBUTTONS");
          tname.off_flags |= MWM_BUTTON_FLAG;
        }
#ifdef MINI_ICONS
	else if (ITIS("MINIICON"))
	{
          SKIP("MINIICON");
          GETWORD;
          if(len > 0)
          {
            tname.mini_value = safemalloc(len+1);
            strncpy(tname.mini_value,restofline,len);
            tname.mini_value[len] = 0;
            tname.off_flags |= MINIICON_FLAG;
          }
          restofline = tmp;
	}
#endif
        else if(ITIS("MWMBORDER"))
        {
          SKIP("MWMBORDER");
          tname.off_flags |= MWM_BORDER_FLAG;
        }
        else if(ITIS("MWMDECOR"))
        {
          SKIP("MWMDECOR");
          tname.off_flags |= MWM_DECOR_FLAG;
        }
        else if(ITIS("MWMFUNCTIONS"))
        {
          SKIP("MWMFUNCTIONS");
          tname.off_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(ITIS("MOUSEFOCUS"))
        {
          SKIP("MOUSEFOCUS");
          tname.on_flags |= CLICK_FOCUS_FLAG;
          tname.on_flags |= SLOPPY_FOCUS_FLAG;
        }
        break;
      case 'n':
        if(ITIS("NoIconTitle"))
        {
          SKIP("NoIconTitle");
          tname.off_flags |= NOICON_TITLE_FLAG;
        }
        else if(ITIS("NOICON"))
        {
          SKIP("NOICON");
          tname.off_flags |= SUPPRESSICON_FLAG;
        }
        else if(ITIS("NOTITLE"))
        {
          SKIP("NOTITLE");
          tname.off_flags |= NOTITLE_FLAG;
        }
        else if(ITIS("NoPPosition"))
        {
          SKIP("NoPPosition");
          tname.off_flags |= NO_PPOSITION_FLAG;
        }
        else if(ITIS("NakedTransient"))
        {
          SKIP("NakedTransient");
          tname.on_flags |= DECORATE_TRANSIENT_FLAG;
        }
        else if(ITIS("NODECORHINT"))
        {
          SKIP("NODECORHINT");
          tname.on_flags |= MWM_DECOR_FLAG;
        }
        else if(ITIS("NOFUNCHINT"))
        {
          SKIP("NOFUNCHINT");
          tname.on_flags |= MWM_FUNCTIONS_FLAG;
        }
        else if(ITIS("NOOVERRIDE"))
        {
          SKIP("NOOVERRIDE");
          tname.on_flags |= MWM_OVERRIDE_FLAG;
        }
        else if(ITIS("NOHANDLES"))
        {
          SKIP("NOHANDLES");
          tname.off_flags |= NOBORDER_FLAG;
        }
        else if(ITIS("NOLENIENCE"))
        {
          SKIP("NOLENIENCE");
          tname.on_flags |= LENIENCE_FLAG;
        }
        else if (ITIS("NOBUTTON"))
        {
          SKIP("NOBUTTON");

	  butt = -1; /* just in case sscanf fails */
          sscanf(restofline,"%d",&butt);
          GETWORD;
          SKIPSPACE;

          if (butt == 0) butt = 10;
          if (butt > 0 && butt <= 10)
            tname.off_buttons |= (1<<(butt-1));
          restofline = tmp;
        }
        else if(ITIS("NOOLDECOR"))
        {
          SKIP("NOOLDECOR");
          tname.on_flags |= OL_DECOR_FLAG;
        }
        break;
      case 'o':
        if(ITIS("OLDECOR"))
        {
          SKIP("OLDECOR");
          tname.off_flags |= OL_DECOR_FLAG;
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
          tname.off_flags |= RANDOM_PLACE_FLAG;
        }
        break;
      case 's':
        if(ITIS("SMARTPLACEMENT"))
        {
          SKIP("SMARTPLACEMENT");
          tname.off_flags |= SMART_PLACE_FLAG;
        }
        else if(ITIS("SkipMapping"))
        {
          SKIP("SkipMapping");
          tname.off_flags |= SHOW_MAPPING;
        }
        else if(ITIS("ShowMapping"))
        {
          SKIP("ShowMapping");
          tname.on_flags |= SHOW_MAPPING;
        }
        else if(ITIS("StickyIcon"))
        {
          SKIP("StickyIcon");
          tname.off_flags |= STICKY_ICON_FLAG;
        }
        else if(ITIS("SlipperyIcon"))
        {
          SKIP("SlipperyIcon");
          tname.on_flags |= STICKY_ICON_FLAG;
        }
        else if(ITIS("SLOPPYFOCUS"))
        {
          SKIP("SLOPPYFOCUS");
          tname.on_flags |= CLICK_FOCUS_FLAG;
          tname.off_flags |= SLOPPY_FOCUS_FLAG;
        }
        else if(ITIS("StartIconic"))
        {
          SKIP("StartIconic");
          tname.off_flags |= START_ICONIC_FLAG;
        }
        else if(ITIS("StartNormal"))
        {
          SKIP("StartNormal");
          tname.on_flags |= START_ICONIC_FLAG;
        }
        else if(ITIS("StaysOnTop"))
        {
          SKIP("StaysOnTop");
          tname.off_flags |= STAYSONTOP_FLAG;
        }
        else if(ITIS("StaysPut"))
        {
          SKIP("StaysPut");
          tname.on_flags |= STAYSONTOP_FLAG;
        }
        else if(ITIS("Sticky"))
        {
          tname.off_flags |= STICKY_FLAG;
          SKIP("Sticky");
        }
        else if(ITIS("Slippery"))
        {
          tname.on_flags |= STICKY_FLAG;
          SKIP("Slippery");
        }
        else if(ITIS("STARTSONDESK"))
        {
          SKIP("STARTSONDESK");
          tname.off_flags |= STARTSONDESK_FLAG;
          /*  RBW - 11/02/1998  */
          spargs = sscanf(restofline,"%d",&tmpno1);
          if (spargs == 1)
            {
              /*  RBW - 11/20/1998 - allow for the special case of -1  */
              tname.Desk = (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
            }
          else
	    {
              tname.off_flags &= ~STARTSONDESK_FLAG;
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
           tname.off_flags |= STARTSONDESK_FLAG;
           spargs = sscanf(restofline,"%d %d %d", &tmpno1, &tmpno2, &tmpno3);

          if (spargs == 1 || spargs == 3)
            {
            /*  We have a desk no., with or without page.  */
              /*  RBW - 11/20/1998 - allow for the special case of -1  */
              tname.Desk = (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;  /*  Desk is now actual + 1  */
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
                  tname.PageX = (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
                  tname.PageY = (tmpno3 > -1) ? tmpno3 + 1 : tmpno3;
                }
              else
                {
                  tname.PageX       =  (tmpno1 > -1) ? tmpno1 + 1 : tmpno1;
                  tname.PageY       =  (tmpno2 > -1) ? tmpno2 + 1 : tmpno2;
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
              tname.off_flags &= ~STARTSONDESK_FLAG;
              fvwm_msg(ERR,"ProcessNewStyle",
                       "bad StartsOnPage args: %s", restofline);
            }

	 }
       /**/
        else if(ITIS("STARTSANYWHERE"))
        {
          SKIP("STARTSANYWHERE");
          tname.on_flags |= STARTSONDESK_FLAG;
        }
        break;
      case 't':
        if(ITIS("TITLE"))
        {
          SKIP("TITLE");
          tname.on_flags |= NOTITLE_FLAG;
        }
        break;
      case 'u':
        if(ITIS("UsePPosition"))
        {
          SKIP("UsePPosition");
          tname.on_flags |= NO_PPOSITION_FLAG;
        }
#ifdef USEDECOR
        if(ITIS("UseDecor"))
        {
          SKIP("UseDecor");
          GETQUOTEDWORD;
          if (len > 0)
          {
            tname.Decor = safemalloc(len+1);
            strncpy(tname.Decor,restofline,len);
            tname.Decor[len] = 0;
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
            for ( nptr = Scr.TheList; nptr; nptr = nptr->next ) {
              if (!strncasecmp(restofline,nptr->name,len)) { /* match style */
                if (!hit) {             /* first match */
		  char *save_name;
		  save_name = tname.name;
                  memcpy((void*)&tname, (const void*)nptr, sizeof(name_list)); /* copy everything */
                  tname.next = 0;       /* except the next pointer */
		  tname.name = save_name; /* and the name */
                  hit = 1;              /* set not first match */
                } else {                /* subsequent match */
                  tname.off_flags     |= nptr->off_flags;
                  tname.on_flags      &= ~(nptr->on_flags);
                  tname.off_buttons   |= nptr->off_buttons;
                  tname.on_buttons    &= ~(nptr->on_buttons);
                  if(nptr->value) tname.value = nptr->value;
#ifdef MINI_ICONS
                  if(nptr->mini_value) tname.mini_value = nptr->mini_value;
#endif
#ifdef USEDECOR
                  if(nptr->Decor) tname.Decor = nptr->Decor;
#endif
                  if(nptr->off_flags & STARTSONDESK_FLAG)
		    /*  RBW - 11/02/1998  */
		    {
                      tname.Desk = nptr->Desk;
                      tname.PageX = nptr->PageX;
                      tname.PageY = nptr->PageY;
		    }
		    /**/
                  if(nptr->off_flags & BW_FLAG)
                    tname.border_width = nptr->border_width;
                  if(nptr->off_flags & NOBW_FLAG)
                    tname.resize_width = nptr->resize_width;
                  if(nptr->off_flags & FORE_COLOR_FLAG)
                    tname.ForeColor = nptr->ForeColor;
                  if(nptr->off_flags & BACK_COLOR_FLAG)
                    tname.BackColor = nptr->BackColor;
                  tname.IconBoxes = nptr->IconBoxes; /* use same chain */
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
          }
          while(isspace(*restofline)) restofline++;
        }
        break;
      case 'v':
        break;
      case 'w':
        if(ITIS("WindowListSkip"))
        {
          SKIP("WindowListSkip");
          tname.off_flags |= LISTSKIP_FLAG;
        }
        else if(ITIS("WindowListHit"))
        {
          SKIP("WindowListHit");
          tname.on_flags |= LISTSKIP_FLAG;
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
  if(strcmp(tname.name,"*") == 0)
  {
    if(tname.off_flags & ICON_FLAG)
      Scr.DefaultIcon = tname.value;
    tname.off_flags &= ~ICON_FLAG;
    tname.value = NULL;
  }
  AddToList(&tname);                /* add temp name list to list */
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

static void AddToList(name_list *tname)
{
  name_list *nptr,*lastptr = NULL;

  /* This used to contain logic that returned if the style didn't contain
     anything.  I don't see why we should bother. dje. */

  /* used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */

  /* seems like a pretty inefficient way to keep track of the end
     of the list, but how long can the style list be? dje */
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next) {
    lastptr=nptr;                       /* find end of style list */
  }

  nptr = (name_list *)safemalloc(sizeof(name_list)); /* malloc area */
  memcpy((void*)nptr, (const void*)tname, sizeof(name_list)); /* copy term area into list */
  if(lastptr != NULL)                   /* If not first entry in list */
    lastptr->next = nptr;               /* chain this entry to the list */
  else                                  /* else first entry in list */
    Scr.TheList = nptr;                 /* set the list root pointer. */
} /* end function */

