/* -*-c-*- */
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

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>
#include <assert.h>
#include "libs/log.h"
#include "libs/fvwm_x11.h"
#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/ColorUtils.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/Graphics.h"
#include "fvwm.h"
#include "externs.h"
#include "execcontext.h"
#include "functions.h"
#include "misc.h"
#include "screen.h"
#include "colorset.h"
#include "menustyle.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static MenuStyle *default_menu_style;

/* ---------------------------- exported variables (globals) --------------- */

/* ---------------------------- local functions ---------------------------- */

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

static void parse_vertical_margins_line(
	char *args, unsigned char *top, unsigned char *bottom,
	signed char top_default, signed char bottom_default)
{
	int val[2];

	if (GetIntegerArguments(args, NULL, val, 2) != 2 ||
	    val[0] < 0 || val[0] > MAX_MENU_MARGIN ||
	    val[1] < 0 || val[1] > MAX_MENU_MARGIN)
	{
		/* invalid or missing parameters, return to default */
		*top = top_default;
		*bottom = bottom_default;
		return;
	}
	*top = val[0];
	*bottom = val[1];

	return;
}

static int menustyle_get_styleopt_index(char *option)
{
	char *optlist[] = {
		"fvwm", "mwm", "win",
		"HilightBack", "ActiveFore",
		"Hilight3DThick", "Hilight3DThin", "Hilight3DOff",
		"Animation",
		"Font",
		"PopupDelay", "PopupOffset",
		"TitleWarp",
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
		"AutomaticHotkeys",
		"VerticalItemSpacing",
		"VerticalTitleSpacing",
		"MenuColorset", "ActiveColorset", "GreyedColorset",
		"SelectOnRelease",
		"PopdownImmediately", "PopdownDelayed",
		"PopdownDelay",
		"PopupActiveArea",
		"PopupIgnore", "PopupClose",
		"MouseWheel", "ScrollOffPage",
		"TrianglesUseFore",
		"TitleColorset", "HilightTitleBack",
		"TitleFont",
		"VerticalMargins",
		"UniqueHotkeyActivatesImmediate",
		"Translucent",
		NULL
	};

	return GetTokenIndex(option, optlist, 0, NULL);
}

static void change_or_make_gc(GC *gc, unsigned long gcm, XGCValues *gcv)
{
	if (*gc != None)
	{
		XChangeGC(dpy, *gc, gcm, gcv);
	}
	else
	{
		*gc = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, gcv);
	}

	return;
}

/* ---------------------------- interface functions ------------------------ */

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
	if (FORE_GC(ST_MENU_INACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, FORE_GC(ST_MENU_INACTIVE_GCS(ms)));
	}
	if (FORE_GC(ST_MENU_ACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, FORE_GC(ST_MENU_ACTIVE_GCS(ms)));
	}
	if (BACK_GC(ST_MENU_ACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, BACK_GC(ST_MENU_ACTIVE_GCS(ms)));
	}
	if (HILIGHT_GC(ST_MENU_ACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, HILIGHT_GC(ST_MENU_ACTIVE_GCS(ms)));
	}
	if (SHADOW_GC(ST_MENU_ACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, SHADOW_GC(ST_MENU_ACTIVE_GCS(ms)));
	}
	if (HILIGHT_GC(ST_MENU_INACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, HILIGHT_GC(ST_MENU_INACTIVE_GCS(ms)));
	}
	if (SHADOW_GC(ST_MENU_INACTIVE_GCS(ms)))
	{
		XFreeGC(dpy, SHADOW_GC(ST_MENU_INACTIVE_GCS(ms)));
	}
	if (FORE_GC(ST_MENU_STIPPLE_GCS(ms)))
	{
		XFreeGC(dpy, FORE_GC(ST_MENU_STIPPLE_GCS(ms)));
	}
	if (FORE_GC(ST_MENU_TITLE_GCS(ms)))
	{
		XFreeGC(dpy, FORE_GC(ST_MENU_TITLE_GCS(ms)));
	}
	if (BACK_GC(ST_MENU_TITLE_GCS(ms)))
	{
		XFreeGC(dpy, BACK_GC(ST_MENU_TITLE_GCS(ms)));
	}
	if (HILIGHT_GC(ST_MENU_TITLE_GCS(ms)))
	{
		XFreeGC(dpy, HILIGHT_GC(ST_MENU_TITLE_GCS(ms)));
	}
	if (SHADOW_GC(ST_MENU_TITLE_GCS(ms)))
	{
		XFreeGC(dpy, SHADOW_GC(ST_MENU_TITLE_GCS(ms)));
	}
	if (ST_SIDEPIC(ms))
	{
		PDestroyFvwmPicture(dpy, ST_SIDEPIC(ms));
	}
	if (ST_HAS_SIDE_COLOR(ms) == 1)
	{
		fvwmlib_free_colors(dpy, &ST_SIDE_COLOR(ms), 1, True);
	}
	if (ST_PSTDFONT(ms) && !ST_USING_DEFAULT_FONT(ms))
	{
		FlocaleUnloadFont(dpy, ST_PSTDFONT(ms));
	}
	if (ST_PTITLEFONT(ms) && !ST_USING_DEFAULT_TITLEFONT(ms))
	{
		FlocaleUnloadFont(dpy, ST_PTITLEFONT(ms));
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
	colorset_t *menu_cs = &Colorset[ST_CSET_MENU(ms)];
	colorset_t *active_cs = &Colorset[ST_CSET_ACTIVE(ms)];
	colorset_t *greyed_cs = &Colorset[ST_CSET_GREYED(ms)];
	colorset_t *title_cs = &Colorset[ST_CSET_TITLE(ms)];

	if (ST_USAGE_COUNT(ms) != 0)
	{
		fvwm_debug(__func__, "menu style %s is in use",
			   ST_NAME(ms));
		return;
	}
	ST_IS_UPDATED(ms) = 1;
	if (ST_USING_DEFAULT_FONT(ms))
	{
		ST_PSTDFONT(ms) = Scr.DefaultFont;
	}
	if (ST_USING_DEFAULT_TITLEFONT(ms))
	{
		ST_PTITLEFONT(ms) = ST_PSTDFONT(ms);
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
	/* update inactive menu gcs */
	gcv.foreground = menu_cs->fg;
	gcv.background = menu_cs->bg;
	change_or_make_gc(&FORE_GC(ST_MENU_INACTIVE_GCS(ms)), gcm, &gcv);
	BACK_GC(ST_MENU_INACTIVE_GCS(ms)) = FORE_GC(ST_MENU_INACTIVE_GCS(ms));
	gcv.foreground = menu_cs->hilite;
	gcv.background = menu_cs->shadow;
	change_or_make_gc(&HILIGHT_GC(ST_MENU_INACTIVE_GCS(ms)), gcm, &gcv);
	gcv.foreground = menu_cs->shadow;
	gcv.background = menu_cs->hilite;
	change_or_make_gc(&SHADOW_GC(ST_MENU_INACTIVE_GCS(ms)), gcm, &gcv);
	/* update active menu gcs */
	gcv.foreground = (ST_DO_HILIGHT_FORE(ms)) ? active_cs->fg
						  : menu_cs->fg;
	gcv.background = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->bg
						  : menu_cs->bg;
	change_or_make_gc(&FORE_GC(ST_MENU_ACTIVE_GCS(ms)), gcm, &gcv);
	gcv.foreground = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->bg
						  : menu_cs->bg;
	gcv.background = (ST_DO_HILIGHT_FORE(ms)) ? active_cs->fg
						  : menu_cs->fg;

	if (active_cs->pixmap && active_cs->pixmap_type == PIXMAP_TILED)
	{
		gcv.tile = active_cs->pixmap;
		gcv.fill_style = FillTiled;
		change_or_make_gc(&BACK_GC(ST_MENU_ACTIVE_GCS(ms)),
				  gcm | GCFillStyle | GCTile , &gcv);
	}
	else
	{
		gcv.fill_style = FillSolid;
		change_or_make_gc(&BACK_GC(ST_MENU_ACTIVE_GCS(ms)),
				  gcm | GCFillStyle , &gcv);
	}

	gcv.foreground = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->hilite
						  : menu_cs->hilite;
	gcv.background = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->shadow
						  : menu_cs->shadow;
	change_or_make_gc(&HILIGHT_GC(ST_MENU_ACTIVE_GCS(ms)), gcm, &gcv);
	gcv.foreground = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->shadow
						  : menu_cs->shadow;
	gcv.background = (ST_DO_HILIGHT_BACK(ms)) ? active_cs->hilite
						  : menu_cs->hilite;
	change_or_make_gc(&SHADOW_GC(ST_MENU_ACTIVE_GCS(ms)), gcm, &gcv);
	/* update title gcs */
	if (ST_PTITLEFONT(ms)->font != NULL && ST_PSTDFONT(ms)->font == NULL)
	{
		if (ST_PSTDFONT(ms)->font == NULL)
		{
			gcm |= GCFont;
		}
		gcv.font = ST_PTITLEFONT(ms)->font->fid;
	}
	gcv.foreground = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->fg
							: menu_cs->fg;
	gcv.background = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->bg
							: menu_cs->bg;
	change_or_make_gc(&FORE_GC(ST_MENU_TITLE_GCS(ms)), gcm, &gcv);
	gcv.foreground = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->bg
							: menu_cs->bg;
	gcv.background = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->fg
							: menu_cs->fg;

	if (title_cs->pixmap && title_cs->pixmap_type == PIXMAP_TILED)
	{
		gcv.tile = title_cs->pixmap;
		gcv.fill_style = FillTiled;
		change_or_make_gc(&BACK_GC(ST_MENU_TITLE_GCS(ms)),
				  gcm | GCFillStyle | GCTile , &gcv);
	}
	else
	{
		gcv.fill_style = FillSolid;
		change_or_make_gc(&BACK_GC(ST_MENU_TITLE_GCS(ms)),
				  gcm | GCFillStyle , &gcv);
	}

	gcv.foreground = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->hilite
							: menu_cs->hilite;
	gcv.background = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->shadow
							: menu_cs->shadow;
	change_or_make_gc(&HILIGHT_GC(ST_MENU_TITLE_GCS(ms)), gcm, &gcv);
	gcv.foreground = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->shadow
							: menu_cs->shadow;
	gcv.background = (ST_DO_HILIGHT_TITLE_BACK(ms)) ? title_cs->hilite
							: menu_cs->hilite;
	change_or_make_gc(&SHADOW_GC(ST_MENU_TITLE_GCS(ms)), gcm, &gcv);
	/* update stipple menu gcs */
	SHADOW_GC(ST_MENU_STIPPLE_GCS(ms)) =
		SHADOW_GC(ST_MENU_INACTIVE_GCS(ms));
	if (Pdepth < 2)
	{
		gcv.fill_style = FillStippled;
		gcv.stipple = Scr.gray_bitmap;
		/* no need to reset fg/bg, FillStipple wins */
		gcm |= GCStipple | GCFillStyle;
		HILIGHT_GC(ST_MENU_STIPPLE_GCS(ms)) =
			SHADOW_GC(ST_MENU_INACTIVE_GCS(ms));
	}
	else
	{
		gcv.foreground = greyed_cs->fg;
		gcv.background = greyed_cs->bg;
		HILIGHT_GC(ST_MENU_STIPPLE_GCS(ms)) =
			HILIGHT_GC(ST_MENU_INACTIVE_GCS(ms));
	}
	change_or_make_gc(&FORE_GC(ST_MENU_STIPPLE_GCS(ms)), gcm, &gcv);
	BACK_GC(ST_MENU_STIPPLE_GCS(ms)) = BACK_GC(ST_MENU_INACTIVE_GCS(ms));

	return;
}

MenuStyle *menustyle_parse_style(F_CMD_ARGS)
{
	char *name;
	char *option = NULL;
	char *poption = NULL;
	char *optstring = NULL;
	char *args = NULL;
	char *arg1;
	int on;
	MenuStyle *ms;
	MenuStyle *tmpms;
	Bool is_initialised = True;
	Bool has_gc_changed = False;
	int val[2];
	int n;
	FlocaleFont *new_font = NULL;
	int i;
	KeyCode keycode;
	FvwmPictureAttributes fpa;

	action = GetNextToken(action, &name);
	if (!name)
	{
		fvwm_debug(__func__,
			   "error in %s style specification",action);
		return NULL;
	}

	ms = menustyle_find(name);
	if (ms && ST_USAGE_COUNT(ms) != 0)
	{
		fvwm_debug(__func__, "menu style %s is in use", name);
		return ms;
	}
	tmpms = fxmalloc(sizeof(MenuStyle));
	if (ms)
	{
		/* copy the structure over our temporary menu face. */
		memcpy(tmpms, ms, sizeof(MenuStyle));
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
		on = 1;
		if (!is_initialised)
		{
			/* some default configuration goes here for the new
			 * menu style */
			ST_CSET_MENU(tmpms) = 0;
			ST_CSET_ACTIVE(tmpms) = 0;
			ST_CSET_TITLE(tmpms) = 0;
			ST_CSET_GREYED(tmpms) = 0;
			ST_PSTDFONT(tmpms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(tmpms) = True;
			ST_HAS_ACTIVE_FORE(tmpms) = 0;
			ST_HAS_ACTIVE_BACK(tmpms) = 0;
			ST_DO_POPUP_AS(tmpms) = MDP_POST_MENU;
			ST_DOUBLE_CLICK_TIME(tmpms) = DEFAULT_MENU_CLICKTIME;
			ST_POPUP_DELAY(tmpms) = DEFAULT_POPUP_DELAY;
			ST_POPDOWN_DELAY(tmpms) = DEFAULT_POPDOWN_DELAY;
			ST_MOUSE_WHEEL(tmpms) = MMW_POINTER;
			ST_SCROLL_OFF_PAGE(tmpms) = 1;
			ST_DO_HILIGHT_TITLE_BACK(tmpms) = 0;
			ST_USING_DEFAULT_TITLEFONT(tmpms) = True;
			ST_TRANSLUCENT_PERCENT(tmpms) = 100;
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
			(void)GetNextToken(args, &arg1);
		}
		poption = option;
		while (poption[0] == '!')
		{
			on ^= 1;
			poption++;
		}
		switch((i = menustyle_get_styleopt_index(poption)))
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
				ST_DO_HILIGHT_BACK(tmpms) = 0;
				ST_DO_HILIGHT_FORE(tmpms) = 0;
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
				ST_DO_HILIGHT_BACK(tmpms) = 0;
				ST_DO_HILIGHT_FORE(tmpms) = 0;
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
				ST_DO_HILIGHT_BACK(tmpms) = 1;
				ST_DO_HILIGHT_FORE(tmpms) = 1;
			}

			/* common settings */
			ST_VERTICAL_MARGIN_TOP(tmpms) = 0;
			ST_VERTICAL_MARGIN_BOTTOM(tmpms) = 0;
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
			/* Pressing a hotkey on an item which only has a
			 * single entry will activate that action; turning
			 * those off will make the menu persist until enter or
			 * space is pressed; the default behaviour is to
			 * always close the menu and run the action.
			 */
			ST_HOTKEY_ACTIVATES_IMMEDIATE(tmpms) = 1;
			if (ST_PSTDFONT(tmpms) && !ST_USING_DEFAULT_FONT(tmpms))
			{
				FlocaleUnloadFont(dpy, ST_PSTDFONT(tmpms));
			}
			ST_PSTDFONT(tmpms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(tmpms) = True;
			has_gc_changed = True;
			if (ST_HAS_SIDE_COLOR(tmpms) == 1)
			{
				fvwmlib_free_colors(
					dpy, &ST_SIDE_COLOR(tmpms), 1, True);
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

		case 3: /* HilightBack */
			ST_DO_HILIGHT_BACK(tmpms) = on;
			has_gc_changed = True;
			break;

		case 4: /* ActiveFore */
			ST_DO_HILIGHT_FORE(tmpms) = on;
			has_gc_changed = True;
			break;

		case 5: /* Hilight3DThick */
			ST_RELIEF_THICKNESS(tmpms) = 2;
			break;

		case 6: /* Hilight3DThin */
			ST_RELIEF_THICKNESS(tmpms) = 1;
			break;

		case 7: /* Hilight3DOff */
			ST_RELIEF_THICKNESS(tmpms) = 0;
			break;

		case 8: /* Animation */
			ST_IS_ANIMATED(tmpms) = on;
			break;

		case 9: /* Font */
			if (arg1 != NULL &&
			    !(new_font = FlocaleLoadFont(dpy, arg1, "fvwm")))
			{
				fvwm_debug(__func__,
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

		case 10: /* PopupDelay */
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

		case 11: /* PopupOffset */
			if ((n = GetIntegerArguments(args, NULL, val, 2)) == 0)
			{
				fvwm_debug(__func__,
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

		case 12: /* TitleWarp */
			ST_DO_WARP_TO_TITLE(tmpms) = on;
			break;

		case 13: /* TitleUnderlines0 */
			ST_TITLE_UNDERLINES(tmpms) = 0;
			break;

		case 14: /* TitleUnderlines1 */
			ST_TITLE_UNDERLINES(tmpms) = 1;
			break;

		case 15: /* TitleUnderlines2 */
			ST_TITLE_UNDERLINES(tmpms) = 2;
			break;

		case 16: /* SeparatorsLong */
			ST_HAS_LONG_SEPARATORS(tmpms) = on;
			break;

		case 17: /* SeparatorsShort */
			ST_HAS_LONG_SEPARATORS(tmpms) = !on;
			break;

		case 18: /* TrianglesSolid */
			ST_HAS_TRIANGLE_RELIEF(tmpms) = !on;
			break;

		case 19: /* TrianglesRelief */
			ST_HAS_TRIANGLE_RELIEF(tmpms) = on;
			break;

		case 20: /* PopupImmediately */
			ST_DO_POPUP_IMMEDIATELY(tmpms) = on;
			break;

		case 21: /* PopupDelayed */
			ST_DO_POPUP_IMMEDIATELY(tmpms) = !on;
			break;

		case 22: /* DoubleClickTime */
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

		case 23: /* SidePic */
			if (ST_SIDEPIC(tmpms))
			{
				PDestroyFvwmPicture(dpy, ST_SIDEPIC(tmpms));
				ST_SIDEPIC(tmpms) = NULL;
			}
			if (arg1)
			{
				fpa.mask = (Pdepth <= 8)?  FPAM_DITHER:0;
				ST_SIDEPIC(tmpms) = PCacheFvwmPicture(
					dpy, Scr.NoFocusWin, NULL, arg1, fpa);
				if (!ST_SIDEPIC(tmpms))
				{
					fvwm_debug(__func__,
						   "Couldn't find pixmap %s",
						   arg1);
				}
			}
			break;

		case 24: /* SideColor */
			if (ST_HAS_SIDE_COLOR(tmpms) == 1)
			{
				fvwmlib_free_colors(
					dpy, &ST_SIDE_COLOR(tmpms), 1, True);
				ST_HAS_SIDE_COLOR(tmpms) = 0;
			}
			if (arg1)
			{
				ST_SIDE_COLOR(tmpms) = GetColor(arg1);
				ST_HAS_SIDE_COLOR(tmpms) = 1;
			}
			break;

		case 25: /* PopupAsRootmenu */
			ST_DO_POPUP_AS(tmpms) = MDP_ROOT_MENU;
			break;

		case 26: /* PopupAsSubmenu */
			ST_DO_POPUP_AS(tmpms) = MDP_POST_MENU;
			break;

		case 27: /* RemoveSubmenus */
			ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = on;
			break;

		case 28: /* HoldSubmenus */
			ST_DO_UNMAP_SUBMENU_ON_POPDOWN(tmpms) = !on;
			break;

		case 29: /* SubmenusRight */
			ST_USE_LEFT_SUBMENUS(tmpms) = !on;
			break;

		case 30: /* SubmenusLeft */
			ST_USE_LEFT_SUBMENUS(tmpms) = on;
			break;

		case 31: /* BorderWidth */
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

		case 32: /* Hilight3DThickness */
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

		case 33: /* ItemFormat */
			free(ST_ITEM_FORMAT(tmpms));
			ST_ITEM_FORMAT(tmpms) = NULL;
			if (arg1)
			{
				ST_ITEM_FORMAT(tmpms) = fxstrdup(arg1);
			}
			break;

		case 34: /* AutomaticHotkeys */
			ST_USE_AUTOMATIC_HOTKEYS(tmpms) = on;
			break;

		case 35: /* VerticalItemSpacing */
			parse_vertical_spacing_line(
				args, &ST_ITEM_GAP_ABOVE(tmpms),
				&ST_ITEM_GAP_BELOW(tmpms),
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET,
				DEFAULT_MENU_ITEM_TEXT_Y_OFFSET2);
			break;

		case 36: /* VerticalTitleSpacing */
			parse_vertical_spacing_line(
				args, &ST_TITLE_GAP_ABOVE(tmpms),
				&ST_TITLE_GAP_BELOW(tmpms),
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET,
				DEFAULT_MENU_TITLE_TEXT_Y_OFFSET2);
			break;

		case 37: /* MenuColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_CSET_MENU(tmpms) = 0;
			}
			else
			{
				ST_CSET_MENU(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 38: /* ActiveColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_CSET_ACTIVE(tmpms) = 0;
			}
			else
			{
				ST_CSET_ACTIVE(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 39: /* GreyedColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_CSET_GREYED(tmpms) = 0;
			}
			else
			{
				ST_CSET_GREYED(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 40: /* SelectOnRelease */
			keycode = 0;
			if (arg1)
			{
				keycode = XKeysymToKeycode(
					dpy, FvwmStringToKeysym(dpy, arg1));
			}
			ST_SELECT_ON_RELEASE_KEY(tmpms) = keycode;
			break;

		case 41: /* PopdownImmediately */
			ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 1;
			break;

		case 42: /* PopdownDelayed */
			ST_DO_POPDOWN_IMMEDIATELY(tmpms) = 0;
			break;

		case 43: /* PopdownDelay */
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

		case 44: /* PopupActiveArea */
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

		case 45: /* PopupIgnore */
			ST_DO_POPUP_AS(tmpms) = MDP_IGNORE;
			break;

		case 46: /* PopupClose */
			ST_DO_POPUP_AS(tmpms) = MDP_CLOSE;
			break;

		case 47: /* MouseWheel */
			if (arg1)
			{
				if (StrEquals(arg1, "ActivatesItem"))
				{
					ST_MOUSE_WHEEL(tmpms) = MMW_OFF;
				}
				else if (StrEquals(arg1,
						   "ScrollsMenuBackwards"))
				{
					ST_MOUSE_WHEEL(tmpms) =
						MMW_MENU_BACKWARDS;
				}
				else if (StrEquals(arg1, "ScrollsMenu"))
				{
					ST_MOUSE_WHEEL(tmpms) = MMW_MENU;
				}
				else if (StrEquals(arg1, "ScrollsPointer"))
				{
					ST_MOUSE_WHEEL(tmpms) = MMW_POINTER;
				}
				else
				{
					fvwm_debug(__func__,
						   "unknown argument to"
						   " MouseWheel '%s'",
						   arg1);
					ST_MOUSE_WHEEL(tmpms) = MMW_POINTER;
				}
			}
			else
			{
				ST_MOUSE_WHEEL(tmpms) =
					(on) ? MMW_POINTER : MMW_OFF;
			}
			break;

		case 48: /* ScrollOffPage */
			ST_SCROLL_OFF_PAGE(tmpms) = on;
			break;

		case 49: /* TrianglesUseFore */
			ST_TRIANGLES_USE_FORE(tmpms) = on;
			break;

		case 50: /* TitleColorset */
			if (GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0)
			{
				ST_CSET_TITLE(tmpms) = 0;
			}
			else
			{
				ST_CSET_TITLE(tmpms) = *val;
				alloc_colorset(*val);
			}
			has_gc_changed = True;
			break;

		case 51: /* TitleHilightBack */
			ST_DO_HILIGHT_TITLE_BACK(tmpms) = on;
			has_gc_changed = True;
			break;

		case 52: /* TitleFont */
			if (arg1 != NULL &&
			    !(new_font = FlocaleLoadFont(dpy, arg1, "fvwm")))
			{
				fvwm_debug(__func__,
					   "Couldn't load font '%s'\n", arg1);
				break;
			}
			if (
				ST_PTITLEFONT(tmpms) &&
				!ST_USING_DEFAULT_TITLEFONT(tmpms))
			{
				FlocaleUnloadFont(dpy, ST_PTITLEFONT(tmpms));
			}
			if (arg1 == NULL)
			{
				/* reset to screen font */
				ST_PTITLEFONT(tmpms) = Scr.DefaultFont;
				ST_USING_DEFAULT_TITLEFONT(tmpms) = True;
			}
			else
			{
				ST_PTITLEFONT(tmpms) = new_font;
				ST_USING_DEFAULT_TITLEFONT(tmpms) = False;
			}
			has_gc_changed = True;
			break;

		case 53: /* VerticalMargins */
			parse_vertical_margins_line(
				args, &ST_VERTICAL_MARGIN_TOP(tmpms),
				&ST_VERTICAL_MARGIN_BOTTOM(tmpms),
				0, 0);
			break;

		case 54: /* UniqueHotKeyActivatesImmediate */
			ST_HOTKEY_ACTIVATES_IMMEDIATE(tmpms) = on;
			break;

		case 55: /* Translucent */
			if (!on ||
			    GetIntegerArguments(args, NULL, val, 1) == 0 ||
			    *val < 0 || *val > 100)
			{
				/* 100 percent means not translucent */
				ST_TRANSLUCENT_PERCENT(tmpms) = 100;
			} else {
				ST_TRANSLUCENT_PERCENT(tmpms) = *val;
			}
			break;

#if 0
		case 99: /* PositionHints */
			/* to be implemented */
			break;
#endif

		default:
			fvwm_debug(__func__, "unknown option '%s'",
				   poption);
			break;
		} /* switch */
		free(option);
		option = NULL;
		free(optstring);
		optstring = NULL;
		free(arg1);
		arg1 = NULL;
	} /* while */

	if (has_gc_changed)
	{
		menustyle_update(tmpms);
	}

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
		return ms;
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

	return tmpms;
}

void menustyle_copy(MenuStyle *origms, MenuStyle *destms)
{
	FvwmPictureAttributes fpa;
	/* Copy origms to destms, be aware of all pointers in the MenuStyle
	   structure. Use  the same order as in menustyle_parse_style */

	/* HilightBack / ActiveFore */
	ST_DO_HILIGHT_BACK(destms) = ST_DO_HILIGHT_BACK(origms);
	ST_DO_HILIGHT_FORE(destms) = ST_DO_HILIGHT_FORE(origms);

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
		      FlocaleLoadFont(dpy, ST_PSTDFONT(origms)->name, "fvwm")))
		{
			ST_PSTDFONT(destms) = Scr.DefaultFont;
			ST_USING_DEFAULT_FONT(destms) = True;
			fvwm_debug(__func__,
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
	/* TitleFont */
	if (ST_PTITLEFONT(destms) &&  !ST_USING_DEFAULT_TITLEFONT(destms))
	{
		FlocaleUnloadFont(dpy, ST_PTITLEFONT(destms));
	}
	if (ST_PTITLEFONT(origms) && !ST_USING_DEFAULT_TITLEFONT(origms))
	{
		if (
			!(ST_PTITLEFONT(destms) = FlocaleLoadFont(
				  dpy, ST_PTITLEFONT(origms)->name, "fvwm")))
		{
			ST_PTITLEFONT(destms) = Scr.DefaultFont;
			ST_USING_DEFAULT_TITLEFONT(destms) = True;
			fvwm_debug(__func__,
				   "Couldn't load font '%s' use Default Font\n",
				   ST_PTITLEFONT(origms)->name);
		}
		else
		{
			ST_USING_DEFAULT_TITLEFONT(destms) = False;
		}
	}
	else
	{
		ST_USING_DEFAULT_TITLEFONT(destms) = True;
		ST_PTITLEFONT(destms) = Scr.DefaultFont;
	}

	/* PopupDelay */
	ST_POPUP_DELAY(destms) = ST_POPUP_DELAY(origms);
	/* PopupOffset */
	ST_POPUP_OFFSET_PERCENT(destms) = ST_POPUP_OFFSET_PERCENT(origms);
	ST_POPUP_OFFSET_ADD(destms) = ST_POPUP_OFFSET_ADD(origms);
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
	/* VerticalMargins */
	ST_VERTICAL_MARGIN_TOP(destms) = ST_VERTICAL_MARGIN_TOP(origms);
	ST_VERTICAL_MARGIN_BOTTOM(destms) = ST_VERTICAL_MARGIN_BOTTOM(origms);

	/* SidePic */
	if (ST_SIDEPIC(destms))
	{
		PDestroyFvwmPicture(dpy, ST_SIDEPIC(destms));
		ST_SIDEPIC(destms) = NULL;
	}
	if (ST_SIDEPIC(origms))
	{
		fpa.mask = (Pdepth <= 8)?  FPAM_DITHER:0;
		ST_SIDEPIC(destms) = PCacheFvwmPicture(
			dpy, Scr.NoFocusWin, NULL, ST_SIDEPIC(origms)->name,
			fpa);
	}

	/* side color */
	fvwmlib_copy_color(
		dpy, &ST_SIDE_COLOR(destms), &ST_SIDE_COLOR(origms),
		ST_HAS_SIDE_COLOR(destms), ST_HAS_SIDE_COLOR(origms));
	ST_HAS_SIDE_COLOR(destms) = ST_HAS_SIDE_COLOR(origms);

	/* PopupAsRootmenu */
	ST_DO_POPUP_AS(destms) = ST_DO_POPUP_AS(origms);
	/* RemoveSubmenus */
	ST_DO_UNMAP_SUBMENU_ON_POPDOWN(destms) =
		ST_DO_UNMAP_SUBMENU_ON_POPDOWN(origms);
	/* SubmenusRight */
	ST_USE_LEFT_SUBMENUS(destms) = ST_USE_LEFT_SUBMENUS(origms);
	/* BorderWidth */
	ST_BORDER_WIDTH(destms) = ST_BORDER_WIDTH(origms);
	/* Hilight3DThickness */
	ST_IS_ITEM_RELIEF_REVERSED(destms) =
		ST_IS_ITEM_RELIEF_REVERSED(origms);
	free(ST_ITEM_FORMAT(destms));
	ST_ITEM_FORMAT(destms) = NULL;
	if (ST_ITEM_FORMAT(origms))
	{
		ST_ITEM_FORMAT(destms) = fxstrdup(ST_ITEM_FORMAT(origms));
	}

	/* AutomaticHotkeys */
	ST_USE_AUTOMATIC_HOTKEYS(destms) = ST_USE_AUTOMATIC_HOTKEYS(origms);
	ST_HOTKEY_ACTIVATES_IMMEDIATE(destms) =
		ST_HOTKEY_ACTIVATES_IMMEDIATE(origms);
	/* Item and Title Spacing */
	ST_ITEM_GAP_ABOVE(destms) =  ST_ITEM_GAP_ABOVE(origms);
	ST_ITEM_GAP_BELOW(destms) = ST_ITEM_GAP_BELOW(origms);
	ST_TITLE_GAP_ABOVE(destms) = ST_TITLE_GAP_ABOVE(origms);
	ST_TITLE_GAP_BELOW(destms) = ST_TITLE_GAP_BELOW(origms);
	/* MenuColorsets */
	ST_CSET_MENU(destms) = ST_CSET_MENU(origms);
	ST_CSET_ACTIVE(destms) = ST_CSET_ACTIVE(origms);
	ST_CSET_GREYED(destms) = ST_CSET_GREYED(origms);
	ST_CSET_TITLE(destms) = ST_CSET_TITLE(origms);
	/* SelectOnRelease */
	ST_SELECT_ON_RELEASE_KEY(destms) = ST_SELECT_ON_RELEASE_KEY(origms);
	/* PopdownImmediately */
	ST_DO_POPDOWN_IMMEDIATELY(destms) = ST_DO_POPDOWN_IMMEDIATELY(origms);
	/* PopdownDelay */
	ST_POPDOWN_DELAY(destms) = ST_POPDOWN_DELAY(origms);
	/* Scroll */
	ST_MOUSE_WHEEL(destms) = ST_MOUSE_WHEEL(origms);
	/* ScrollOffPage */
	ST_SCROLL_OFF_PAGE(destms) = ST_SCROLL_OFF_PAGE(origms);
	/* TrianglesUseFore */
	ST_TRIANGLES_USE_FORE(destms) = ST_TRIANGLES_USE_FORE(origms);
	/* Title */
	ST_DO_HILIGHT_TITLE_BACK(destms) = ST_DO_HILIGHT_TITLE_BACK(origms);

	menustyle_update(destms);

	return;
}

/* ---------------------------- builtin commands --------------------------- */

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
		fvwm_debug(__func__, "need two arguments");
		return;
	}

	origms = menustyle_find(origname);
	if (!origms)
	{
		fvwm_debug(__func__, "%s: no such menu style",
			   origname);
		return;
	}

	destname = PeekToken(action, &action);
	if (destname == NULL)
	{
		fvwm_debug(__func__, "need two arguments");
		return;
	}

	if (action && *action)
	{
		fvwm_debug(__func__, "too many arguments");
		return;
	}

	destms = menustyle_find(destname);
	if (!destms)
	{
		/* create destms menu style */
		xasprintf(&buffer, "\"%s\"", destname);
		action = buffer;
		destms = menustyle_parse_style(F_PASS_ARGS);
		free(buffer);
		if (!destms)
		{
			/* this must never happen */
			fvwm_debug(__func__,
				   "impossible to create %s menu style",
				   destname);
			return;
		}
	}

	if (strcasecmp("*",destname) == 0)
	{
		fvwm_debug(__func__,
			   "You cannot copy on the default menu style");
		return;
	}
	if (strcasecmp(ST_NAME(origms),destname) == 0)
	{
		fvwm_debug(__func__,
			   "%s and %s identify the same menu style",
			   ST_NAME(origms),destname);
		return;
	}

	if (ST_USAGE_COUNT(destms) != 0)
	{
		fvwm_debug(__func__, "menu style %s is in use",
			   destname);
		return;
	}

	menustyle_copy(origms, destms);

	return;
}

void CMD_MenuStyle(F_CMD_ARGS)
{
	(void)menustyle_parse_style(F_PASS_ARGS);
	return;
}
