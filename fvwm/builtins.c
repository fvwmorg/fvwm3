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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/****************************************************************************
 * This module is all original code
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/* ---------------------------- included header files ----------------------- */

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <X11/keysym.h>

#include "libs/fvwmlib.h"
#include "libs/fvwmsignal.h"
#include "libs/setpgrp.h"
#include "libs/Picture.h"
#include "libs/PictureUtils.h"
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
#ifdef HAVE_STROKE
#include "stroke.h"
#endif /* HAVE_STROKE */

/* ---------------------------- local definitions --------------------------- */

#define ENV_LIST_INC 10

/* ---------------------------- local macros -------------------------------- */

/* ---------------------------- imports ------------------------------------- */

extern float rgpctMovementDefault[32];
extern int cpctMovementDefault;
extern int cmsDelayDefault;

/* ---------------------------- included code files ------------------------- */

/* ---------------------------- local types --------------------------------- */

typedef struct
{
	char *var;
	char *env;
} env_list_item;

/* ---------------------------- forward declarations ------------------------ */

/* ---------------------------- local variables ----------------------------- */

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
	NULL
};

/* ---------------------------- exported variables (globals) ---------------- */

char *ModulePath = FVWM_MODULEDIR;
int moduleTimeout = DEFAULT_MODULE_TIMEOUT;

/* ---------------------------- local functions ----------------------------- */

/* add_to_env_list
 *   This function keeps a list of all strings that were set in the environment
 *   via SetEnv or UnsetEnv. If a variable is written again, the old memory is
 *   freed. */
static void add_to_env_list(char *var, char *env)
{
	static env_list_item *env_list = NULL;
	static unsigned int env_len = 0;
	unsigned int i;

	/* find string in list */
	if (env_list && env_len)
	{
		for (i = 0; i < env_len; i++)
		{
			if (strcmp(var, env_list[i].var) == 0)
			{
				/* found it - replace old string */
				free(env_list[i].var);
				free(env_list[i].env);
				env_list[i].var = var;
				env_list[i].env = env;
				return;
			}
		}
		/* not found, add to list */
		if (env_len % ENV_LIST_INC == 0)
		{
			/* need more memory */
			env_list = (env_list_item *)saferealloc(
				(void *)env_list, (env_len + ENV_LIST_INC) *
				sizeof(env_list_item));
		}
	}
	else if (env_list == NULL)
	{
		/* list is still empty */
		env_list = (env_list_item *)safecalloc(
			sizeof(env_list_item), ENV_LIST_INC);
	}
	env_list[env_len].var = var;
	env_list[env_len].env = env;
	env_len++;

	return;
}

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

/*****************************************************************************
 *
 * Reads a title button description (veliaa@rpi.edu)
 *
 ****************************************************************************/
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

	s = SkipSpaces(s, NULL, 0);
	t = GetNextTokenIndex(s, button_states, 0, &bs);
	if (bs != BS_All)
	{
		s = SkipSpaces(t, NULL, 0);
	}

	bs_start = bs_end = bs;
	if (bs == BS_All)
	{
		bs_start = 0;
		bs_end = BS_MaxButtonState - 1;
	}
	else if (bs == BS_Active)
	{
		bs_start = BS_ActiveUp;
		bs_end = BS_ActiveDown;
	}
	else if (bs == BS_Inactive)
	{
		bs_start = BS_InactiveUp;
		bs_end = BS_InactiveDown;
	}
	else if (bs == BS_ToggledActive)
	{
		bs_start = BS_ToggledActiveUp;
		bs_end = BS_ToggledActiveDown;
	}
	else if (bs == BS_ToggledInactive)
	{
		bs_start = BS_ToggledInactiveUp;
		bs_end = BS_ToggledInactiveDown;
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
		spec = safemalloc(len);
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
			tail->next = (DecorFace *)safemalloc(sizeof(DecorFace));
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
				head = &TB_STATE(*tb)[i];
				tail = head;
				while (tail->next)
				{
					tail = tail->next;
				}
				tail->next = (DecorFace *)safemalloc(
					sizeof(DecorFace));
				memset(
					&DFS_FLAGS(tail->next->style), 0,
					sizeof(DFS_FLAGS(tail->next->style)));
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
static void __remove_window_decors(FvwmDecor *d)
{
	FvwmWindow *fw;

	for (fw = Scr.FvwmRoot.next; fw; fw = fw->next)
	{
		if (fw->decor == d)
		{
			/* remove the extra title height now because we delete
			 * the current decor before calling ChangeDecor(). */
			fw->frame_g.height -= fw->decor->title_height;
			fw->decor = NULL;
			old_execute_function(
				NULL, "ChangeDecor Default", fw, eventp,
				C_WINDOW, *Module, 0, NULL);
		}
	}

	return;
}

static void do_title_style(F_CMD_ARGS, Bool do_add)
{
	char *parm;
	char *prev;
#ifdef USEDECOR
	FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;
#else
	FvwmDecor *decor = &Scr.DefaultDecor;
#endif

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
			if (decor->title_height != height)
			{
				decor->title_height = height;
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

#ifdef FANCY_TITLEBARS
/*****************************************************************************
 *
 * Reads a multi-pixmap titlebar config. (tril@igs.net)
 *
 ****************************************************************************/
static char *ReadMultiPixmapDecor(char *s, DecorFace *df)
{
	static char *pm_names[NUM_TB_PIXMAPS+1] =
		{
			"Main",
			"LeftMain",
			"RightMain",
			"UnderText",
			"LeftOfText",
			"RightOfText",
			"LeftEnd",
			"RightEnd",
			"Buttons",
			"LeftButtons",
			"RightButtons",
			NULL
		};
	FvwmPicture **pm;
	char *token;
	Bool stretched;
	int pm_id, i = 0;
	FvwmPictureAttributes fpa;

	df->style.face_type = MultiPixmap;
	df->u.multi_pixmaps = pm =
		(FvwmPicture**)safecalloc(NUM_TB_PIXMAPS, sizeof(FvwmPicture*));

	s = GetNextTokenIndex(s, pm_names, 0, &pm_id);
	while (pm_id >= 0)
	{
		s = DoPeekToken(s, &token, ",()", NULL, NULL);
		stretched = False;
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
		if (pm[pm_id])
		{
			fvwm_msg(WARN, "ReadMultiPixmapDecor",
				 "Ignoring already-specified %s pixmap",
				 pm_names[i]);
			continue;
		}
		if (stretched)
		{
			df->u.multi_stretch_flags |= (1 << pm_id);
		}
		fpa.mask = (Pdepth <= 8)? FPAM_DITHER:0; /* ? */
		pm[pm_id] = PCacheFvwmPicture(
			dpy, Scr.NoFocusWin, NULL, token, fpa);
		if (!pm[pm_id])
		{
			fvwm_msg(ERR, "ReadMultiPixmapDecor",
				 "Pixmap '%s' could not be loaded",
				 token);
		}
		s = GetNextTokenIndex(s, pm_names, 0, &pm_id);
	}

	if (!pm[TBP_MAIN] && !(pm[TBP_LEFT_MAIN] && pm[TBP_RIGHT_MAIN]))
	{
		fvwm_msg(ERR, "ReadMultiPixmapDecor",
			 "No Main pixmap found for TitleStyle MultiPixmap "
			 "(you must specify either Main, or both LeftMain"
			 " and RightMain)");
		for (i=0; i < NUM_TB_PIXMAPS; i++)
		{
			if (pm[i])
			{
				PDestroyFvwmPicture(dpy, pm[i]);
			}
		}
		free(pm);
		return NULL;
	}

	return s;
}
#endif /* FANCY_TITLEBARS */

/***********************************************************************
 *
 *  DestroyFvwmDecor -- frees all memory assocated with an FvwmDecor
 *      structure, but does not free the FvwmDecor itself
 *
 ************************************************************************/
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
#ifdef USEDECOR
	if (decor->tag)
	{
		free(decor->tag);
		decor->tag = NULL;
	}
#endif

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
		for (i = start; i < NUMBER_OF_BUTTONS; i += add)
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

/*****************************************************************************
 *
 * Changes a button decoration style (changes by veliaa@rpi.edu)
 *
 ****************************************************************************/
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
		for (i = start; i < NUMBER_OF_BUTTONS; i += add)
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
	char *text = NULL;
	char *prev = NULL;
	char *parm = NULL;
	TitleButton *tb = NULL;
#ifdef USEDECOR
	FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;
#else
	FvwmDecor *decor = &Scr.DefaultDecor;
#endif

	parm = PeekToken(action, &text);
	if (parm && isdigit(*parm))
	{
		button = atoi(parm);
		button = BUTTON_INDEX(button);
	}

	if (parm == NULL || button >= NUMBER_OF_BUTTONS || button < 0)
	{
		fvwm_msg(
			ERR, "ButtonStyle", "Bad button style (1) in line %s",
			action);
		return;
	}

	Scr.flags.do_need_window_update = 1;

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
			return;
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
		for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
		{
			if (((multi & 1) && !(i & 1)) ||
			    ((multi & 2) && (i & 1)))
			{
				TB_FLAGS(decor->buttons[i]).has_changed = 1;
			}
		}
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
						     i < NUMBER_OF_BUTTONS; ++i)
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
				for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
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

/* ---------------------------- interface functions ------------------------- */

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
#ifdef FANCY_TITLEBARS
	int i;
#endif

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

			p = (Pixel *)safemalloc(
				df->u.grad.npixels * sizeof(Pixel));
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
		if (df->u.p)
		{
			PDestroyFvwmPicture(dpy, df->u.p);
		}
		break;

#ifdef FANCY_TITLEBARS
	case MultiPixmap:
		if (df->u.multi_pixmaps)
		{
			for (i = 0; i < NUM_TB_PIXMAPS; i++)
			{
				if (df->u.multi_pixmaps[i])
				{
					PDestroyFvwmPicture(
						dpy, df->u.multi_pixmaps[i]);
				}
			}
			free(df->u.multi_pixmaps);
		}
		break;
#endif

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

/*****************************************************************************
 *
 * Reads a button face line into a structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
Bool ReadDecorFace(char *s, DecorFace *df, int button, int verbose)
{
	int offset;
	char style[256], *file;
	char *action = s;

	/* some variants of scanf do not increase the assign count when %n is
	 * used, so a return value of 1 is no error. */
	if (sscanf(s, "%256s%n", style, &offset) < 1)
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
			if (b >= 0 && b < NUMBER_OF_BUTTONS)
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

			if (num != 1 || num_coords<2 ||
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
			vc->x = safemalloc(sizeof(char) * num_coords);
			vc->y = safemalloc(sizeof(char) * num_coords);
			vc->c = safemalloc(sizeof(char) * num_coords);

			/* get the points */
			for (i = 0; i < vc->num; ++i)
			{
				int x;
				int y;
				int c;

				/* X x Y @ line_style */
				num = sscanf(
					s,"%dx%d@%d%n", &x, &y, &c, &offset);
				if (num != 3)
				{
					if (verbose)
					{
						fvwm_msg(
							ERR, "ReadDecorFace",
							"Bad button style (3)"
							" in line %s", action);
					}
					free(vc->x);
					free(vc->y);
					free(vc->c);
					vc->x = NULL;
					vc->y = NULL;
					vc->c = NULL;
					return False;
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
				if (c == 2 || c == 3)
				{
					vc->use_fgbg = 1;
				}
				s += offset;
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
			 || strncasecmp(style,"TiledPixmap",11)==0)
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
			else
			{
				DFS_FACE_TYPE(df->style) = PixmapButton;
			}
		}
#ifdef FANCY_TITLEBARS
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
#endif
		else if (FMiniIconsSupported &&
			 strncasecmp (style, "MiniIcon", 8) == 0)
		{
			memset(&df->style, 0, sizeof(df->style));
			DFS_FACE_TYPE(df->style) = MiniIconButton;
#if 0
			/* Have to remove this again. This is all so badly
			 * written there is no chance to prevent a coredump
			 * and a memory leak the same time without a rewrite of
			 * large parts of the code. */
			if (df->u.p)
			{
				PDestroyFvwmPicture(dpy, df->u.p);
			}
#endif
			/* pixmap read in when the window is created */
			df->u.p = NULL;
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

#ifdef USEDECOR
/*****************************************************************************
 *
 * Diverts a style definition to an FvwmDecor structure (veliaa@rpi.edu)
 *
 ****************************************************************************/
void AddToDecor(FvwmDecor *decor, char *s)
{
	if (!s)
	{
		return;
	}
	while (*s && isspace((unsigned char)*s))
	{
		++s;
	}
	if (!*s)
	{
		return;
	}
	Scr.cur_decor = decor;
	old_execute_function(NULL, s, NULL, &Event, C_ROOT, -1, 0, NULL);
	Scr.cur_decor = NULL;

	return;
}

/***********************************************************************
 *
 *  InitFvwmDecor -- initializes an FvwmDecor structure to defaults
 *
 ************************************************************************/
void InitFvwmDecor(FvwmDecor *decor)
{
	int i;
	DecorFace tmpdf;

	/* zero out the structures */
	memset(decor, 0, sizeof (FvwmDecor));
	memset(&tmpdf, 0, sizeof(DecorFace));

	/* initialize title-bar button styles */
	DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
	for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
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
#ifndef USEDECOR
	Scr.DefaultDecor.flags.has_changed = 0;
	Scr.DefaultDecor.flags.has_title_height_changed = 0;
#else
	FvwmDecor *decor;
	for (decor = &Scr.DefaultDecor; decor; decor = decor->next)
	{
		decor->flags.has_changed = 0;
		decor->flags.has_title_height_changed = 0;
	}
	/* todo: must reset individual change flags too */
#endif

	return;
}

/* ---------------------------- builtin commands ---------------------------- */

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
	int virtual_x, virtual_y;
	int pan_x, pan_y;
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
	x = x + val1 * val1_unit / 100;
	y = y + val2 * val2_unit / 100;
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
	if (virtual_x != Scr.Vx || virtual_y != Scr.Vy)
		MoveViewport(virtual_x, virtual_y, True);
	pan_x = (Scr.EdgeScrollX != 0) ? 2 : 0;
	pan_y = (Scr.EdgeScrollY != 0) ? 2 : 0;
	/* prevent paging if EdgeScroll is active */
	if (x >= Scr.MyDisplayWidth - pan_x)
	{
		x = Scr.MyDisplayWidth - pan_x -1;
	}
	else if (x < pan_x)
	{
		x = pan_x;
	}
	if (y >= Scr.MyDisplayHeight - pan_y)
	{
		y = Scr.MyDisplayHeight - pan_y - 1;
	}
	else if (y < pan_y)
	{
		y = pan_y;
	}

	FWarpPointer(dpy, None, Scr.Root, 0, 0, Scr.MyDisplayWidth,
		     Scr.MyDisplayHeight, x, y);
	return;
}

void CMD_Delete(F_CMD_ARGS)
{
	if (!is_function_allowed(F_DELETE, NULL, fw, True, True))
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
	if (XGetGeometry(dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		destroy_window(fw);
	}
	else
	{
		XKillClient(dpy, FW_W(fw));
	}
	XFlush(dpy);

	return;
}

void CMD_Close(F_CMD_ARGS)
{
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
	else if (XGetGeometry(
			 dpy, FW_W(fw), &JunkRoot, &JunkX, &JunkY,
			 &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	{
		destroy_window(fw);
	}
	else
	{
		XKillClient(dpy, FW_W(fw));
	}
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
			exec_shell_name = safestrdup(getenv("SHELL"));
		}
		else
		{
			/* if $SHELL not set, use default */
			exec_shell_name = safestrdup("/bin/sh");
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
		cmd = safestrdup(action);
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
	refresh_window(
		(context == C_ICON) ? FW_W_ICON_TITLE(fw) : FW_W_FRAME(fw),
		True);

	return;
}

void CMD_Wait(F_CMD_ARGS)
{
	Bool done = False;
	Bool redefine_cursor = False;
	Bool is_ungrabbed;
	char *escape;
	Window nonewin = None;
	extern FvwmWindow *Fw;
	char *wait_string, *rest;
	FvwmWindow *s_Fw = Fw;

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
			wait_string = safestrdup(temp);
			for (i = strlen(wait_string) - 1; i >= 0 &&
				     isspace(wait_string[i]); i--)
			{
				wait_string[i] = 0;
			}
		}
	}
	else
	{
		wait_string = safestrdup("");
	}

	is_ungrabbed = UngrabEm(GRAB_NORMAL);
	while (!done && !isTerminated)
	{
		if (BUSY_WAIT & Scr.BusyCursor)
		{
			XDefineCursor(dpy, Scr.Root, Scr.FvwmCursors[CRS_WAIT]);
			redefine_cursor = True;
		}
		if (My_XNextEvent(dpy, &Event))
		{
			dispatch_event(&Event, False);
			if (Event.type == MapNotify)
			{
				if (!*wait_string)
				{
					done = True;
				}
				if (Fw &&
				    matchWildcards(wait_string, Fw->name.name)
				    == True)
				{
					done = True;
				}
				else if (Fw && Fw->class.res_class &&
					 matchWildcards(
						 wait_string,
						 Fw->class.res_class) == True)
				{
					done = True;
				}
				else if (Fw && Fw->class.res_name &&
					 matchWildcards(
						 wait_string,
						 Fw->class.res_name) == True)
				{
					done = True;
				}
			}
			else if (Event.type == KeyPress)
			{
				escape = CheckBinding(
					Scr.AllBindings, STROKE_ARG(0)
					Event.xkey.keycode,
					Event.xkey.state, GetUnusedModifiers(),
					GetContext(Fw, &Event, &nonewin),
					KEY_BINDING);
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
	Fw = (check_if_fvwm_window_exists(s_Fw)) ? s_Fw : NULL;
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
	unsigned int len;

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
	fvwm_msg(ECHO,"Echo",action);

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
#ifdef USEDECOR
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
#endif
	action = GetNextToken(action, &fore);
	GetNextToken(action, &back);
	if (fore && back)
	{
		action = safemalloc(strlen(fore) + strlen(back) + 29);
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

#ifdef USEDECOR
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
#endif
	if (action)
	{
		newaction = safemalloc(strlen(action) + 32);
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

/*****************************************************************************
 *
 * Appends a titlestyle (veliaa@rpi.edu)
 *
 ****************************************************************************/
void CMD_AddTitleStyle(F_CMD_ARGS)
{
	do_title_style(F_PASS_ARGS, True);

	return;
}

void CMD_Colorset(F_CMD_ARGS)
{
	int n;
	char *token;

	if (GetIntegerArguments(action, &token, &n, 1) != 1)
	{
		return;
	}
	if (n < 0)
	{
		return;
	}
	if (token == NULL)
	{
		return;
	}
	parse_colorset(n, token);
	BroadcastColorset(n);
	if (n == Scr.DefaultColorset)
	{
		Scr.flags.do_need_window_update = 1;
		Scr.flags.has_default_color_changed = 1;
	}
	UpdateMenuColorset(n);
	update_style_colorset(n);

	return;
}

void CMD_CleanupColorsets(F_CMD_ARGS)
{
	cleanup_colorsets();
}

void CMD_PropertyChange(F_CMD_ARGS)
{
	char string[256] = "\0";
	int ret;
	unsigned long argument, data1 = 0, data2 = 0;

	if (action == NULL || action == "\0")
	{
		return;
	}
	ret = sscanf(
		action,"%lu %lu %lu %255c", &argument, &data1, &data2, string);
	if (ret < 1)
	{
		return;
	}
	BroadcastPropertyChange(
		argument, data1, data2, (string == NULL)? "":string);

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
		back = safestrdup(DEFAULT_BACK_COLOR);
	}
	if (!fore)
	{
		fore = safestrdup(DEFAULT_FORE_COLOR);
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

	font = PeekToken(action, &action);
	if (!font)
	{
		/* Try 'fixed', pass NULL font name */
	}
	if (!(new_font = FlocaleLoadFont(dpy, font, "FVWM")))
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
	/* set flags to indicate that the font has changed */
	Scr.flags.do_need_window_update = 1;
	Scr.flags.has_default_font_changed = 1;

	return;
}

void CMD_IconFont(F_CMD_ARGS)
{
	char *newaction;

#ifdef USEDECOR
	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "LoadIconFont",
			"Decors do not support the IconFont command anymore."
			" Please use 'Style <stylename> IconFont <fontname>'"
			" instead.  Sorry for the inconvenience.");
		return;
	}
#endif
	if (action)
	{
		newaction = safemalloc(strlen(action) + 16);
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

#ifdef USEDECOR
	if (Scr.cur_decor && Scr.cur_decor != &Scr.DefaultDecor)
	{
		fvwm_msg(
			ERR, "LoadWindowFont",
			"Decors do not support the WindowFont command anymore."
			" Please use 'Style <stylename> Font <fontname>'"
			" instead.  Sorry for the inconvenience.");
		return;
	}
#endif
	if (action)
	{
		newaction = safemalloc(strlen(action) + 16);
		sprintf(newaction, "* Font %s", action);
		action = newaction;
		CMD_Style(F_PASS_ARGS);
		free(newaction);
	}

	return;
}

/*****************************************************************************
 *
 * Changes the window's FvwmDecor pointer (veliaa@rpi.edu)
 *
 ****************************************************************************/
void CMD_ChangeDecor(F_CMD_ARGS)
{
	char *item;
	int old_height;
	int extra_height;
	FvwmDecor *decor = &Scr.DefaultDecor;
	FvwmDecor *found = NULL;

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
	old_height = (fw->decor) ? fw->decor->title_height : 0;
	fw->decor = found;
	apply_decor_change(fw);
	extra_height = (HAS_TITLE(fw) && old_height) ?
		(old_height - fw->decor->title_height) : 0;
	frame_force_setup_window(
		fw, fw->frame_g.x, fw->frame_g.y, fw->frame_g.width,
		fw->frame_g.height - extra_height, True);
	border_draw_decorations(
		fw, PART_ALL, (Scr.Hilite == fw), 2, CLEAR_ALL, NULL, NULL);

	return;
}

/*****************************************************************************
 *
 * Destroys an FvwmDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
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
		FvwmWindow *fw2;

		if (!do_recreate)
		{
			__remove_window_decors(found);
		}
		DestroyFvwmDecor(found);
		if (do_recreate)
		{
			int i;

			InitFvwmDecor(found);
			found->tag = safestrdup(item);
			Scr.flags.do_need_window_update = 1;
			found->flags.has_changed = 1;
			found->flags.has_title_height_changed = 0;
			found->titlebar.flags.has_changed = 1;
			for (i = 0; i < NUMBER_OF_BUTTONS; ++i)
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

/*****************************************************************************
 *
 * Initiates an AddToDecor (veliaa@rpi.edu)
 *
 ****************************************************************************/
void CMD_AddToDecor(F_CMD_ARGS)
{
	FvwmDecor *decor, *found = NULL;
	char *item = NULL, *s = action;

	s = GetNextToken(s, &item);

	if (!item)
	{
		return;
	}
	if (!s)
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
		found = (FvwmDecor *)safemalloc(sizeof( FvwmDecor ));
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
		AddToDecor(found, s);
		/* Set + state to last decor */
		set_last_added_item(ADDED_DECOR, found);
	}

	return;
}
#endif /* USEDECOR */


/*****************************************************************************
 *
 * Updates window decoration styles (veliaa@rpi.edu)
 *
 ****************************************************************************/
void CMD_UpdateDecor(F_CMD_ARGS)
{
	FvwmWindow *fw2;
#ifdef USEDECOR
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
#endif

	for (fw2 = Scr.FvwmRoot.next; fw2; fw2 = fw2->next)
	{
#ifdef USEDECOR
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
#endif
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

/*****************************************************************************
 *
 * Appends a button decoration style (veliaa@rpi.edu)
 *
 ****************************************************************************/
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

	action = GetNextToken(action,&szVar);
	if (!szVar)
	{
		return;
	}
	action = GetNextToken(action,&szValue);
	if (!szValue)
	{
		/* no value, treat as unset */
		CMD_UnsetEnv(F_PASS_ARGS);
		free(szVar);
		return;
	}
	szPutenv = safemalloc(strlen(szVar)+strlen(szValue)+2);
	sprintf(szPutenv,"%s=%s",szVar,szValue);
	putenv(szPutenv);
	add_to_env_list(szVar, szPutenv);
	/* szVar is stored in the env list. do not free it */
	free(szValue);

	return;
}

void CMD_UnsetEnv(F_CMD_ARGS)
{
	char *szVar = NULL;
	char *szPutenv = NULL;

	action = GetNextToken(action,&szVar);
	if (!szVar)
	{
		return;
	}
	szPutenv = (char *)safemalloc(strlen(szVar) + 2);
	sprintf(szPutenv, "%s=", szVar);
	putenv(szPutenv);
	add_to_env_list(szVar, szPutenv);
	/* szVar is stored in the env list. do not free it */

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
	char delim;
	int toggle;

	/* fvwm_msg(DBG,"SetGlobalOptions","init action == '%s'\n",action); */
	while (action)
	{
		action = DoGetNextToken(action, &opt, NULL, ",", &delim);
		if (!opt)
		{
			/* no more options */
			return;
		}
		if (delim == '\n' || delim == ',')
		{
			/* missing toggle argument */
			toggle = 2;
		}
		else
		{
			toggle = ParseToggleArgument(action, &action, 1, False);
		}

		if (StrEquals(opt, "FlickeringMoveWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.DisableConfigureNotify ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.DisableConfigureNotify = toggle;
				break;
			default:
				Scr.bo.DisableConfigureNotify = 0;
				break;
			}
		}
		else if (StrEquals(opt, "MixedVisualWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.InstallRootCmap ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.InstallRootCmap = toggle;
				break;
			default:
				Scr.bo.InstallRootCmap = 0;
				break;
			}
		}
		else if (StrEquals(opt, "ModalityIsEvil"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.ModalityIsEvil ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.ModalityIsEvil = toggle;
				break;
			default:
				Scr.bo.ModalityIsEvil = 0;
				break;
			}
			if (Scr.bo.ModalityIsEvil)
			{
				SetMWM_INFO(Scr.NoFocusWin);
			}
		}
		else if (StrEquals(opt, "RaiseOverNativeWindows"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.RaiseHackNeeded ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.RaiseHackNeeded = toggle;
				break;
			default:
				Scr.bo.RaiseHackNeeded = 0;
				break;
			}
		}
		else if (StrEquals(opt, "RaiseOverUnmanaged"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.RaiseOverUnmanaged ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.RaiseOverUnmanaged = toggle;
				break;
			default:
				Scr.bo.RaiseOverUnmanaged = 0;
				break;
			}
		}
		else if (StrEquals(opt, "FlickeringQtDialogsWorkaround"))
		{
			switch (toggle)
			{
			case -1:
				Scr.bo.FlickeringQtDialogsWorkaround ^= 1;
				break;
			case 0:
			case 1:
				Scr.bo.FlickeringQtDialogsWorkaround = toggle;
				break;
			default:
				Scr.bo.FlickeringQtDialogsWorkaround = 0;
				break;
			}
		}
		else if (EWMH_BugOpts(opt, toggle))
		{
			/* work is done in EWMH_BugOpts */
		}
		else
		{
			fvwm_msg(ERR, "SetBugOptions",
				 "Unknown Bug Option '%s'", opt);
		}
		free(opt);
	}

	return;
}

void CMD_Emulate(F_CMD_ARGS)
{
	char *style;

	style = PeekToken(action, NULL);
	if (!style || StrEquals(style, "fvwm"))
	{
		Scr.gs.EmulateMWM = False;
		Scr.gs.EmulateWIN = False;
	}
	else if (StrEquals(style, "mwm"))
	{
		Scr.gs.EmulateMWM = True;
		Scr.gs.EmulateWIN = False;
	}
	else if (StrEquals(style, "win"))
	{
		Scr.gs.EmulateMWM = False;
		Scr.gs.EmulateWIN = True;
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

void CMD_FakeClick(F_CMD_ARGS)
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

	/* get the mask of pressed/released buttons */
	if (FQueryPointer(
		    dpy, Scr.Root, &root, &JunkRoot, &JunkX, &JunkY, &JunkX,
		    &JunkY, &mask) == False)
	{
		/* pointer is on a different screen - that's okay here */
	}
	token = PeekToken(action, &action);
	while (token && action)
	{
		int index = GetTokenIndex(token, optlist, 0, NULL);
		int val;
		XEvent e;
		Window w;
		Window child_w;
		int x = 0;
		int y = 0;
		int rx = 0;
		int ry = 0;
		Bool do_unset;
		long add_mask = 0;

		XFlush(dpy);
		if (GetIntegerArguments(action, &action, &val, 1) != 1)
		{
			/* error */
			return;
		}
		do_unset = True;
		switch (index)
		{
		case 0:
		case 1:
			do_unset = False;
			/* fall through */
		case 2:
		case 3:
			/* button press or release */
			if (val >= 1 && val <= NUMBER_OF_MOUSE_BUTTONS)
			{
				int depth = 1;

				w = None;
				child_w = root;
				for (depth = 1; depth != maxdepth &&
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
				if (do_unset)
				{
					e.type = ButtonRelease;
					add_mask = ButtonPressMask;
				}
				else
				{
					e.type = ButtonPress;
					add_mask = ButtonPressMask |
						ButtonReleaseMask;
				}
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
				FSendEvent(
					dpy, PointerWindow, True,
					SubstructureNotifyMask | add_mask, &e);
				XFlush(dpy);
				if (do_unset)
				{
					mask &= ~(Button1Mask << (val - 1));
				}
				else
				{
					mask |= (Button1Mask << (val - 1));
				}
			}
			else
			{
				/* error */
				return;
			}
			break;
		case 4:
		case 5:
			/* wait */
			if (val > 0 && val <= 1000000)
			{
				usleep(1000 * val);
				if (FQueryPointer(
					    dpy, Scr.Root, &root, &JunkRoot,
					    &JunkX, &JunkY, &JunkX, &JunkY,
					    &mask) == False)
				{
					/* pointer is on a different screen -
					 * that's okay here */
				}
			}
			else
			{
				/* error */
				return;
			}
			break;
		case 6:
		case 7:
			/* set modifier */
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
			maxdepth = val;
			break;
		default:
			/* error */
			return;
		}
		if (action)
		{
			token = PeekToken(action, &action);
		}
	}

	return;
}

/* A function to handle stroke (olicha Nov 11, 1999) */
#ifdef HAVE_STROKE
void CMD_StrokeFunc(F_CMD_ARGS)
{
	int finished = 0;
	int abort = 0;
	int modifiers = eventp->xbutton.state;
	int start_event_type = eventp->type;
	char sequence[STROKE_MAX_SEQUENCE + 1];
	char *stroke_action;
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

	if (!GrabEm(CRS_STROKE, GRAB_NORMAL))
	{
		XBell(dpy, 0);
		return;
	}
	x = (int*)safemalloc(coords_size * sizeof(int));
	y = (int*)safemalloc(coords_size * sizeof(int));
	e = *eventp;
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
	 * Some computers do not support keyRelease events, can we
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
			if (draw_motion)
			{
				if ((x[i] != tmpx || y[i] != tmpy))
				{
					i++;
					if (i >= coords_size)
					{
						coords_size +=
							STROKE_CHUNK_SIZE;
						x = (int*)saferealloc(
							(void *)x, coords_size *
							sizeof(int));
						y = (int*)saferealloc(
							(void *)y, coords_size *
							sizeof(int));
						if (!x || !y)
						{
							/* unlikely */
							fvwm_msg(WARN,
								 "strokeFunc",
								 "unable to"
								 " allocate %d"
								 " bytes ..."
								 " aborted.",
								 coords_size *
								 sizeof(int));
							abort = 1;
							i = -1; /* no undrawing
								 * possible
								 * since
								 * either x or
								 * y == 0 */
							break;
						}
					}
					x[i] = tmpx;
					y[i] = tmpy;
					XDrawLine(
						dpy, Scr.Root, Scr.XorGC,
						x[i-1], y[i-1], x[i], y[i]);
				}
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

		for (i=0;sequence[i] != '\0';i++)
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

	/* check for a binding */
	stroke_action = CheckBinding(
		Scr.AllBindings, sequence, 0, modifiers, GetUnusedModifiers(),
		context, STROKE_BINDING);

	/* execute the action */
	if (stroke_action != NULL)
	{
		if (feed_back && atoi(sequence) != 0)
		{
			GrabEm(CRS_WAIT, GRAB_BUSY);
			usleep(200000);
			UngrabEm(GRAB_BUSY);
		}
		old_execute_function(
			NULL, stroke_action, fw, &e, context, -1, 0, NULL);
	}

	return;
}
#endif /* HAVE_STROKE */

void CMD_State(F_CMD_ARGS)
{
	unsigned int state;
	int toggle;
	int n;

	n = GetIntegerArguments(action, &action, &state, 1);
	if (n <= 0)
	{
		return;
	}
	if (state < 0 || state > 31)
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
