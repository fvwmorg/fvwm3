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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/setpgrp.h"
#include "libs/Grab.h"
#include "libs/Parse.h"
#include "libs/ColorUtils.h"
#include "libs/Graphics.h"
#include "libs/wild.h"
#include "libs/envvar.h"
#include "libs/ClientMsg.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
#include "libs/FGettext.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "libs/Flocale.h"
#include "libs/Ficonv.h"
#include "fvwm.h"
#include "externs.h"
#include "colorset.h"
#include "bindings.h"
#include "misc.h"
#include "cursor.h"
#include "functions.h"
#include "commands.h"
#include "screen.h"
#include "builtins.h"
#include "module_interface.h"
#include "borders.h"
#include "frame.h"
#include "events.h"
#include "ewmh.h"
#include "virtual.h"
#include "decorations.h"
#include "add_window.h"
#include "update.h"
#include "style.h"
#include "move_resize.h"
#include "menus.h"
#include "infostore.h"
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

/* ---------------------------- imports ------------------------------------ */

extern float rgpctMovementDefault[32];
extern int cpctMovementDefault;
extern int cmsDelayDefault;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */
typedef enum {FakeMouseEvent, FakeKeyEvent} FakeEventType;
/* ---------------------------- forward declarations ----------------------- */

/* ---------------------------- local variables ---------------------------- */

static char *exec_shell_name="/bin/sh";

/* button state strings must match the enumerated states */
static char *button_states[BS_MaxButtonStateName + 1] =
{
	"ActiveUp",
	"ActiveDown",
	"InactiveUp",
	"InactiveDown",
	"ToggledActiveUp",
	"ToggledActiveDown",
	"ToggledInactiveUp",
	"ToggledInactiveDown",
	"Active",
	"Inactive",
	"ToggledActive",
	"ToggledInactive",
	"AllNormal",
	"AllToggled",
	"AllActive",
	"AllInactive",
	"AllUp",
	"AllDown",
	"AllActiveUp",
	"AllActiveDown",
	"AllInactiveUp",
	"AllInactiveDown",
	NULL
};

/* ---------------------------- exported variables (globals) --------------- */

char *ModulePath = FVWM_MODULEDIR;
int moduleTimeout = DEFAULT_MODULE_TIMEOUT;

/* ---------------------------- local functions ---------------------------- */

/** Prepend rather than replace the image path.
    Used for obsolete PixmapPath and IconPath **/
static void obsolete_imagepaths( const char* pre_path )
{
	char* tmp = stripcpy( pre_path );
	char* path = alloca(strlen( tmp ) + strlen(PictureGetImagePath()) + 2 );

	strcpy( path, tmp );
	free( tmp );

	strcat( path, ":" );
	strcat( path, PictureGetImagePath() );

	PictureSetImagePath( path );

	return;
}

/*
 *
 * Reads a title button description (veliaa@rpi.edu)
 *
 */
static char *ReadTitleButton(
	char *s, TitleButton *tb, Boolean append, int button)
{
	char *end = NULL;
	char *spec;
	char *t;
	int i;
	int bs;
	int bs_start, bs_end;
	int pstyle = 0;
	DecorFace tmpdf;

	Bool multiple;
	int use_mask = 0;
	int set_mask = 0;

	s = SkipSpaces(s, NULL, 0);
	t = GetNextTokenIndex(s, button_states, 0, &bs);
	if (bs != BS_All)
	{
		s = SkipSpaces(t, NULL, 0);
	}

	if (bs == BS_All)
	{
		use_mask = 0;
		set_mask = 0;
	}
	else if (bs == BS_Active)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_TOGGLED;
		set_mask = 0;
	}
	else if (bs == BS_Inactive)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_TOGGLED;
		set_mask = BS_MASK_INACTIVE;
	}
	else if (bs == BS_ToggledActive)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_TOGGLED;
		set_mask = BS_MASK_TOGGLED;
	}
	else if (bs == BS_ToggledInactive)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_TOGGLED;
		set_mask = BS_MASK_INACTIVE | BS_MASK_TOGGLED;
	}
	else if (bs == BS_AllNormal)
	{
		use_mask = BS_MASK_TOGGLED;
		set_mask = 0;
	}
	else if (bs == BS_AllToggled)
	{
		use_mask = BS_MASK_TOGGLED;
		set_mask = BS_MASK_TOGGLED;
	}
	else if (bs == BS_AllActive)
	{
		use_mask = BS_MASK_INACTIVE;
		set_mask = 0;
	}
	else if (bs == BS_AllInactive)
	{
		use_mask = BS_MASK_INACTIVE;
		set_mask = BS_MASK_INACTIVE;
	}
	else if (bs == BS_AllUp)
	{
		use_mask = BS_MASK_DOWN;
		set_mask = 0;
	}
	else if (bs == BS_AllDown)
	{
		use_mask = BS_MASK_DOWN;
		set_mask = BS_MASK_DOWN;
	}
	else if (bs == BS_AllActiveUp)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_DOWN;
		set_mask = 0;
	}
	else if (bs == BS_AllActiveDown)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_DOWN;
		set_mask = BS_MASK_DOWN;
	}
	else if (bs == BS_AllInactiveUp)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_DOWN;
		set_mask = BS_MASK_INACTIVE;
	}
	else if (bs == BS_AllInactiveDown)
	{
		use_mask = BS_MASK_INACTIVE | BS_MASK_DOWN;
		set_mask = BS_MASK_INACTIVE | BS_MASK_DOWN;
	}

	if ((bs & BS_MaxButtonStateMask) == bs)
	{
		multiple = False;
		bs_start = bs;
		bs_end = bs;
	}
	else
	{
		multiple = True;
		bs_start = 0;
		bs_end = BS_MaxButtonState - 1;
		for (i = bs_start; (i & use_mask) != set_mask && i <= bs_end;
			i++)
		{
			bs_start++;
		}
	}

	if (*s == '(')
	{
		int len;
		pstyle = 1;
		if (!(end = strchr(++s, ')')))
		{
			fvwm_msg(
				ERR, "ReadTitleButton",
				"missing parenthesis: %s", s);
			return NULL;
		}
		s = SkipSpaces(s, NULL, 0);
		len = end - s + 1;
		/* TA:  FIXME!  xasprintf() */
		spec = xmalloc(len);
		strncpy(spec, s, len - 1);
		spec[len - 1] = 0;
	}
	else
	{
		spec = s;
	}

	spec = SkipSpaces(spec, NULL, 0);
	/* setup temporary in case button read fails */
	memset(&tmpdf, 0, sizeof(DecorFace));
	DFS_FACE_TYPE(tmpdf.style) = SimpleButton;

	if (strncmp(spec, "--", 2) == 0)
	{
		/* only change flags */
		Bool verbose = True;
		for (i = bs_start; i <= bs_end; ++i)
		{
			if (multiple && (i & use_mask) != set_mask)
			{
				continue;
			}
			ReadDecorFace(spec, &TB_STATE(*tb)[i], button, verbose);
			verbose = False;
		}
	}
	else if (ReadDecorFace(spec, &tmpdf, button, True))
	{
		if (append)
		{
			DecorFace *head = &TB_STATE(*tb)[bs_start];
			DecorFace *tail = head;
			DecorFace *next;

			while (tail->next)
			{
				tail = tail->next;
			}
			tail->next = xmalloc(sizeof(DecorFace));
			memcpy(tail->next, &tmpdf, sizeof(DecorFace));
			if (DFS_FACE_TYPE(tail->next->style) == VectorButton &&
			    DFS_FACE_TYPE((&TB_STATE(*tb)[bs_start])->style) ==
			    DefaultVectorButton)
			{
				/* override the default vector style */
				memcpy(
					&tail->next->style, &head->style,
					sizeof(DecorFaceStyle));
				DFS_FACE_TYPE(tail->next->style) = VectorButton;
				next = head->next;
				head->next = NULL;
				FreeDecorFace(dpy, head);
				memcpy(head, next, sizeof(DecorFace));
				free(next);
			}
			for (i = bs_start + 1; i <= bs_end; ++i)
			{
				if (multiple && (i & use_mask) != set_mask)
				{
					continue;
				}
				head = &TB_STATE(*tb)[i];
				tail = head;
				while (tail->next)
				{
					tail = tail->next;
				}
				tail->next = xcalloc(1, sizeof(DecorFace));
				DFS_FACE_TYPE(tail->next->style) =
					SimpleButton;
				tail->next->next = NULL;
				ReadDecorFace(spec, tail->next, button, False);
				if (DFS_FACE_TYPE(tail->next->style) ==
				    VectorButton &&
				    DFS_FACE_TYPE((&TB_STATE(*tb)[i])->style) ==
				    DefaultVectorButton)
				{
					/* override the default vector style */
					memcpy(
						&tail->next->style,
						&head->style,
						sizeof(DecorFaceStyle));
					DFS_FACE_TYPE(tail->next->style) =
						VectorButton;
					next = head->next;
					head->next = NULL;
					FreeDecorFace(dpy, head);
					memcpy(head, next, sizeof(DecorFace));
					free(next);
				}
			}
		}
		else
		{
			FreeDecorFace(dpy, &TB_STATE(*tb)[bs_start]);
			memcpy(
				&(TB_STATE(*tb)[bs_start]), &tmpdf,
				sizeof(DecorFace));
			for (i = bs_start + 1; i <= bs_end; ++i)
			{
				if (multiple && (i & use_mask) != set_mask)
				{
					continue;
				}
				ReadDecorFace(
					spec, &TB_STATE(*tb)[i], button, False);
			}
		}
	}
	if (pstyle)
	{
		free(spec);
		end++;
		end = SkipSpaces(end, NULL, 0);
	}

	return end;
}

/* Remove the given decor from all windows */
static void __remove_window_decors(F_CMD_ARGS, FvwmDecor *d)
{
	const exec_context_t *exc2;
	exec_context_changes_t ecc;
	FvwmWindow *t;

	for (t = Scr.FvwmRoot.next; t; t = t->next)
	{
		if (t->decor == d)
		{
			/* remove the extra title height now because we delete
			 * the current decor before calling ChangeDecor(). */
			t->g.frame.height -= t->decor->title_height;
			t->decor = NULL;
			ecc.w.fw = t;
			ecc.w.wcontext = C_WINDOW;
			exc2 = exc_clone_context(
				exc, &ecc, ECC_FW | ECC_WCONTEXT);
			execute_function(
				cond_rc, exc2, "ChangeDecor Default", 0);
			exc_destroy_context(exc2);
		}
	}

	return;
}

static void do_title_style(F_CMD_ARGS, Bool do_add)
{
	char *parm;
	char *prev;
	FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;

	Scr.flags.do_need_window_update = 1;
	decor->flags.has_changed = 1;
	decor->titlebar.flags.has_changed = 1;

	for (prev = action ; (parm = PeekToken(action, &action)); prev = action)
	{
		if (!do_add && StrEquals(parm,"centered"))
		{
			TB_JUSTIFICATION(decor->titlebar) = JUST_CENTER;
		}
		else if (!do_add && StrEquals(parm,"leftjustified"))
		{
			TB_JUSTIFICATION(decor->titlebar) = JUST_LEFT;
		}
		else if (!do_add && StrEquals(parm,"rightjustified"))
		{
			TB_JUSTIFICATION(decor->titlebar) = JUST_RIGHT;
		}
		else if (!do_add && StrEquals(parm,"height"))
		{
			int height = 0;
			int next = 0;

			if (!action ||
			    sscanf(action, "%d%n", &height, &next) <= 0 ||
			    height < MIN_FONT_HEIGHT ||
			    height > MAX_FONT_HEIGHT)
			{
				if (height != 0)
				{
					fvwm_msg(ERR, "do_title_style",
						 "bad height argument (height"
						 " must be from 5 to 256)");
					height = 0;
				}
			}
			if (decor->title_height != height ||
			    decor->min_title_height != 0)
			{
				decor->title_height = height;
				decor->min_title_height = 0;
				decor->flags.has_title_height_changed = 1;
			}
			if (action)
				action += next;
		}
		else if (!do_add && StrEquals(parm,"MinHeight"))
		{
			int height = 0;
			int next = 0;

			if (!action ||
			    sscanf(action, "%d%n", &height, &next) <= 0 ||
			    height < MIN_FONT_HEIGHT ||
			    height > MAX_FONT_HEIGHT)
			{
				if (height < MIN_FONT_HEIGHT)
					height = MIN_FONT_HEIGHT;
				else if (height > MAX_FONT_HEIGHT)
					height = 0;
			}
			if (decor->min_title_height != height)
			{
				decor->title_height = 0;
				decor->min_title_height = height;
				decor->flags.has_title_height_changed = 1;
			}
			if (action)
				action += next;
		}
		else
		{
			action = ReadTitleButton(
				prev, &decor->titlebar, do_add, -1);
		}
	}

	return;
}

/*
 *
 * Reads a multi-pixmap titlebar config. (tril@igs.net)
 *
 */
static char *ReadMultiPixmapDecor(char *s, DecorFace *df)
{
	static char *pm_names[TBMP_NUM_PIXMAPS+1] =
		{
			"Main",
			"LeftMain",
			"RightMain",
			"LeftButtons",
			"RightButtons",
			"UnderText",
			"LeftOfText",
			"RightOfText",
			"LeftEnd",
			"RightEnd",
			"Buttons",
			NULL
		};
	FvwmPicture **pm;
	FvwmAcs *acs;
	Pixel *pixels;
	char *token;
	Bool stretched;
	Bool load_pixmap = False;
	int pm_id, i = 0;
	FvwmPictureAttributes fpa;

	df->style.face_type = MultiPixmap;
	df->u.mp.pixmaps = pm = xcalloc(TBMP_NUM_PIXMAPS, sizeof(FvwmPicture*));
	df->u.mp.acs = acs = xmalloc(TBMP_NUM_PIXMAPS * sizeof(FvwmAcs));
	df->u.mp.pixels = pixels = xmalloc(TBMP_NUM_PIXMAPS * sizeof(Pixel));
	for(i=0; i < TBMP_NUM_PIXMAPS; i++)
	{
		acs[i].cs = -1;
		acs[i].alpha_percent = 100;
	}
	s = GetNextTokenIndex(s, pm_names, 0, &pm_id);
	while (pm_id >= 0)
	{
		stretched = False;
		load_pixmap = False;
		s = DoPeekToken(s, &token, ",()", NULL, NULL);
		if (StrEquals(token, "stretched"))
		{
			stretched = True;
			s = DoPeekToken(s, &token, ",", NULL, NULL);
		}
		else if (StrEquals(token, "tiled"))
		{
			s = DoPeekToken(s, &token, ",", NULL, NULL);
		}
		if (!token)
		{
			break;
		}
		if (pm[pm_id] || acs[pm_id].cs >= 0 ||
		    (df->u.mp.solid_flags & (1 << pm_id)))
		{
			fvwm_msg(WARN, "ReadMultiPixmapDecor",
				 "Ignoring: already-specified %s",
				 pm_names[i]);
			continue;
		}
		if (stretched)
		{
			df->u.mp.stretch_flags |= (1 << pm_id);
		}
		if (strncasecmp (token, "Colorset", 8) == 0)
		{
			int val;
			char *tmp;

			tmp = DoPeekToken(s, &token, ",", NULL, NULL);
			if (!GetIntegerArguments(token, NULL, &val, 1) ||
			    val < 0)
			{
				fvwm_msg(
					ERR, "ReadMultiPixmapDecor",
					"Colorset should take one or two "
					"positive integers as argument");
			}
			else
			{
				acs[pm_id].cs = val;
				alloc_colorset(val);
				s = tmp;
				tmp = DoPeekToken(s, &token, ",", NULL, NULL);
				if (GetIntegerArguments(token, NULL, &val, 1))
				{
					acs[pm_id].alpha_percent =
						max(0, min(100,val));
					s = tmp;
				}
			}
		}
		else if (strncasecmp(token, "TiledPixmap", 11) == 0)
		{
			s = DoPeekToken(s, &token, ",", NULL, NULL);
			load_pixmap = True;
		}
		else if (strncasecmp(token, "AdjustedPixmap", 14) == 0)
		{
			s = DoPeekToken(s, &token, ",", NULL, NULL);
			load_pixmap = True;
			df->u.mp.stretch_flags |= (1 << pm_id);
		}
		else if (strncasecmp(token, "Solid", 5) == 0)
		{
			s = DoPeekToken(s, &token, ",", NULL, NULL);
			if (token)
			{
				df->u.mp.pixels[pm_id] = GetColor(token);
				df->u.mp.solid_flags |= (1 << pm_id);
			}
		}
		else
		{
			load_pixmap = True;
		}
		if (load_pixmap && token)
		{
			fpa.mask = (Pdepth <= 8)? FPAM_DITHER:0; /* ? */
			pm[pm_id] = PCacheFvwmPicture(
				dpy, Scr.NoFocusWin, NULL, token, fpa);
			if (!pm[pm_id])
			{
				fvwm_msg(ERR, "ReadMultiPixmapDecor",
					 "Pixmap '%s' could not be loaded",
					 token);
			}
		}
		if (pm_id == TBMP_BUTTONS)
		{
			if (pm[TBMP_LEFT_BUTTONS])
			{
				PDestroyFvwmPicture(dpy, pm[TBMP_LEFT_BUTTONS]);
			}
			if (pm[TBMP_RIGHT_BUTTONS])
			{
				PDestroyFvwmPicture(dpy, pm[TBMP_RIGHT_BUTTONS]);
			}
			df->u.mp.stretch_flags &= ~(1 << TBMP_LEFT_BUTTONS);
			df->u.mp.stretch_flags &= ~(1 << TBMP_RIGHT_BUTTONS);
			df->u.mp.solid_flags &= ~(1 << TBMP_LEFT_BUTTONS);
			df->u.mp.solid_flags &= ~(1 << TBMP_RIGHT_BUTTONS);
			if (pm[TBMP_BUTTONS])
			{
				pm[TBMP_LEFT_BUTTONS] =
					PCloneFvwmPicture(pm[TBMP_BUTTONS]);
				acs[TBMP_LEFT_BUTTONS].cs = -1;
				pm[TBMP_RIGHT_BUTTONS] =
					PCloneFvwmPicture(pm[TBMP_BUTTONS]);
				acs[TBMP_RIGHT_BUTTONS].cs = -1;
			}
			else
			{
				pm[TBMP_RIGHT_BUTTONS] =
					pm[TBMP_LEFT_BUTTONS] = NULL;
				acs[TBMP_RIGHT_BUTTONS].cs =
					acs[TBMP_LEFT_BUTTONS].cs =
					acs[TBMP_BUTTONS].cs;
				acs[TBMP_RIGHT_BUTTONS].alpha_percent =
					acs[TBMP_LEFT_BUTTONS].alpha_percent =
					acs[TBMP_BUTTONS].alpha_percent;
				pixels[TBMP_LEFT_BUTTONS] =
					pixels[TBMP_RIGHT_BUTTONS] =
					pixels[TBMP_BUTTONS];
			}
			if (stretched)
			{
				df->u.mp.stretch_flags |=
					(1 << TBMP_LEFT_BUTTONS) |
					(1 << TBMP_RIGHT_BUTTONS);
			}
			if (df->u.mp.solid_flags & (1 << TBMP_BUTTONS))
			{
				df->u.mp.solid_flags |=
					(1 << TBMP_LEFT_BUTTONS);
				df->u.mp.solid_flags |=
					(1 << TBMP_RIGHT_BUTTONS);
			}
			if (pm[TBMP_BUTTONS])
			{
				PDestroyFvwmPicture(dpy, pm[TBMP_BUTTONS]);
				pm[TBMP_BUTTONS] = NULL;
			}
			acs[TBMP_BUTTONS].cs = -1;
			df->u.mp.solid_flags &= ~(1 << TBMP_BUTTONS);
		}
		s = SkipSpaces(s, NULL, 0);
		s = GetNextTokenIndex(s, pm_names, 0, &pm_id);
	}

	if (!(pm[TBMP_MAIN] || acs[TBMP_MAIN].cs >= 0 ||
	      (df->u.mp.solid_flags & TBMP_MAIN))
	    &&
	    !(pm[TBMP_LEFT_MAIN] || acs[TBMP_LEFT_MAIN].cs >= 0 ||
	      (df->u.mp.solid_flags & TBMP_LEFT_MAIN))
	      &&
	    !(pm[TBMP_RIGHT_MAIN] || acs[TBMP_RIGHT_MAIN].cs >= 0 ||
	      (df->u.mp.solid_flags & TBMP_RIGHT_MAIN)))
	{
		fvwm_msg(ERR, "ReadMultiPixmapDecor",
			 "No Main pixmap/colorset/solid found for TitleStyle "
			 "MultiPixmap  (you must specify either Main, "
			 "or both LeftMain and RightMain)");
		for (i=0; i < TBMP_NUM_PIXMAPS; i++)
		{
			if (pm[i])
			{
				PDestroyFvwmPicture(dpy, pm[i]);
			}
			else if (!!(df->u.mp.solid_flags & i))
			{
				PictureFreeColors(
					dpy, Pcmap, &df->u.mp.pixels[i], 1, 0,
					False);
			}
		}
		free(pm);
		free(acs);
		free(pixels);
		return NULL;
	}

	return s;
}

/*
 *
 *  DestroyFvwmDecor -- frees all memory assocated with an FvwmDecor
 *      structure, but does not free the FvwmDecor itself
 *
 */
static void DestroyFvwmDecor(FvwmDecor *decor)
{
	int i;
	/* reset to default button set (frees allocated mem) */
	DestroyAllButtons(decor);
	for (i = 0; i < BS_MaxButtonState; ++i)
	{
		FreeDecorFace(dpy, &TB_STATE(decor->titlebar)[i]);
	}
	FreeDecorFace(dpy, &decor->BorderStyle.active);
	FreeDecorFace(dpy, &decor->BorderStyle.inactive);
	if (decor->tag)
	{
		free(decor->tag);
		decor->tag = NULL;
	}

	return;
}

static void SetLayerButtonFlag(
	int layer, int multi, int set, FvwmDecor *decor, TitleButton *tb)
{
	int i;
	int start = 0;
	int add = 2;

	if (multi)
	{
		if (multi == 2)
		{
			start = 1;
		}
		else if (multi == 3)
		{
			add = 1;
		}
		for (i = start; i < NUMBER_OF_TITLE_BUTTONS; i += add)
		{
			if (set)
			{
				TB_FLAGS(decor->buttons[i]).has_layer = 1;
				TB_LAYER(decor->buttons[i]) = layer;
			}
			else
			{
				TB_FLAGS(decor->buttons[i]).has_layer = 0;
			}
		}
	}
	else
	{
		if (set)
		{
			TB_FLAGS(*tb).has_layer = 1;
			TB_LAYER(*tb) = layer;
		}
		else
		{
			TB_FLAGS(*tb).has_layer = 0;
		}
	}

	return;
}

/*
 *
 * Changes a button decoration style (changes by veliaa@rpi.edu)
 *
 */
static void SetMWMButtonFlag(
	mwm_flags flag, int multi, int set, FvwmDecor *decor, TitleButton *tb)
{
	int i;
	int start = 0;
	int add = 2;

	if (multi)
	{
		if (multi == 2)
		{
			start = 1;
		}
		else if (multi == 3)
		{
			add = 1;
		}
		for (i = start; i < NUMBER_OF_TITLE_BUTTONS; i += add)
		{
			if (set)
			{
				TB_MWM_DECOR_FLAGS(decor->buttons[i]) |= flag;
			}
			else
			{
				TB_MWM_DECOR_FLAGS(decor->buttons[i]) &= ~flag;
			}
		}
	}
	else
	{
		if (set)
		{
			TB_MWM_DECOR_FLAGS(*tb) |= flag;
		}
		else
		{
			TB_MWM_DECOR_FLAGS(*tb) &= ~flag;
		}
	}

	return;
}

static void do_button_style(F_CMD_ARGS, Bool do_add)
{
	int i;
	int multi = 0;
	int button = 0;
	int do_return;
	char *text = NULL;
	char *prev = NULL;
	char *parm = NULL;
	TitleButton *tb = NULL;
	FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;

	parm = PeekToken(action, &text);
	if (parm && isdigit(*parm))
	{
		button = atoi(parm);
		button = BUTTON_INDEX(button);
	}

	if (parm == NULL || button >= NUMBER_OF_TITLE_BUTTONS || button < 0)
	{
		fvwm_msg(
			ERR, "ButtonStyle", "Bad button style (1) in line %s",
			action);
		return;
	}

	Scr.flags.do_need_window_update = 1;

	do_return = 0;
	if (!isdigit(*parm))
	{
		if (StrEquals(parm,"left"))
		{
			multi = 1; /* affect all left buttons */
		}
		else if (StrEquals(parm,"right"))
		{
			multi = 2; /* affect all right buttons */
		}
		else if (StrEquals(parm,"all"))
		{
			multi = 3; /* affect all buttons */
		}
		else
		{
			/* we're either resetting buttons or an invalid button
			 * set was specified */
			if (StrEquals(parm,"reset"))
			{
				ResetAllButtons(decor);
			}
			else
			{
				fvwm_msg(
					ERR, "ButtonStyle",
					"Bad button style (2) in line %s",
					action);
			}
			multi = 3;
			do_return = 1;
		}
	}
	/* mark button style and decor as changed */
	decor->flags.has_changed = 1;
	if (multi == 0)
	{
		/* a single button was specified */
		tb = &decor->buttons[button];
		TB_FLAGS(*tb).has_changed = 1;
	}
	else
	{
		for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
		{
			if (((multi & 1) && !(i & 1)) ||
			    ((multi & 2) && (i & 1)))
			{
				TB_FLAGS(decor->buttons[i]).has_changed = 1;
			}
		}
	}
	if (do_return == 1)
	{
		return;
	}
	for (prev = text; (parm = PeekToken(text, &text)); prev = text)
	{
		if (!do_add && strcmp(parm,"-") == 0)
		{
			char *tok;
			text = GetNextToken(text, &tok);
			while (tok)
			{
				int set = 1;
				char *old_tok = NULL;

				if (*tok == '!')
				{
					/* flag negate */
					set = 0;
					old_tok = tok;
					tok++;
				}
				if (StrEquals(tok,"Clear"))
				{
					if (multi)
					{
						for (i = 0;
						     i < NUMBER_OF_TITLE_BUTTONS; ++i)
						{
							if (((multi & 1) &&
							     !(i & 1)) ||
							    ((multi & 2) &&
							     (i & 1)))
							{
								TB_JUSTIFICATION(decor->buttons[i]) =
									(set) ? JUST_CENTER : JUST_RIGHT;
								memset(&TB_FLAGS(decor->buttons[i]), (set) ? 0 : 0xff,
								       sizeof(TB_FLAGS(decor->buttons[i])));
								/* ? not very useful if set == 0 ? */
							}
						}
					}
					else
					{
						TB_JUSTIFICATION(*tb) = (set) ?
							JUST_CENTER :
							JUST_RIGHT;
						memset(&TB_FLAGS(*tb), (set) ?
						       0 : 0xff,
						       sizeof(TB_FLAGS(*tb)));
						/* ? not very useful if
						 * set == 0 ? */
					}
				}
				else if (StrEquals(tok, "MWMDecorMenu"))
				{
					SetMWMButtonFlag(
						MWM_DECOR_MENU, multi, set,
						decor, tb);
				}
				else if (StrEquals(tok, "MWMDecorMin"))
				{
					SetMWMButtonFlag(
						MWM_DECOR_MINIMIZE, multi, set,
						decor, tb);
				}
				else if (StrEquals(tok, "MWMDecorMax"))
				{
					SetMWMButtonFlag(
						MWM_DECOR_MAXIMIZE, multi, set,
						decor, tb);
				}
				else if (StrEquals(tok, "MWMDecorShade"))
				{
					SetMWMButtonFlag(
						MWM_DECOR_SHADE, multi, set,
						decor, tb);
				}
				else if (StrEquals(tok, "MWMDecorStick"))
				{
					SetMWMButtonFlag(
						MWM_DECOR_STICK, multi, set,
						decor, tb);
				}
				else if (StrEquals(tok, "MwmDecorLayer"))
				{
					int layer, got_number;
					char *ltok;
					text = GetNextToken(text, &ltok);
					if (ltok)
					{
						got_number =
							(sscanf(ltok, "%d",
								&layer) == 1);
						free (ltok);
					}
					else
					{
						got_number = 0;
					}
					if (!ltok || !got_number)
					{
						fvwm_msg(ERR, "ButtonStyle",
							 "could not read"
							 " integer value for"
							 " layer -- line: %s",
							 text);
					}
					else
					{
						SetLayerButtonFlag(
							layer, multi, set,
							decor, tb);
					}

				}
				else
				{
					fvwm_msg(ERR, "ButtonStyle",
						 "unknown title button flag"
						 " %s -- line: %s", tok, text);
				}
				if (set)
				{
					free(tok);
				}
				else
				{
					free(old_tok);
				}
				text = GetNextToken(text, &tok);
			}
			break;
		}
		else
		{
			if (multi)
			{
				for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
				{
					if (((multi & 1) && !(i & 1)) ||
					    ((multi & 2) && (i & 1)))
					{
						text = ReadTitleButton(
							prev,
							&decor->buttons[i],
							do_add, i);
					}
				}
			}
			else if (!(text = ReadTitleButton(
					   prev, tb, do_add, button)))
			{
				break;
			}
		}
	}

	return;
}

static
int update_decorface_colorset(DecorFace *df, int cset)
{
	DecorFace *tdf;
	int has_changed = 0;

	for(tdf = df; tdf != NULL; tdf = tdf->next)
	{
		if (DFS_FACE_TYPE(tdf->style) == ColorsetButton &&
		    tdf->u.acs.cs == cset)
		{
			tdf->flags.has_changed = 1;
			has_changed = 1;
		}
		else if (DFS_FACE_TYPE(tdf->style) == MultiPixmap)
		{
			int i;

			for (i = 0; i < TBMP_NUM_PIXMAPS; i++)
			{
				if (tdf->u.mp.acs[i].cs == cset)
				{
					tdf->flags.has_changed = 1;
					has_changed = 1;
				}
			}
		}
}

	return has_changed;
}

static
int update_titlebutton_colorset(TitleButton *tb, int cset)
{
	int i;
	int has_changed = 0;

	for(i = 0; i < BS_MaxButtonState; i++)
	{
		tb->state[i].flags.has_changed =
			update_decorface_colorset(&(tb->state[i]), cset);
		has_changed |= tb->state[i].flags.has_changed;
	}
	return has_changed;
}

static
void update_decors_colorset(int cset)
{
	int i;
	FvwmDecor *decor = &Scr.DefaultDecor;

	for(decor = &Scr.DefaultDecor; decor != NULL; decor = decor->next)
	{
		for(i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
		{
			decor->flags.has_changed |= update_titlebutton_colorset(
				&(decor->buttons[i]), cset);
		}
		decor->flags.has_changed |= update_titlebutton_colorset(
			&(decor->titlebar), cset);
		decor->flags.has_changed |= update_decorface_colorset(
			&(decor->BorderStyle.active), cset);
		decor->flags.has_changed |= update_decorface_colorset(
			&(decor->BorderStyle.inactive), cset);
		if (decor->flags.has_changed)
		{
			Scr.flags.do_need_window_update = 1;
		}
	}
}

static Bool __parse_vector_line_one_coord(
	char **ret_action, int *pcoord, int *poff, char *action)
{
	int offset;
	int n;

	*ret_action = action;
	n = sscanf(action, "%d%n", pcoord, &offset);
	if (n < 1)
	{
		return False;
	}
	action += offset;
	/* check for offest */
	if (*action == '+' || *action == '-')
	{
		n = sscanf(action, "%dp%n", poff, &offset);
		if (n < 1)
		{
			return False;
		}
		if (*poff < -128)
		{
			*poff = -128;
		}
		else if (*poff > 127)
		{
			*poff = 127;
		}
		action += offset;
	}
	else
	{
		*poff = 0;
	}
	*ret_action = action;

	return True;
}

static Bool __parse_vector_line(
	char **ret_action, int *px, int *py, int *pxoff, int *pyoff, int *pc,
	char *action)
{
	Bool is_valid = True;
	int offset;
	int n;

	*ret_action = action;
	if (__parse_vector_line_one_coord(&action, px, pxoff, action) == False)
	{
		return False;
	}
	if (*action != 'x')
	{
		return False;
	}
	action++;
	if (__parse_vector_line_one_coord(&action, py, pyoff, action) == False)
	{
		return False;
	}
	if (*action != '@')
	{
		return False;
	}
	action++;
	/* read the line style */
	n = sscanf(action, "%d%n", pc, &offset);
	if (n < 1)
	{
		return False;
	}
	action += offset;
	*ret_action = action;

	return is_valid;
}

/* ---------------------------- interface functions ------------------------ */

void refresh_window(Window w, Bool window_update)
{
	XSetWindowAttributes attributes;
	unsigned long valuemask;

	valuemask = CWOverrideRedirect | CWBackingStore | CWSaveUnder |
		CWBackPixmap;
	attributes.override_redirect = True;
	attributes.save_under = False;
	attributes.background_pixmap = None;
	attributes.backing_store = NotUseful;
	w = XCreateWindow(
		dpy, w, 0, 0, Scr.MyDisplayWidth, Scr.MyDisplayHeight, 0,
		CopyFromParent, CopyFromParent, CopyFromParent, valuemask,
		&attributes);
	XMapWindow(dpy, w);
	if (Scr.flags.do_need_window_update && window_update)
	{
		flush_window_updates();
	}
	XDestroyWindow(dpy, w);
	XSync(dpy, 0);
	handle_all_expose();

	return;
}

void ApplyDefaultFontAndColors(void)
{
	XGCValues gcv;
	unsigned long gcm;
	int cset = Scr.DefaultColorset;

	/* make GC's */
	gcm = GCFunction|GCLineWidth|GCForeground|GCBackground;
	gcv.function = GXcopy;
	if (Scr.DefaultFont->font)
	{
		gcm |= GCFont;
		gcv.font = Scr.DefaultFont->font->fid;
	}
	gcv.line_width = 0;
	if (cset >= 0)
	{
		gcv.foreground = Colorset[cset].fg;
		gcv.background = Colorset[cset].bg;
	}
	else
	{
		gcv.foreground = Scr.StdFore;
		gcv.background = Scr.StdBack;
	}
	if (Scr.StdGC)
	{
		XChangeGC(dpy, Scr.StdGC, gcm, &gcv);
	}
	else
	{
		Scr.StdGC = fvwmlib_XCreateGC(dpy, Scr.NoFocusWin, gcm, &gcv);
	}

	gcm = GCFunction|GCLineWidth|GCForeground;
	if (cset >= 0)
	{
		gcv.foreground = Colorset[cset].hilite;
	}
	else
	{
		gcv.foreground = Scr.StdHilite;
	}
	if (Scr.StdReliefGC)
	{
		XChangeGC(dpy, Scr.StdReliefGC, gcm, &gcv);
	}
	else
	{
		Scr.StdReliefGC = fvwmlib_XCreateGC(
			dpy, Scr.NoFocusWin, gcm, &gcv);
	}
	if (cset >= 0)
	{
		gcv.foreground = Colorset[cset].shadow;
	}
	else
	{
		gcv.foreground = Scr.StdShadow;
	}
	if (Scr.StdShadowGC)
	{
		XChangeGC(dpy, Scr.StdShadowGC, gcm, &gcv);
	}
	else
	{
		Scr.StdShadowGC = fvwmlib_XCreateGC(
			dpy, Scr.NoFocusWin, gcm, &gcv);
	}
	/* update the geometry window for move/resize */
	if (Scr.SizeWindow != None)
	{
		resize_geometry_window();
	}
	UpdateAllMenuStyles();

	return;
}

void FreeDecorFace(Display *dpy, DecorFace *df)
{
	int i;

	switch (DFS_FACE_TYPE(df->style))
	{
	case GradientButton:
		if (df->u.grad.d_pixels != NULL && df->u.grad.d_npixels)
		{
			PictureFreeColors(
				dpy, Pcmap, df->u.grad.d_pixels,
				df->u.grad.d_npixels, 0, False);
			free(df->u.grad.d_pixels);
		}
		else if (Pdepth <= 8 && df->u.grad.xcs != NULL &&
			 df->u.grad.npixels > 0 && !df->u.grad.do_dither)
		{
			Pixel *p;
			int i;

			p = xmalloc(df->u.grad.npixels * sizeof(Pixel));
			for(i=0; i < df->u.grad.npixels; i++)
			{
				p[i] = df->u.grad.xcs[i].pixel;
			}
			PictureFreeColors(
				dpy, Pcmap, p, df->u.grad.npixels, 0, False);
			free(p);
		}
		if (df->u.grad.xcs != NULL)
		{
			free(df->u.grad.xcs);
		}
		break;

	case PixmapButton:
	case TiledPixmapButton:
	case StretchedPixmapButton:
	case AdjustedPixmapButton:
	case ShrunkPixmapButton:
		if (df->u.p)
		{
			PDestroyFvwmPicture(dpy, df->u.p);
		}
		break;

	case MultiPixmap:
		if (df->u.mp.pixmaps)
		{
			for (i = 0; i < TBMP_NUM_PIXMAPS; i++)
			{
				if (df->u.mp.pixmaps[i])
				{
					PDestroyFvwmPicture(
						dpy, df->u.mp.pixmaps[i]);
				}
				else if (!!(df->u.mp.solid_flags & i))
				{
					PictureFreeColors(
						dpy, Pcmap, &df->u.mp.pixels[i],
						1, 0, False);
				}
			}
			free(df->u.mp.pixmaps);
		}
		if (df->u.mp.acs)
		{
			free(df->u.mp.acs);
		}
		if (df->u.mp.pixels)
		{
			free(df->u.mp.pixels);
		}
		break;
	case VectorButton:
	case DefaultVectorButton:
		if (df->u.vector.x)
		{
			free (df->u.vector.x);
		}
		if (df->u.vector.y)
		{
			free (df->u.vector.y);
		}
		/* free offsets for coord */
		if (df->u.vector.xoff)
		{
			free(df->u.vector.xoff);
		}
		if (df->u.vector.yoff)
		{
			free(df->u.vector.yoff);
		}
		if (df->u.vector.c)
		{
			free (df->u.vector.c);
		}
		break;
	default:
		/* see below */
		break;
	}
	/* delete any compound styles */
	if (df->next)
	{
		FreeDecorFace(dpy, df->next);
		free(df->next);
	}
	df->next = NULL;
	memset(&df->style, 0, sizeof(df->style));
	memset(&df->u, 0, sizeof(df->u));
	DFS_FACE_TYPE(df->style) = SimpleButton;

	return;
}

/*
 *
 * Reads a button face line into a structure (veliaa@rpi.edu)
 *
 */
Bool ReadDecorFace(char *s, DecorFace *df, int button, int verbose)
{
	int offset;
	char style[256], *file;
	char *action = s;

	/* some variants of scanf do not increase the assign count when %n is
	 * used, so a return value of 1 is no error. */
	if (sscanf(s, "%255s%n", style, &offset) < 1)
	{
		if (verbose)
		{
			fvwm_msg(ERR, "ReadDecorFace", "error in face `%s'", s);
		}
		return False;
	}
	style[255] = 0;

	if (strncasecmp(style, "--", 2) != 0)
	{
		s += offset;

		FreeDecorFace(dpy, df);

		/* determine button style */
		if (strncasecmp(style,"Simple",6)==0)
		{
			memset(&df->style, 0, sizeof(df->style));
			DFS_FACE_TYPE(df->style) = SimpleButton;
		}
		else if (strncasecmp(style,"Default",7)==0)
		{
			int b = -1, n = sscanf(s, "%d%n", &b, &offset);

			if (n < 1)
			{
				if (button == -1)
				{
					if (verbose)
					{
						fvwm_msg(
							ERR,"ReadDecorFace",
							"need default button"
							" number to load");
					}
					return False;
				}
				b = button;
			}
			else
			{
				b = BUTTON_INDEX(b);
				s += offset;
			}
			if (b >= 0 && b < NUMBER_OF_TITLE_BUTTONS)
			{
				LoadDefaultButton(df, b);
			}
			else
			{
				if (verbose)
				{
					fvwm_msg(
						ERR, "ReadDecorFace",
						"button number out of range:"
						" %d", b);
				}
				return False;
			}
		}
		else if (strncasecmp(style,"Vector",6)==0 ||
			 (strlen(style)<=2 && isdigit(*style)))
		{
			/* normal coordinate list button style */
			int i, num_coords, num;
			struct vector_coords *vc = &df->u.vector;

			/* get number of points */
			if (strncasecmp(style,"Vector",6)==0)
			{
				num = sscanf(s,"%d%n",&num_coords,&offset);
				s += offset;
			}
			else
			{
				num = sscanf(style,"%d",&num_coords);
			}

			if (num < 1 || num_coords<2 ||
			    num_coords > MAX_TITLE_BUTTON_VECTOR_LINES)
			{
				if (verbose)
				{
					fvwm_msg(
						ERR, "ReadDecorFace",
						"Bad button style (2) in line:"
						" %s",action);
				}
				return False;
			}

			vc->num = num_coords;
			vc->use_fgbg = 0;
			vc->x = xmalloc(sizeof(char) * num_coords);
			vc->y = xmalloc(sizeof(char) * num_coords);
			vc->xoff = xmalloc(sizeof(char) * num_coords);
			vc->yoff = xmalloc(sizeof(char) * num_coords);
			vc->c = xmalloc(sizeof(char) * num_coords);

			/* get the points */
			for (i = 0; i < vc->num; ++i)
			{
				int x;
				int y;
				int xoff = 0;
				int yoff = 0;
				int c;

				if (__parse_vector_line(
					    &s, &x, &y, &xoff, &yoff, &c, s) ==
				    False)
				{
					break;
				}
				if (x < 0)
				{
					x = 0;
				}
				if (x > 100)
				{
					x = 100;
				}
				if (y < 0)
				{
					y = 0;
				}
				if (y > 100)
				{
					y = 100;
				}
				if (c < 0 || c > 4)
				{
					c = 4;
				}
				vc->x[i] = x;
				vc->y[i] = y;
				vc->c[i] = c;
				vc->xoff[i] = xoff;
				vc->yoff[i] = yoff;
				if (c == 2 || c == 3)
				{
					vc->use_fgbg = 1;
				}
			}
			if (i < vc->num)
			{
				if (verbose)
				{
					fvwm_msg(
						ERR, "ReadDecorFace",
						"Bad button style (3) in line"
						" %s", action);
				}
				free(vc->x);
				free(vc->y);
				free(vc->c);
				free(vc->xoff);
				free(vc->yoff);
				vc->x = NULL;
				vc->y = NULL;
				vc->c = NULL;
				vc->xoff = NULL;
				vc->yoff = NULL;
				return False;
			}
			memset(&df->style, 0, sizeof(df->style));
			DFS_FACE_TYPE(df->style) = VectorButton;
		}
		else if (strncasecmp(style,"Solid",5)==0)
		{
			s = GetNextToken(s, &file);
			if (file)
			{
				memset(&df->style, 0, sizeof(df->style));
				DFS_FACE_TYPE(df->style) = SolidButton;
				df->u.back = GetColor(file);
				free(file);
			}
			else
			{
				if (verbose)
				{
					fvwm_msg(
						ERR, "ReadDecorFace",
						"no color given for Solid"
						" face type: %s", action);
				}
				return False;
			}
		}
		else if (strncasecmp(style+1, "Gradient", 8)==0)
		{
			char **s_colors;
			int npixels, nsegs, *perc;
			XColor *xcs;
			Bool do_dither = False;

			if (!IsGradientTypeSupported(style[0]))
			{
				return False;
			}
			/* translate the gradient string into an array of
			 * colors etc */
			npixels = ParseGradient(
				s, &s, &s_colors, &perc, &nsegs);
			while (*s && isspace(*s))
			{
				s++;
			}
			if (npixels <= 0)
			{
				return False;
			}
			/* grab the colors */
			if (Pdepth <= 16)
			{
				do_dither = True;
			}
			xcs = AllocAllGradientColors(
				s_colors, perc, nsegs, npixels, do_dither);
			if (xcs == None)
				return False;
			df->u.grad.xcs = xcs;
			df->u.grad.npixels = npixels;
			df->u.grad.do_dither = do_dither;
			df->u.grad.d_pixels = NULL;
			memset(&df->style, 0, sizeof(df->style));
			DFS_FACE_TYPE(df->style) = GradientButton;
			df->u.grad.gradient_type = toupper(style[0]);
		}
		else if (strncasecmp(style,"Pixmap",6)==0
			 || strncasecmp(style,"TiledPixmap",11)==0
			 || strncasecmp(style,"StretchedPixmap",15)==0
			 || strncasecmp(style,"AdjustedPixmap",14)==0
			 || strncasecmp(style,"ShrunkPixmap",12)==0)
		{
			FvwmPictureAttributes fpa;

			s = GetNextToken(s, &file);
			fpa.mask = (Pdepth <= 8)? FPAM_DITHER:0; /* ? */
			df->u.p = PCacheFvwmPicture(
				dpy, Scr.NoFocusWin, NULL, file, fpa);
			if (df->u.p == NULL)
			{
				if (file)
				{
					if (verbose)
					{
						fvwm_msg(
							ERR, "ReadDecorFace",
							"couldn't load pixmap"
							" %s", file);
					}
					free(file);
				}
				return False;
			}
			if (file)
			{
				free(file);
				file = NULL;
			}

			memset(&df->style, 0, sizeof(df->style));
			if (strncasecmp(style,"Tiled",5)==0)
			{
				DFS_FACE_TYPE(df->style) = TiledPixmapButton;
			}
			else if (strncasecmp(style,"Stretched",9)==0)
			{
				DFS_FACE_TYPE(df->style) =
					StretchedPixmapButton;
			}
			else if (strncasecmp(style,"Adjusted",8)==0)
			{
				DFS_FACE_TYPE(df->style) =
					AdjustedPixmapButton;
			}
			else if (strncasecmp(style,"Shrunk",6)==0)
			{
				DFS_FACE_TYPE(df->style) =
					ShrunkPixmapButton;
			}
			else
			{
				DFS_FACE_TYPE(df->style) = PixmapButton;
			}
		}
		else if (strncasecmp(style,"MultiPixmap",11)==0)
		{
			if (button != -1)
			{
				if (verbose)
				{
					fvwm_msg(
						ERR, "ReadDecorFace",
						"MultiPixmap is only valid"
						" for TitleStyle");
				}
				return False;
			}
			s = ReadMultiPixmapDecor(s, df);
			if (!s)
			{
				return False;
			}
		}
		else if (FMiniIconsSupported &&
			 strncasecmp (style, "MiniIcon", 8) == 0)
		{
			memset(&df->style, 0, sizeof(df->style));
			DFS_FACE_TYPE(df->style) = MiniIconButton;
			/* pixmap read in when the window is created */
			df->u.p = NULL;
		}
		else if (strncasecmp (style, "Colorset", 8) == 0)
		{
			int val[2];
			int n;

			n = GetIntegerArguments(s, NULL, val, 2);
			if (n == 0)
			{
			}
			memset(&df->style, 0, sizeof(df->style));
			if (n > 0 && val[0] >= 0)
			{

				df->u.acs.cs = val[0];
				alloc_colorset(val[0]);
				DFS_FACE_TYPE(df->style) = ColorsetButton;
			}
			df->u.acs.alpha_percent = 100;
			if (n > 1)
			{
				df->u.acs.alpha_percent =
					max(0, min(100,val[1]));
			}
			s = SkipNTokens(s, n);
		}
		else
		{
			if (verbose)
			{
				fvwm_msg(
					ERR, "ReadDecorFace",
					"unknown style %s: %s", style, action);
			}
			return False;
		}
	}

	/* Process button flags ("--" signals start of flags,
	   it is also checked for above) */
	s = GetNextToken(s, &file);
	if (file && (strcmp(file,"--")==0))
	{
		char *tok;
		s = GetNextToken(s, &tok);
		while (tok && *tok)
		{
			int set = 1;
			char *old_tok = NULL;

			if (*tok == '!')
			{ /* flag negate */
				set = 0;
				old_tok = tok;
				tok++;
			}
			if (StrEquals(tok,"Clear"))
			{
				memset(&DFS_FLAGS(df->style), (set) ? 0 : 0xff,
				       sizeof(DFS_FLAGS(df->style)));
				/* ? what is set == 0 good for ? */
			}
			else if (StrEquals(tok,"Left"))
			{
				if (set)
				{
					DFS_H_JUSTIFICATION(df->style) =
						JUST_LEFT;
				}
				else
				{
					DFS_H_JUSTIFICATION(df->style) =
						JUST_RIGHT;
				}
			}
			else if (StrEquals(tok,"Right"))
			{
				if (set)
				{
					DFS_H_JUSTIFICATION(df->style) =
						JUST_RIGHT;
				}
				else
				{
					DFS_H_JUSTIFICATION(df->style) =
						JUST_LEFT;
				}
			}
			else if (StrEquals(tok,"Centered"))
			{
				DFS_H_JUSTIFICATION(df->style) = JUST_CENTER;
				DFS_V_JUSTIFICATION(df->style) = JUST_CENTER;
			}
			else if (StrEquals(tok,"Top"))
			{
				if (set)
				{
					DFS_V_JUSTIFICATION(df->style) =
						JUST_TOP;
				}
				else
				{
					DFS_V_JUSTIFICATION(df->style) =
						JUST_BOTTOM;
				}
			}
			else if (StrEquals(tok,"Bottom"))
			{
				if (set)
				{
					DFS_V_JUSTIFICATION(df->style) =
						JUST_BOTTOM;
				}
				else
				{
					DFS_V_JUSTIFICATION(df->style) =
						JUST_TOP;
				}
			}
			else if (StrEquals(tok,"Flat"))
			{
				if (set)
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_FLAT;
				}
				else if (DFS_BUTTON_RELIEF(df->style) ==
					 DFS_BUTTON_IS_FLAT)
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_UP;
				}
			}
			else if (StrEquals(tok,"Sunk"))
			{
				if (set)
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_SUNK;
				}
				else if (DFS_BUTTON_RELIEF(df->style) ==
					 DFS_BUTTON_IS_SUNK)
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_UP;
				}
			}
			else if (StrEquals(tok,"Raised"))
			{
				if (set)
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_UP;
				}
				else
				{
					DFS_BUTTON_RELIEF(df->style) =
						DFS_BUTTON_IS_SUNK;
				}
			}
			else if (StrEquals(tok,"UseTitleStyle"))
			{
				if (set)
				{
					DFS_USE_TITLE_STYLE(df->style) = 1;
					DFS_USE_BORDER_STYLE(df->style) = 0;
				}
				else
					DFS_USE_TITLE_STYLE(df->style) = 0;
			}
			else if (StrEquals(tok,"HiddenHandles"))
			{
				DFS_HAS_HIDDEN_HANDLES(df->style) = !!set;
			}
			else if (StrEquals(tok,"NoInset"))
			{
				DFS_HAS_NO_INSET(df->style) = !!set;
			}
			else if (StrEquals(tok,"UseBorderStyle"))
			{
				if (set)
				{
					DFS_USE_BORDER_STYLE(df->style) = 1;
					DFS_USE_TITLE_STYLE(df->style) = 0;
				}
				else
				{
					DFS_USE_BORDER_STYLE(df->style) = 0;
				}
			}
			else if (verbose)
			{
				fvwm_msg(
					ERR, "ReadDecorFace",
					"unknown button face flag '%s'"
					" -- line: %s", tok, action);
			}
			if (set)
			{
				free(tok);
			}
			else
			{
				free(old_tok);
			}
			s = GetNextToken(s, &tok);
		}
	}
	if (file)
	{
		free(file);
	}

	return True;
}

/*
 *
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 *
 */
void AddToDecor(F_CMD_ARGS, FvwmDecor *decor)
{
	if (!action)
	{
		return;
	}
	while (*action && isspace((unsigned char)*action))
	{
		++action;
	}
	if (!*action)
	{
		return;
	}
	Scr.cur_decor = decor;
	execute_function(cond_rc, exc, action, 0);
	Scr.cur_decor = NULL;

	return;
}

/*
 *
 *  InitFvwmDecor -- initializes an FvwmDecor structure to defaults
 *
 */
void InitFvwmDecor(FvwmDecor *decor)
{
	int i;
	DecorFace tmpdf;

	/* zero out the structures */
	memset(decor, 0, sizeof (FvwmDecor));
	memset(&tmpdf, 0, sizeof(DecorFace));

	/* initialize title-bar button styles */
	DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
	{
		int j = 0;
		for (; j < BS_MaxButtonState; ++j)
		{
			TB_STATE(decor->buttons[i])[j] = tmpdf;
		}
	}
	/* reset to default button set */
	ResetAllButtons(decor);
	/* initialize title-bar styles */
	for (i = 0; i < BS_MaxButtonState; ++i)
	{
		DFS_FACE_TYPE(
			TB_STATE(decor->titlebar)[i].style) = SimpleButton;
	}

	/* initialize border texture styles */
	DFS_FACE_TYPE(decor->BorderStyle.active.style) = SimpleButton;
	DFS_FACE_TYPE(decor->BorderStyle.inactive.style) = SimpleButton;

	return;
}

void reset_decor_changes(void)
{
	FvwmDecor *decor;
	for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
	{
		decor->flags.has_changed = 0;
		decor->flags.has_title_height_changed = 0;
	}
	/* todo: must reset individual change flags too */

	return;
}

void update_fvwm_colorset(int cset)
{
	if (cset == Scr.DefaultColorset)
	{
		Scr.flags.do_need_window_update = 1;
		Scr.flags.has_default_color_changed = 1;
	}
	UpdateMenuColorset(cset);
	update_style_colorset(cset);
	update_decors_colorset(cset);

	return;
}

/* ---------------------------- builtin commands --------------------------- */

void CMD_Beep(F_CMD_ARGS)
{
	XBell(dpy, 0);

	return;
}

void CMD_Nop(F_CMD_ARGS)
{
	return;
}

void CMD_EscapeFunc(F_CMD_ARGS)
{
	return;
}

void CMD_CursorMove(F_CMD_ARGS)
{
	int x = 0, y = 0;
	int val1, val2, val1_unit, val2_unit;
	int x_unit, y_unit;
	int virtual_x, virtual_y;
	int x_pages, y_pages;

	if (GetTwoArguments(action, &val1, &val2, &val1_unit, &val2_unit) != 2)
	{
		fvwm_msg(ERR, "movecursor", "CursorMove needs 2 arguments");
		return;
	}
	if (FQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			  &x, &y, &JunkX, &JunkY, &JunkMask) == False)
	{
		/* pointer is on a different screen */
		return;
	}

	x_unit = val1 * val1_unit / 100;
	y_unit = val2 * val2_unit / 100;

	x += x_unit;
	y += y_unit;

	virtual_x = Scr.Vx;
	virtual_y = Scr.Vy;
	if (x >= 0)
	{
		x_pages = x / Scr.MyDisplayWidth;
	}
	else
	{
		x_pages = ((x + 1) / Scr.MyDisplayWidth) - 1;
	}
	virtual_x += x_pages * Scr.MyDisplayWidth;
	x -= x_pages * Scr.MyDisplayWidth;
	if (virtual_x < 0)
	{
		x += virtual_x;
		virtual_x = 0;
	}
	else if (virtual_x > Scr.VxMax)
	{
		x += virtual_x - Scr.VxMax;
		virtual_x = Scr.VxMax;
	}

	if (y >= 0)
	{
		y_pages = y / Scr.MyDisplayHeight;
	}
	else
	{
		y_pages = ((y + 1) / Scr.MyDisplayHeight) - 1;
	}
	virtual_y += y_pages * Scr.MyDisplayHeight;
	y -= y_pages * Scr.MyDisplayHeight;

	if (virtual_y < 0)
	{
		y += virtual_y;
		virtual_y = 0;
	}
	else if (virtual_y > Scr.VyMax)
	{
		y += virtual_y - Scr.VyMax;
		virtual_y = Scr.VyMax;
	}

	/* TA:  (2010/12/19):  Only move to the new page if scrolling is
	 * enabled and the viewport is able to change based on where the
	 * pointer is.
	 */
	if ((virtual_x != Scr.Vx && Scr.EdgeScrollX != 0) ||
	    (virtual_y != Scr.Vy && Scr.EdgeScrollY != 0))
	{
		MoveViewport(virtual_x, virtual_y, True);
	}

	/* TA:  (2010/12/19):  If the cursor is about to enter a pan-window, or
	 * is one, or the cursor's next step is to go beyond the page
	 * boundary, stop the cursor from moving in that direction, *if* we've
	 * disallowed edge scrolling.
	 *
	 * Whilst this stops the cursor short of the edge of the screen in a
	 * given direction, this is the desired behaviour.
	 */
	if (Scr.EdgeScrollX == 0 && (x >= Scr.MyDisplayWidth ||
	    x + x_unit >= Scr.MyDisplayWidth))
		return;

	if (Scr.EdgeScrollY == 0 && (y >= Scr.MyDisplayHeight ||
	    y + y_unit >= Scr.MyDisplayHeight))
		return;

	FWarpPointerUpdateEvpos(
		exc->x.elast, dpy, None, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		Scr.MyDisplayHeight, x, y);

	return;
}

void CMD_Delete(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;

	if (!is_function_allowed(F_DELETE, NULL, fw, RQORIG_PROGRAM_US, True))
	{
		XBell(dpy, 0);

		return;
	}
	if (IS_TEAR_OFF_MENU(fw))
	{
		/* 'soft' delete tear off menus.  Note: we can't send the
		 * message to the menu window directly because it was created
		 * using a different display.  The client message would never
		 * be read from there. */
		send_clientmessage(
			dpy, FW_W_PARENT(fw), _XA_WM_DELETE_WINDOW,
			CurrentTime);

		return;
	}
	if (WM_DELETES_WINDOW(fw))
	{
		send_clientmessage(
			dpy, FW_W(fw), _XA_WM_DELETE_WINDOW, CurrentTime);

		return;
	}
	else
	{
		XBell(dpy, 0);
	}
	XFlush(dpy);

	return;
}

void CMD_Destroy(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;

	if (IS_TEAR_OFF_MENU(fw))
	{
		CMD_Delete(F_PASS_ARGS);
		return;
	}
	if (!is_function_allowed(F_DESTROY, NULL, fw, True, True))
	{
		XBell(dpy, 0);
		return;
	}
	if (
		XGetGeometry(
			dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			(unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
			(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth)
		!= 0)
	{
		XKillClient(dpy, FW_W(fw));
	}
	destroy_window(fw);
	XFlush(dpy);

	return;
}

void CMD_Close(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;

	if (IS_TEAR_OFF_MENU(fw))
	{
		CMD_Delete(F_PASS_ARGS);
		return;
	}
	if (!is_function_allowed(F_CLOSE, NULL, fw, True, True))
	{
		XBell(dpy, 0);
		return;
	}
	if (WM_DELETES_WINDOW(fw))
	{
		send_clientmessage(
			dpy, FW_W(fw), _XA_WM_DELETE_WINDOW, CurrentTime);
		return;
	}
	if (
		XGetGeometry(
			dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			(unsigned int*)&JunkWidth, (unsigned int*)&JunkHeight,
			(unsigned int*)&JunkBW, (unsigned int*)&JunkDepth)
		!= 0)
	{
		XKillClient(dpy, FW_W(fw));
	}

	destroy_window(fw);
	XFlush(dpy);

	return;
}

void CMD_Restart(F_CMD_ARGS)
{
	Done(1, action);

	return;
}

void CMD_ExecUseShell(F_CMD_ARGS)
{
	char *arg=NULL;
	static char shell_set = 0;

	if (shell_set)
	{
		free(exec_shell_name);
	}
	shell_set = 1;
	action = GetNextToken(action,&arg);
	if (arg) /* specific shell was specified */
	{
		exec_shell_name = arg;
	}
	else /* no arg, so use $SHELL -- not working??? */
	{
		if (getenv("SHELL"))
		{
			exec_shell_name = xstrdup(getenv("SHELL"));
		}
		else
		{
			/* if $SHELL not set, use default */
			exec_shell_name = xstrdup("/bin/sh");
		}
	}
}

void CMD_Exec(F_CMD_ARGS)
{
	char *cmd=NULL;

	/* if it doesn't already have an 'exec' as the first word, add that
	 * to keep down number of procs started */
	/* need to parse string better to do this right though, so not doing
	 * this for now... */
#if 0
	if (strncasecmp(action,"exec",4)!=0)
	{
		cmd = (char *)safemalloc(strlen(action)+6);
		strcpy(cmd,"exec ");
		strcat(cmd,action);
	}
	else
#endif
	{
		cmd = xstrdup(action);
	}
	if (!cmd)
	{
		return;
	}
	/* Use to grab the pointer here, but the fork guarantees that
	 * we wont be held up waiting for the function to finish,
	 * so the pointer-gram just caused needless delay and flashing
	 * on the screen */
	/* Thought I'd try vfork and _exit() instead of regular fork().
	 * The man page says that its better. */
	/* Not everyone has vfork! */
	/* According to the man page, vfork should never be used at all.
	 */

	if (!(fork())) /* child process */
	{
		/* This is for fixing a problem with rox filer */
		int fd;

		fvmm_deinstall_signals();
		fd = open("/dev/null", O_RDONLY, 0);
		dup2(fd,STDIN_FILENO);

		if (fd != STDIN_FILENO)
			close(fd);

		if (fvwm_setpgrp() == -1)
		{
			fvwm_msg(ERR, "exec_function", "setpgrp failed (%s)",
				 strerror(errno));
			exit(100);
		}
		if (execl(exec_shell_name, exec_shell_name, "-c", cmd, NULL) ==
		    -1)
		{
			fvwm_msg(ERR, "exec_function", "execl failed (%s)",
				 strerror(errno));
			exit(100);
		}
	}
	free(cmd);

	return;
}

void CMD_Refresh(F_CMD_ARGS)
{
	refresh_window(Scr.Root, True);

	return;
}

void CMD_RefreshWindow(F_CMD_ARGS)
{
	FvwmWindow * const fw = exc->w.fw;

	refresh_window(
		(exc->w.wcontext == C_ICON) ?
		FW_W_ICON_TITLE(fw) : FW_W_FRAME(fw), True);

	return;
}

void CMD_Wait(F_CMD_ARGS)
{
	Bool done = False;
	Bool redefine_cursor = False;
	Bool is_ungrabbed;
	char *escape;
	Window nonewin = None;
	char *wait_string, *rest;
	FvwmWindow *t;

	/* try to get a single token */
	rest = GetNextToken(action, &wait_string);
	if (wait_string)
	{
		while (*rest && isspace((unsigned char)*rest))
		{
			rest++;
		}
		if (*rest)
		{
			int i;
			char *temp;

			/* nope, multiple tokens - try old syntax */

			/* strip leading and trailing whitespace */
			temp = action;
			while (*temp && isspace((unsigned char)*temp))
			{
				temp++;
			}
			free(wait_string);
			wait_string = xstrdup(temp);

			for (i = strlen(wait_string) - 1; i >= 0 &&
				     isspace(wait_string[i]); i--)
			{
				wait_string[i] = 0;
			}
		}
	}
	else
	{
		wait_string = xstrdup("");
	}

	is_ungrabbed = UngrabEm(GRAB_NORMAL);
	while (!done && !isTerminated)
	{
		XEvent e;
		if (BUSY_WAIT & Scr.BusyCursor)
		{
			XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_WAIT]);
			redefine_cursor = True;
		}
		if (My_XNextEvent(dpy, &e))
		{
			dispatch_event(&e);
			if (XFindContext(
				    dpy, e.xmap.window, FvwmContext,
				    (caddr_t *)&t) == XCNOENT)
			{
				t = NULL;
			}

			if (e.type == MapNotify && e.xmap.event == Scr.Root)
			{
				if (!*wait_string)
				{
					done = True;
				}
				if (t && matchWildcards(
					    wait_string, t->name.name) == True)
				{
					done = True;
				}
				else if (t && t->class.res_class &&
					 matchWildcards(
						 wait_string,
						 t->class.res_class) == True)
				{
					done = True;
				}
				else if (t && t->class.res_name &&
					 matchWildcards(
						 wait_string,
						 t->class.res_name) == True)
				{
					done = True;
				}
			}
			else if (e.type == KeyPress)
			{
				/* should I be using <t> or <exc->w.fw>?
				 * DV: t
				 */
				int context;
				XClassHint *class;
				char *name;

				context = GetContext(&t, t, &e, &nonewin);
				if (t != NULL)
				{
					class = &(t->class);
					name = t->name.name;
				}
				else
				{
					class = NULL;
					name = NULL;
				}
				escape = CheckBinding(
					Scr.AllBindings, STROKE_ARG(0)
					e.xkey.keycode, e.xkey.state,
					GetUnusedModifiers(), context,
					BIND_KEYPRESS, class, name);
				if (escape != NULL)
				{
					if (!strcasecmp(escape,"escapefunc"))
					{
						done = True;
					}
				}
			}
		}
	}
	if (redefine_cursor)
	{
		XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_ROOT]);
	}
	if (is_ungrabbed)
	{
		GrabEm(CRS_NONE, GRAB_NORMAL);
	}
	free(wait_string);

	return;
}

void CMD_Quit(F_CMD_ARGS)
{
	if (master_pid != getpid())
	{
		kill(master_pid, SIGTERM);
	}
	Done(0,NULL);

	return;
}

void CMD_QuitScreen(F_CMD_ARGS)
{
	Done(0,NULL);

	return;
}

void CMD_Echo(F_CMD_ARGS)
{
	int len;

	if (!action)
	{
		action = "";
	}
	len = strlen(action);
	if (len != 0)
	{
		if (action[len-1]=='\n')
		{
			action[len-1]='\0';
		}
	}
	fvwm_msg(ECHO, "Echo", "%s", action);

	return;
}

void CMD_PrintInfo(F_CMD_ARGS)
{
	int verbose;
	char *rest, *subject = NULL;

	rest = GetNextToken(action, &subject);
	if (!rest || GetIntegerArguments(rest, NULL, &verbose, 1) != 1)
	{
		verbose = 0;
	}
	if (StrEquals(subject, "Colors"))
	{
		PicturePrintColorInfo(verbose);
	}
	else if (StrEquals(subject, "Locale"))
	{
		FlocalePrintLocaleInfo(dpy, verbose);
	}
	else if (StrEquals(subject, "NLS"))
	{
		FGettextPrintLocalePath(verbose);
	}
	else if (StrEquals(subject, "style"))
	{
		print_styles(verbose);
	}
	else if (StrEquals(subject, "ImageCache"))
	{
		PicturePrintImageCache(verbose);
	}
	else if (StrEquals(subject, "Bindings"))
	{
		print_bindings();
	}
	else if (StrEquals(subject, "InfoStore"))
	{
		print_infostore();
	}
	else
	{
		fvwm_msg(ERR, "PrintInfo",
			 "Unknown subject '%s'", action);
	}
	if (subject)
	{
		free(subject);
	}
	return;
}

void CMD_ColormapFocus(F_CMD_ARGS)
{
	if (MatchToken(action,"FollowsFocus"))
	{
		Scr.ColormapFocus = COLORMAP_FOLLOWS_FOCUS;
	}
	else if (MatchToken(action,"FollowsMouse"))
	{
		Scr.ColormapFocus = COLORMAP_FOLLOWS_MOUSE;
	}
	else
	{
		fvwm_msg(ERR, "SetColormapFocus",
			 "ColormapFocus requires 1 arg: FollowsFocus or"
			 " FollowsMouse");
		return;
	}

	return;
}

void CMD_ClickTime(F_CMD_ARGS)
{
	int val;

	if (GetIntegerArguments(action, NULL, &val, 1) != 1)
	{
		Scr.ClickTime = DEFAULT_CLICKTIME;
	}
	else
	{
		Scr.ClickTime = (val < 0)? 0 : val;
	}

	/* Use a negative value during startup and change sign afterwards. This
	 * speeds things up quite a bit. */
	if (fFvwmInStartup)
	{
		Scr.ClickTime = -Scr.ClickTime;
	}

	return;
}


void CMD_ImagePath(F_CMD_ARGS)
{
	PictureSetImagePath( action );

	return;
}

void CMD_IconPath(F_CMD_ARGS)
{
	fvwm_msg(ERR, "iconPath_function",
		 "IconPath is deprecated since 2.3.0; use ImagePath instead.");
	obsolete_imagepaths( action );

	return;
}

void CMD_PixmapPath(F_CMD_ARGS)
{
	fvwm_msg(ERR, "pixmapPath_function",
		 "PixmapPath is deprecated since 2.3.0; use ImagePath"
		 " instead." );
	obsolete_imagepaths( action );

	return;
}

void CMD_LocalePath(F_CMD_ARGS)
{
	FGettextSetLocalePath( action );

	return;
}

void CMD_ModulePath(F_CMD_ARGS)
{
	static int need_to_free = 0;

	setPath( &ModulePath, action, need_to_free );
	need_to_free = 1;

	return;
}

void CMD_ModuleTimeout(F_CMD_ARGS)
{
	int timeout;

	moduleTimeout = DEFAULT_MODULE_TIMEOUT;
	if (GetIntegerArguments(action, NULL, &timeout, 1) == 1 && timeout > 0)
	{
		moduleTimeout = timeout;
	}

	return;
}

void CMD_HilightColor(F_CMD_ARGS)
{
	char *fore;
	char *back;

	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "SetHiColor",
			"Decors do not support the HilightColor command"
			" anymore. Please use"
			" 'Style <stylename> HilightFore <forecolor>' and"
			" 'Style <stylename> HilightBack <backcolor>' instead."
			" Sorry for the inconvenience.");
		return;
	}
	action = GetNextToken(action, &fore);
	GetNextToken(action, &back);
	if (fore && back)
	{
		/* TA:  FIXME:  xasprintf() */
		action = xmalloc(strlen(fore) + strlen(back) + 29);
		sprintf(action, "* HilightFore %s, HilightBack %s", fore, back);
		CMD_Style(F_PASS_ARGS);
	}
	if (fore)
	{
		free(fore);
	}
	if (back)
	{
		free(back);
	}

	return;
}

void CMD_HilightColorset(F_CMD_ARGS)
{
	char *newaction;

	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "SetHiColorset",
			"Decors do not support the HilightColorset command "
			"anymore. Please use "
			"'Style <stylename> HilightColorset <colorset>'"
			" instead. Sorry for the inconvenience.");
		return;
	}

	if (action)
	{
		/* TA:  FIXME!  xasprintf() */
		newaction = xmalloc(strlen(action) + 32);
		sprintf(newaction, "* HilightColorset %s", action);
		action = newaction;
		CMD_Style(F_PASS_ARGS);
		free(newaction);
	}

	return;
}

void CMD_TitleStyle(F_CMD_ARGS)
{
	do_title_style(F_PASS_ARGS, False);

	return;
} /* SetTitleStyle */

/*
 *
 * Appends a titlestyle (veliaa@rpi.edu)
 *
 */
void CMD_AddTitleStyle(F_CMD_ARGS)
{
	do_title_style(F_PASS_ARGS, True);

	return;
}

void CMD_PropertyChange(F_CMD_ARGS)
{
	char string[256];
	char *token;
	char *rest;
	int ret;
	unsigned long argument;
	unsigned long data1;
	unsigned long data2;

	/* argument */
	token = PeekToken(action, &rest);
	if (token == NULL)
	{
		return;
	}
	ret = sscanf(token, "%lu", &argument);
	if (ret < 1)
	{
		return;
	}
	/* data1 */
	data1 = 0;
	token = PeekToken(rest, &rest);
	if (token != NULL)
	{
		ret = sscanf(token, "%lu", &data1);
		if (ret < 1)
		{
			rest = NULL;
		}
	}
	/* data2 */
	data2 = 0;
	token = PeekToken(rest, &rest);
	if (token != NULL)
	{
		ret = sscanf(token, "%lu", &data2);
		if (ret < 1)
		{
			rest = NULL;
		}
	}
	/* string */
	memset(string, 0, 256);
	if (rest != NULL)
	{
		ret = sscanf(rest, "%255c", &(string[0]));
	}
	BroadcastPropertyChange(argument, data1, data2, string);

	return;
}

void CMD_DefaultIcon(F_CMD_ARGS)
{
	if (Scr.DefaultIcon)
	{
		free(Scr.DefaultIcon);
	}
	GetNextToken(action, &Scr.DefaultIcon);

	return;
}

void CMD_DefaultColorset(F_CMD_ARGS)
{
	int cset;

	if (GetIntegerArguments(action, NULL, &cset, 1) != 1)
	{
		return;
	}
	Scr.DefaultColorset = cset;
	if (Scr.DefaultColorset < 0)
	{
		Scr.DefaultColorset = -1;
	}
	alloc_colorset(Scr.DefaultColorset);
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_default_color_changed = 1;

	return;
}

void CMD_DefaultColors(F_CMD_ARGS)
{
	char *fore = NULL;
	char *back = NULL;

	action = GetNextToken(action, &fore);
	if (action)
	{
		action = GetNextToken(action, &back);
	}
	if (!back)
	{
		back = xstrdup(DEFAULT_BACK_COLOR);
	}
	if (!fore)
	{
		fore = xstrdup(DEFAULT_FORE_COLOR);
	}
	if (!StrEquals(fore, "-"))
	{
		PictureFreeColors(dpy, Pcmap, &Scr.StdFore, 1, 0, True);
		Scr.StdFore = GetColor(fore);
	}
	if (!StrEquals(back, "-"))
	{
		PictureFreeColors(dpy, Pcmap, &Scr.StdBack, 3, 0, True);
		Scr.StdBack = GetColor(back);
		Scr.StdHilite = GetHilite(Scr.StdBack);
		Scr.StdShadow = GetShadow(Scr.StdBack);
	}
	free(fore);
	free(back);

	Scr.DefaultColorset = -1;
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_default_color_changed = 1;

	return;
}

void CMD_DefaultFont(F_CMD_ARGS)
{
	char *font;
	FlocaleFont *new_font;
	FvwmWindow *t;

	font = PeekToken(action, &action);
	if (!font)
	{
		/* Try 'fixed', pass NULL font name */
	}
	if (!(new_font = FlocaleLoadFont(dpy, font, "fvwm")))
	{
		if (Scr.DefaultFont == NULL)
		{
			exit(1);
		}
		else
		{
			return;
		}
	}
	FlocaleUnloadFont(dpy, Scr.DefaultFont);
	Scr.DefaultFont = new_font;
	/* we should do that here because a redraw can happen before
	   flush_window_updates is called ... */
	for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
		if (USING_DEFAULT_ICON_FONT(t))
		{
			t->icon_font = Scr.DefaultFont;
		}
		if (USING_DEFAULT_WINDOW_FONT(t))
		{
			t->title_font = Scr.DefaultFont;
		}
	}
	/* set flags to indicate that the font has changed */
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_default_font_changed = 1;

	return;
}

void CMD_IconFont(F_CMD_ARGS)
{
	char *newaction;

	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "LoadIconFont",
			"Decors do not support the IconFont command anymore."
			" Please use 'Style <stylename> IconFont <fontname>'"
			" instead.  Sorry for the inconvenience.");
		return;
	}

	if (action)
	{
		/* TA:  FIXME!  xasprintf() */
		newaction = xmalloc(strlen(action) + 16);
		sprintf(newaction, "* IconFont %s", action);
		action = newaction;
		CMD_Style(F_PASS_ARGS);
		free(newaction);
	}

	return;
}

void CMD_WindowFont(F_CMD_ARGS)
{
	char *newaction;

	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "LoadWindowFont",
			"Decors do not support the WindowFont command anymore."
			" Please use 'Style <stylename> Font <fontname>'"
			" instead.  Sorry for the inconvenience.");
		return;
	}

	if (action)
	{
		/* TA;  FIXME!  xasprintf() */
		newaction = xmalloc(strlen(action) + 16);
		sprintf(newaction, "* Font %s", action);
		action = newaction;
		CMD_Style(F_PASS_ARGS);
		free(newaction);
	}

	return;
}

/*
 *
 * Changes the window's FvwmDecor pointer (veliaa@rpi.edu)
 *
 */
void CMD_ChangeDecor(F_CMD_ARGS)
{
	char *item;
	FvwmDecor *decor = &Scr.DefaultDecor;
	FvwmDecor *found = NULL;
	FvwmWindow * const fw = exc->w.fw;

	item = PeekToken(action, &action);
	if (!action || !item)
	{
		return;
	}
	/* search for tag */
	for (; decor; decor = decor->next)
	{
		if (decor->tag && StrEquals(item, decor->tag))
		{
			found = decor;
			break;
		}
	}
	if (!found)
	{
		XBell(dpy, 0);
		return;
	}
	SET_DECOR_CHANGED(fw, 1);
	fw->decor = found;
	apply_decor_change(fw);

	return;
}

/*
 *
 * Destroys an FvwmDecor (veliaa@rpi.edu)
 *
 */
void CMD_DestroyDecor(F_CMD_ARGS)
{
	char *item;
	FvwmDecor *decor = Scr.DefaultDecor.next;
	FvwmDecor *prev = &Scr.DefaultDecor, *found = NULL;
	Bool do_recreate = False;

	item = PeekToken(action, &action);
	if (!item)
	{
		return;
	}
	if (StrEquals(item, "recreate"))
	{
		do_recreate = True;
		item = PeekToken(action, NULL);
	}
	if (!item)
	{
		return;
	}

	/* search for tag */
	for (; decor; decor = decor->next)
	{
		if (decor->tag && StrEquals(item, decor->tag))
		{
			found = decor;
			break;
		}
		prev = decor;
	}

	if (found && (found != &Scr.DefaultDecor || do_recreate))
	{
		if (!do_recreate)
		{
			__remove_window_decors(F_PASS_ARGS, found);
		}
		DestroyFvwmDecor(found);
		if (do_recreate)
		{
			int i;

			InitFvwmDecor(found);
			found->tag = xstrdup(item);
			Scr.flags.do_need_window_update = 1;
			found->flags.has_changed = 1;
			found->flags.has_title_height_changed = 0;
			found->titlebar.flags.has_changed = 1;
			for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; ++i)
			{
				TB_FLAGS(found->buttons[i]).has_changed = 1;
			}
		}
		else
		{
			prev->next = found->next;
			free(found);
		}
	}

	return;
}

/*
 *
 * Initiates an AddToDecor (veliaa@rpi.edu)
 *
 */
void CMD_AddToDecor(F_CMD_ARGS)
{
	FvwmDecor *decor;
	FvwmDecor *found = NULL;
	char *item = NULL;

	action = GetNextToken(action, &item);
	if (!item)
	{
		return;
	}
	if (!action)
	{
		free(item);
		return;
	}
	/* search for tag */
	for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
	{
		if (decor->tag && StrEquals(item, decor->tag))
		{
			found = decor;
			break;
		}
	}
	if (!found)
	{
		/* then make a new one */
		found = xmalloc(sizeof *found);
		InitFvwmDecor(found);
		found->tag = item; /* tag it */
		/* add it to list */
		for (decor = &Scr.DefaultDecor; decor->next;
		     decor = decor->next)
		{
			/* nop */
		}
		decor->next = found;
	}
	else
	{
		free(item);
	}

	if (found)
	{
		AddToDecor(F_PASS_ARGS, found);
		/* Set + state to last decor */
		set_last_added_item(ADDED_DECOR, found);
	}

	return;
}


/*
 *
 * Updates window decoration styles (veliaa@rpi.edu)
 *
 */
void CMD_UpdateDecor(F_CMD_ARGS)
{
	FvwmWindow *fw2;

	FvwmDecor *decor, *found = NULL;
	FvwmWindow *hilight = Scr.Hilite;
	char *item = NULL;

	action = GetNextToken(action, &item);
	if (item)
	{
		/* search for tag */
		for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
		{
			if (decor->tag && StrEquals(item, decor->tag))
			{
				found = decor;
				break;
			}
		}
		free(item);
	}

	for (fw2 = Scr.FvwmRoot.next; fw2; fw2 = fw2->next)
	{
		/* update specific decor, or all */
		if (found)
		{
			if (fw2->decor == found)
			{
				border_draw_decorations(
					fw2, PART_ALL, True, True, CLEAR_ALL,
					NULL, NULL);
				border_draw_decorations(
					fw2, PART_ALL, False, True, CLEAR_ALL,
					NULL, NULL);
			}
		}
		else
		{
			border_draw_decorations(
				fw2, PART_ALL, True, True, CLEAR_ALL, NULL,
				NULL);
			border_draw_decorations(
				fw2, PART_ALL, False, True, CLEAR_ALL, NULL,
				NULL);
		}
	}
	border_draw_decorations(
		hilight, PART_ALL, True, True, CLEAR_ALL, NULL, NULL);
}

void CMD_ButtonStyle(F_CMD_ARGS)
{
	do_button_style(F_PASS_ARGS, False);

	return;
}

/*
 *
 * Appends a button decoration style (veliaa@rpi.edu)
 *
 */
void CMD_AddButtonStyle(F_CMD_ARGS)
{
	do_button_style(F_PASS_ARGS, True);

	return;
}

void CMD_SetEnv(F_CMD_ARGS)
{
	char *szVar = NULL;
	char *szValue = NULL;
	char *szPutenv = NULL;

	action = GetNextToken(action, &szVar);
	if (!szVar)
	{
		return;
	}
	action = GetNextToken(action, &szValue);
	if (!szValue)
	{
		szValue = xstrdup("");
	}
	szPutenv = xmalloc(strlen(szVar) + strlen(szValue) + 2);
	sprintf(szPutenv,"%s=%s", szVar, szValue);
	flib_putenv(szVar, szPutenv);
	free(szVar);
	free(szPutenv);
	free(szValue);

	return;
}

void CMD_UnsetEnv(F_CMD_ARGS)
{
	char *szVar = NULL;

	szVar = PeekToken(action, &action);
	if (!szVar)
	{
		return;
	}
	flib_unsetenv(szVar);

	return;
}

void CMD_GlobalOpts(F_CMD_ARGS)
{
	char *opt;
	char *replace;
	char buf[64];
	int i;
	Bool is_bugopt;
	char *optlist[] = {
		"WindowShadeShrinks",
		"WindowShadeScrolls",
		"SmartPlacementIsReallySmart",
		"SmartPlacementIsNormal",
		"ClickToFocusDoesntPassClick",
		"ClickToFocusPassesClick",
		"ClickToFocusDoesntRaise",
		"ClickToFocusRaises",
		"MouseFocusClickDoesntRaise",
		"MouseFocusClickRaises",
		"NoStipledTitles",
		"StipledTitles",
		"CaptureHonorsStartsOnPage",
		"CaptureIgnoresStartsOnPage",
		"RecaptureHonorsStartsOnPage",
		"RecaptureIgnoresStartsOnPage",
		"ActivePlacementHonorsStartsOnPage",
		"ActivePlacementIgnoresStartsOnPage",
		"RaiseOverNativeWindows",
		"IgnoreNativeWindows",
		NULL
	};
	char *replacelist[] = {
		/* These options are mapped to the Style * command */
		NULL, /* NULL means to use "Style * <optionname>" */
		NULL,
		"* MinOverlapPlacement",
		"* TileCascadePlacement",
		"* ClickToFocusPassesClickOff",
		"* ClickToFocusPassesClick",
		"* ClickToFocusRaisesOff",
		"* ClickToFocusRaises",
		"* MouseFocusClickRaisesOff",
		"* MouseFocusClickRaises",
		"* StippledTitleOff",
		"* StippledTitle",
		NULL,
		NULL,
		NULL,
		NULL,
		"* ManualPlacementHonorsStartsOnPage",
		"* ManualPlacementIgnoresStartsOnPage",
		/* These options are mapped to the BugOpts command */
		"RaiseOverNativeWindows on",
		"RaiseOverNativeWindows off"
	};

	fvwm_msg(ERR, "SetGlobalOptions",
		 "The GlobalOpts command is obsolete.");
	for (action = GetNextSimpleOption(action, &opt); opt;
	     action = GetNextSimpleOption(action, &opt))
	{
		replace = NULL;
		is_bugopt = False;

		i = GetTokenIndex(opt, optlist, 0, NULL);
		if (i > -1)
		{
			char *cmd;
			char *tmp;

			replace = replacelist[i];
			if (replace == NULL)
			{
				replace = &(buf[0]);
				sprintf(buf, "* %s", opt);
			}
			else if (*replace != '*')
			{
				is_bugopt = True;
			}
			tmp = action;
			action = replace;
			if (!is_bugopt)
			{
				CMD_Style(F_PASS_ARGS);
				cmd = "Style";
			}
			else
			{
				CMD_BugOpts(F_PASS_ARGS);
				cmd = "BugOpts";
			}
			action = tmp;
			fvwm_msg(
				ERR, "SetGlobalOptions",
				"Please replace 'GlobalOpts %s' with '%s %s'.",
				opt, cmd, replace);
		}
		else
		{
			fvwm_msg(ERR, "SetGlobalOptions",
				 "Unknown Global Option '%s'", opt);
		}
		/* should never be null, but checking anyways... */
		if (opt)
		{
			free(opt);
		}
	}
	if (opt)
	{
		free(opt);
	}

	return;
}

void CMD_BugOpts(F_CMD_ARGS)
{
	char *opt;
	int toggle;
	char *optstring;

	/* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
	while (action && *action && *action != '\n')
	{
		action = GetNextFullOption(action, &optstring);
		if (!optstring)
		{
			/* no more options */
			return;
		}
		toggle = ParseToggleArgument(
			SkipNTokens(optstring,1), NULL, 2, False);
		opt = PeekToken(optstring, NULL);
		free(optstring);

		if (!opt)
		{
			return;
		}
		/* toggle = ParseToggleArgument(rest, &rest, 2, False);*/

		if (StrEquals(opt, "FlickeringMoveWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_disable_configure_notify ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_disable_configure_notify = toggle;
				break;
			default:
				Scr.bo.do_disable_configure_notify = 0;
				break;
			}
		}
		else if (StrEquals(opt, "MixedVisualWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_install_root_cmap ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_install_root_cmap = toggle;
				break;
			default:
				Scr.bo.do_install_root_cmap = 0;
				break;
			}
		}
		else if (StrEquals(opt, "ModalityIsEvil"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.is_modality_evil ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.is_modality_evil = toggle;
				break;
			default:
				Scr.bo.is_modality_evil = 0;
				break;
			}
			if (Scr.bo.is_modality_evil)
			{
				SetMWM_INFO(Scr.NoFocusWin);
			}
		}
		else if (StrEquals(opt, "RaiseOverNativeWindows"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.is_raise_hack_needed ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.is_raise_hack_needed = toggle;
				break;
			default:
				Scr.bo.is_raise_hack_needed = 0;
				break;
			}
		}
		else if (StrEquals(opt, "RaiseOverUnmanaged"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_raise_over_unmanaged ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_raise_over_unmanaged = toggle;
				break;
			default:
				Scr.bo.do_raise_over_unmanaged = 0;
				break;
			}
		}
		else if (StrEquals(opt, "FlickeringQtDialogsWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_enable_flickering_qt_dialogs_workaround ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_enable_flickering_qt_dialogs_workaround = toggle;
				break;
			default:
				Scr.bo.do_enable_flickering_qt_dialogs_workaround = 0;
				break;
			}
		}
		else if (StrEquals(opt, "QtDragnDropWorkaround") )
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_enable_qt_drag_n_drop_workaround ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_enable_qt_drag_n_drop_workaround = toggle;
				break;
			default:
				Scr.bo.do_enable_qt_drag_n_drop_workaround = 0;
				break;
			}
		}
		else if (EWMH_BugOpts(opt, toggle))
		{
			/* work is done in EWMH_BugOpts */
		}
		else if (StrEquals(opt, "DisplayNewWindowNames"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_display_new_window_names ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_display_new_window_names = toggle;
				break;
			default:
				Scr.bo.do_display_new_window_names = 0;
				break;
			}
		}
		else if (StrEquals(opt, "ExplainWindowPlacement"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_explain_window_placement ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_explain_window_placement = toggle;
				break;
			default:
				Scr.bo.do_explain_window_placement = 0;
				break;
			}
		}
		else if (StrEquals(opt, "DebugCRMotionMethod"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.do_debug_cr_motion_method ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.do_debug_cr_motion_method = toggle;
				break;
			default:
				Scr.bo.do_debug_cr_motion_method = 0;
				break;
			}
		}
		else if (StrEquals(opt, "TransliterateUtf8"))
		{
			FiconvSetTransliterateUtf8(toggle);
		}
		else
		{
			fvwm_msg(ERR, "SetBugOptions",
				 "Unknown Bug Option '%s'", opt);
		}
	}

	return;
}

void CMD_Emulate(F_CMD_ARGS)
{
	char *style;

	style = PeekToken(action, NULL);
	if (!style || StrEquals(style, "fvwm"))
	{
		Scr.gs.do_emulate_mwm = False;
		Scr.gs.do_emulate_win = False;
	}
	else if (StrEquals(style, "mwm"))
	{
		Scr.gs.do_emulate_mwm = True;
		Scr.gs.do_emulate_win = False;
	}
	else if (StrEquals(style, "win"))
	{
		Scr.gs.do_emulate_mwm = False;
		Scr.gs.do_emulate_win = True;
	}
	else
	{
		fvwm_msg(ERR, "Emulate", "Unknown style '%s'", style);
		return;
	}
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_default_font_changed = 1;
	Scr.flags.has_default_color_changed = 1;

	return;
}

void CMD_ColorLimit(F_CMD_ARGS)
{
	fvwm_msg(
		WARN, "ColorLimit", "ColorLimit is obsolete,\n\tuse the "
		"fvwm -color-limit option");

	return;
}


/* set animation parameters */
void CMD_SetAnimation(F_CMD_ARGS)
{
	char *opt;
	int delay;
	float pct;
	int i = 0;

	opt = PeekToken(action, &action);
	if (!opt || sscanf(opt,"%d",&delay) != 1)
	{
		fvwm_msg(ERR,"SetAnimation",
			 "Improper milli-second delay as first argument");
		return;
	}
	if (delay > 500)
	{
		fvwm_msg(WARN,"SetAnimation",
			 "Using longer than .5 seconds as between frame"
			 " animation delay");
	}
	cmsDelayDefault = delay;
	for (opt = PeekToken(action, &action); opt;
	     opt = PeekToken(action, &action))
	{
		if (sscanf(opt,"%f",&pct) != 1)
		{
			fvwm_msg(ERR,"SetAnimation",
				 "Use fractional values ending in 1.0 as args"
				 " 2 and on");
			return;
		}
		rgpctMovementDefault[i++] = pct;
	}
	/* No pct entries means don't change them at all */
	if (i > 0 && rgpctMovementDefault[i-1] != 1.0)
	{
		rgpctMovementDefault[i++] = 1.0;
	}

	return;
}

/* Determine which modifiers are required with a keycode to make <keysym>. */
static Bool FKeysymToKeycode (Display *dpy, KeySym keysym,
	unsigned int *keycode, unsigned int *modifiers)
{
	int m;

	*keycode = XKeysymToKeycode(dpy, keysym);
	*modifiers = 0;

	for (m = 0; m <= 8; ++m)
	{
		KeySym ks = fvwm_KeycodeToKeysym(dpy, *keycode, m, 0);
		if (ks == keysym)
		{
			switch (m)
			{
				case 0: /* No modifiers */
					break;
				case 1: /* Shift modifier */
					*modifiers |= ShiftMask;
					break;
				default:
					fvwm_msg(ERR, "FKeysymToKeycode",
						"Unhandled modifier %d", m);
					break;
			}
			return True;
		}
	}
	return False;
}

static void __fake_event(F_CMD_ARGS, FakeEventType type)
{
	char *token;
	char *optlist[] = {
		"press", "p",
		"release", "r",
		"wait", "w",
		"modifiers", "m",
		"depth", "d",
		NULL
	};
	unsigned int mask = 0;
	Window root = Scr.Root;
	int maxdepth = 0;
	static char args[128];
	strncpy(args, action, sizeof(args) - 1);

	/* get the mask of pressed/released buttons/keys */
	FQueryPointer(
		dpy, Scr.Root, &root, &JunkRoot, &JunkX, &JunkY, &JunkX,
		&JunkY, &mask);

	token = PeekToken(action, &action);
	while (token && action)
	{
		int index = GetTokenIndex(token, optlist, 0, NULL);
		int val, depth;
		XEvent e;
		Window w;
		Window child_w;
		int x = 0;
		int y = 0;
		int rx = 0;
		int ry = 0;
		Bool do_unset;
		long add_mask = 0;
		KeySym keysym = NoSymbol;

		XFlush(dpy);
		do_unset = True;
		switch (index)
		{
		case 0:
		case 1:
			do_unset = False;
			/* fall through */
		case 2:
		case 3:
			/* key/button press or release */
			if (type == FakeMouseEvent)
			{
				if ((GetIntegerArguments(
					     action, &action, &val, 1) != 1) ||
				    val < 1 ||
				    val > NUMBER_OF_EXTENDED_MOUSE_BUTTONS)
				{
					fvwm_msg(
						ERR, "__fake_event",
						"Invalid button specifier in"
						" \"%s\" for FakeClick.", args);
					return; /* error */
				}
			}
			else /* type == FakeKeyEvent */
			{
				char *key = PeekToken(action, &action);
				if (key == NULL)
				{
					fvwm_msg(
						ERR, "__fake_event",
						"No keysym specifier in \"%s\""
						" for FakeKeypress.", args);
					return;
				}

				/* Do *NOT* use FvwmStringToKeysym() as it is
				 * case insensitive. */
				keysym = XStringToKeysym(key);
				if (keysym == NoSymbol)
				{
					fvwm_msg(
						ERR, "__fake_event",
						"Invalid keysym specifier (%s)"
						" in \"%s\" for FakeKeypress.",
						key, args);
					return;
				}
			}

			w = None;
			child_w = root;
			for (depth = 1;
				 depth != maxdepth &&
				     w != child_w && child_w != None;
				 depth++)
			{
				w = child_w;
				if (FQueryPointer(
						dpy, w, &root, &child_w,
						&rx, &ry, &x, &y,
						&JunkMask) == False)
				{
					/* pointer is on a different
					 * screen - that's okay here */
				}
			}

			if (type == FakeMouseEvent)
			{
				e.type = (do_unset) ?
					ButtonRelease : ButtonPress;
				e.xbutton.display = dpy;
				e.xbutton.window = w;
				e.xbutton.subwindow = None;
				e.xbutton.root = root;
				e.xbutton.time = fev_get_evtime();
				e.xbutton.x = x;
				e.xbutton.y = y;
				e.xbutton.x_root = rx;
				e.xbutton.y_root = ry;
				e.xbutton.button = val;
				e.xbutton.state = mask;
				e.xbutton.same_screen = (Scr.Root == root);
				/* SS: I think this mask handling code is
				 * buggy.
				 * The value of <mask> is overridden during a
				 * "wait" operation. Also why are we only using
				 * Button1Mask? What if the user has requested
				 * a FakeClick using some other button? */
				/* DV: Button1Mask is actually a bit.  Shifting
				 * it by (val -1) bits to the left gives
				 * Button2Mask, Button3Mask etc. */
				if (do_unset)
				{
					mask &= ~(Button1Mask << (val - 1));
				}
				else
				{
					mask |= (Button1Mask << (val - 1));
				}
				add_mask = (do_unset) ?
					ButtonPressMask : ButtonReleaseMask;
			}
			else
			{
				/* type == FakeKeyEvent */
				e.type = (do_unset ? KeyRelease : KeyPress);
				e.xkey.display = dpy;
				e.xkey.subwindow = None;
				e.xkey.root = root;
				e.xkey.time = fev_get_evtime();
				e.xkey.x = x;
				e.xkey.y = y;
				e.xkey.x_root = rx;
				e.xkey.y_root = ry;
				e.xkey.same_screen = (Scr.Root == root);

				w = e.xkey.window = exc->w.w;

				if (FKeysymToKeycode(
					    dpy, keysym, &(e.xkey.keycode),
					    &(e.xkey.state)) != True)
				{
					fvwm_msg(DBG, "__fake_event",
						"FKeysymToKeycode failed");
					return;
				}
				e.xkey.state |= mask;
				add_mask = (do_unset) ?
					KeyReleaseMask : KeyPressMask;
			}
			FSendEvent(dpy, w, True,
				SubstructureNotifyMask | add_mask, &e);
			XFlush(dpy);
			break;
		case 4:
		case 5:
			/* wait */
			if ((GetIntegerArguments(
				     action, &action, &val, 1) != 1) ||
			    val <= 0 || val > 1000000)
			{
				fvwm_msg(ERR, "__fake_event",
					"Invalid wait value in \"%s\"", args);
				return;
			}

			usleep(1000 * val);
			if (FQueryPointer(
					dpy, Scr.Root, &root, &JunkRoot,
					&JunkX, &JunkY, &JunkX, &JunkY,
					&mask) == False)
			{
				/* pointer is on a different screen -
				 * that's okay here */
			}
			break;
		case 6:
		case 7:
			/* set modifier */
			if (GetIntegerArguments(action, &action, &val, 1) != 1)
			{
				fvwm_msg(
					ERR, "__fake_event",
					"Invalid modifier value in \"%s\"",
					args);
				return;
			}
			do_unset = False;
			if (val < 0)
			{
				do_unset = True;
				val = -val;
			}
			if (val == 6)
			{
				val = ShiftMask;
			}
			else if (val == 7)
			{
				val = LockMask;
			}
			else if (val == 8)
			{
				val = ControlMask;
			}
			else if (val >=1 && val <= 5)
			{
				val = (Mod1Mask << (val - 1));
			}
			else
			{
				/* error */
				return;
			}
			/* SS: Could be buggy if a "modifier" operation
			 * preceeds a "wait" operation. */
			if (do_unset)
			{
				mask &= ~val;
			}
			else
			{
				mask |= val;
			}
			break;
		case 8:
		case 9:
			/* new max depth */
			if (GetIntegerArguments(action, &action, &val, 1) != 1)
			{
				fvwm_msg(ERR, "__fake_event",
					"Invalid depth value in \"%s\"", args);
				return;
			}
			maxdepth = val;
			break;
		default:
			fvwm_msg(ERR, "__fake_event",
				"Invalid command (%s) in \"%s\"", token, args);
			return;
		}
		if (action)
		{
			token = PeekToken(action, &action);
		}
	}

	return;
}

void CMD_FakeClick(F_CMD_ARGS)
{
	__fake_event(F_PASS_ARGS, FakeMouseEvent);

	return;
}

void CMD_FakeKeypress(F_CMD_ARGS)
{
	__fake_event(F_PASS_ARGS, FakeKeyEvent);

	return;
}

/* A function to handle stroke (olicha Nov 11, 1999) */
#ifdef HAVE_STROKE
void CMD_StrokeFunc(F_CMD_ARGS)
{
	int finished = 0;
	int abort = 0;
	int modifiers = exc->x.etrigger->xbutton.state;
	int start_event_type = exc->x.etrigger->type;
	char sequence[STROKE_MAX_SEQUENCE + 1];
	char *stroke_action, *name;
	char *opt = NULL;
	Bool finish_on_release = True;
	KeySym keysym;
	Bool restore_repeat = False;
	Bool echo_sequence = False;
	Bool draw_motion = False;
	int i = 0;
	int *x = NULL;
	int *y = NULL;
	const int STROKE_CHUNK_SIZE = 0xff;
	int coords_size = STROKE_CHUNK_SIZE;
	Window JunkRoot, JunkChild;
	int JunkX, JunkY;
	int tmpx, tmpy;
	unsigned int JunkMask;
	Bool feed_back = False;
	int stroke_width = 1;
	XEvent e;
	XClassHint *class;



	if (!GrabEm(CRS_STROKE, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return;
	}
	x = xmalloc(coords_size * sizeof(int));
	y = xmalloc(coords_size * sizeof(int));
	e = *exc->x.etrigger;
	/* set the default option */
	if (e.type == KeyPress || e.type == ButtonPress)
	{
		finish_on_release = True;
	}
	else
	{
		finish_on_release = False;
	}

	/* parse the option */
	for (action = GetNextSimpleOption(action, &opt); opt;
	     action = GetNextSimpleOption(action, &opt))
	{
		if (StrEquals("NotStayPressed",opt))
		{
			finish_on_release = False;
		}
		else if (StrEquals("EchoSequence",opt))
		{
			echo_sequence = True;
		}
		else if (StrEquals("DrawMotion",opt))
		{
			draw_motion = True;
		}
		else if (StrEquals("FeedBack",opt))
		{
			feed_back = True;
		}
		else if (StrEquals("StrokeWidth",opt))
		{
			/* stroke width takes a positive integer argument */
			if (opt)
			{
				free(opt);
			}
			action = GetNextToken(action, &opt);
			if (!opt)
			{
				fvwm_msg(
					WARN, "StrokeWidth",
					"needs an integer argument");
			}
			/* we allow stroke_width == 0 which means drawing a
			 * `fast' line of width 1; the upper level of 100 is
			 * arbitrary */
			else if (!sscanf(opt, "%d", &stroke_width) ||
				 stroke_width < 0 || stroke_width > 100)
			{
				fvwm_msg(
					WARN, "StrokeWidth",
					"Bad integer argument %d",
					stroke_width);
				stroke_width = 1;
			}
		}
		else
		{
			fvwm_msg(WARN,"StrokeFunc","Unknown option %s", opt);
		}
		if (opt)
		{
			free(opt);
		}
	}
	if (opt)
	{
		free(opt);
	}

	/* Force auto repeat off and grab the Keyboard to get proper
	 * KeyRelease events if we need it.
	 * Some computers do not support KeyRelease events, can we
	 * check this here ? No ? */
	if (start_event_type == KeyPress && finish_on_release)
	{
		XKeyboardState kstate;

		XGetKeyboardControl(dpy, &kstate);
		if (kstate.global_auto_repeat == AutoRepeatModeOn)
		{
			XAutoRepeatOff(dpy);
			restore_repeat = True;
		}
		MyXGrabKeyboard(dpy);
	}

	/* be ready to get a stroke sequence */
	stroke_init();

	if (draw_motion)
	{
		MyXGrabServer(dpy);
		if (FQueryPointer(
			    dpy, Scr.Root, &JunkRoot, &JunkChild, &x[0], &y[0],
			    &JunkX, &JunkY, &JunkMask) == False)
		{
			/* pointer is on a different screen */
			x[0] = 0;
			y[0] = 0;
		}
		XSetLineAttributes(
			dpy,Scr.XorGC,stroke_width,LineSolid,CapButt,JoinMiter);
	}

	while (!finished && !abort)
	{
		/* block until there is an event */
		FMaskEvent(
			dpy,  ButtonPressMask | ButtonReleaseMask |
			KeyPressMask | KeyReleaseMask | ButtonMotionMask |
			PointerMotionMask, &e);
		switch (e.type)
		{
		case MotionNotify:
			if (e.xmotion.same_screen == False)
			{
				continue;
			}
			if (e.xany.window != Scr.Root)
			{
				if (FQueryPointer(
					    dpy, Scr.Root, &JunkRoot,
					    &JunkChild, &tmpx, &tmpy, &JunkX,
					    &JunkY, &JunkMask) == False)
				{
					/* pointer is on a different screen */
					tmpx = 0;
					tmpy = 0;
				}
			}
			else
			{
				tmpx = e.xmotion.x;
				tmpy = e.xmotion.y;
			}
			stroke_record(tmpx,tmpy);
			if (draw_motion && (x[i] != tmpx || y[i] != tmpy))
			{
				i++;
				if (i >= coords_size)
				{
					coords_size += STROKE_CHUNK_SIZE;
					x = xrealloc((void *)x,
						     coords_size * sizeof(int),
						     sizeof((void *)x));
					y = xrealloc((void *)y,
						     coords_size * sizeof(int),
						     sizeof((void *)y));
				}
				x[i] = tmpx;
				y[i] = tmpy;
				XDrawLine(
					dpy, Scr.Root, Scr.XorGC, x[i-1],
					y[i-1], x[i], y[i]);
			}
			break;
		case ButtonRelease:
			if (finish_on_release && start_event_type ==
			    ButtonPress)
			{
				finished = 1;
			}
			break;
		case KeyRelease:
			if (finish_on_release &&  start_event_type == KeyPress)
			{
				finished = 1;
			}
			break;
		case KeyPress:
			keysym = XLookupKeysym(&e.xkey, 0);
			/* abort if Escape or Delete is pressed (as in menus.c)
			 */
			if (keysym == XK_Escape || keysym == XK_Delete ||
			    keysym == XK_KP_Separator)
			{
				abort = 1;
			}
			/* finish on enter or space (as in menus.c) */
			if (keysym == XK_Return || keysym == XK_KP_Enter ||
			    keysym ==  XK_space)
			{
				finished = 1;
			}
			break;
		case ButtonPress:
			if (!finish_on_release)
			{
				finished = 1;
			}
			break;
		default:
			break;
		}
	}

	if (draw_motion)
	{
		while (i > 0)
		{
			XDrawLine(
				dpy, Scr.Root, Scr.XorGC, x[i-1], y[i-1], x[i],
				y[i]);
			i--;
		}
		XSetLineAttributes(dpy,Scr.XorGC,0,LineSolid,CapButt,JoinMiter);
		MyXUngrabServer(dpy);
	}
	if (x != NULL)
	{
		free(x);
		free(y);
	}
	if (start_event_type == KeyPress && finish_on_release)
	{
		MyXUngrabKeyboard(dpy);
	}
	UngrabEm(GRAB_NORMAL);
	if (restore_repeat)
	{
		XAutoRepeatOn(dpy);
	}

	/* get the stroke sequence */
	stroke_trans(sequence);

	if (echo_sequence)
	{
		char num_seq[STROKE_MAX_SEQUENCE + 1];

		for (i = 0; sequence[i] != '\0';i++)
		{
			/* Telephone to numeric pad */
			if ('7' <= sequence[i] && sequence[i] <= '9')
			{
				num_seq[i] = sequence[i]-6;
			}
			else if ('1' <= sequence[i] && sequence[i] <= '3')
			{
				num_seq[i] = sequence[i]+6;
			}
			else
			{
				num_seq[i] = sequence[i];
			}
		}
		num_seq[i++] = '\0';
		fvwm_msg(INFO, "StrokeFunc", "stroke sequence: %s (N%s)",
			 sequence, num_seq);
	}
	if (abort)
	{
		return;
	}
	if (exc->w.fw == NULL)
	{
		class = NULL;
		name = NULL;
	}
	else
	{
		class = &exc->w.fw->class;
		name = exc->w.fw->name.name;
	}
	/* check for a binding */
	stroke_action = CheckBinding(
		Scr.AllBindings, sequence, 0, modifiers, GetUnusedModifiers(),
		exc->w.wcontext, BIND_STROKE, class, name);

	/* execute the action */
	if (stroke_action != NULL)
	{
		const exec_context_t *exc2;
		exec_context_changes_t ecc;

		if (feed_back && atoi(sequence) != 0)
		{
			GrabEm(CRS_WAIT, GRAB_BUSY);
			usleep(200000);
			UngrabEm(GRAB_BUSY);
		}
		ecc.x.etrigger = &e;
		exc2 = exc_clone_context(exc, &ecc, ECC_ETRIGGER);
		execute_function(cond_rc, exc2, stroke_action, 0);
		exc_destroy_context(exc2);
	}

	return;
}
#endif /* HAVE_STROKE */

void CMD_State(F_CMD_ARGS)
{
	unsigned int state;
	int toggle;
	int n;
	FvwmWindow * const fw = exc->w.fw;

	n = GetIntegerArguments(action, &action, (int *)&state, 1);
	if (n <= 0)
	{
		return;
	}
	if (state > 31)
	{
		fvwm_msg(ERR, "CMD_State", "Illegal state %d\n", state);
		return;
	}
	toggle = ParseToggleArgument(action, NULL, -1, 0);
	state = (1 << state);
	switch (toggle)
	{
	case -1:
		TOGGLE_USER_STATES(fw, state);
		break;
	case 0:
		CLEAR_USER_STATES(fw, state);
		break;
	case 1:
	default:
		SET_USER_STATES(fw, state);
		break;
	}

	return;
}
