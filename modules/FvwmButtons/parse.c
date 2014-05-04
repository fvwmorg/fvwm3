/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
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
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include <ctype.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "libs/fvwmlib.h"
#include "libs/FScreen.h"
#include "libs/Module.h"
#include "libs/Colorset.h"
#include "libs/Parse.h"
#include "libs/Strings.h"
#include "FvwmButtons.h"
#include "button.h"
#include "parse.h"

extern int w, h, x, y, xneg, yneg; /* used in ParseConfigLine */
extern char *config_file;
extern int button_width;
extern int button_height;
extern int has_button_geometry;
extern int screen;

/* contains the character that terminated the last string from seekright */
static char terminator = '\0';

/* ----------------------------------- macros ------------------------------ */

static char *trimleft(char *s)
{
	while (s && isspace((unsigned char)*s))
	{
		s++;
	}
	return s;
}

/**
*** seekright()
***
*** split off the first continous section of the string into a new allocated
*** string and move the old pointer forward. Accepts and strips quoting with
*** ['"`], and the current quote q can be escaped inside the string with \q.
**/
static char *seekright(char **s)
{
	char *token = NULL;
	char *line = *s;

	line = DoGetNextToken(line, &token, NULL, "),", &terminator);
	if (*s != NULL && line == NULL)
	{
		line = strchr(*s, '\0');
	}
	*s = line;

	return token;
}

static char *my_get_font(char **s)
{
	char *option;
	int len;

	*s = GetNextFullOption(*s, &option);
	if (option)
	{
		for (len = strlen(option) - 1; len >= 0 && isspace(option[len]); len--)
		{
			/* remove trailing whitespace */
			option[len] = 0;
		}
		for ( ; *option && isspace(*option); option++)
		{
			/* remove leading whitespace */
		}
	}

	len = strlen(option) -1;
	if (option[len] == ')' )
	{
		option[len] = 0;
	}

	return option;
}

/**
*** ParseBack()
*** Parses the options possible to Back
*** This function appears to be obsolete & should be removed -- SS.
**/
static int ParseBack(char **ss)
{
	char *opts[] =
	{
		"icon",
		NULL
	};
	char *t, *s = *ss;
	int r = 0;

	while (*s && *s != ')')
	{
		s = trimleft(s);
		if (*s == ',')
		{
			s++;
			continue;
		}

		switch (GetTokenIndex(s, opts, -1, &s))
		{
		case 0: /* Icon */
			r = 1;
			fprintf(stderr,
				"%s: Back(Icon) not supported yet\n",
				MyName);
			break;
		default:
			t = seekright(&s);
			fprintf(stderr,
				"%s: Illegal back option \"%s\"\n",
				MyName, (t) ? t : "");
			if (t)
			{
				free(t);
			}
		}
	}
	if (*s)
	{
		s++;
	}
	*ss = s;

	return r;
}

/**
*** ParseBoxSize()
*** Parses the options possible to BoxSize
**/
static void ParseBoxSize(char **ss, flags_type *flags)
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
	{
		return;
	}
	*ss = GetNextTokenIndex(*ss, opts, 0, &m);
	switch (m)
	{
	case 0:
		flags->b_SizeFixed = 0;
		flags->b_SizeSmart = 0;
		break;
	case 1:
		flags->b_SizeFixed = 1;
		flags->b_SizeSmart = 0;
		break;
	case 2:
		flags->b_SizeSmart = 1;
		flags->b_SizeFixed = 0;
		break;
	default:
		flags->b_SizeFixed = 0;
		flags->b_SizeSmart = 0;
		fprintf(stderr,
			"%s: Illegal boxsize option \"%s\"\n",
			MyName, s);
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
	char *t, *s = *ss;

	while (*s && *s != ')')
	{
		s = trimleft(s);
		if (*s == ',')
		{
			s++;
			continue;
		}

		switch (GetTokenIndex(s, titleopts, -1, &s))
		{
		case 0: /* Left */
			*flags &= ~b_TitleHoriz;
			*flags &= ~3;
			*mask |= b_TitleHoriz;
			break;
		case 1: /* Right */
			*flags &= ~b_TitleHoriz;
			*flags &= ~3;
			*flags |= 2;
			*mask |= b_TitleHoriz;
			break;
		case 2: /* Center */
			*flags &= ~b_TitleHoriz;
			*flags &= ~3;
			*flags |= 1;
			*mask |= b_TitleHoriz;
			break;
		case 3: /* Side */
			*flags |= b_Horizontal;
			*flags &= ~3;
			*mask |= b_Horizontal;
			break;
		default:
			t = seekright(&s);
			fprintf(stderr,
				"%s: Illegal title option \"%s\"\n",
				MyName, (t) ? t : "");
			if (t)
			{
				free(t);
			}
		}
	}
	if (*s)
	{
		s++;
	}
	*ss = s;
}

/**
*** ParseSwallow()
*** Parses the options possible to Swallow
**/
static void ParseSwallow(
	char **ss, unsigned int *flags, unsigned int *mask, button_info *b)
{
	char *swallowopts[] =
	{
		"nohints", "hints",
		"nokill", "kill",
		"noclose", "close",
		"respawn", "norespawn",
		"useold", "noold",
		"usetitle", "notitle",
		"fvwmmodule", "nofvwmmodule",
		"swallownew",
		NULL
	};
	char *t, *s = *ss;

	while (*s && *s != ')')
	{
		s = trimleft(s);
		if (*s == ',')
		{
			s++;
			continue;
		}

		switch (GetTokenIndex(s, swallowopts, -1, &s))
		{
		case 0: /* NoHints */
			*flags |= b_NoHints;
			*mask |= b_NoHints;
			break;
		case 1: /* Hints */
			*flags &= ~b_NoHints;
			*mask |= b_NoHints;
			break;
		case 2: /* NoKill */
			*flags &= ~b_Kill;
			*mask |= b_Kill;
			break;
		case 3: /* Kill */
			*flags |= b_Kill;
			*mask |= b_Kill;
			break;
		case 4: /* NoClose */
			*flags |= b_NoClose;
			*mask |= b_NoClose;
			break;
		case 5: /* Close */
			*flags &= ~b_NoClose;
			*mask |= b_NoClose;
			break;
		case 6: /* Respawn */
			*flags |= b_Respawn;
			*mask |= b_Respawn;
			b->newflags.do_swallow_new = 0;
			break;
		case 7: /* NoRespawn */
			*flags &= ~b_Respawn;
			*mask |= b_Respawn;
			b->newflags.do_swallow_new = 0;
			break;
		case 8: /* UseOld */
			*flags |= b_UseOld;
			*mask |= b_UseOld;
			break;
		case 9: /* NoOld */
			*flags &= ~b_UseOld;
			*mask |= b_UseOld;
			break;
		case 10: /* UseTitle */
			*flags |= b_UseTitle;
			*mask |= b_UseTitle;
			break;
		case 11: /* NoTitle */
			*flags &= ~b_UseTitle;
			*mask |= b_UseTitle;
			break;
		case 12: /* FvwmModule */
			*flags |= b_FvwmModule;
			*mask |= b_FvwmModule;
			break;
		case 13: /* NoFvwmModule */
			*flags &= ~b_FvwmModule;
			*mask |= b_FvwmModule;
			break;
		case 14: /* SwallowNew */
			*flags &= ~b_Respawn;
			*mask |= b_Respawn;
			b->newflags.do_swallow_new = 1;
			break;
		default:
			t = seekright(&s);
			fprintf(stderr,
				"%s: Illegal Swallow option \"%s\"\n", MyName,
				(t) ? t : "");
			if (t)
			{
				free(t);
			}
		}
	}
	if (*s)
	{
		s++;
	}
	*ss = s;
}

/**
*** ParsePanel()
*** Parses the options possible to Panel
**/
static void ParsePanel(
	char **ss, unsigned int *flags, unsigned int *mask, char *direction,
	int *steps, int *delay, panel_flags_type *panel_flags,
	int *indicator_size, int *rela_x, int *rela_y, char *position,
	char *context)
{
	char *swallowopts[] =
	{
		"nohints", "hints",
		"nokill", "kill",
		"noclose", "close",
		"respawn", "norespawn",
		"useold", "noold",
		"usetitle", "notitle",
		"up", "down", "left", "right",
		"steps",
		"delay",
		"smooth",
		"noborder",
		"indicator",
		"position",
		NULL
	};

	char *positionopts[] =
	{
		"button", "module", "root", "center", "left", "top", "right",
		"bottom", "noplr", "noptb", "mlr", "mtb", NULL
	};

	char *t, *s = *ss;
	int n;

	while (*s && *s != ')')
	{
		s = trimleft(s);
		if (*s == ',')
		{
			s++;
			continue;
		}

		switch (GetTokenIndex(s, swallowopts, -1, &s))
		{
		case 0: /* NoHints */
			*flags |= b_NoHints;
			*mask |= b_NoHints;
			break;
		case 1: /* Hints */
			*flags &= ~b_NoHints;
			*mask |= b_NoHints;
			break;
		case 2: /* NoKill */
			*flags &= ~b_Kill;
			*mask |= b_Kill;
			break;
		case 3: /* Kill */
			*flags |= b_Kill;
			*mask |= b_Kill;
			break;
		case 4: /* NoClose */
			*flags |= b_NoClose;
			*mask |= b_NoClose;
			break;
		case 5: /* Close */
			*flags &= ~b_NoClose;
			*mask |= b_NoClose;
			break;
		case 6: /* Respawn */
			*flags |= b_Respawn;
			*mask |= b_Respawn;
			break;
		case 7: /* NoRespawn */
			*flags &= ~b_Respawn;
			*mask |= b_Respawn;
			break;
		case 8: /* UseOld */
			*flags |= b_UseOld;
			*mask |= b_UseOld;
			break;
		case 9: /* NoOld */
			*flags &= ~b_UseOld;
			*mask |= b_UseOld;
			break;
		case 10: /* UseTitle */
			*flags |= b_UseTitle;
			*mask |= b_UseTitle;
			break;
		case 11: /* NoTitle */
			*flags &= ~b_UseTitle;
			*mask |= b_UseTitle;
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
		case 18: /* smooth */
			(*panel_flags).smooth = 1;
			break;
		case 19: /* noborder */
			(*panel_flags).ignore_lrborder = 1;
			(*panel_flags).ignore_tbborder = 1;
			break;
		case 20: /* indicator */
			n = 0;
			(*panel_flags).panel_indicator = 1;
			*indicator_size = 0;
			sscanf(s, "%d%n", indicator_size, &n);
			if (*indicator_size < 0 || *indicator_size > 100)
			{
				*indicator_size = 0;
			}
			s += n;
			break;
		case 21: /* position */
			n = 0;
			*rela_x = *rela_y = 0;
			while (*s != ',' && *s != ')' && *s)
			{
				s = trimleft(s);
				/* get x and y offset */
				if (isdigit(*s) || *s == '+' || *s == '-')
				{
					sscanf(s, "%i%n", rela_x, &n);
					s += n;
					if (*s == 'p' || *s == 'P')
					{
						(*panel_flags).relative_x_pixel = 1;
						s++;
					}
					n = 0;
					s = trimleft(s);
					sscanf(s, "%i%n", rela_y, &n);
					s += n;
					if (*s == 'p' || *s == 'P')
					{
						(*panel_flags).relative_y_pixel = 1;
						s++;
					}
					s = trimleft(s);
				}
				switch (GetTokenIndex(s, positionopts, -1, &s))
				{
				case 0: /* button */
					*context = SLIDE_CONTEXT_PB;
					break;
				case 1: /* module */
					*context = SLIDE_CONTEXT_MODULE;
					break;
				case 2: /* root */
					*context = SLIDE_CONTEXT_ROOT;
					break;
				case 3: /* center */
					*position = SLIDE_POSITION_CENTER;
					break;
				case 4: /* left */
					*position = SLIDE_POSITION_LEFT_TOP;
					break;
				case 5: /* top  */
					*position = SLIDE_POSITION_LEFT_TOP;
					break;
				case 6: /* right */
					*position = SLIDE_POSITION_RIGHT_BOTTOM;
					break;
				case 7: /* bottom */
					*position = SLIDE_POSITION_RIGHT_BOTTOM;
					break;
				case 8: /* noplr */
					(*panel_flags).ignore_lrborder = 1;
					break;
				case 9: /* noptb */
					(*panel_flags).ignore_tbborder = 1;
					break;
				case 10: /* mlr */
					(*panel_flags).buttons_lrborder = 1;
					break;
				case 11: /* mtb */
					(*panel_flags).buttons_tbborder = 1;
					break;
				default:
					t = seekright(&s);
					s--;
					if (t)
					{
						fprintf(stderr,
							"%s: Illegal Panel "
							"position option %s\n",
							MyName,	(t) ? t : "");
						free(t);
					}
				}
			}
			break;
		default:
			t = seekright(&s);
			fprintf(stderr,
				"%s: Illegal Panel option \"%s\"\n", MyName,
				(t) ? t : "");
			if (t)
			{
				free(t);
			}
		}
	}
	if (*s)
	{
		s++;
	}
	*ss = s;
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
	char *t, *o, *s = *ss;
	int i, j;

	while (*s && *s != ')')
	{
		s = trimleft(s);
		if (*s == ',')
		{
			s++;
			continue;
		}

		switch (GetTokenIndex(s, conts, -1, &s))
		{
		case 0: /* Columns */
			b->c->num_columns = max(1, strtol(s, &t, 10));
			s = t;
			break;
		case 1: /* Rows */
			b->c->num_rows = max(1, strtol(s, &t, 10));
			s = t;
			break;
		case 2: /* Font */
			if (b->c->font_string)
			{
				free(b->c->font_string);
			}
			b->c->font_string = my_get_font(&s);
			b->c->flags.b_Font = (b->c->font_string ? 1 : 0);
			break;
		case 3: /* Frame */
			b->c->framew = strtol(s, &t, 10);
			b->c->flags.b_Frame = 1;
			s = t;
			break;
		case 4: /* Back */
			s = trimleft(s);
			if (*s == '(' && s++ && ParseBack(&s))
			{
				b->c->flags.b_IconBack = 1;
			}
			if (b->c->back)
			{
				free(b->c->back);
			}
			b->c->back = seekright(&s);
			if (b->c->back)
			{
				b->c->flags.b_Back = 1;
			}
			else
			{
				b->c->flags.b_IconBack = 0;
				b->c->flags.b_Back = 0;
			}
			break;
    case 5: /* Fore */
      if (b->c->fore) free(b->c->fore);
      b->c->fore = seekright(&s);
      b->c->flags.b_Fore = (b->c->fore ? 1 : 0);
      break;
    case 6: /* Padding */
      i = strtol(s, &t, 10);
      if (t > s)
      {
	b->c->xpad = b->c->ypad = i;
	s = t;
	i = strtol(s, &t, 10);
	if (t > s)
	{
	  b->c->ypad = i;
	  s = t;
	}
	b->c->flags.b_Padding = 1;
      }
      else
	fprintf(stderr,"%s: Illegal padding argument\n",MyName);
      break;
    case 7: /* Title - flags */
      s = trimleft(s);
      if (*s == '(' && s++)
      {
	b->c->justify = 0;
	b->c->justify_mask = 0;
	ParseTitle(&s, &b->c->justify, &b->c->justify_mask);
	if (b->c->justify_mask)
	{
	  b->c->flags.b_Justify = 1;
	}
      }
      else
      {
	char *temp;
	fprintf(stderr,
		"%s: Illegal title in container options\n",
		MyName);
	temp = seekright(&s);
	if (temp)
	{
	  free(temp);
	}
      }
      break;
    case 8: /* Swallow - flags */
      {
	Bool failed = False;

	s = trimleft(s);
	if (b->c->flags.b_Swallow || b->c->flags.b_Panel)
	{
	  fprintf(stderr, "%s: Multiple Swallow or Panel options are not"
		" allowed in a single button", MyName);
	  failed = True;
	}
	else if (*s == '(' && s++)
	{
	  b->c->swallow = 0;
	  b->c->swallow_mask = 0;
	  ParseSwallow(&s, &b->c->swallow, &b->c->swallow_mask, b);
	  if (b->c->swallow_mask)
	  {
	    b->c->flags.b_Swallow = 1;
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
      b->c->flags.b_Size = 1;
      b->c->minx = b->c->miny = 0;
      break;

    case 10: /* Size */
      i = strtol(s, &t, 10);
      j = strtol(t, &o, 10);
      if (t > s && o > t)
      {
	b->c->minx = i;
	b->c->miny = j;
	b->c->flags.b_Size = 1;
	s = o;
      }
      else
	fprintf(stderr,"%s: Illegal size arguments\n",MyName);
      break;

    case 11: /* BoxSize */
      ParseBoxSize(&s, &b->c->flags);
      break;

    case 12: /* Colorset */
      i = strtol(s, &t, 10);
      if (t > s)
      {
	b->c->colorset = i;
	b->c->flags.b_Colorset = 1;
	AllocColorset(i);
	s = t;
      }
      else
      {
	b->c->flags.b_Colorset = 0;
      }
      break;

    default:
      t = seekright(&s);
      fprintf(stderr,"%s: Illegal container option \"%s\"\n",MyName, (t)?t:"");
      if (t)
	free(t);
    }
  }
  if (*s)
  {
    s++;
  }
  *ss = s;
}

/**
*** ParseButton()
***
*** parse a buttonconfig string.
*** *FvwmButtons: (option[ options]) title iconname command
**/
/*#define DEBUG_PARSER*/
static void ParseButton(button_info **uberb, char *s)
{
	button_info *b, *ub = *uberb;
	int i, j;
	char *t, *o;

	b = alloc_button(ub, (ub->c->num_buttons)++);
	s = trimleft(s);

	if (*s == '(' && s++)
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
			"actionignoresclientwindow",
			"actiononpress",
			"container",
			"end",
			"nosize",
			"size",
			"left",
			"right",
			"center",
			"colorset",
			"action",
			"id",
			"activeicon",
			"activetitle",
			"pressicon",
			"presstitle",
			"activecolorset",
			"presscolorset",
			"top",
			NULL
		};
		s = trimleft(s);
		while (*s && *s != ')')
		{
			Bool is_swallow = False;

			if (*s == ',')
			{
				s++;
				s = trimleft(s);
				continue;
			}
			if (isdigit(*s) || *s == '+' || *s == '-')
			{
				char *geom;
				int x, y, flags;
				unsigned int w, h;
				geom = seekright(&s);
				if (geom)
				{
					flags = XParseGeometry(geom, &x, &y, &w, &h);
					if (flags&WidthValue)
					{
						b->BWidth = w;
					}
					if (flags&HeightValue)
					{
						b->BHeight = h;
					}
					if (flags & XValue)
					{
						b->BPosX = x;
						b->flags.b_PosFixed = 1;
					}
					if (flags & YValue)
					{
						b->BPosY = y;
						b->flags.b_PosFixed = 1;
					}
					if (flags & XNegative)
					{
						b->BPosX = -1 - x;
					}
					if (flags & YNegative)
					{
						b->BPosY = -1 - y;
					}
					free(geom);
				}
				s = trimleft(s);
				continue;
			}
			switch (GetTokenIndex(s, opts, -1, &s))
			{
			case 0: /* Back */
				s = trimleft(s);
				if (*s == '(' && s++ && ParseBack(&s))
				{
					b->flags.b_IconBack = 1;
				}
				if (b->flags.b_Back && b->back)
				{
					free(b->back);
				}
				b->back = seekright(&s);
				if (b->back)
				{
					b->flags.b_Back = 1;
				}
				else
				{
					b->flags.b_IconBack = 0;
					b->flags.b_Back = 0;
				}
				break;

			case 1: /* Fore */
				if (b->flags.b_Fore && b->fore)
				{
					free(b->fore);
				}
				b->fore = seekright(&s);
				b->flags.b_Fore = (b->fore ? 1 : 0);
				break;

			case 2: /* Font */
				if (b->flags.b_Font && b->font_string)
				{
					free(b->font_string);
				}
				b->font_string = my_get_font(&s);
				b->flags.b_Font = (b->font_string ? 1 : 0);
				break;

			/* ----------------- title --------------- */

			case 3: /* Title */
				s = trimleft(s);
				if (*s == '(' && s++)
				{
					b->justify = 0;
					b->justify_mask = 0;
					ParseTitle(
						&s, &b->justify,
						&b->justify_mask);
					if (b->justify_mask)
					{
						b->flags.b_Justify = 1;
					}
				}
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->title)
					{
						free(b->title);
					}
					b->title = t;
#ifdef DEBUG_PARSER
					fprintf(stderr,
						"PARSE: Title \"%s\"\n",
						b->title);
#endif
					b->flags.b_Title = 1;
				}
				else
				{
					fprintf(stderr,
						"%s: Missing title argument\n",
						MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* ------------------ icon --------------- */

			case 4: /* Icon */
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->flags.b_Swallow)
					{
						fprintf(stderr,
							"%s: a button can not "
							"have an icon and a "
							"swallowed window at "
							"the same time. "
							"Ignoring icon\n",
							MyName);
					}
					else
					{
						if (b->icon_file)
						{
							free(b->icon_file);
						}
						b->icon_file = t;
						b->flags.b_Icon = 1;
					}
				}
				else
				{
					fprintf(stderr,
						"%s: Missing Icon argument\n",
						MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* ----------------- frame --------------- */

			case 5: /* Frame */
				i = strtol(s, &t, 10);
				if (t > s)
				{
					b->flags.b_Frame = 1;
					b->framew = i;
					s = t;
				}
				else
				{
					fprintf(stderr,
						"%s: Illegal frame argument\n",
						MyName);
				}
				break;

			/* ---------------- padding -------------- */

			case 6: /* Padding */
				i = strtol(s,&t,10);
				if (t>s)
				{
					b->xpad = b->ypad = i;
					b->flags.b_Padding = 1;
					s = t;
					i = strtol(s, &t, 10);
					if (t > s)
					{
						b->ypad = i;
						s = t;
					}
				}
				else
				{
					fprintf(stderr,
						"%s: Illegal padding "
						"argument\n", MyName);
				}
				break;

			/* ---------------- swallow -------------- */

			case 7: /* Swallow */
				is_swallow = True;
				/* fall through */

			case 8: /* Panel */
				s = trimleft(s);
				if (is_swallow)
				{
					b->swallow = 0;
					b->swallow_mask = 0;
				}
				else
				{
					/* set defaults */
					b->swallow = b_Respawn;
					b->swallow_mask = b_Respawn;
					b->slide_direction = SLIDE_UP;
					b->slide_position = SLIDE_POSITION_CENTER;
					b->slide_context = SLIDE_CONTEXT_PB;
					b->relative_x = 0;
					b->relative_y = 0;
					b->slide_steps = 12;
					b->slide_delay_ms = 5;
				}
				if (*s == '(' && s++)
				{
					if (is_swallow)
					{
						ParseSwallow(
							&s, &b->swallow,
							&b->swallow_mask, b);
					}
					else
					{
						ParsePanel(
							&s, &b->swallow,
							&b->swallow_mask,
							&b->slide_direction,
							&b->slide_steps,
							&b->slide_delay_ms,
							&b->panel_flags,
							&b->indicator_size,
							&b->relative_x,
							&b->relative_y,
							&b->slide_position,
							&b->slide_context);
					}
				}
				t = seekright(&s);
				o = seekright(&s);
				if (t)
				{
					if (b->hangon)
					{
						free(b->hangon);
					}
					b->hangon = t;
					if (is_swallow)
					{
						if (b->flags.b_Icon)
						{
							fprintf(stderr,
							"%s: a button can not "
							"have an icon and a "
							"swallowed window at "
							"the same time. "
							"Ignoring icon\n",
							MyName);
							b->flags.b_Icon = 0;
						}

						b->flags.b_Swallow = 1;
						b->flags.b_Hangon = 1;
					}
					else
					{
						b->flags.b_Panel = 1;
						b->flags.b_Hangon = 1;
						b->newflags.is_panel = 1;
						b->newflags.panel_mapped = 0;
					}
					b->swallow |= 1;
					if (!(b->swallow & b_NoHints))
					{
						b->hints = xmalloc(
							sizeof(XSizeHints));
					}
					if (o)
					{
						char *p;

						p = module_expand_action(
							Dpy, screen, o, NULL,
							UberButton->c->fore,
							UberButton->c->back);
						if (p)
						{
							if (!(buttonSwallow(b)&b_UseOld))
							{
								exec_swallow(p,b);
							}
							if (b->spawn)
							{
								free(b->spawn);
							}

							/* Might be needed if
							 * respawning sometime
							 */
							b->spawn = o;
							free(p);
						}
					}
				}
				else
				{
					fprintf(stderr,
						"%s: Missing swallow "
						"argument\n", MyName);
					if (t)
					{
						free(t);
					}
					if (o)
					{
						free(o);
					}
				}
				/* check if it is a module by command line inspection if
				 * this hints has not been given in the swallow option */
				if (is_swallow && !(b->swallow_mask & b_FvwmModule))
				{
					if (b->spawn != NULL &&
					    (strncasecmp(b->spawn, "module", 6) == 0 ||
					     strncasecmp(b->spawn, "fvwm", 4) == 0))
					{
						b->swallow |= b_FvwmModule;
					}
				}
				break;

			case 9: /* ActionIgnoresClientWindow */
				b->flags.b_ActionIgnoresClientWindow = 1;
				break;

			case 10: /* ActionOnPress */
				b->flags.b_ActionOnPress = 1;
				break;

			/* ---------------- container ------------ */

			case 11: /* Container */
				/*
				 * SS: This looks very buggy! The FvwmButtons
				 * man page suggests it's here to restrict the
				 * options used with "Container", but it only
				 * restricts those options appearing _before_
				 * the "Container" keyword for a button.
				 * b->flags&=b_Frame|b_Back|b_Fore|b_Padding|b_Action;
				 */

				MakeContainer(b);
				*uberb = b;
				s = trimleft(s);
				if (*s == '(' && s++)
				{
					ParseContainer(&s,b);
				}
				break;

			case 12: /* End */
				*uberb = ub->parent;
				ub->c->buttons[--(ub->c->num_buttons)] = NULL;
				free(b);
				if (!ub->parent)
				{
					fprintf(stderr,"%s: Unmatched END in config file\n",MyName);
					exit(1);
				}
				if (ub->parent->c->flags.b_Colorset ||
				    ub->parent->c->flags.b_ColorsetParent)
				{
					ub->c->flags.b_ColorsetParent = 1;
				}
				if (ub->parent->c->flags.b_IconBack || ub->parent->c->flags.b_IconParent)
				{
					ub->c->flags.b_IconParent = 1;
				}
				return;

			case 13: /* NoSize */
				b->flags.b_Size = 1;
				b->minx = b->miny = 0;
				break;

			case 14: /* Size */
				i = strtol(s, &t, 10);
				j = strtol(t, &o, 10);
				if (t > s && o > t)
				{
					b->minx = i;
					b->miny = j;
					b->flags.b_Size = 1;
					s = o;
				}
				else
				{
					fprintf(stderr,
						"%s: Illegal size arguments\n",
						MyName);
				}
				break;

			case 15: /* Left */
				b->flags.b_Left = 1;
				b->flags.b_Right = 0;
				break;

			case 16: /* Right */
				b->flags.b_Right = 1;
				b->flags.b_Left = 0;
				break;

			case 17: /* Center */
				b->flags.b_Right = 0;
				b->flags.b_Left = 0;
				break;

			case 18: /* Colorset */
				i = strtol(s, &t, 10);
				if (t > s)
				{
					b->colorset = i;
					b->flags.b_Colorset = 1;
					s = t;
					AllocColorset(i);
				}
				else
				{
					b->flags.b_Colorset = 0;
				}
				break;

			/* ----------------- action -------------- */

			case 19: /* Action */
				s = trimleft(s);
				i = 0;
				if (*s == '(')
				{
					s++;
					if (strncasecmp(s, "mouse", 5) != 0)
					{
						fprintf(stderr,
							"%s: Couldn't parse "
							"action\n", MyName);
					}
					s += 5;
					i = strtol(s, &t, 10);
					s = t;
					while (*s && *s != ')')
					{
						s++;
					}
					if (*s == ')')
					{
						s++;
					}
				}
				{
					char *r;
					char *u = s;

					s = GetQuotedString(s, &t, ",)", NULL, "(", ")");
					r = s;
					if (t && r > u + 1)
					{
						/* remove unquoted trailing spaces */
						r -= 2;
						while (r >= u && isspace(*r))
						{
							r--;
						}
						r++;
						if (isspace(*r))
						{
							t[strlen(t) - (s - r - 1)] = 0;
						}
					}
				}
				if (t)
				{
					AddButtonAction(b,i,t);
					free(t);
				}
				else
				{
					fprintf(stderr,
						"%s: Missing action argument\n",
						MyName);
				}
				break;

			case 20: /* Id */
				s = trimleft(s);
				s = DoGetNextToken(s, &t, NULL, ",)", &terminator);

				/* it should include the delimiter */
				if (s && terminator == ')')
				{
					s--;
				}

				if (t)
				{
					if (isalpha(t[0]))
					{
						/* should check for duplicate ids first... */
						b->flags.b_Id = 1;
						if (b->id)
						{
							free(b->id);
						}
						CopyString(&b->id, t);
					}
			        	else
					{
						fprintf(stderr,
							"%s: Incorrect id '%s' "
							"ignored\n", MyName, t);
					}
					free(t);
				}
				else
				{
					fprintf(stderr,
						"%s: Missing id argument\n",
						MyName);
				}
				break;

			/* ------------------ ActiveIcon --------------- */
			case 21: /* ActiveIcon */
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->flags.b_Swallow)
					{
						fprintf(stderr,
							"%s: a button can not "
							"have a ActiveIcon and "
							"a swallowed window at "
							"the same time. "
							"Ignoring ActiveIcon.\n",
							MyName);
					}
					else
					{
						if (b->active_icon_file)
						{
							free(b->active_icon_file);
						}
						b->active_icon_file = t;
						b->flags.b_ActiveIcon = 1;
					}
				}
				else
				{
					fprintf(stderr,
						"%s: Missing ActiveIcon "
						"argument\n", MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* --------------- ActiveTitle --------------- */
			case 22: /* ActiveTitle */
				s = trimleft(s);
				if (*s == '(')
				{
					fprintf(stderr,
						"%s: justification not allowed "
						"for ActiveTitle.\n", MyName);
				}
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->activeTitle)
					{
						free(b->activeTitle);
					}
					b->activeTitle = t;
#ifdef DEBUG_PARSER
					fprintf(stderr,
						"PARSE: ActiveTitle \"%s\"\n",
						b->activeTitle);
#endif
					b->flags.b_ActiveTitle = 1;
				}
				else
				{
					fprintf(stderr,
						"%s: Missing ActiveTitle "
						"argument\n", MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* --------------- PressIcon --------------- */
			case 23: /* PressIcon */
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->flags.b_Swallow)
					{
						fprintf(stderr,
							"%s: a button can not "
							"have a PressIcon and "
							"a swallowed window at "
							"the same time. "
							"Ignoring PressIcon.\n",
							MyName);
					}
					else
					{
						if (b->press_icon_file)
						{
							free(b->press_icon_file);
						}
						b->press_icon_file = t;
						b->flags.b_PressIcon = 1;
					}
				}
				else
				{
					fprintf(stderr,
						"%s: Missing PressIcon "
						"argument\n", MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* --------------- PressTitle --------------- */
			case 24: /* PressTitle */
				s = trimleft(s);
				if (*s == '(')
				{
					fprintf(stderr,
						"%s: justification not allowed "
						"for PressTitle.\n", MyName);
				}
				t = seekright(&s);
				if (t && *t && (t[0] != '-' || t[1] != 0))
				{
					if (b->pressTitle)
					{
						free(b->pressTitle);
					}
					b->pressTitle = t;
#ifdef DEBUG_PARSER
					fprintf(stderr,
						"PARSE: PressTitle \"%s\"\n",
						b->pressTitle);
#endif
					b->flags.b_PressTitle = 1;
				}
				else
				{
					fprintf(stderr,
						"%s: Missing PressTitle "
						"argument\n", MyName);
					if (t)
					{
						free(t);
					}
				}
				break;

			/* --------------- --------------- */
			case 25: /* ActiveColorset */
				i = strtol(s, &t, 10);
				if (t > s)
				{
					b->activeColorset = i;
					b->flags.b_ActiveColorset = 1;
					s = t;
					AllocColorset(i);
				}
				else
				{
					b->flags.b_ActiveColorset = 0;
				}
				break;

			/* --------------- --------------- */
			case 26: /* PressColorset */
				i = strtol(s, &t, 10);
				if (t > s)
				{
					b->pressColorset = i;
					b->flags.b_PressColorset = 1;
					s = t;
					AllocColorset(i);
				}
				else
				{
					b->flags.b_PressColorset = 0;
				}
				break;

			case 27: /* top */
				b->flags.b_Top = 1;
				break;
			/* --------------- --------------- */
			default:
				t = seekright(&s);
				fprintf(stderr,
					"%s: Illegal button option \"%s\"\n",
					MyName, (t) ? t : "");
				if (t)
				{
					free(t);
				}
				break;
			} /* end switch */
			s = trimleft(s);
		}
		if (s && *s)
		{
			s++;
			s = trimleft(s);
		}
	}

	/* get title and iconname */
	if (!b->flags.b_Title)
	{
		b->title = seekright(&s);
		if (b->title && *b->title &&
			((b->title)[0] != '-' || (b->title)[1] != 0))
		{
			b->flags.b_Title = 1;
		}
		else if (b->title)
		{
			free(b->title);
		}
	}
	else
	{
		char *temp;
		temp = seekright(&s);
		if (temp)
		{
			free(temp);
		}
	}

	if (!b->flags.b_Icon)
	{
		b->icon_file = seekright(&s);
		if (b->icon_file && b->icon_file &&
			 ((b->icon_file)[0] != '-'||(b->icon_file)[1] != 0))
		{
			b->flags.b_Icon = 1;
		}
		else if (b->icon_file)
		{
			free(b->icon_file);
		}
	}
	else
	{
		char *temp;
		temp = seekright(&s);
		if (temp)
		{
			free(temp);
		}
	}

	s = trimleft(s);

	/* Swallow hangon command */
	if (strncasecmp(s, "swallow", 7) == 0 || strncasecmp(s, "panel", 7) == 0)
	{
		if (b->flags.b_Swallow || b->flags.b_Panel)
		{
			fprintf(stderr,
				"%s: Illegal with both old and new swallow!\n",
				MyName);
			exit(1);
		}
		s += 7;
		/*
		 * Swallow old 'swallowmodule' command
		 */
		if (strncasecmp(s, "module", 6) == 0)
		{
			s += 6;
		}
		if (b->hangon)
		{
			free(b->hangon);
		}
		b->hangon = seekright(&s);
		if (!b->hangon)
		{
			b->hangon = xstrdup("");
		}
		if (tolower(*s) == 's')
		{
			b->flags.b_Swallow = 1;
			b->flags.b_Hangon = 1;
		}
		else
		{
			b->flags.b_Panel = 1;
			b->flags.b_Hangon = 1;
		}
		b->swallow |= 1;
		s = trimleft(s);
		if (!(b->swallow & b_NoHints))
		{
			b->hints = xmalloc(sizeof(XSizeHints));
		}
		if (*s)
		{
			if (!(buttonSwallow(b) & b_UseOld))
			{
				SendText(fd, s, 0);
			}
			b->spawn = xstrdup(s);
		}
	}
	else if (*s)
	{
		AddButtonAction(b, 0, s);
	}
	return;
}

/**
*** ParseConfigLine
**/
static void ParseConfigLine(button_info **ubb, char *s)
{
	button_info *ub = *ubb;
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
		"activecolorset",
		"presscolorset",
		NULL
	};
	int i, j, k;

	switch (GetTokenIndex(s, opts, -1, &s))
	{
	case 0:/* Geometry */
	{
		char geom[64];

		i = sscanf(s, "%63s", geom);
		if (i == 1)
		{
			parse_window_geometry(geom, 0);
		}
		break;
	}
	case 1:/* ButtonGeometry */
	{
		char geom[64];

		i = sscanf(s, "%63s", geom);
		if (i == 1)
		{
			parse_window_geometry(geom, 1);
		}
		break;
	}
	case 2:/* Font */
		if (ub->c->font_string)
		{
			free(ub->c->font_string);
		}
		CopyStringWithQuotes(&ub->c->font_string, s);
		break;
	case 3:/* Padding */
		i = sscanf(s, "%d %d", &j, &k);
		if (i > 0)
		{
			ub->c->xpad = ub->c->ypad = j;
		}
		if (i > 1)
		{
			ub->c->ypad = k;
		}
		break;
	case 4:/* Columns */
		i = sscanf(s, "%d", &j);
		if (i > 0)
		{
			ub->c->num_columns = j;
		}
		break;
	case 5:/* Rows */
		i = sscanf(s, "%d", &j);
		if (i > 0)
		{
			ub->c->num_rows = j;
		}
		break;
	case 6:/* Back */
		if (ub->c->back)
		{
			free(ub->c->back);
		}
		CopyString(&(ub->c->back), s);
		break;
	case 7:/* Fore */
		if (ub->c->fore)
		{
			free(ub->c->fore);
		}
		CopyString(&(ub->c->fore), s);
		break;
	case 8:/* Frame */
		i = sscanf(s,"%d",&j);
		if (i > 0)
		{
			ub->c->framew = j;
		}
		break;
	case 9:/* File */
		s = trimleft(s);
		if (config_file)
		{
			free(config_file);
		}
		config_file = seekright(&s);
		break;
	case 10:/* Pixmap */
		s = trimleft(s);
		if (strncasecmp(s, "none", 4) == 0)
		{
			ub->c->flags.b_TransBack = 1;
		}
		else
		{
			if (ub->c->back_file)
			{
				free(ub->c->back_file);
			}
			CopyString(&(ub->c->back_file),s);
		}
		ub->c->flags.b_IconBack = 1;
		break;
	case 11: /* BoxSize */
		ParseBoxSize(&s, &ub->c->flags);
		break;
	case 12: /* Colorset */
		i = sscanf(s, "%d", &j);
		if (i > 0)
		{
			ub->c->colorset = j;
			ub->c->flags.b_Colorset = 1;
			AllocColorset(j);
		}
		else
		{
			ub->c->flags.b_Colorset = 0;
		}
		break;
	case 13: /* ActiveColorset */
		i = sscanf(s, "%d", &j);
		if (i > 0)
		{
			ub->c->activeColorset = j;
			ub->c->flags.b_ActiveColorset = 1;
			AllocColorset(j);
		}
		else
		{
			ub->c->flags.b_ActiveColorset = 0;
		}
		break;
	case 14: /* PressColorset */
		i = sscanf(s, "%d", &j);
		if (i > 0)
		{
			ub->c->pressColorset = j;
			ub->c->flags.b_PressColorset = 1;
			AllocColorset(j);
		}
		else
		{
			ub->c->flags.b_PressColorset = 0;
		}
		break;

	default:
		s = trimleft(s);
		ParseButton(ubb, s);
		break;
	}
}

/**
*** ParseConfigFile()
*** Parses optional separate configuration file for FvwmButtons
**/
static void ParseConfigFile(button_info *ub)
{
	char s[1024], *t;
	FILE *f = fopen(config_file, "r");
	int l;
	if (!f)
	{
		fprintf(stderr,
			"%s: Couldn't open config file %s\n", MyName,
			config_file);
		return;
	}

	while (fgets(s, 1023, f))
	{
		/* Allow for line continuation: */
		while ((l = strlen(s)) < sizeof(s)
			&& l >= 2 && s[l - 1] == '\n' && s[l - 2] == '\\')
		{
			char *p;

			p = fgets(s + l - 2, sizeof(s) - l, f);
			(void)p;
		}

		/* And comments: */
		t = s;
		while (*t)
		{
			if (*t == '#' && (t == s || *(t - 1) != '\\'))
			{
				*t = 0;
				break;
			}
			t++;
		}
		t = s;
		t = trimleft(t);
		if (*t)
		{
			ParseConfigLine(&ub, t);
		}
	}

	fclose(f);
}

void parse_window_geometry(char *geom, int is_button_geometry)
{
	int flags;
	int g_x;
	int g_y;
	unsigned int width;
	unsigned int height;

	flags = FScreenParseGeometry(geom, &g_x, &g_y, &width, &height);
	UberButton->w = 0;
	UberButton->h = 0;
	UberButton->x = 0;
	UberButton->y = 0;
	if (is_button_geometry)
	{
		if (flags&WidthValue)
		{
			button_width = width;
		}
		if (flags&HeightValue)
		{
			button_height = height;
		}
	}
	else
	{
		if (flags&WidthValue)
		{
			w = width;
		}
		if (flags&HeightValue)
		{
			h = height;
		}
	}
	if (flags&XValue)
	{
		UberButton->x = g_x;
	}
	if (flags&YValue)
	{
		UberButton->y = g_y;
	}
	if (flags&XNegative)
	{
		UberButton->w = 1;
	}
	if (flags&YNegative)
	{
		UberButton->h = 1;
	}
	has_button_geometry = is_button_geometry;

	return;
}

/**
*** ParseOptions()
**/
void ParseConfiguration(button_info *ub)
{
	char *s;
	char *items[] =
	{
		NULL, /* filled out below */
		"imagepath",
		"colorset",
		XINERAMA_CONFIG_STRING,
		NULL
	};

	items[0] = xmalloc(strlen(MyName) + 2);
	sprintf(items[0], "*%s", MyName);

	/* send config lines with MyName */
	InitGetConfigLine(fd, items[0]);
	GetConfigLine(fd, &s);
	while (s && s[0])
	{
		char *rest;
		switch (GetTokenIndex(s,items,-1,&rest))
		{
		case -1:
			break;
		case 0:
			if (rest && rest[0] && !config_file)
			{
				ParseConfigLine(&ub, rest);
			}
			break;
		case 1:
			if (imagePath)
			{
				free(imagePath);
			}
			CopyString(&imagePath, rest);
			break;
		case 2:
			/* store colorset sent by fvwm */
			LoadColorset(rest);
			break;
		case 3:
			/* Xinerama state */
			FScreenConfigureModule(rest);
			break;
		}
		GetConfigLine(fd,&s);
	}

	if (config_file)
	{
		ParseConfigFile(ub);
	}

	free(items[0]);
	return;
}
