/* -*-c-*- */
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <X11/keysym.h>

#include <libs/fvwmlib.h>
#include "fvwm.h"
#include "externs.h"
#include "events.h"
#include "menus.h"
#include "cursor.h"
#include "functions.h"
#include "repeat.h"
#include "misc.h"
#include "move_resize.h"
#include "screen.h"
#include "borders.h"
#include "colors.h"
#include "colormaps.h"
#include "decorations.h"
#include "colorset.h"
#include "defaults.h"
#include "libs/FScreen.h"
#include "libs/Flocale.h"
#include "libs/Picture.h"

#include "menustyle.h"

/* ---------------------------- local definitions --------------------------- */

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

static MenuStyle *default_menu_style;

/* ---------------------------- exported variables (globals) ---------------- */

/* ---------------------------- local functions ----------------------------- */

static void menustyle_free_face(MenuFace *mf)
{
	switch (mf->type)
	{
	case GradientMenu:
		/* - should we check visual is not TrueColor before doing this?
		 */
#if 0
		XFreeColors(
			dpy, PictureCMap, ms->u.grad.pixels,
			ms->u.grad.npixels, AllPlanes);
#endif
		free(mf->u.grad.pixels);
		mf->u.grad.pixels = NULL;
		break;
	case PixmapMenu:
	case TiledPixmapMenu:
		if (mf->u.p)
		{
			PDestroyFvwmPicture(dpy, mf->u.p);
		}
		mf->u.p = NULL;
		break;
	case SolidMenu:
		FreeColors(&mf->u.back, 1);
	default:
		break;
	}
	mf->type = SimpleMenu;

	return;
}

/*****************************************************************************
 *
 * Reads a menu face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
static Boolean menustyle_parse_face(char *s, MenuFace *mf, int verbose)
{
	char *style;
	char *token;
	char *action = s;

	s = GetNextToken(s, &style);
	if (style && strncasecmp(style, "--", 2) == 0)
	{
		free(style);
		return True;
	}

	menustyle_free_face(mf);
	mf->type = SimpleMenu;

	/* determine menu style */
	if (!style)
	{
		return True;
	}
	else if (StrEquals(style,"Solid"))
	{
		s = GetNextToken(s, &token);
		if (token)
		{
			mf->type = SolidMenu;
			mf->u.back = GetColor(token);
			free(token);
		}
		else
		{
			if (verbose)
			{
				fvwm_msg(ERR, "menustyle_parse_face",
					 "no color given for Solid face type:"
					 " %s", action);
			}
			free(style);
			return False;
		}
	}

	else if (StrEquals(style+1, "Gradient"))
	{
		char **s_colors;
		int npixels, nsegs, *perc;
		Pixel *pixels;

		if (!IsGradientTypeSupported(style[0]))
		{
			return False;
		}

		/* translate the gradient string into an array of colors etc */
		npixels = ParseGradient(s, NULL, &s_colors, &perc, &nsegs);
		if (npixels <= 0)
		{
			return False;
		}
		/* grab the colors */
		pixels = AllocAllGradientColors(s_colors, perc, nsegs, npixels);
		if (pixels == None)
		{
			return False;
		}

		mf->u.grad.pixels = pixels;
		mf->u.grad.npixels = npixels;
		mf->type = GradientMenu;
		mf->gradient_type = toupper(style[0]);
	}

	else if (StrEquals(style,"Pixmap") || StrEquals(style,"TiledPixmap"))
	{
		s = GetNextToken(s, &token);
		if (token)
		{
			mf->u.p = PCacheFvwmPicture(dpy, Scr.NoFocusWin,
						    NULL,
						    token,
						    Scr.ColorLimit);
			if (mf->u.p == NULL)
			{
				if (verbose)
				{
					fvwm_msg(ERR, "menustyle_parse_face",
						 "couldn't load pixmap %s",
						 token);
				}
				free(token);
				free(style);
				return False;
			}
			free(token);
			mf->type = (StrEquals(style,"TiledPixmap")) ?
				TiledPixmapMenu : PixmapMenu;
		}
		else
		{
			if (verbose)
			{
				fvwm_msg(ERR, "menustyle_parse_face",
					 "missing pixmap name for style %s",
					 style);
			}
			free(style);
			return False;
		}
	}
	else
	{
		if (verbose)
		{
			fvwm_msg(ERR, "menustyle_parse_face", "unknown style %s: %s",
				 style, action);
		}
		free(style);
		return False;
	}
	free(style);

	return True;
}

static void parse_vertical_spacing_line(
	char *args, signed char *above, signed char *below,
	signed char above_default, signed char below_default)
{
	int val[2];

	if (GetIntegerArguments(args, NULL, val, 2) != 2 ||
	    val[0] < MIN_VERTICAL_SPACING || val[0] > MAX_VERTICAL_SPACING ||
	    val[1] < MIN_VERTICAL_SPACING || val[1] > MAX_VERTICAL_SPACING)
	{
		/* illegal or missing parameters, return to default */
		*above = above_default;
		*below = below_default;
		return;
	}
	*above = val[0];
	*below = val[1];

	return;
}

static void menustyle_parse_old_style(F_CMD_ARGS)
{
	char *buffer, *rest;
	char *fore, *back, *stipple, *font, *style, *animated;

	rest = GetNextToken(action,&fore);
	rest = GetNextToken(rest,&back);
	rest = GetNextToken(rest,&stipple);
	rest = GetNextToken(rest,&font);
	rest = GetNextToken(rest,&style);
	rest = GetNextToken(rest,&animated);

	if (!fore || !back || !stipple || !font || !style)
	{
		fvwm_msg(ERR, "menustyle_parse_old_style",
			 "error in %s style specification", action);
	}
	else
	{
		buffer = (char *)alloca(strlen(action) + 100);
		sprintf(buffer,
			"* \"%s\", Foreground \"%s\", Background \"%s\", "
			"Greyed \"%s\", Font \"%s\", \"%s\"",
			style, fore, back, stipple, font,
			(animated && StrEquals(animated, "anim")) ?
			"Animation" : "AnimationOff");
		action = buffer;
		menustyle_parse_style(F_PASS_ARGS);
	}

	if (fore)
        {
		free(fore);
        }
	if (back)
        {
		free(back);
        }
	if (stipple)
        {
		free(stipple);
        }
	if (font)
        {
		free(font);
        }
	if (style)
        {
		free(style);
        }
	if (animated)
        {
		free(animated);
        }

	return;
}

static int menustyle_get_styleopt_index(char *option)
{
	char *optlist[] = {
		"fvwm", "mwm", "win",
		"Foreground", "Background", "Greyed",
		"HilightBack", "HilightBackOff",
		"ActiveFore", "ActiveForeOff",
		"Hilight3DThick", "Hilight3DThin", "Hilight3DOff",
		"Animation", "AnimationOff",
		"Font",
		"MenuFace",
		"PopupDelay", "PopupOffset",
		"TitleWarp", "TitleWarpOff",
		"TitleUnderlines0", "TitleUnderlines1", "TitleUnderlines2",
		"SeparatorsLong", "SeparatorsShort",
		"TrianglesSolid", "TrianglesRelief",
		"PopupImmediately", "PopupDelayed",
		"DoubleClickTime",
		"SidePic", "SideColor",
		"PopupAsRootmenu", "PopupAsSubmenu",
		"RemoveSubmenus", "HoldSubmenus",
		"SubmenusRight", "SubmenusLeft",
		"BorderWidth",
		"Hilight3DThickness",
		"ItemFormat",
		"AutomaticHotkeys", "AutomaticHotkeysOff",
		"VerticalItemSpacing",
		"VerticalTitleSpacing",
		"MenuColorset", "ActiveColorset", "GreyedColorset",
		"SelectOnRelease",
		"PopdownImmediately", "PopdownDelayed",
		"PopdownDelay",
		"PopupActiveArea",
		NULL
	};

	return GetTokenIndex(option, optlist, 0, NULL);
}

/* ---------------------------- interface functions ------------------------- */

MenuStyle *menustyle_get_default_style(void)
{
	return default_menu_style;
}

void menustyle_free(MenuStyle *ms)
{
	MenuStyle *before = default_menu_style;

	if (!ms)
	{
		return;
	}
	menustyle_free_face(&ST_FACE(ms));
	if (ST_MENU_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_GC(ms));
	}
	if (ST_MENU_ACTIVE_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_ACTIVE_GC(ms));
	}
	if (ST_MENU_ACTIVE_BACK_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_ACTIVE_BACK_GC(ms));
	}
	if (ST_MENU_ACTIVE_RELIEF_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_ACTIVE_RELIEF_GC(ms));
	}
	if (ST_MENU_ACTIVE_SHADOW_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_ACTIVE_SHADOW_GC(ms));
	}
	if (ST_MENU_RELIEF_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_RELIEF_GC(ms));
	}
	if (ST_MENU_STIPPLE_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_STIPPLE_GC(ms));
	}
	if (ST_MENU_SHADOW_GC(ms))
	{
		XFreeGC(dpy, ST_MENU_SHADOW_GC(ms));
	}
	if (ST_SIDEPIC(ms))
	{
		PDestroyFvwmPicture(dpy, ST_SIDEPIC(ms));
	}
	if (ST_HAS_SIDE_COLOR(ms) == 1)
	{
		FreeColors(&ST_SIDE_COLOR(ms), 1);
	}
	if (ST_PSTDFONT(ms) && !ST_USING_DEFAULT_FONT(ms))
	{
		FlocaleUnloadFont(dpy, ST_PSTDFONT(ms));
	}
	if (ST_ITEM_FORMAT(ms))
	{
		free(ST_ITEM_FORMAT(ms));
	}

	while (ST_NEXT_STYLE(before) != ms)
	{
		/* Not too many checks, may segfault in race conditions */
		before = ST_NEXT_STYLE(before);
	}

	ST_NEXT_STYLE(before) = ST_NEXT_STYLE(ms);
	free(ST_NAME(ms));
	free(ms);

	return;
}

MenuStyle *menustyle_find(char *name)
{
	MenuStyle *ms = default_menu_style;

	while (ms)
	{
		if (strcasecmp(ST_NAME(ms),name)==0)
		{
			return ms;
		}
		ms = ST_NEXT_STYLE(ms);
	}

	return NULL;
}

void menustyle_update(MenuStyle *ms)
{
	XGCValues gcv;
	unsigned long gcm;
	Pixel menu_fore;
	Pixel menu_back;
	Pixel relief_fore;
	Pixel relief_back;
	Pixel active_fore;
	Pixel active_back;
	Pixel active_relief_fore;
	Pixel active_relief_back;
	colorset_struct *menu_cs = &Colorset[ST_CSET_MENU(ms)];
	colorset_struct *active_cs = &Colorset[ST_CSET_ACTIVE(ms)];
	colorset_struct *greyed_cs = &Colorset[ST_CSET_GREYED(ms)];

	if (ST_USAGE_COUNT(ms) != 0)
	{
		fvwm_msg(ERR,"menustyle_update", "menu style %s is in use",
			 ST_NAME(ms));
		return;
	}
	ST_IS_UPDATED(ms) = 1;

	/* calculate colors based on foreground */
	if (!ST_HAS_ACTIVE_FORE(ms))
	{
		ST_MENU_ACTIVE_COLORS(ms).fore = ST_MENU_COLORS(ms).fore;
	}
	/* calculate colors based on background */
	if (!ST_HAS_ACTIVE_BACK(ms))
	{
		ST_MENU_ACTIVE_COLORS(ms).back = ST_MENU_COLORS(ms).back;
	}
	if (!ST_HAS_STIPPLE_FORE(ms))
	{
		ST_MENU_STIPPLE_COLORS(ms).fore = ST_MENU_COLORS(ms).back;
	}
	if (Pdepth > 2)
	{
		/* if not black and white */
		ST_MENU_RELIEF_COLORS(ms).back =
			GetShadow(ST_MENU_COLORS(ms).back);
		ST_MENU_RELIEF_COLORS(ms).fore =
			GetHilite(ST_MENU_COLORS(ms).back);
	}
	else
	{
		/* black and white */
		ST_MENU_RELIEF_COLORS(ms).back =
			GetColor(DEFAULT_SHADOW_COLOR);
		ST_MENU_RELIEF_COLORS(ms).fore =
			GetColor(DEFAULT_HILIGHT_COLOR);
	}
	ST_MENU_STIPPLE_COLORS(ms).back = ST_MENU_COLORS(ms).back;

	/* calculate some pixel values for convenience reasons */
	if (ST_HAS_MENU_CSET(ms))
	{
		menu_fore = menu_cs->fg;
		menu_back = menu_cs->bg;
		relief_fore = menu_cs->hilite;
		relief_back = menu_cs->shadow;
		active_relief_fore = menu_cs->hilite;
		active_relief_back = menu_cs->shadow;
	}
	else
	{
		menu_fore = ST_MENU_COLORS(ms).fore;
		menu_back = ST_MENU_COLORS(ms).back;
		relief_fore = ST_MENU_RELIEF_COLORS(ms).fore;
		relief_back = ST_MENU_RELIEF_COLORS(ms).back;
		active_relief_fore = ST_MENU_RELIEF_COLORS(ms).fore;
		active_relief_back = ST_MENU_RELIEF_COLORS(ms).back;
	}
	if (ST_HAS_ACTIVE_CSET(ms))
	{
		active_fore = active_cs->fg;
		active_back = active_cs->bg;
		active_relief_fore = active_cs->hilite;
		active_relief_back = active_cs->shadow;
	}
	else
	{
		active_fore = (ST_HAS_ACTIVE_FORE(ms)) ?
			ST_MENU_ACTIVE_COLORS(ms).fore : menu_fore;
		active_back = (ST_HAS_ACTIVE_BACK(ms)) ?
			ST_MENU_ACTIVE_COLORS(ms).back : menu_back;
	}
	if (ST_USING_DEFAULT_FONT(ms))
	{
		ST_PSTDFONT(ms) = Scr.DefaultFont;
	}

	/* make GC's */
	gcm = GCFunction|GCLineWidth|GCForeground|GCBackground;
	if (ST_PSTDFONT(ms)->font != NULL)
	{
		gcm |= GCFont;
		gcv.font = ST_PSTDFONT(ms)->font->fid;
	}
	gcv.function = GXcopy;
	gcv.line_width = 0;

	/* update relief gc */
	gcv.foreground = relief_fore;
	gcv.background = relief_back;
	if (ST_MENU_RELIEF_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_RELIEF_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_RELIEF_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update shadow gc */
	gcv.foreground = relief_back;
	gcv.background = relief_fore;
	if (ST_MENU_SHADOW_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_SHADOW_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_SHADOW_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update active gc */
	gcv.foreground = active_fore;
	gcv.background = active_back;
	if (ST_MENU_ACTIVE_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_ACTIVE_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_ACTIVE_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update active relief gc */
	gcv.foreground = active_relief_fore;
	gcv.background = active_relief_back;
	if (ST_MENU_ACTIVE_RELIEF_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_ACTIVE_RELIEF_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_ACTIVE_RELIEF_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update active shadow gc */
	gcv.foreground = active_relief_back;
	gcv.background = active_relief_fore;
	if (ST_MENU_ACTIVE_SHADOW_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_ACTIVE_SHADOW_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_ACTIVE_SHADOW_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update active back gc */
	gcv.foreground = active_back;
	gcv.background = active_fore;
	if (ST_MENU_ACTIVE_BACK_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_ACTIVE_BACK_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_ACTIVE_BACK_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update menu gc */
	gcv.foreground = menu_fore;
	gcv.background = menu_back;
	if (ST_MENU_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	/* update stipple gc */
	if (Pdepth < 2)
	{
		gcv.fill_style = FillStippled;
		gcv.stipple = Scr.gray_bitmap;
		/* no need to reset fg/bg, FillStipple wins */
		gcm |= GCStipple | GCFillStyle;
	}
	else if (ST_HAS_GREYED_CSET(ms))
	{
		gcv.foreground = greyed_cs->fg;
		gcv.background = greyed_cs->bg;
	}
	else
	{
		gcv.foreground = ST_MENU_STIPPLE_COLORS(ms).fore;
		gcv.background = ST_MENU_STIPPLE_COLORS(ms).back;
	}
	if (ST_MENU_STIPPLE_GC(ms))
	{
		XChangeGC(dpy, ST_MENU_STIPPLE_GC(ms), gcm, &gcv);
	}
	else
	{
		ST_MENU_STIPPLE_GC(ms) =
			fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	return;
}

void menustyle_parse_style(F_CMD_ARGS)
{
	char *name;
	char *option = NULL;
	char *optstring = NULL;
	char *nextarg;
	char *args = NULL;
	char *arg1;
	MenuStyle *ms;
	MenuStyle *tmpms;
	Bool is_initialised = True;
	Bool has_gc_changed = False;
	Bool is_default_style = False;
	int val[2];
	int n;
	FlocaleFont *new_font = NULL;
	int i;
	KeyCode keycode;

	action = GetNextToken(action, &name);
	if (!name)
	{
		fvwm_msg(ERR, "NewMenuStyle",
			 "error in %s style specification",action);
		return;
	}

	ms = menustyle_find(name);
	if (ms && ST_USAGE_COUNT(ms) != 0)
	{
		fvwm_msg(ERR,"NewMenuStyle", "menu style %s is in use", name);
		return;
	}
	tmpms = (MenuStyle *)safemalloc(sizeof(MenuStyle));
	if (ms)
	{
		/* copy the structure over our temporary menu face. */
		memcpy(tmpms, ms, sizeof(MenuStyle));
		if (ms == default_menu_style)
		{
			is_default_style = True;
		}
		free(name);
	}
	else
	{
		memset(tmpms, 0, sizeof(MenuStyle));
		ST_NAME(tmpms) = name;
		is_initialised = False;
	}
	ST_IS_UPDATED(tmpms) = 1;

	/* Parse the options. */
	while (!is_initialised || (action && *action))
	{
		if (!is_initialised)
		{
			/* some default configuration goes here for the new
			 * menu style */
			ST_MENU_COLORS(tmpms).back =
				GetColor(DEFAULT_BACK_COLOR);
			ST_MENU_COLORS(tmpms).fore =
				GetColor(DEFAULT_FORE_COLOR);
			ST_PSTDFONT(tmpms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(tmpms) = True;
			ST_FACE(tmpms).type = SimpleMenu;
			ST_HAS_ACTIVE_FORE(tmpms) = 0;
			ST_HAS_ACTIVE_BACK(tmpms) = 0;
			ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 0;
			ST_DOUBLE_CLICK_TIME(tmpms) = DEFAULT_MENU_CLICKTIME;
			ST_POPUP_DELAY(tmpms) = DEFAULT_POPUP_DELAY;
			ST_POPDOWN_DELAY(tmpms) = DEFAULT_POPDOWN_DELAY;
			has_gc_changed = True;
			option = "fvwm";
		}
		else
		{
			/* Read next option specification (delimited by a comma
			 * or \0). */
			args = action;
			action = GetQuotedString(
				action, &optstring, ",", NULL, NULL, NULL);
			if (!optstring)
			{
				break;
			}
			args = GetNextToken(optstring, &option);
			if (!option)
			{
				free(optstring);
				break;
			}
			nextarg = GetNextToken(args, &arg1);
		}

		switch((i = menustyle_get_styleopt_index(option)))
		{
		case 0: /* fvwm */
		case 1: /* mwm */
		case 2: /* win */
			if (i == 0)
			{
				ST_POPUP_OFFSET_PERCENT(tmpms) = 67;
				ST_POPUP_OFFSET_ADD(tmpms) = 0;
				ST_DO_POPUP_IMMEDIATELY(tmpms) = 0;
				ST_DO_WARP_TO_TITLE(tmpms) = 1;
				ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
				ST_RELIEF_THICKNESS(tmpms) = 1;
				ST_TITLE_UNDERLINES(tmpms) = 1;
				ST_HAS_LONG_SEPARATORS(tmpms) = 0;
				ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
				ST_DO_HILIGHT(tmpms) = 0;
			}
			else if (i == 1)
			{
				ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
				ST_POPUP_OFFSET_ADD(tmpms) =
					-DEFAULT_MENU_BORDER_WIDTH - 1;
				ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
				ST_DO_WARP_TO_TITLE(tmpms) = 0;
				ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
				ST_RELIEF_THICKNESS(tmpms) = 2;
				ST_TITLE_UNDERLINES(tmpms) = 2;
				ST_HAS_LONG_SEPARATORS(tmpms) = 1;
				ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
				ST_DO_HILIGHT(tmpms) = 0;
			}
			else /* i == 2 */
			{
				ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
				ST_POPUP_OFFSET_ADD(tmpms) =
					-DEFAULT_MENU_BORDER_WIDTH - 3;
				ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
				ST_DO_WARP_TO_TITLE(tmpms) = 0;
				ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 1;
				ST_RELIEF_THICKNESS(tmpms) = 0;
				ST_TITLE_UNDERLINES(tmpms) = 1;
				ST_HAS_LONG_SEPARATORS(tmpms) = 0;
				ST_HAS_TRIANGLE_RELIEF(tmpms) = 0;
				ST_DO_HILIGHT(tmpms) = 1;
			}

			/* common settings */
			ST_BORDER_WIDTH(tmpms) = DEFAULT_MENU_BORDER_WIDTH;
			ST_ACTIVE_AREA_PERCENT(tmpms) =
				DEFAULT_MENU_POPUP_NOW_RATIO;
			ST_ITEM_GAP_ABOVE(tmpms) =
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET;
			ST_ITEM_GAP_BELOW(tmpms) =
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2;
			ST_TITLE_GAP_ABOVE(tmpms) =
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET;
			ST_TITLE_GAP_BELOW(tmpms) =
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2;
			ST_USE_LEFT_SUBMENUS(tmpms) = 0;
			ST_IS_ANIMATED(tmpms) = 0;
			ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 0;
			menustyle_free_face(&ST_FACE(tmpms));
			ST_FACE(tmpms).type = SimpleMenu;
			if (ST_PSTDFONT(tmpms) && !ST_USING_DEFAULT_FONT(tmpms))
			{
				FlocaleUnloadFont(dpy, ST_PSTDFONT(tmpms));
			}
			ST_PSTDFONT(tmpms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(tmpms) = True;
			has_gc_changed = True;
			if (ST_HAS_SIDE_COLOR(tmpms) == 1)
			{
				FreeColors(&ST_SIDE_COLOR(tmpms), 1);
				ST_HAS_SIDE_COLOR(tmpms) = 0;
			}
			ST_HAS_SIDE_COLOR(tmpms) = 0;
			if (ST_SIDEPIC(tmpms))
			{
				PDestroyFvwmPicture(dpy, ST_SIDEPIC(tmpms));
				ST_SIDEPIC(tmpms) = NULL;
			}

			if (is_initialised == False)
			{
				/* now begin the real work */
				is_initialised = True;
				continue;
			}
			break;

		case 3: /* Foreground */
			FreeColors(&ST_MENU_COLORS(tmpms).fore, 1);
			if (arg1)
			{
				ST_MENU_COLORS(tmpms).fore = GetColor(arg1);
			}
			else
			{
				ST_MENU_COLORS(tmpms).fore =
					GetColor(DEFAULT_FORE_COLOR);
			}
			has_gc_changed = True;
			break;

		case 4: /* Background */
			FreeColors(&ST_MENU_COLORS(tmpms).back, 1);
			if (arg1)
			{
				ST_MENU_COLORS(tmpms).back = GetColor(arg1);
			}
			else
			{
				ST_MENU_COLORS(tmpms).back =
					GetColor(DEFAULT_BACK_COLOR);
			}
			has_gc_changed = True;
			break;

		case 5: /* Greyed */
			if (ST_HAS_STIPPLE_FORE(tmpms))
			{
				FreeColors(
					&ST_MENU_STIPPLE_COLORS(tmpms).fore, 1);
			}
			if (arg1 == NULL)
			{
				ST_HAS_STIPPLE_FORE(tmpms) = 0;
			}
			else
			{
				ST_MENU_STIPPLE_COLORS(tmpms).fore =
					GetColor(arg1);
				ST_HAS_STIPPLE_FORE(tmpms) = 1;
			}
			has_gc_changed = True;
			break;

		case 6: /* HilightBack */
			if (ST_HAS_ACTIVE_BACK(tmpms))
			{
				FreeColors(
					&ST_MENU_ACTIVE_COLORS(tmpms).back, 1);
			}
			if (arg1 == NULL)
			{
				ST_HAS_ACTIVE_BACK(tmpms) = 0;
			}
			else
			{
				ST_MENU_ACTIVE_COLORS(tmpms).back =
					GetColor(arg1);
				ST_HAS_ACTIVE_BACK(tmpms) = 1;
			}
			ST_DO_HILIGHT(tmpms) = 1;
			has_gc_changed = True;
			break;

		case 7: /* HilightBackOff */
			ST_DO_HILIGHT(tmpms) = 0;
			has_gc_changed = True;
			break;

		case 8: /* ActiveFore */
			if (ST_HAS_ACTIVE_FORE(tmpms))
			{
				FreeColors(
					&ST_MENU_ACTIVE_COLORS(tmpms).fore, 1);
			}
			if (arg1 == NULL)
			{
				ST_HAS_ACTIVE_FORE(tmpms) = 0;
			}
			else
			{
				ST_MENU_ACTIVE_COLORS(tmpms).fore =
					GetColor(arg1);
				ST_HAS_ACTIVE_FORE(tmpms) = 1;
			}
			has_gc_changed = True;
			break;

		case 9: /* ActiveForeOff */
			ST_HAS_ACTIVE_FORE(tmpms) = 0;
			has_gc_changed = True;
			break;

		case 10: /* Hilight3DThick */
			ST_RELIEF_THICKNESS(tmpms) = 2;
			break;

		case 11: /* Hilight3DThin */
			ST_RELIEF_THICKNESS(tmpms) = 1;
			break;

		case 12: /* Hilight3DOff */
			ST_RELIEF_THICKNESS(tmpms) = 0;
			break;

		case 13: /* Animation */
			ST_IS_ANIMATED(tmpms) = 1;
			break;

		case 14: /* AnimationOff */
			ST_IS_ANIMATED(tmpms) = 0;
			break;

		case 15: /* Font */
			if (arg1 != NULL &&
			    !(new_font = FlocaleLoadFont(dpy, arg1, "FVWM")))
			{
				fvwm_msg(ERR, "NewMenuStyle",
					 "Couldn't load font '%s'\n", arg1);
				break;
			}
			if (ST_PSTDFONT(tmpms) && !ST_USING_DEFAULT_FONT(tmpms))
			{
				FlocaleUnloadFont(dpy, ST_PSTDFONT(tmpms));
			}
			if (arg1 == NULL)
			{
				/* reset to screen font */
				ST_PSTDFONT(tmpms) = Scr.DefaultFont;
				ST_USING_DEFAULT_FONT(tmpms) = True;
			}
			else
			{
				ST_PSTDFONT(tmpms) = new_font;
				ST_USING_DEFAULT_FONT(tmpms) = False;
			}
			has_gc_changed = True;
			break;

		case 16: /* MenuFace */
			while (args && *args != '\0' &&
			       isspace((unsigned char)*args))
			{
				args++;
			}
			menustyle_parse_face(args, &ST_FACE(tmpms), True);
			break;

		case 17: /* PopupDelay */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_POPUP_DELAY(tmpms) = DEFAULT_POPUP_DELAY;
			}
			else
			{
				ST_POPUP_DELAY(tmpms) = (*val+9)/10;
			}
			break;

		case 18: /* PopupOffset */
			if ((n = GetIntegerArguments(args, NULL, val, 2)) == 0)
			{
				fvwm_msg(ERR,"NewMenuStyle",
					 "PopupOffset requires one or two"
					 " arguments");
			}
			else
			{
				ST_POPUP_OFFSET_ADD(tmpms) = val[0];
				if (n == 2 && val[1] <= 100 && val[1] >= 0)
				{
					ST_POPUP_OFFSET_PERCENT(tmpms) = val[1];
				}
				else
				{
					ST_POPUP_OFFSET_PERCENT(tmpms) = 100;
				}
			}
			break;

		case 19: /* TitleWarp */
			ST_DO_WARP_TO_TITLE(tmpms) = 1;
			break;

		case 20: /* TitleWarpOff */
			ST_DO_WARP_TO_TITLE(tmpms) = 0;
			break;

		case 21: /* TitleUnderlines0 */
			ST_TITLE_UNDERLINES(tmpms) = 0;
			break;

		case 22: /* TitleUnderlines1 */
			ST_TITLE_UNDERLINES(tmpms) = 1;
			break;

		case 23: /* TitleUnderlines2 */
			ST_TITLE_UNDERLINES(tmpms) = 2;
			break;

		case 24: /* SeparatorsLong */
			ST_HAS_LONG_SEPARATORS(tmpms) = 1;
			break;

		case 25: /* SeparatorsShort */
			ST_HAS_LONG_SEPARATORS(tmpms) = 0;
			break;

		case 26: /* TrianglesSolid */
			ST_HAS_TRIANGLE_RELIEF(tmpms) = 0;
			break;

		case 27: /* TrianglesRelief */
			ST_HAS_TRIANGLE_RELIEF(tmpms) = 1;
			break;

		case 28: /* PopupImmediately */
			ST_DO_POPUP_IMMEDIATELY(tmpms) = 1;
			break;

		case 29: /* PopupDelayed */
			ST_DO_POPUP_IMMEDIATELY(tmpms) = 0;
			break;

		case 30: /* DoubleClickTime */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_DOUBLE_CLICK_TIME(tmpms) =
					DEFAULT_MENU_CLICKTIME;
			}
			else
			{
				ST_DOUBLE_CLICK_TIME(tmpms) = *val;
			}
			break;

		case 31: /* SidePic */
			if (ST_SIDEPIC(tmpms))
			{
				PDestroyFvwmPicture(dpy, ST_SIDEPIC(tmpms));
				ST_SIDEPIC(tmpms) = NULL;
			}
			if (arg1)
			{
				ST_SIDEPIC(tmpms) = PCacheFvwmPicture(
					dpy, Scr.NoFocusWin, NULL, arg1,
					Scr.ColorLimit);
				if (!ST_SIDEPIC(tmpms))
				{
					fvwm_msg(WARN, "NewMenuStyle",
						 "Couldn't find pixmap %s",
						 arg1);
				}
			}
			break;

		case 32: /* SideColor */
			if (ST_HAS_SIDE_COLOR(tmpms) == 1)
			{
				FreeColors(&ST_SIDE_COLOR(tmpms), 1);
				ST_HAS_SIDE_COLOR(tmpms) = 0;
			}
			if (arg1)
			{
				ST_SIDE_COLOR(tmpms) = GetColor(arg1);
				ST_HAS_SIDE_COLOR(tmpms) = 1;
			}
			break;

		case 33: /* PopupAsRootmenu */
			ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 1;
			break;

		case 34: /* PopupAsSubmenu */
			ST_DO_POPUP_AS_ROOT_MENU(tmpms) = 0;
			break;

		case 35: /* RemoveSubmenus */
			ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 1;
			break;

		case 36: /* HoldSubmenus */
			ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = 0;
			break;

		case 37: /* SubmenusRight */
			ST_USE_LEFT_SUBMENUS(tmpms) = 0;
			break;

		case 38: /* SubmenusLeft */
			ST_USE_LEFT_SUBMENUS(tmpms) = 1;
			break;

		case 39: /* BorderWidth */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0 || *val > MAX_MENU_BORDER_WIDTH)
			{
				ST_BORDER_WIDTH(tmpms) =
					DEFAULT_MENU_BORDER_WIDTH;
			}
			else
			{
				ST_BORDER_WIDTH(tmpms) = *val;
			}
			break;

		case 40: /* Hilight3DThickness */
			if (GetIntegerArguments(args, NULL, val, 1) > 0)
			{
				if (*val < 0)
				{
					*val = -*val;
					ST_IS_ITEM_RELIEF_REVERSED(tmpms) = 1;
				}
				else
				{
					ST_IS_ITEM_RELIEF_REVERSED(tmpms) = 0;
				}
				if (*val > MAX_MENU_ITEM_RELIEF_THICKNESS)
					*val = MAX_MENU_ITEM_RELIEF_THICKNESS;
				ST_RELIEF_THICKNESS(tmpms) = *val;
			}
			break;

		case 41: /* ItemFormat */
			if (ST_ITEM_FORMAT(tmpms))
			{
				free(ST_ITEM_FORMAT(tmpms));
				ST_ITEM_FORMAT(tmpms) = NULL;
			}
			if (arg1)
			{
				ST_ITEM_FORMAT(tmpms) = safestrdup(arg1);
			}
			break;

		case 42: /* AutomaticHotkeys */
			ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 1;
			break;

		case 43: /* AutomaticHotkeysOff */
			ST_USE_AUTOMATIC_HOTKEYS(tmpms) = 0;
			break;

		case 44: /* VerticalItemSpacing */
			parse_vertical_spacing_line(
				args, &ST_ITEM_GAP_ABOVE(tmpms),
				&ST_ITEM_GAP_BELOW(tmpms),
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET,
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2);
			break;

		case 45: /* VerticalTitleSpacing */
			parse_vertical_spacing_line(
				args, &ST_TITLE_GAP_ABOVE(tmpms),
				&ST_TITLE_GAP_BELOW(tmpms),
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET,
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2);
			break;
		case 46: /* MenuColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_HAS_MENU_CSET(tmpms) = 0;
			}
			else
			{
				ST_HAS_MENU_CSET(tmpms) = 1;
				ST_CSET_MENU(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;
		case 47: /* ActiveColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_HAS_ACTIVE_CSET(tmpms) = 0;
			}
			else
			{
				ST_HAS_ACTIVE_CSET(tmpms) = 1;
				ST_CSET_ACTIVE(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 48: /* GreyedColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_HAS_GREYED_CSET(tmpms) = 0;
			}
			else
			{
				ST_HAS_GREYED_CSET(tmpms) = 1;
				ST_CSET_GREYED(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 49: /* SelectOnRelease */
			keycode = 0;
			if (arg1)
			{
				keycode = XKeysymToKeycode(
					dpy, FvwmStringToKeysym(dpy, arg1));
			}
			ST_SELECT_ON_RELEASE_KEY(tmpms) = keycode;
			break;

		case 50: /* PopdownImmediately */
			ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 1;
			break;

		case 51: /* PopdownDelayed */
			ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 0;
			break;

		case 52: /* PopdownDelay */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_POPDOWN_DELAY(tmpms) = DEFAULT_POPDOWN_DELAY;
			}
			else
			{
				ST_POPDOWN_DELAY(tmpms) = (*val+9)/10;
			}
			break;

		case 53: /* PopupActiveArea */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val <= 50 || *val > 100)
			{
				ST_ACTIVE_AREA_PERCENT(tmpms) =
					DEFAULT_MENU_POPUP_NOW_RATIO;
			}
			else
			{
				ST_ACTIVE_AREA_PERCENT(tmpms) = *val;
			}
			break;

#if 0
		case 99: /* PositionHints */
			/* to be implemented */
			break;
#endif

		default:
			fvwm_msg(ERR, "NewMenuStyle", "unknown option '%s'",
				 option);
			break;
		} /* switch */

		if (option)
		{
			free(option);
			option = NULL;
		}
		free(optstring);
		optstring = NULL;
		if (arg1)
		{
			free(arg1);
			arg1 = NULL;
		}
	} /* while */

	if (has_gc_changed)
	{
		menustyle_update(tmpms);
	} /* if (has_gc_changed) */

	if (default_menu_style == NULL)
	{
		/* First MenuStyle MUST be the default style */
		default_menu_style = tmpms;
		ST_NEXT_STYLE(tmpms) = NULL;
	}
	else if (ms)
	{
		/* copy our new menu face over the old one */
		memcpy(ms, tmpms, sizeof(MenuStyle));
		free(tmpms);
	}
	else
	{
		MenuStyle *before = default_menu_style;

		/* add a new menu face to list */
		ST_NEXT_STYLE(tmpms) = NULL;
		while (ST_NEXT_STYLE(before))
			before = ST_NEXT_STYLE(before);
		ST_NEXT_STYLE(before) = tmpms;
	}

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

void CMD_CopyMenuStyle(F_CMD_ARGS)
{
	char *origname = NULL;
	char *destname = NULL;
	char *buffer;
	MenuStyle *origms;
	MenuStyle *destms;

	origname = PeekToken(action, &action);
	if (origname == NULL)
	{
		fvwm_msg(ERR,"CopyMenuStyle", "need two arguments");
		return;
	}

	origms = menustyle_find(origname);
	if (!origms)
	{
		fvwm_msg(ERR, "CopyMenuStyle", "%s: no such menu style",
			 origname);
		return;
	}

	destname = PeekToken(action, &action);
	if (destname == NULL)
	{
		fvwm_msg(ERR,"CopyMenuStyle", "need two arguments");
		return;
	}

	if (action && *action)
	{
		fvwm_msg(ERR,"CopyMenuStyle", "too many arguments");
		return;
	}

	destms = menustyle_find(destname);
	if (!destms)
	{
		/* create destms menu style */
		buffer = (char *)safemalloc(strlen(destname) + 3);
		sprintf(buffer,"\"%s\"",destname);
		action = buffer;
		menustyle_parse_style(F_PASS_ARGS);
		free(buffer);
		destms = menustyle_find(destname);
		if (!destms)
		{
			/* this must never happen */
			fvwm_msg(ERR, "CopyMenuStyle",
				 "impossible to create %s menu style",
				 destname);
			return;
		}
	}

	if (strcasecmp("*",destname) == 0)
	{
		fvwm_msg(ERR, "CopyMenuStyle",
			 "You cannot copy on the default menu style");
		return;
	}
	if (strcasecmp(ST_NAME(origms),destname) == 0)
	{
		fvwm_msg(ERR, "CopyMenuStyle",
			 "%s and %s identify the same menu style",
			 ST_NAME(origms),destname);
		return;
	}

	if (ST_USAGE_COUNT(destms) != 0)
	{
		fvwm_msg(ERR, "CopyMenuStyle", "menu style %s is in use",
			 destname);
		return;
	}

	/* Copy origms to destms, be award of all pointers in the MenuStyle
	   strcture. Use  the same order as in menustyle_parse_style */

	/* menu colors */
	FreeColors(&ST_MENU_COLORS(destms).fore, 1);
	FreeColors(&ST_MENU_COLORS(destms).back, 1);
	memcpy(&ST_MENU_COLORS(destms), &ST_MENU_COLORS(origms),
	       sizeof(ColorPair));

	/* Greyed */
	if (ST_HAS_STIPPLE_FORE(destms))
	{
		FreeColors(&ST_MENU_STIPPLE_COLORS(destms).fore, 1);
	}
	ST_HAS_STIPPLE_FORE(destms) = ST_HAS_STIPPLE_FORE(origms);
	if (ST_HAS_STIPPLE_FORE(origms))
	{
		memcpy(&ST_MENU_STIPPLE_COLORS(destms).fore,
		       &ST_MENU_STIPPLE_COLORS(origms).fore, sizeof(Pixel));
	}

	/* HilightBack */
	if (ST_HAS_ACTIVE_BACK(destms))
	{
		FreeColors(&ST_MENU_ACTIVE_COLORS(destms).back, 1);
	}
	ST_HAS_ACTIVE_BACK(destms) = ST_HAS_ACTIVE_BACK(origms);
	if (ST_HAS_ACTIVE_BACK(origms))
	{
		memcpy(&ST_MENU_ACTIVE_COLORS(destms).back,
		       &ST_MENU_ACTIVE_COLORS(origms).back, sizeof(Pixel));
	}

	ST_DO_HILIGHT(destms) = ST_DO_HILIGHT(origms);

	/* ActiveFore */
	if (ST_HAS_ACTIVE_FORE(destms))
	{
		FreeColors(&ST_MENU_ACTIVE_COLORS(destms).fore, 1);
	}
	ST_HAS_ACTIVE_FORE(destms) = ST_HAS_ACTIVE_FORE(origms);
	if (ST_HAS_ACTIVE_FORE(origms))
	{
		memcpy(&ST_MENU_ACTIVE_COLORS(destms).fore,
		       &ST_MENU_ACTIVE_COLORS(origms).fore, sizeof(Pixel));
	}

	/* Hilight3D */
	ST_RELIEF_THICKNESS(destms) = ST_RELIEF_THICKNESS(origms);
	/* Animation */
	ST_IS_ANIMATED(destms) = ST_IS_ANIMATED(origms);

	/* font */
	if (ST_PSTDFONT(destms) &&  !ST_USING_DEFAULT_FONT(destms))
	{
		FlocaleUnloadFont(dpy, ST_PSTDFONT(destms));
	}
	if (ST_PSTDFONT(origms) && !ST_USING_DEFAULT_FONT(origms))
	{
		if (!(ST_PSTDFONT(destms) =
		      FlocaleLoadFont(dpy, ST_PSTDFONT(origms)->name, "FVWM")))
		{
			ST_PSTDFONT(destms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(destms) = True;
			fvwm_msg(ERR, "CopyMenuStyle",
				 "Couldn't load font '%s' use Default Font\n",
				 ST_PSTDFONT(origms)->name);
		}
		else
		{
			ST_USING_DEFAULT_FONT(destms) = False;
		}
	}
	else
	{
		ST_USING_DEFAULT_FONT(destms) = True;
		ST_PSTDFONT(destms) = Scr.DefaultFont;
	}
	/* MenuFace */
	menustyle_free_face(&ST_FACE(destms));
	ST_FACE(destms).type = SimpleMenu;
	switch (ST_FACE(origms).type)
	{
	case SolidMenu:
		memcpy(&ST_FACE(destms).u.back, &ST_FACE(origms).u.back,
		       sizeof(Pixel));
		ST_FACE(destms).type = SolidMenu;
		break;
	case GradientMenu:
		ST_FACE(destms).u.grad.pixels =
			(Pixel *)safemalloc(sizeof(Pixel) *
					    ST_FACE(origms).u.grad.npixels);
		memcpy(ST_FACE(destms).u.grad.pixels,
		       ST_FACE(origms).u.grad.pixels,
		       sizeof(Pixel) * ST_FACE(origms).u.grad.npixels);
		ST_FACE(destms).u.grad.npixels = ST_FACE(origms).u.grad.npixels;
		ST_FACE(destms).type = GradientMenu;
		ST_FACE(destms).gradient_type = ST_FACE(origms).gradient_type;
		break;
	case PixmapMenu:
	case TiledPixmapMenu:
		ST_FACE(destms).u.p = PCacheFvwmPicture(
			dpy, Scr.NoFocusWin, NULL, ST_FACE(origms).u.p->name,
			Scr.ColorLimit);
		memcpy(&ST_FACE(destms).u.back, &ST_FACE(origms).u.back,
		       sizeof(Pixel));
		ST_FACE(destms).type = ST_FACE(origms).type;
		break;
	default:
		break;
	}

	/* PopupDelay */
	ST_POPUP_DELAY(destms) = ST_POPUP_DELAY(origms);
	/* PopupOffset */
	ST_POPUP_OFFSET_PERCENT(destms) = ST_POPUP_OFFSET_PERCENT(origms);
	/* TitleWarp */
	ST_DO_WARP_TO_TITLE(destms) = ST_DO_WARP_TO_TITLE(origms);
	/* TitleUnderlines */
	ST_TITLE_UNDERLINES(destms) = ST_TITLE_UNDERLINES(origms);
	/* Separators */
	ST_HAS_LONG_SEPARATORS(destms) = ST_HAS_LONG_SEPARATORS(origms);
	/* Triangles */
	ST_HAS_TRIANGLE_RELIEF(destms) = ST_HAS_TRIANGLE_RELIEF(origms);
	/* PopupDelayed */
	ST_DO_POPUP_IMMEDIATELY(destms) = ST_DO_POPUP_IMMEDIATELY(origms);
	/* DoubleClickTime */
	ST_DOUBLE_CLICK_TIME(destms) = ST_DOUBLE_CLICK_TIME(origms);

	/* SidePic */
	if (ST_SIDEPIC(destms))
	{
		PDestroyFvwmPicture(dpy, ST_SIDEPIC(destms));
		ST_SIDEPIC(destms) = NULL;
	}
	if (ST_SIDEPIC(origms))
	{
		ST_SIDEPIC(destms) = PCacheFvwmPicture(
			dpy, Scr.NoFocusWin, NULL, ST_SIDEPIC(origms)->name,
			Scr.ColorLimit);
	}

	/* side color */
	if (ST_HAS_SIDE_COLOR(destms) == 1)
	{
		FreeColors(&ST_SIDE_COLOR(destms), 1);
	}
	ST_HAS_SIDE_COLOR(destms) = ST_HAS_SIDE_COLOR(origms);
	if (ST_HAS_SIDE_COLOR(origms) == 1)
	{
		memcpy(&ST_SIDE_COLOR(destms), &ST_SIDE_COLOR(origms),
		       sizeof(Pixel));
	}

	/* PopupAsRootmenu */
	ST_DO_POPUP_AS_ROOT_MENU(destms) = ST_DO_POPUP_AS_ROOT_MENU(origms);
	/* RemoveSubmenus */
	ST_DO_UNMAP_SUBMENU_ON_POPDOWN(destms) =
		ST_DO_UNMAP_SUBMENU_ON_POPDOWN(origms);
	/* SubmenusRight */
	ST_USE_LEFT_SUBMENUS(destms) = ST_USE_LEFT_SUBMENUS(origms);
	/* BorderWidth */
	ST_BORDER_WIDTH(destms) = ST_BORDER_WIDTH(origms);
	/* Hilight3DThickness */
	ST_IS_ITEM_RELIEF_REVERSED(destms) = ST_IS_ITEM_RELIEF_REVERSED(origms);

	/* ItemFormat */
	if (ST_ITEM_FORMAT(destms))
	{
		free(ST_ITEM_FORMAT(destms));
		ST_ITEM_FORMAT(destms) = NULL;
	}
	if (ST_ITEM_FORMAT(origms))
	{
		ST_ITEM_FORMAT(destms) = safestrdup(ST_ITEM_FORMAT(origms));
	}

	/* AutomaticHotkeys */
	ST_USE_AUTOMATIC_HOTKEYS(destms) = ST_USE_AUTOMATIC_HOTKEYS(origms);
	/* Item and Title Spacing */
	ST_ITEM_GAP_ABOVE(destms) =  ST_ITEM_GAP_ABOVE(origms);
	ST_ITEM_GAP_BELOW(destms) = ST_ITEM_GAP_BELOW(origms);
	ST_TITLE_GAP_ABOVE(destms) = ST_TITLE_GAP_ABOVE(origms);
	ST_TITLE_GAP_BELOW(destms) = ST_TITLE_GAP_BELOW(origms);
	/* MenuColorset */
	ST_HAS_MENU_CSET(destms) = ST_HAS_MENU_CSET(origms);
	ST_CSET_MENU(destms) = ST_CSET_MENU(origms);
	/* ActiveColorset */
	ST_HAS_ACTIVE_CSET(destms) = ST_HAS_ACTIVE_CSET(origms);
	ST_CSET_ACTIVE(destms) = ST_CSET_ACTIVE(origms);
	/* MenuColorset */
	ST_HAS_GREYED_CSET(destms) = ST_HAS_GREYED_CSET(origms);
	ST_CSET_GREYED(destms) = ST_CSET_GREYED(origms);
	/* SelectOnRelease */
	ST_SELECT_ON_RELEASE_KEY(destms) = ST_SELECT_ON_RELEASE_KEY(origms);
	/* PopdownImmediately */
	ST_DO_POPDOWN_IMMEDIATELY(destms) = ST_DO_POPDOWN_IMMEDIATELY(origms);
	/* PopdownDelay */
	ST_POPDOWN_DELAY(destms) = ST_POPDOWN_DELAY(destms);

	menustyle_update(destms);

	return;
}

void CMD_MenuStyle(F_CMD_ARGS)
{
	char *option;

	GetNextSimpleOption(SkipNTokens(action, 1), &option);
	if (option == NULL || menustyle_get_styleopt_index(option) != -1)
	{
		menustyle_parse_style(F_PASS_ARGS);
	}
	else
	{
		menustyle_parse_old_style(F_PASS_ARGS);
	}
	if (option)
	{
		free(option);
	}

	return;
}
