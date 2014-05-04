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

/* IMPORTANT NOTE:
 *
 * The functions in this module *must not* assume that the geometries in the
 * FvwmWindow structure reflect the desired geometry of the window or its
 * parts.  While the window is resized or shaded, they may hold the old
 * geometry instead of the new one (but you can not rely on this).  Therefore,
 * these geometries must not be accessed directly or indirectly (by the
 * functions from geometry,c).  Use the geometries that are passed in via
 * structure pointers, e.d. "td".
 */

/* ---------------------------- included header files ---------------------- */

#include "config.h"

#include <stdio.h>

#include "libs/fvwmlib.h"
#include "libs/Parse.h"
#include "libs/Graphics.h"
#include "libs/Picture.h"
#include "libs/PictureGraphics.h"
#include "libs/FRenderInit.h"
#include "libs/charmap.h"
#include "libs/wcontext.h"
#include "fvwm.h"
#include "execcontext.h"
#include "externs.h"
#include "misc.h"
#include "screen.h"
#include "geometry.h"
#include "borders.h"
#include "builtins.h"
#include "icons.h"
#include "frame.h"

/* ---------------------------- local definitions -------------------------- */

/* ---------------------------- local macros ------------------------------- */

#define SWAP_ARGS(f,a1,a2) (f)?(a2):(a1),(f)?(a1):(a2)

/* ---------------------------- imports ------------------------------------ */

extern Window PressedW;

/* ---------------------------- included code files ------------------------ */

/* ---------------------------- local types -------------------------------- */

typedef struct
{
	struct
	{
		unsigned use_pixmap : 1;
	} flags;
	Pixel pixel;
	struct
	{
		Pixmap p;
		Pixmap shape;
		Pixmap alpha;
		int depth;
		FvwmRenderAttributes fra;
		rectangle g;
		int stretch_w;
		int stretch_h;
		struct
		{
			unsigned is_tiled : 1;
			unsigned is_stretched : 1;
		} flags;
	} pixmap;
} pixmap_background_type;

typedef struct
{
	Pixmap p;
	FvwmPicture *mp_created_pic;
	int cs;
	FvwmPicture *mp_pic;
	int mp_part;
	Bool created;
} bar_pixmap;

typedef struct
{
	int count;
	bar_pixmap *bps;
} bar_bs_pixmaps;    /* for UseTitleStyle & Colorset */

typedef struct
{
	Pixmap frame_pixmap;
	bar_bs_pixmaps bar_pixmaps[BS_MaxButtonState];
} dynamic_common_decorations;

typedef struct
{
	int relief_width;
	GC relief_gc;
	GC shadow_gc;
	Pixel fore_color;
	Pixel back_color;
	int cs;
	int border_cs;		/* for UseBorderStyle */
	int bg_border_cs;	/* for UseBorderStyle */
	Pixmap back_pixmap;
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	Pixmap texture_pixmap;
	int texture_pixmap_width;
	int texture_pixmap_height;
	XSetWindowAttributes notex_attributes;
	unsigned long notex_valuemask;
	dynamic_common_decorations dynamic_cd;
} common_decorations_type;

typedef struct
{
	GC relief;
	GC shadow;
	GC transparent;
} draw_border_gcs;

typedef struct
{
	int offset_tl;
	int offset_br;
	int thickness;
	int length;
	unsigned has_x_marks : 1;
	unsigned has_y_marks : 1;
} border_marks_descr;

typedef struct
{
	int w_dout;
	int w_hiout;
	int w_trout;
	int w_c;
	int w_trin;
	int w_shin;
	int w_din;
	int sum;
	int trim;
	unsigned is_flat : 1;
} border_relief_size_descr;

typedef struct
{
	rectangle sidebar_g;
	border_relief_size_descr relief;
	border_marks_descr marks;
	draw_border_gcs gcs;
} border_relief_descr;

typedef struct
{
	unsigned pressed_bmask : NUMBER_OF_TITLE_BUTTONS;
	unsigned lit_bmask : NUMBER_OF_TITLE_BUTTONS;
	unsigned toggled_bmask : NUMBER_OF_TITLE_BUTTONS;
	unsigned clear_bmask : NUMBER_OF_TITLE_BUTTONS;
	unsigned draw_bmask : NUMBER_OF_TITLE_BUTTONS;
	unsigned max_bmask : NUMBER_OF_TITLE_BUTTONS;
	ButtonState bstate[NUMBER_OF_TITLE_BUTTONS];
	unsigned is_title_pressed : 1;
	unsigned is_title_lit : 1;
	unsigned do_clear_title : 1;
	ButtonState tstate;
} border_titlebar_state;

typedef struct
{
	GC rgc;
	GC sgc;
	FlocaleWinString fstr;
	DecorFaceStyle *tstyle;
	DecorFace *df;
	unsigned is_toggled : 1;
} title_draw_descr;

typedef struct
{
	common_decorations_type *cd;
	rectangle frame_g;
	rectangle bar_g;		/* titlebar geo vs the frame */
	rectangle left_buttons_g;	/* vs the frame */
	rectangle right_buttons_g;	/* vs the frame */
	frame_title_layout_t layout;
	frame_title_layout_t old_layout;
	border_titlebar_state tbstate;
	int length;			/* text */
	int offset;			/* text offset */
	/* MultiPixmap Geometries */
	rectangle under_text_g;		/* vs the titlebar */
	rectangle left_main_g;		/* vs the titlebar */
	rectangle right_main_g;		/* vs the titlebar */
	rectangle full_left_main_g;	/* vs the frame */
	rectangle full_right_main_g;	/* vs the frame */
	int left_end_length;
	int left_of_text_length;
	int right_end_length;
	int right_of_text_length;
	rotation_t draw_rotation;
	rotation_t restore_rotation;
	unsigned td_is_rotated : 1;
	unsigned has_been_saved : 1;
	unsigned has_vt : 1;		/* vertical title ? */
	unsigned has_an_upsidedown_rotation : 1;  /* 270 || 180 */
} titlebar_descr;

/* ---------------------------- forward declarations ----------------------- */
/*  forward declarations are not so good */

/* for grouping titlebar_descr computation */
static void border_rotate_titlebar_descr(FvwmWindow *fw, titlebar_descr *td);

/* for grouping the MultiPixmap stuff */
static Bool border_mp_get_use_title_style_parts_and_geometry(
	titlebar_descr *td, FvwmPicture **pm, FvwmAcs *acs,
	unsigned short sf, int is_left, rectangle *g, int *part);

/* ---------------------------- local variables ---------------------------- */

static const char ulgc[] = { 1, 0, 0, 0x7f, 2, 1, 1 };
static const char brgc[] = { 1, 1, 2, 0x7f, 0, 0, 3 };

/* ---------------------------- exported variables (globals) --------------- */

XGCValues Globalgcv;
unsigned long Globalgcm;

/* ---------------------------- local functions ---------------------------- */

static Bool is_button_toggled(
	FvwmWindow *fw, int button)
{
	mwm_flags mf;

	if (!HAS_MWM_BUTTONS(fw))
	{
		return False;
	}
	mf = TB_MWM_DECOR_FLAGS(GetDecor(fw, buttons[button]));
	if ((mf & MWM_DECOR_MAXIMIZE) && IS_MAXIMIZED(fw))
	{
		return True;
	}
	if ((mf & MWM_DECOR_SHADE) && IS_SHADED(fw))
	{
		return True;
	}
	if ((mf & MWM_DECOR_STICK) &&
	    (IS_STICKY_ACROSS_PAGES(fw) || IS_STICKY_ACROSS_DESKS(fw)))
	{
		return True;
	}
	if (TB_FLAGS(fw->decor->buttons[button]).has_layer &&
	    fw->layer == TB_LAYER(fw->decor->buttons[button]))
	{
		return True;
	}

	return False;
}


/* rules to get button state */
static ButtonState border_flags_to_button_state(
	int is_pressed, int is_lit, int is_toggled)
{
	if (!is_lit && Scr.gs.use_inactive_buttons)
	{
		if (is_pressed && Scr.gs.use_inactive_down_buttons)
		{
			return (is_toggled) ?
				BS_ToggledInactiveDown : BS_InactiveDown;
		}
		else
		{
			return (is_toggled) ?
				BS_ToggledInactiveUp : BS_InactiveUp;
		}
	}
	else
	{
		if (is_pressed && Scr.gs.use_active_down_buttons)
		{
			return (is_toggled) ?
				BS_ToggledActiveDown : BS_ActiveDown;
		}
		else
		{
			return (is_toggled) ?
				BS_ToggledActiveUp : BS_ActiveUp;
		}
	}
}

static void get_common_decorations(
	common_decorations_type *cd, FvwmWindow *t,
	window_parts draw_parts, Bool has_focus, Bool is_border,
	Bool do_change_gcs)
{
	DecorFace *df;
	color_quad *draw_colors;

	df = border_get_border_style(t, has_focus);
	cd->bg_border_cs = -1;
	cd->cs = -1;
	if (has_focus)
	{
		/* are we using textured borders? */
		if (DFS_FACE_TYPE(df->style) == TiledPixmapButton &&
		    GetDecor(t, BorderStyle.active.u.p->depth) == Pdepth)
		{
			cd->texture_pixmap = GetDecor(
				t, BorderStyle.active.u.p->picture);
			cd->texture_pixmap_width = GetDecor(
				t, BorderStyle.active.u.p->width);
			cd->texture_pixmap_height = GetDecor(
				t, BorderStyle.active.u.p->height);
		}
		else if (DFS_FACE_TYPE(df->style) == ColorsetButton)
		{
			cd->bg_border_cs = GetDecor(
				t, BorderStyle.active.u.acs.cs);
		}
		cd->back_pixmap = Scr.gray_pixmap;
		if (is_border)
		{
			draw_colors = &(t->border_hicolors);
			cd->cs = t->border_cs_hi;
		}
		else
		{
			draw_colors = &(t->hicolors);
			cd->cs = t->cs_hi;
		}
	}
	else
	{
		if (DFS_FACE_TYPE(df->style) == TiledPixmapButton &&
		    GetDecor(t, BorderStyle.inactive.u.p->depth) == Pdepth)
		{
			cd->texture_pixmap = GetDecor(
				t, BorderStyle.inactive.u.p->picture);
			cd->texture_pixmap_width = GetDecor(
				t, BorderStyle.inactive.u.p->width);
			cd->texture_pixmap_height = GetDecor(
				t, BorderStyle.inactive.u.p->height);
		}
		else if (DFS_FACE_TYPE(df->style) == ColorsetButton)
		{
			cd->bg_border_cs = GetDecor(
				t, BorderStyle.inactive.u.acs.cs);
		}
		if (IS_STICKY_ACROSS_PAGES(t) || IS_STICKY_ACROSS_DESKS(t))
		{
			cd->back_pixmap = Scr.sticky_gray_pixmap;
		}
		else
		{
			cd->back_pixmap = Scr.light_gray_pixmap;
		}
		if (is_border)
		{
			draw_colors = &(t->border_colors);
			cd->cs = t->border_cs;
		}
		else
		{
			draw_colors = &(t->colors);
			cd->cs = t->cs;
		}
	}
	cd->fore_color = draw_colors->fore;
	cd->back_color = draw_colors->back;
	if (do_change_gcs)
	{
		Globalgcv.foreground = draw_colors->hilight;
		Globalgcm = GCForeground;
		XChangeGC(dpy, Scr.ScratchGC1, Globalgcm, &Globalgcv);
		Globalgcv.foreground = draw_colors->shadow;
		XChangeGC(dpy, Scr.ScratchGC2, Globalgcm, &Globalgcv);
		cd->relief_gc = Scr.ScratchGC1;
		cd->shadow_gc = Scr.ScratchGC2;
	}

	/* MWMBorder style means thin 3d effects */
	cd->relief_width = (HAS_MWM_BORDER(t) ? 1 : 2);

	if (cd->texture_pixmap)
	{
		cd->attributes.background_pixmap = cd->texture_pixmap;
		cd->valuemask = CWBackPixmap;
	}
	else
	{
		if (Pdepth < 2)
		{
			cd->attributes.background_pixmap = cd->back_pixmap;
			cd->valuemask = CWBackPixmap;
		}
		else
		{
			cd->attributes.background_pixel = cd->back_color;
			cd->valuemask = CWBackPixel;
		}
	}
	if (Pdepth < 2)
	{
		cd->notex_attributes.background_pixmap = cd->back_pixmap;
		cd->notex_valuemask = CWBackPixmap;
	}
	else
	{
		cd->notex_attributes.background_pixel = cd->back_color;
		cd->notex_valuemask = CWBackPixel;
	}

	return;
}

static window_parts border_get_changed_border_parts(
	FvwmWindow *fw, rectangle *old_sidebar_g, rectangle *new_sidebar_g,
	int cs)
{
	window_parts changed_parts;

	changed_parts = PART_NONE;
	if (!CSET_IS_TRANSPARENT_PR(cs) && CSET_HAS_PIXMAP(cs) &&
	    (old_sidebar_g->x != new_sidebar_g->x ||
	     old_sidebar_g->y != new_sidebar_g->y ||
	     old_sidebar_g->width != new_sidebar_g->width ||
	     old_sidebar_g->height != new_sidebar_g->height))
	{
		/* optimizable? */
		changed_parts |= PART_FRAME;
		return changed_parts;
	}
	if (old_sidebar_g->x != new_sidebar_g->x)
	{
		changed_parts |= (PART_FRAME & (~PART_BORDER_W));
	}
	if (old_sidebar_g->y != new_sidebar_g->y)
	{
		changed_parts |= (PART_FRAME & (~PART_BORDER_N));
	}
	if (old_sidebar_g->width != new_sidebar_g->width)
	{
		changed_parts |=
			PART_BORDER_N | PART_BORDER_S;
		if (DFS_FACE_TYPE(GetDecor(fw, BorderStyle.active.style)) ==
		    TiledPixmapButton)
		{
			changed_parts |=
				PART_BORDER_NE | PART_BORDER_E | PART_BORDER_SE;
		}
	}
	if (old_sidebar_g->height != new_sidebar_g->height)
	{
		changed_parts |=
			PART_BORDER_W | PART_BORDER_E;
		if (DFS_FACE_TYPE(GetDecor(fw, BorderStyle.active.style)) ==
		    TiledPixmapButton)
		{
			changed_parts |=
				PART_BORDER_SW | PART_BORDER_S | PART_BORDER_SE;
		}
	}

	return changed_parts;
}

static int border_get_parts_and_pos_to_draw(
	common_decorations_type *cd, FvwmWindow *fw,
	window_parts pressed_parts, window_parts force_draw_parts,
	rectangle *old_g, rectangle *new_g, Bool do_hilight,
	border_relief_descr *br)
{
	window_parts draw_parts;
	window_parts parts_to_light;
	rectangle sidebar_g_old;
	DecorFaceStyle *borderstyle;
	Bool has_x_marks;
	Bool has_x_marks_old;
	Bool has_y_marks;
	Bool has_y_marks_old;
	int cs = cd->bg_border_cs;

	draw_parts = 0;
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	frame_get_sidebar_geometry(
		fw, borderstyle,  new_g, &br->sidebar_g, &has_x_marks,
		&has_y_marks);
	if (has_x_marks == True)
	{
		draw_parts |= PART_X_HANDLES;
		br->marks.has_x_marks = 1;
	}
	else
	{
		br->marks.has_x_marks = 0;
	}
	if (has_y_marks == True)
	{
		draw_parts |= PART_Y_HANDLES;
		br->marks.has_y_marks = 1;
	}
	else
	{
		br->marks.has_y_marks = 0;
	}
	draw_parts |= (pressed_parts ^ fw->decor_state.parts_inverted);
	parts_to_light = (do_hilight == True) ? PART_FRAME : PART_NONE;
	draw_parts |= (parts_to_light ^ fw->decor_state.parts_lit);
	draw_parts |= (~(fw->decor_state.parts_drawn) & PART_FRAME);
	draw_parts |= force_draw_parts;
	if (old_g == NULL)
	{
		old_g = &fw->g.frame;
	}
	if ((draw_parts & PART_FRAME) == PART_FRAME)
	{
		draw_parts |= PART_FRAME;
		return draw_parts;
	}
	frame_get_sidebar_geometry(
		fw, borderstyle, old_g, &sidebar_g_old, &has_x_marks_old,
		&has_y_marks_old);
	if (has_x_marks_old != has_x_marks)
	{
		draw_parts |= (PART_FRAME & (~(PART_BORDER_N | PART_BORDER_S)));
	}
	if (has_y_marks_old != has_y_marks)
	{
		draw_parts |= (PART_FRAME & (~(PART_BORDER_W | PART_BORDER_E)));
	}
	draw_parts |= border_get_changed_border_parts(
		fw, &sidebar_g_old, &br->sidebar_g, cs);
	draw_parts &= (PART_FRAME | PART_HANDLES);

	return draw_parts;
}

static window_parts border_get_tb_parts_to_draw(
	FvwmWindow *fw, titlebar_descr *td, rectangle *old_g, rectangle *new_g,
	window_parts force_draw_parts)
{
	window_parts draw_parts;
	ButtonState old_state;
	int i;
	DecorFace *df,*tdf;

	td->tbstate.draw_bmask = 0;
	draw_parts = PART_NONE;
	/* first time? */
	draw_parts |= (~(fw->decor_state.parts_drawn) & PART_TITLE);
	td->tbstate.draw_bmask |= (~(fw->decor_state.buttons_drawn));
	/* forced? */
	draw_parts |= force_draw_parts;
	td->tbstate.draw_bmask |= (force_draw_parts & PART_BUTTONS) ? ~0 : 0;
	/* check if state changed */
	old_state = border_flags_to_button_state(
		(fw->decor_state.parts_inverted & PART_TITLE),
		(fw->decor_state.parts_lit & PART_TITLE), 0);
	if (old_state != td->tbstate.tstate)
	{
		draw_parts |= PART_TITLE;
	}
	/* size changed? */
	if ((td->old_layout.title_g.width != td->layout.title_g.width ||
	     td->old_layout.title_g.height != td->layout.title_g.height) &&
	    td->layout.title_g.x >= 0 && td->layout.title_g.y >= 0)
	{
		draw_parts |= PART_TITLE;
	}
	/* same for buttons */
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		unsigned int mask = (1 << i);

		if (FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		old_state = border_flags_to_button_state(
			(fw->decor_state.buttons_inverted & mask),
			(fw->decor_state.buttons_lit & mask),
			(fw->decor_state.buttons_toggled & mask));
		if (old_state != td->tbstate.bstate[i])
		{
			draw_parts |= PART_BUTTONS;
			td->tbstate.draw_bmask |= mask;
		}
		if ((td->old_layout.button_g[i].width !=
		     td->layout.button_g[i].width ||
		     td->old_layout.button_g[i].height !=
		     td->layout.button_g[i].height) &&
		    td->layout.button_g[i].x >= 0 &&
		    td->layout.button_g[i].y >= 0)
		{
			draw_parts |= PART_BUTTONS;
			td->tbstate.draw_bmask |= mask;
		}
	}
	/* position changed and background is tiled or a cset? */
	if ((draw_parts & PART_TITLE) == PART_NONE &&
	    td->layout.title_g.x >= 0 &&
	    td->layout.title_g.y >= 0)
	{
		df = &TB_STATE(GetDecor(fw, titlebar))[td->tbstate.tstate];
		if (DFS_USE_BORDER_STYLE(df->style) &&
		    (((td->old_layout.title_g.x != td->layout.title_g.x ||
		       td->old_layout.title_g.y != td->layout.title_g.y) &&
		      ((td->cd->valuemask & CWBackPixmap) ||
		       CSET_PIXMAP_IS_TILED(td->cd->bg_border_cs)))
		     ||
		     (old_g->width != new_g->width &&
		      CSET_PIXMAP_IS_X_STRETCHED(td->cd->bg_border_cs))
		     ||
		     (old_g->height != new_g->height
		      && CSET_PIXMAP_IS_Y_STRETCHED(td->cd->bg_border_cs))
		     ||
		     ((old_g->x != new_g->x || old_g->y != new_g->y)
		      && CSET_IS_TRANSPARENT_ROOT(td->cd->bg_border_cs))))
		{
				draw_parts |= PART_TITLE;
		}
		if ((draw_parts & PART_TITLE) == PART_NONE &&
		    (old_g->x != new_g->x || old_g->y != new_g->y))
		{
			for (tdf = df; tdf != NULL; tdf = tdf->next)
			{
				if (DFS_FACE_TYPE(tdf->style) ==
				    ColorsetButton &&
				    CSET_IS_TRANSPARENT_ROOT(tdf->u.acs.cs))
				{
					draw_parts |= PART_TITLE;
					break;
				}
			}
		}
	}
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		unsigned int mask;
		DecorFaceStyle *bs;

		mask = (1 << i);
		bs = &TB_STATE(
			GetDecor(fw, buttons[i]))[td->tbstate.bstate[i]].style;
		if ((td->tbstate.draw_bmask & mask) ||
		    td->layout.button_g[i].x < 0 ||
		    td->layout.button_g[i].y < 0)
		{
			continue;
		}
		if (DFS_USE_BORDER_STYLE(*bs) &&
		    (((td->old_layout.button_g[i].x !=
		       td->layout.button_g[i].x||
		       td->old_layout.button_g[i].y !=
		       td->layout.button_g[i].y)
		      && ((td->cd->valuemask & CWBackPixmap) ||
			  CSET_PIXMAP_IS_TILED(td->cd->bg_border_cs)))
		     ||
		     (old_g->width != new_g->width &&
		      CSET_PIXMAP_IS_X_STRETCHED(td->cd->bg_border_cs))
		     ||
		     (old_g->height != new_g->height
		      && CSET_PIXMAP_IS_Y_STRETCHED(td->cd->bg_border_cs))
		     ||
		     ((old_g->x != new_g->x || old_g->y != new_g->y)
		      && CSET_IS_TRANSPARENT_ROOT(td->cd->bg_border_cs))))
		{
			td->tbstate.draw_bmask |= mask;
		}
		else if (DFS_USE_TITLE_STYLE(*bs))
		{
			df = &TB_STATE(GetDecor(
				fw, titlebar))[td->tbstate.bstate[i]];
			for(tdf = df; tdf != NULL; tdf = tdf->next)
			{
				int cs;
				if (DFS_FACE_TYPE(tdf->style) == MultiPixmap)
				{
					/* can be improved */
					td->tbstate.draw_bmask |= mask;
					break;
				}
				if (DFS_FACE_TYPE(tdf->style) != ColorsetButton
				    || !CSET_HAS_PIXMAP(tdf->u.acs.cs))
				{
					continue;
				}
				cs = tdf->u.acs.cs;
				if(((td->old_layout.button_g[i].x !=
				     td->layout.button_g[i].x ||
				     td->old_layout.button_g[i].y !=
				     td->layout.button_g[i].y) ||
				    CSET_PIXMAP_IS_TILED(cs))
				   ||
				   (old_g->width != new_g->width &&
				    CSET_PIXMAP_IS_X_STRETCHED(cs))
				   ||
				   (old_g->height != new_g->height &&
				    CSET_PIXMAP_IS_Y_STRETCHED(cs))
					||
				   ((old_g->x != new_g->x ||
				     old_g->y != new_g->y)
				    && CSET_IS_TRANSPARENT_ROOT(cs)))
				{
					td->tbstate.draw_bmask |= mask;
					break;
				}
			}
		}
		if (td->tbstate.draw_bmask & mask)
		{
			continue;
		}
		if (old_g->x != new_g->x || old_g->y != new_g->y)
		{
			df = &TB_STATE(GetDecor(
				fw, buttons[i]))[td->tbstate.bstate[i]];
			for(tdf = df; tdf != NULL; tdf = tdf->next)
			{
				if (DFS_FACE_TYPE(tdf->style) ==
				    ColorsetButton &&
				    CSET_IS_TRANSPARENT_ROOT(tdf->u.acs.cs))
				{
					td->tbstate.draw_bmask |= mask;
					break;
				}
			}
		}
	}
	td->tbstate.max_bmask = 0;
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		if ((i & 1) == 1 && i / 2 < Scr.nr_right_buttons)
		{
			td->tbstate.max_bmask |= (1 << i);
		}
		else if ((i & 1) == 0 && i / 2 < Scr.nr_left_buttons)
		{
			td->tbstate.max_bmask |= (1 << i);
		}
	}
	td->tbstate.draw_bmask &= td->tbstate.max_bmask;
	td->tbstate.pressed_bmask &= td->tbstate.max_bmask;
	td->tbstate.lit_bmask &= td->tbstate.max_bmask;
	td->tbstate.toggled_bmask &= td->tbstate.max_bmask;
	td->tbstate.clear_bmask &= td->tbstate.max_bmask;
	if (td->tbstate.draw_bmask == 0)
	{
		draw_parts &= ~PART_BUTTONS;
	}
	else
	{
		draw_parts |= PART_BUTTONS;
	}
	draw_parts &= PART_TITLEBAR;

	return draw_parts;
}

static void border_get_border_gcs(
	draw_border_gcs *ret_gcs, common_decorations_type *cd, FvwmWindow *fw,
	Bool do_hilight)
{
	static GC transparent_gc = None;
	DecorFaceStyle *borderstyle;
	Bool is_reversed = False;

	if (transparent_gc == None && !HAS_NO_BORDER(fw) && !HAS_MWM_BORDER(fw))
	{
		XGCValues xgcv;

		xgcv.function = GXnoop;
		xgcv.plane_mask = 0;
		transparent_gc = fvwmlib_XCreateGC(
			dpy, Scr.NoFocusWin, GCFunction | GCPlaneMask, &xgcv);
	}
	ret_gcs->transparent = transparent_gc;
	/* get the border style bits */
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	if (borderstyle->flags.button_relief == DFS_BUTTON_IS_SUNK)
	{
		is_reversed = True;
	}
	if (is_reversed)
	{
		ret_gcs->shadow = cd->relief_gc;
		ret_gcs->relief = cd->shadow_gc;
	}
	else
	{
		ret_gcs->relief = cd->relief_gc;
		ret_gcs->shadow = cd->shadow_gc;
	}

	return;
}

static void trim_border_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
{
	/* If the border is too thin to accomodate the standard look, we remove
	 * parts of the border so that at least one pixel of the original
	 * colour is visible. We make an exception for windows with a border
	 * width of 2, though. */
	if ((!IS_SHADED(fw) || HAS_TITLE(fw)) && fw->boundary_width == 2)
	{
		ret_size_descr->trim--;
	}
	if (ret_size_descr->trim < 0)
	{
		ret_size_descr->trim = 0;
	}
	for ( ; ret_size_descr->trim > 0; ret_size_descr->trim--)
	{
		if (ret_size_descr->w_hiout > 1)
		{
			ret_size_descr->w_hiout--;
		}
		else if (ret_size_descr->w_shin > 0)
		{
			ret_size_descr->w_shin--;
		}
		else if (ret_size_descr->w_hiout > 0)
		{
			ret_size_descr->w_hiout--;
		}
		else if (ret_size_descr->w_trout > 0)
		{
			ret_size_descr->w_trout = 0;
			ret_size_descr->w_trin = 0;
			ret_size_descr->w_din = 0;
			ret_size_descr->w_hiout = 1;
		}
		ret_size_descr->sum--;
	}
	ret_size_descr->w_c = fw->boundary_width - ret_size_descr->sum;

	return;
}

static void check_remove_inset(
	DecorFaceStyle *borderstyle, border_relief_size_descr *ret_size_descr)
{
	if (!DFS_HAS_NO_INSET(*borderstyle))
	{
		return;
	}
	ret_size_descr->w_shin = 0;
	ret_size_descr->sum--;
	ret_size_descr->trim--;
	if (ret_size_descr->w_trin)
	{
		ret_size_descr->w_trout = 0;
		ret_size_descr->w_trin = 0;
		ret_size_descr->w_din = 0;
		ret_size_descr->w_hiout = 1;
		ret_size_descr->sum -= 2;
		ret_size_descr->trim -= 2;
	}

	return;
}

static void border_fetch_mwm_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
{
	/* MWM borders look like this:
	 *
	 * HHCCCCS  from outside to inside on the left and top border
	 * SSCCCCH  from outside to inside on the bottom and right border
	 * |||||||
	 * |||||||__ w_shin	(inner shadow area)
	 * ||||||___ w_c	(transparent area)
	 * |||||____ w_c	(transparent area)
	 * ||||_____ w_c	(transparent area)
	 * |||______ w_c	(transparent area)
	 * ||_______ w_hiout	(outer hilight area)
	 * |________ w_hiout	(outer hilight area)
	 *
	 *
	 * C = original colour
	 * H = hilight
	 * S = shadow
	 */
	ret_size_descr->w_dout = 0;
	ret_size_descr->w_hiout = 2;
	ret_size_descr->w_trout = 0;
	ret_size_descr->w_trin = 0;
	ret_size_descr->w_shin = 1;
	ret_size_descr->w_din = 0;
	ret_size_descr->sum = 3;
	ret_size_descr->trim = ret_size_descr->sum - fw->boundary_width + 1;
	check_remove_inset(borderstyle, ret_size_descr);
	trim_border_layout(fw, borderstyle, ret_size_descr);

	return;
}

static void border_fetch_fvwm_layout(
	FvwmWindow *fw, DecorFaceStyle *borderstyle,
	border_relief_size_descr *ret_size_descr)
{
	/* Fvwm borders look like this:
	 *
	 * SHHCCSS  from outside to inside on the left and top border
	 * SSCCHHS  from outside to inside on the bottom and right border
	 * |||||||
	 * |||||||__ w_din	(inner dark area)
	 * ||||||___ w_shin	(inner shadow area)
	 * |||||____ w_trin	(inner transparent/shadow area)
	 * ||||_____ w_c	(transparent area)
	 * |||______ w_trout	(outer transparent/hilight area)
	 * ||_______ w_hiout	(outer hilight area)
	 * |________ w_dout	(outer dark area)
	 *
	 * C = original colour
	 * H = hilight
	 * S = shadow
	 *
	 * reduced to 5 pixels it looks like this:
	 *
	 * SHHCS
	 * SSCHS
	 * |||||
	 * |||||__ w_din	(inner dark area)
	 * ||||___ w_trin	(inner transparent/shadow area)
	 * |||____ w_trout	(outer transparent/hilight area)
	 * ||_____ w_hiout	(outer hilight area)
	 * |______ w_dout	(outer dark area)
	 */
	ret_size_descr->w_dout = 1;
	ret_size_descr->w_hiout = 1;
	ret_size_descr->w_trout = 1;
	ret_size_descr->w_trin = 1;
	ret_size_descr->w_shin = 1;
	ret_size_descr->w_din = 1;
	/* w_trout + w_trin counts only as one pixel of border because
	 * they let one pixel of the original colour shine through. */
	ret_size_descr->sum = 6;
	ret_size_descr->trim = ret_size_descr->sum - fw->boundary_width;
	check_remove_inset(borderstyle, ret_size_descr);
	trim_border_layout(fw, borderstyle, ret_size_descr);

	return;
}

static void border_get_border_relief_size_descr(
	border_relief_size_descr *ret_size_descr, FvwmWindow *fw,
	Bool do_hilight)
{
	DecorFaceStyle *borderstyle;

	if (is_window_border_minimal(fw))
	{
		/* the border is too small, only a background but no relief */
		ret_size_descr->is_flat = 1;
		return;
	}
	borderstyle = (do_hilight) ?
		&GetDecor(fw, BorderStyle.active.style) :
		&GetDecor(fw, BorderStyle.inactive.style);
	if (borderstyle->flags.button_relief == DFS_BUTTON_IS_FLAT)
	{
		ret_size_descr->is_flat = 1;
		return;
	}
	ret_size_descr->is_flat = 0;
	/* get the relief layout */
	if (HAS_MWM_BORDER(fw))
	{
		border_fetch_mwm_layout(fw, borderstyle, ret_size_descr);
	}
	else
	{
		border_fetch_fvwm_layout(fw, borderstyle, ret_size_descr);
	}

	return;
}

static void border_get_border_marks_descr(
	common_decorations_type *cd, border_relief_descr *br, FvwmWindow *fw)
{
	int inset;

	/* get mark's length and thickness */
	inset = (br->relief.w_shin != 0 || br->relief.w_din != 0);
	br->marks.length = fw->boundary_width - br->relief.w_dout - inset;
	if (br->marks.length <= 0)
	{
		br->marks.has_x_marks = 0;
		br->marks.has_y_marks = 0;
		return;
	}
	br->marks.thickness = cd->relief_width;
	if (br->marks.thickness > br->marks.length)
	{
		br->marks.thickness = br->marks.length;
	}
	/* get offsets from outer side of window */
	br->marks.offset_tl = br->relief.w_dout;
	br->marks.offset_br =
		-br->relief.w_dout - br->marks.length - br->marks.offset_tl;

	return;
}

static Pixmap border_create_decor_pixmap(
	common_decorations_type *cd, rectangle *decor_g)
{
	Pixmap p;

	p = XCreatePixmap(
		dpy, Scr.Root, decor_g->width, decor_g->height, Pdepth);

	return p;
}

static void border_draw_part_relief(
	border_relief_descr *br, rectangle *frame_g, rectangle *part_g,
	Pixmap dest_pix, Bool is_inverted)
{
	int i;
	int off_x = 0;
	int off_y = 0;
	int width = frame_g->width - 1;
	int height = frame_g->height - 1;
	int w[7];
	GC gc[4];

	w[0] = br->relief.w_dout;
	w[1] = br->relief.w_hiout;
	w[2] = br->relief.w_trout;
	w[3] = br->relief.w_c;
	w[4] = br->relief.w_trin;
	w[5] = br->relief.w_shin;
	w[6] = br->relief.w_din;
	gc[(is_inverted == True)] = br->gcs.relief;
	gc[!(is_inverted == True)] = br->gcs.shadow;
	gc[2] = br->gcs.transparent;
	gc[3] = br->gcs.shadow;

	off_x = -part_g->x;
	off_y = -part_g->y;
	width = frame_g->width - 1;
	height = frame_g->height - 1;
	for (i = 0; i < 7; i++)
	{
		if (ulgc[i] != 0x7f && w[i] > 0)
		{
			do_relieve_rectangle(
				dpy, dest_pix, off_x, off_y,
				width, height, gc[(int)ulgc[i]],
				gc[(int)brgc[i]], w[i], False);
		}
		off_x += w[i];
		off_y += w[i];
		width -= 2 * w[i];
		height -= 2 * w[i];
	}

	return;
}

static void border_draw_x_mark(
	border_relief_descr *br, int x, int y, Pixmap dest_pix,
	Bool do_draw_shadow)
{
	int k;
	int length;
	GC gc;

	if (br->marks.has_x_marks == 0)
	{
		return;
	}
	x += br->marks.offset_tl;
	gc = (do_draw_shadow) ? br->gcs.shadow : br->gcs.relief;
	/* draw it */
	for (k = 0, length = br->marks.length - 1; k < br->marks.thickness;
	     k++, length--)
	{
		int x1;
		int x2;
		int y1;
		int y2;

		if (length < 0)
		{
			break;
		}
		if (do_draw_shadow)
		{
			x1 = x + k;
			y1 = y - 1 - k;
		}
		else
		{
			x1 = x;
			y1 = y + k;
		}
		x2 = x1 + length;
		y2 = y1;
		XDrawLine(dpy, dest_pix, gc, x1, y1, x2, y2);
	}

	return;
}

static void border_draw_y_mark(
	border_relief_descr *br, int x, int y, Pixmap dest_pix,
	Bool do_draw_shadow)
{
	int k;
	int length;
	GC gc;

	if (br->marks.has_y_marks == 0)
	{
		return;
	}
	y += br->marks.offset_tl;
	gc = (do_draw_shadow) ? br->gcs.shadow : br->gcs.relief;
	/* draw it */
	for (k = 0, length = br->marks.length; k < br->marks.thickness;
	     k++, length--)
	{
		int x1;
		int x2;
		int y1;
		int y2;

		if (length <= 0)
		{
			break;
		}
		if (do_draw_shadow)
		{
			x1 = x - 1 - k;
			y1 = y + k;
		}
		else
		{
			x1 = x + k;
			y1 = y;
		}
		x2 = x1;
		y2 = y1 + length - 1;
		XDrawLine(dpy, dest_pix, gc, x1, y1, x2, y2);
	}

	return;
}

static void border_draw_part_marks(
	border_relief_descr *br, rectangle *part_g, window_parts part,
	Pixmap dest_pix)
{
	int l;
	int t;
	int w;
	int h;
	int o;

	l = br->sidebar_g.x;
	t = br->sidebar_g.y;
	w = part_g->width;
	h = part_g->height;
	o = br->marks.offset_br;
	switch (part)
	{
	case PART_BORDER_N:
		border_draw_y_mark(br, 0, 0, dest_pix, False);
		border_draw_y_mark(br, w, 0, dest_pix, True);
		break;
	case PART_BORDER_S:
		border_draw_y_mark(br, 0, h + o, dest_pix, False);
		border_draw_y_mark(br, w, h + o, dest_pix, True);
		break;
	case PART_BORDER_E:
		border_draw_x_mark(br, w + o, 0, dest_pix, False);
		border_draw_x_mark(br, w + o, h, dest_pix, True);
		break;
	case PART_BORDER_W:
		border_draw_x_mark(br, 0, 0, dest_pix, False);
		border_draw_x_mark(br, 0, h, dest_pix, True);
		break;
	case PART_BORDER_NW:
		border_draw_x_mark(br, 0, t, dest_pix, True);
		border_draw_y_mark(br, l, 0, dest_pix, True);
		break;
	case PART_BORDER_NE:
		border_draw_x_mark(br, l + o, t, dest_pix, True);
		border_draw_y_mark(br, 0, 0, dest_pix, False);
		break;
	case PART_BORDER_SW:
		border_draw_x_mark(br, 0, 0, dest_pix, False);
		border_draw_y_mark(br, l, t + o, dest_pix, True);
		break;
	case PART_BORDER_SE:
		border_draw_x_mark(br, l + o, 0, dest_pix, False);
		border_draw_y_mark(br, 0, t + o, dest_pix, False);
		break;
	default:
		return;
	}

	return;
}

inline static void border_set_part_background(
	Window w, Pixmap pix)
{
	XSetWindowAttributes xswa;

	xswa.background_pixmap = pix;
	XChangeWindowAttributes(dpy, w, CWBackPixmap, &xswa);

	return;
}

/* render the an image into the pixmap */
static void border_fill_pixmap_background(
	Pixmap dest_pix, rectangle *dest_g, pixmap_background_type *bg,
	common_decorations_type *cd)
{
	Bool do_tile;
	Bool do_stretch;
	XGCValues xgcv;
	unsigned long valuemask;
	Pixmap p = None, shape = None, alpha = None;
	int src_width, src_height;

	do_tile = (bg->flags.use_pixmap && bg->pixmap.flags.is_tiled) ?
		True : False;
	do_stretch = (bg->flags.use_pixmap && bg->pixmap.flags.is_stretched) ?
		True : False;
	xgcv.fill_style = FillSolid;
	valuemask = GCFillStyle;
	if (!bg->flags.use_pixmap)
	{
		/* solid pixel */
		xgcv.foreground = bg->pixel;
		xgcv.clip_x_origin = 0;
		xgcv.clip_y_origin = 0;
		xgcv.clip_mask = None;
		valuemask |= GCForeground | GCClipMask | GCClipXOrigin |
			GCClipYOrigin;
		XChangeGC(dpy, Scr.BordersGC, valuemask, &xgcv);
		XFillRectangle(
			dpy, dest_pix, Scr.BordersGC, dest_g->x, dest_g->y,
			dest_g->width - dest_g->x, dest_g->height - dest_g->y);
		return;
	}

	if (do_stretch)
	{
		if (bg->pixmap.p)
		{
			p = CreateStretchPixmap(
				dpy, bg->pixmap.p,
				bg->pixmap.g.width, bg->pixmap.g.height,
				bg->pixmap.depth,
				bg->pixmap.stretch_w, bg->pixmap.stretch_h,
				(bg->pixmap.depth == 1)?
				Scr.MonoGC:Scr.BordersGC);
		}
		if (bg->pixmap.shape)
		{
			shape = CreateStretchPixmap(
				dpy, bg->pixmap.shape,
				bg->pixmap.g.width, bg->pixmap.g.height, 1,
				bg->pixmap.stretch_w, bg->pixmap.stretch_h,
				Scr.MonoGC);
		}
		if (bg->pixmap.alpha)
		{
			alpha = CreateStretchPixmap(
				dpy, bg->pixmap.alpha,
				bg->pixmap.g.width, bg->pixmap.g.height,
				FRenderGetAlphaDepth(),
				bg->pixmap.stretch_w, bg->pixmap.stretch_h,
				Scr.AlphaGC);
		}
		src_width = bg->pixmap.stretch_w;
		src_height = bg->pixmap.stretch_h;
	}
	else
	{
		p = bg->pixmap.p;
		shape = bg->pixmap.shape;
		alpha =	bg->pixmap.alpha;
		src_width = bg->pixmap.g.width;
		src_height = bg->pixmap.g.height;
	}

	if (do_tile == False)
	{
		/* pixmap, offset stored in dest_g->x/y */
		xgcv.foreground = cd->fore_color;
		xgcv.background = cd->back_color;
		valuemask |= GCForeground|GCBackground;
		XChangeGC(dpy, Scr.BordersGC, valuemask, &xgcv);
		PGraphicsRenderPixmaps(
			dpy, Scr.NoFocusWin, p, shape, alpha,
			bg->pixmap.depth, &(bg->pixmap.fra),
			dest_pix, Scr.BordersGC, Scr.MonoGC, Scr.AlphaGC,
			bg->pixmap.g.x, bg->pixmap.g.y,
			src_width, src_height,
			dest_g->x, dest_g->y, dest_g->width - dest_g->x,
			dest_g->height - dest_g->y, False);
	}
	else
	{
		/* tiled pixmap */
		xgcv.foreground = cd->fore_color;
		xgcv.background = cd->back_color;
		valuemask |= GCForeground|GCBackground;
		XChangeGC(dpy, Scr.BordersGC, valuemask, &xgcv);
		PGraphicsRenderPixmaps(
			dpy, Scr.NoFocusWin, p, shape, alpha,
			bg->pixmap.depth, &(bg->pixmap.fra),
			dest_pix, Scr.BordersGC, Scr.MonoGC, Scr.AlphaGC,
			bg->pixmap.g.x, bg->pixmap.g.y,
			src_width, src_height,
			dest_g->x, dest_g->y,
			dest_g->width - dest_g->x,
			dest_g->height - dest_g->y, True);
	}
	if (p && p != bg->pixmap.p)
	{
		XFreePixmap(dpy, p);
	}
	if (shape && shape != bg->pixmap.shape)
	{
		XFreePixmap(dpy, shape);
	}
	if (alpha && alpha != bg->pixmap.alpha)
	{
		XFreePixmap(dpy, alpha);
	}
	return;
}

/* create a root transparent colorset bg, we take in account a possible
 * drawing rotation */
static Pixmap border_create_root_transparent_pixmap(
	titlebar_descr *td, Window w, int width, int height, int cs)
{
	int my_w, my_h;
	Pixmap p;

	if (!CSET_IS_TRANSPARENT_ROOT(cs))
	{
		return None;
	}
	if (td->td_is_rotated &&
	    (td->draw_rotation == ROTATION_90 ||
	     td->draw_rotation == ROTATION_270))
	{
		my_h = width;
		my_w = height;
	}
	else
	{
		my_w = width;
		my_h = height;
	}
	p = CreateBackgroundPixmap(
		dpy, w, my_w, my_h, &Colorset[cs],
		Pdepth, Scr.BordersGC, False);
	if (p && td->td_is_rotated)
	{
		Pixmap tmp;
		tmp = CreateRotatedPixmap(
			dpy, p, my_w, my_h, Pdepth, Scr.BordersGC,
			td->restore_rotation);
		XFreePixmap(dpy, p);
		p = tmp;
	}
	return p;
}

static void border_get_frame_pixmap(
	common_decorations_type *cd, rectangle *frame_g)
{
	dynamic_common_decorations *dcd = &(cd->dynamic_cd);

	if (dcd->frame_pixmap != None)
	{
		/* should not happen */
		fprintf(stderr, "Bad use of border_get_frame_pixmap!!\n");
		dcd->frame_pixmap = None;
	}

	if (cd->bg_border_cs < 0 || CSET_IS_TRANSPARENT(cd->bg_border_cs))
	{
		/* should not happen */
	}
	else
	{
		dcd->frame_pixmap = CreateBackgroundPixmap(
			dpy, Scr.NoFocusWin, frame_g->width, frame_g->height,
			&Colorset[cd->bg_border_cs], Pdepth, Scr.BordersGC,
			False);
	}
	return;
}

static void border_get_border_background(
	pixmap_background_type *bg, common_decorations_type *cd,
	rectangle *part_g, rectangle *relative_g, int *free_bg_pixmap, Window w)
{
	*free_bg_pixmap = False;

	if (cd->texture_pixmap)
	{
		bg->flags.use_pixmap = 1;
		bg->pixmap.p = cd->texture_pixmap;
		bg->pixmap.g.width = cd->texture_pixmap_width;
		bg->pixmap.g.height = cd->texture_pixmap_height;
		bg->pixmap.shape = None;
		bg->pixmap.alpha = None;
		bg->pixmap.depth = Pdepth;
		bg->pixmap.flags.is_tiled = 1;
		bg->pixmap.flags.is_stretched = 0;
		bg->pixmap.fra.mask = 0;
	}
	else if (cd->bg_border_cs >= 0 &&
		 !CSET_IS_TRANSPARENT_PR(cd->bg_border_cs))
	{
		colorset_t *cs_t = &Colorset[cd->bg_border_cs];
		XGCValues xgcv;

		if (CSET_IS_TRANSPARENT_ROOT(cd->bg_border_cs))
		{
			bg->pixmap.p = CreateBackgroundPixmap(
				dpy, w, part_g->width, part_g->height, cs_t,
				Pdepth, Scr.BordersGC, False);
		}
		else
		{
			/* FIXME */
			if (cd->dynamic_cd.frame_pixmap == None)
			{
				border_get_frame_pixmap(cd, relative_g);
			}
			bg->pixmap.p = XCreatePixmap(
				dpy, cd->dynamic_cd.frame_pixmap, part_g->width,
				part_g->height, Pdepth);
			xgcv.fill_style = FillTiled;
			xgcv.tile = cd->dynamic_cd.frame_pixmap;
			xgcv.ts_x_origin = - relative_g->x;
			xgcv.ts_y_origin = - relative_g->y;
			XChangeGC(
				dpy, Scr.BordersGC, GCTile | GCTileStipXOrigin |
				GCTileStipYOrigin | GCFillStyle, &xgcv);
			XFillRectangle(
				dpy, bg->pixmap.p, Scr.BordersGC, 0, 0,
				part_g->width, part_g->height);
			xgcv.fill_style = FillSolid;
			XChangeGC(dpy, Scr.BordersGC, GCFillStyle, &xgcv);
		}
		bg->pixmap.g.width = part_g->width;
		bg->pixmap.g.height = part_g->height;
		bg->flags.use_pixmap = 1;
		bg->pixmap.shape = None;
		bg->pixmap.alpha = None;
		bg->pixmap.depth = Pdepth;
		bg->pixmap.flags.is_tiled = 1;
		bg->pixmap.flags.is_stretched = 0;
		bg->pixmap.fra.mask = 0;
		*free_bg_pixmap = True;
	}
	else
	{
		bg->flags.use_pixmap = 0;
		bg->pixel = cd->attributes.background_pixel;
	}

	return;
}

static void border_draw_one_border_part(
	common_decorations_type *cd, FvwmWindow *fw, rectangle *sidebar_g,
	rectangle *frame_g, border_relief_descr *br, window_parts part,
	window_parts draw_handles, Bool is_inverted, Bool do_clear)
{
	pixmap_background_type bg;
	rectangle part_g;
	rectangle pix_g;
	rectangle relative_g;
	Pixmap p;
	Window w;
	Bool free_bg_pixmap = False;

	/* make a pixmap */
	border_get_part_geometry(fw, part, sidebar_g, &part_g, &w);
	if (part_g.width <= 0 || part_g.height <= 0)
	{
		return;
	}
	p = border_create_decor_pixmap(cd, &part_g);
	/* set the background tile */
	relative_g.width = fw->g.frame.width;
	relative_g.height = fw->g.frame.height;
	relative_g.x = part_g.x;
	relative_g.y = part_g.y;
	border_get_border_background(
		&bg, cd, &part_g, &relative_g, &free_bg_pixmap, w);
	if (cd->texture_pixmap)
	{
		switch (part)
		{
		case PART_BORDER_E:
			bg.pixmap.g.x = frame_g->width - fw->boundary_width;
			break;
		case PART_BORDER_NE:
		case PART_BORDER_SE:
			bg.pixmap.g.x = frame_g->width - fw->corner_width;
			break;
		case PART_BORDER_N:
		case PART_BORDER_S:
			bg.pixmap.g.x = fw->corner_width;
			break;
		default:
			bg.pixmap.g.x = 0;
			break;
		}
		switch (part)
		{
		case PART_BORDER_S:
			bg.pixmap.g.y = frame_g->height - fw->boundary_width;
			break;
		case PART_BORDER_SW:
		case PART_BORDER_SE:
			bg.pixmap.g.y = frame_g->height - fw->corner_width;
			break;
		case PART_BORDER_W:
		case PART_BORDER_E:
			bg.pixmap.g.y = fw->corner_width;
			break;
		default:
			bg.pixmap.g.y = 0;
			break;
		}
	}
	else
	{
		bg.pixmap.g.x = 0;
		bg.pixmap.g.y = 0;
	}
	/* set the geometry for drawing the Tiled pixmap; maybe add the relief
	 * as offset? */
	pix_g.x = 0;
	pix_g.y = 0;
	pix_g.width = part_g.width;
	pix_g.height = part_g.height;
	border_fill_pixmap_background(p, &pix_g, &bg, cd);
	if (free_bg_pixmap && bg.pixmap.p)
	{
		XFreePixmap(dpy, bg.pixmap.p);
	}
	/* draw the relief over the background */
	if (!br->relief.is_flat)
	{
		border_draw_part_relief(br, frame_g, &part_g, p, is_inverted);
		/* draw the handle marks */
		if (br->marks.has_x_marks || br->marks.has_y_marks)
		{
			border_draw_part_marks(br, &part_g, part, p);
		}
	}
	/* apply the pixmap and destroy it */
	border_set_part_background(w, p);
	if (do_clear == True)
	{
		XClearWindow(dpy,w);
	}
	XFreePixmap(dpy, p);

	return;
}

static void border_draw_all_border_parts(
	common_decorations_type *cd, FvwmWindow *fw, border_relief_descr *br,
	rectangle *frame_g, window_parts draw_parts,
	window_parts pressed_parts, Bool do_hilight, Bool do_clear)
{
	window_parts part;
	window_parts draw_handles;

	/* get the description of the drawing directives */
	border_get_border_relief_size_descr(&br->relief, fw, do_hilight);
	border_get_border_marks_descr(cd, br, fw);
	/* fetch the gcs used to draw the border */
	border_get_border_gcs(&br->gcs, cd, fw, do_hilight);
	/* draw everything in a big loop */
	draw_parts &= (PART_FRAME | PART_HANDLES);
	draw_handles = (draw_parts & PART_HANDLES);

	for (part = PART_BORDER_N; (part & PART_FRAME); part <<= 1)
	{
		if (part & draw_parts)
		{
			border_draw_one_border_part(
				cd, fw, &br->sidebar_g, frame_g, br, part,
				draw_handles,
				(pressed_parts & part) ? True : False,
				do_clear);
		}
	}

	return;
}

/*
 *
 *  Draws a little pattern within a window (more complex)
 *
 */
static void border_draw_vector_to_pixmap(
	Pixmap dest_pix, common_decorations_type *cd, int is_toggled,
	struct vector_coords *coords, rectangle *pixmap_g)
{
	GC gcs[4];
	int i;

	if (coords->use_fgbg == 1)
	{
		Globalgcv.foreground = cd->fore_color;
		Globalgcm = GCForeground;
		XChangeGC(dpy, Scr.ScratchGC3, Globalgcm, &Globalgcv);
		Globalgcv.foreground = cd->back_color;
		XChangeGC(dpy, Scr.ScratchGC4, Globalgcm, &Globalgcv);
		gcs[3] = Scr.ScratchGC3; /* @3 is fg */
		gcs[2] = Scr.ScratchGC4; /* @2 is bg */
	}
	if (is_toggled)
	{
		gcs[0] = cd->relief_gc;
		gcs[1] = cd->shadow_gc;
	}
	else
	{
		gcs[0] = cd->shadow_gc;
		gcs[1] = cd->relief_gc;
	}
	for (i = 1; i < coords->num; i++)
	{
		if (coords->c[i] < 0 || coords->c[i] >= 4)
		{
			/* don't draw a line */
			continue;
		}
		XDrawLine(
			dpy, dest_pix, gcs[coords->c[i]],
			pixmap_g->width * coords->x[i-1] / 100 +
			coords->xoff[i-1],
			pixmap_g->height * coords->y[i-1] / 100 +
			coords->yoff[i-1],
			pixmap_g->width * coords->x[i] / 100 +
			coords->xoff[i],
			pixmap_g->height * coords->y[i] / 100 +
			coords->yoff[i]);
	}

	return;
}

/*
 *
 *  Handle Title pixmaps used for UseTitleStyle
 *
 */
static void border_setup_bar_pixmaps(
	titlebar_descr *td, dynamic_common_decorations *dcd, DecorFace *df,
	ButtonState bs)
{
	int count = dcd->bar_pixmaps[bs].count;
	DecorFace *tsdf;
	int i, j, mp_part_left, mp_part_right;

	if (count != 0)
	{
		/* ok */
		return;
	}

	for (tsdf = df; tsdf != NULL; tsdf = tsdf->next)
	{
		if (DFS_FACE_TYPE(tsdf->style) == ColorsetButton)
		{
			count++;
		}
		else if (DFS_FACE_TYPE(tsdf->style) == MultiPixmap)
		{
			border_mp_get_use_title_style_parts_and_geometry(
				td, tsdf->u.mp.pixmaps, tsdf->u.mp.acs,
				tsdf->u.mp.solid_flags, True, NULL,
				&mp_part_left);
			border_mp_get_use_title_style_parts_and_geometry(
				td, tsdf->u.mp.pixmaps, tsdf->u.mp.acs,
				tsdf->u.mp.solid_flags, False, NULL,
				&mp_part_right);
			for (j = 0; j < UTS_TBMP_NUM_PIXMAPS; j++)
			{
				if (j != mp_part_left && j != mp_part_right)
				{
					continue;
				}
				if (tsdf->u.mp.acs[j].cs >= 0 ||
				    tsdf->u.mp.pixmaps[j])
				{
					count++;
				}
			}
		}
	}
	if (count == 0)
	{
		dcd->bar_pixmaps[bs].count = -1;
		return;
	}
	else
	{
		dcd->bar_pixmaps[bs].bps = xmalloc(count * sizeof(bar_pixmap));
	}
	dcd->bar_pixmaps[bs].count = count;
	i = 0;
	for (tsdf = df; tsdf != NULL; tsdf = tsdf->next)
	{
		if (DFS_FACE_TYPE(tsdf->style) == ColorsetButton)
		{
			dcd->bar_pixmaps[bs].bps[i].p = None;
			dcd->bar_pixmaps[bs].bps[i].mp_created_pic = NULL;
			dcd->bar_pixmaps[bs].bps[i].cs = tsdf->u.acs.cs;
			dcd->bar_pixmaps[bs].bps[i].mp_pic = NULL;
			dcd->bar_pixmaps[bs].bps[i].created = 0;
			dcd->bar_pixmaps[bs].bps[i].mp_part = TBMP_NONE;
			i++;
		}
		else if (DFS_FACE_TYPE(tsdf->style) == MultiPixmap)
		{
			border_mp_get_use_title_style_parts_and_geometry(
				td, tsdf->u.mp.pixmaps, tsdf->u.mp.acs,
				tsdf->u.mp.solid_flags, True, NULL,
				&mp_part_left);
			border_mp_get_use_title_style_parts_and_geometry(
				td, tsdf->u.mp.pixmaps, tsdf->u.mp.acs,
				tsdf->u.mp.solid_flags, False, NULL,
				&mp_part_right);
			for (j = 0; j < UTS_TBMP_NUM_PIXMAPS; j++)
			{
				if (j != mp_part_left && j != mp_part_right)
				{
					continue;
				}
				if (tsdf->u.mp.acs[j].cs >= 0 ||
				    tsdf->u.mp.pixmaps[j])
				{
					dcd->bar_pixmaps[bs].bps[i].p = None;
					dcd->bar_pixmaps[bs].bps[i].
						mp_created_pic = NULL;
					dcd->bar_pixmaps[bs].bps[i].cs =
						tsdf->u.mp.acs[j].cs;
					dcd->bar_pixmaps[bs].bps[i].mp_pic =
						tsdf->u.mp.pixmaps[j];
					dcd->bar_pixmaps[bs].bps[i].created = 0;
					dcd->bar_pixmaps[bs].bps[i].mp_part = j;
					i++;
				}
			}
		}
	}
}

static Pixmap border_get_bar_pixmaps(
	dynamic_common_decorations *dcd, rectangle *bar_g, ButtonState bs,
	int cset, FvwmPicture *mp_pic, int mp_part, int stretch,
	FvwmPicture **mp_ret_pic)
{
	ButtonState b;
	int i,j;
	int count = dcd->bar_pixmaps[bs].count;

	if (count <= 0)
	{
		return None;
	}

	i = 0;
	while(i < count &&
	(dcd->bar_pixmaps[bs].bps[i].cs != cset ||
	      dcd->bar_pixmaps[bs].bps[i].mp_part != mp_part ||
	      dcd->bar_pixmaps[bs].bps[i].mp_pic != mp_pic))
	{
		i++;
	}
	if (i == count)
	{
		return None;
	}
	if (mp_ret_pic)
	{
		*mp_ret_pic = dcd->bar_pixmaps[bs].bps[i].mp_created_pic;
	}
	if (dcd->bar_pixmaps[bs].bps[i].p == None)
	{
		/* see if we have it */
		b = 0;
		while (b < BS_MaxButtonState)
		{
			int c = dcd->bar_pixmaps[b].count;
			j = 0;
			while(j < c &&
			      (dcd->bar_pixmaps[b].bps[j].cs != cset ||
			       dcd->bar_pixmaps[b].bps[j].mp_part != mp_part ||
			       dcd->bar_pixmaps[b].bps[j].mp_pic != mp_pic))
			{
				j++;
			}
			if (j < c && dcd->bar_pixmaps[b].bps[j].p)
			{
				dcd->bar_pixmaps[bs].bps[i].p =
					dcd->bar_pixmaps[b].bps[j].p;
				if (mp_pic && mp_ret_pic)
				{
					*mp_ret_pic =
						dcd->bar_pixmaps[bs].bps[i].
						mp_created_pic =
						dcd->bar_pixmaps[b].bps[j].
						mp_created_pic;
				}
				break;
			}
			b++;
		}
	}
	if (dcd->bar_pixmaps[bs].bps[i].p == None)
	{
		if (cset >= 0)
		{
			dcd->bar_pixmaps[bs].bps[i].p = CreateBackgroundPixmap(
				dpy, Scr.NoFocusWin, bar_g->width, bar_g->height,
				&Colorset[cset], Pdepth, Scr.BordersGC, False);
			dcd->bar_pixmaps[bs].bps[i].created = True;
		}
		else if (mp_pic && mp_ret_pic)
		{
			if (stretch)
			{
				dcd->bar_pixmaps[bs].bps[i].mp_created_pic =
					PGraphicsCreateStretchPicture(
						dpy, Scr.NoFocusWin, mp_pic,
						bar_g->width, bar_g->height,
						Scr.BordersGC, Scr.MonoGC,
						Scr.AlphaGC);
			}
			else
			{
				dcd->bar_pixmaps[bs].bps[i].mp_created_pic =
					PGraphicsCreateTiledPicture(
						dpy, Scr.NoFocusWin, mp_pic,
						bar_g->width, bar_g->height,
						Scr.BordersGC, Scr.MonoGC,
						Scr.AlphaGC);
			}
			if (dcd->bar_pixmaps[bs].bps[i].mp_created_pic)
			{
				dcd->bar_pixmaps[bs].bps[i].created = True;
				*mp_ret_pic =
					dcd->bar_pixmaps[bs].bps[i].
					mp_created_pic;
				dcd->bar_pixmaps[bs].bps[i].p =
					dcd->bar_pixmaps[bs].bps[i].
					mp_created_pic->picture;
			}
		}
	}
	return dcd->bar_pixmaps[bs].bps[i].p;
}

static void border_free_bar_pixmaps(
	dynamic_common_decorations *dcd)
{
	ButtonState bs;
	int i;

	for (bs = 0; bs < BS_MaxButtonState; bs++)
	{
		if (dcd->bar_pixmaps[bs].count < 1)
		{
			continue;
		}
		for (i = 0; i < dcd->bar_pixmaps[bs].count; i++)
		{
			if (dcd->bar_pixmaps[bs].bps[i].mp_created_pic &&
			    dcd->bar_pixmaps[bs].bps[i].created)
			{
				PDestroyFvwmPicture(
					dpy,
					dcd->bar_pixmaps[bs].bps[i].
					mp_created_pic);
			}
			else if (dcd->bar_pixmaps[bs].bps[i].p != None &&
				 dcd->bar_pixmaps[bs].bps[i].created)
			{
				XFreePixmap(
					dpy, dcd->bar_pixmaps[bs].bps[i].p);
			}
		}
		free(dcd->bar_pixmaps[bs].bps);
	}
}

/*
 *
 *  MultiPixmap (aka, fancy title bar) (tril@igs.net)
 *
 */
#define TBMP_HAS_PART(p, pm, acs, sf) \
       (pm[p] || acs[p].cs >= 0 || (sf & (1 << p)))

/*  Tile or stretch src into dest, starting at the given location and
 *  continuing for the given width and height. This is a utility function used
 *  by border_mp_draw_mp_titlebar. (tril@igs.net) */
static void border_mp_render_into_pixmap(
	titlebar_descr *td, common_decorations_type *cd, FvwmPicture **src,
	FvwmAcs *acs, Pixel *pixels, unsigned short solid_flags,
	unsigned short stretch_flags, int part, Pixmap dest, Window w,
	rectangle *full_g, rectangle *title_g, ButtonState bs, rectangle *g)
{
	int x = 0;
	int y = 0;
	pixmap_background_type bg;
	rectangle dest_g;
	dynamic_common_decorations *dcd;

	dcd = &cd->dynamic_cd;
	/* setup some default */
	bg.pixmap.fra.mask = 0;
	bg.pixmap.flags.is_stretched = 0;
	bg.pixmap.flags.is_tiled = 0;
	bg.flags.use_pixmap = 1;
	bg.pixmap.p = bg.pixmap.alpha = bg.pixmap.shape = None;
	bg.pixmap.g.x = 0;
	bg.pixmap.g.y = 0;
	dest_g.width = g->width + g->x;
	dest_g.height = g->height + g->y;
	dest_g.x = g->x;
	dest_g.y = g->y;

	if (solid_flags & (1 << part))
	{
		bg.flags.use_pixmap = 0;
		bg.pixel = pixels[part];
		border_fill_pixmap_background(dest, &dest_g, &bg, cd);
		return;
	}
	else if (acs[part].cs >= 0)
	{
		Pixmap p = None;

		bg.pixmap.fra.mask = FRAM_HAVE_ADDED_ALPHA;
		bg.pixmap.fra.added_alpha_percent = acs[part].alpha_percent;
		if (CSET_IS_TRANSPARENT_PR(acs[part].cs))
		{
			return;
		}
		if (CSET_IS_TRANSPARENT_ROOT(acs[part].cs))
		{
			p = border_create_root_transparent_pixmap(
				td, w, g->width + g->x, g->height + g->y,
				acs[part].cs);
			bg.pixmap.p = p;
			bg.pixmap.depth = Pdepth;
			bg.pixmap.g.width = g->width;
			bg.pixmap.g.height = g->height;
			bg.pixmap.g.x = g->x;
			bg.pixmap.g.y = g->y;
		}
		else if (full_g != NULL)
		{
			bg.pixmap.p = border_get_bar_pixmaps(
				dcd, full_g, bs, acs[part].cs, NULL, part,
				(stretch_flags & (1 << part)), NULL);
			if (bg.pixmap.p)
			{
				if (part != TBMP_RIGHT_MAIN)
				{
					/* left buttons offset */
					x = title_g->x - full_g->x;
					y = title_g->y - full_g->y;
				}
				bg.pixmap.g.width = full_g->width;
				bg.pixmap.g.height = full_g->height;
				bg.pixmap.flags.is_tiled = 1;
				bg.pixmap.g.x = x;
				bg.pixmap.g.y = y;
				bg.pixmap.depth = Pdepth;
			}
		}
		if (!bg.pixmap.p)
		{
			int bg_w, bg_h;

			p = CreateBackgroundPixmap(
				dpy, w, g->width, g->height,
				&Colorset[acs[part].cs], Pdepth, Scr.BordersGC,
				False);
			bg.pixmap.p = p;
			GetWindowBackgroundPixmapSize(
				&Colorset[acs[part].cs], g->width, g->height,
				&bg_w, &bg_h);
			bg.pixmap.g.width = bg_w;
			bg.pixmap.g.height = bg_h;
			bg.pixmap.depth = Pdepth;
			bg.pixmap.flags.is_tiled = 1;
		}
		if (bg.pixmap.p)
		{
			border_fill_pixmap_background(dest, &dest_g, &bg, cd);
		}
		if (p)
		{
			XFreePixmap(dpy, p);
		}
	}
	else if (src[part])
	{
		FvwmPicture *full_pic = NULL;
		Pixmap p;

		if (full_g != NULL)
		{
			p = border_get_bar_pixmaps(
				dcd, full_g, bs, -1, src[part], part,
				(stretch_flags & (1 << part)), &full_pic);
			if (p && full_pic)
			{
				if (part != TBMP_RIGHT_MAIN)
				{
					/* left buttons offset */
					x = title_g->x - full_g->x;
					y = title_g->y - full_g->y;
				}
				bg.pixmap.p = full_pic->picture;
				bg.pixmap.shape = full_pic->mask;
				bg.pixmap.alpha = full_pic->alpha;
				bg.pixmap.depth = full_pic->depth;
				bg.pixmap.g.width = full_pic->width;
				bg.pixmap.g.height = full_pic->height;
				bg.pixmap.g.x = x;
				bg.pixmap.g.y = y;
			}
		}
		if (!bg.pixmap.p)
		{
			if (stretch_flags & (1 << part))
			{
				bg.pixmap.flags.is_stretched = 1;
			}
			else
			{
				bg.pixmap.flags.is_tiled = 1;
			}
			bg.pixmap.p = src[part]->picture;
			bg.pixmap.shape = src[part]->mask;
			bg.pixmap.alpha = src[part]->alpha;
			bg.pixmap.depth = src[part]->depth;
			bg.pixmap.g.width = src[part]->width;
			bg.pixmap.g.height = src[part]->height;
			bg.pixmap.stretch_w = dest_g.width - dest_g.x;
			bg.pixmap.stretch_h = dest_g.height - dest_g.y;

		}
		if (bg.pixmap.p)
		{
			border_fill_pixmap_background(dest, &dest_g, &bg, cd);
		}
	}

	return;
}

static int border_mp_get_length(
	titlebar_descr *td, FvwmPicture **pm, FvwmAcs *acs,
	unsigned int solid_flags, int part)
{
	if (acs[part].cs >= 0 || (solid_flags & (1 << part)))
	{
		/* arbitrary */
		if (td->has_vt)
		{
			return td->bar_g.width/2;
		}
		else
		{
			return td->bar_g.height/2;
		}
	}
	if (pm[part] == NULL)
	{
		return 0;
	}
	else if (td->has_vt)
	{
		return pm[part]->height;
	}
	else
	{
		return pm[part]->width;
	}
}

/* geometries relatively to the frame */
static void border_mp_get_titlebar_descr(
	FvwmWindow *fw, titlebar_descr *td, DecorFace *df)
{
	DecorFace *tsdf;
	FvwmPicture **pm;
	FvwmAcs *acs;
	int add,tmpi;
	int left_of_text = 0;
	int right_of_text = 0;
	int left_end = 0;
	int right_end = 0;
	int before_space, after_space, under_offset, under_width;
	Bool has_mp = False;
	JustificationType just;
	unsigned short sf;
	int is_start = 0;

	just = TB_JUSTIFICATION(GetDecor(fw, titlebar));
	/* first compute under text width */
	if (td->length > 0)
	{
		under_width = td->length + 2*TBMP_TITLE_PADDING;
	}
	else
	{
		under_width = 0;
	}
	if (under_width > fw->title_length)
	{
		under_width = fw->title_length;
		td->offset = (fw->title_length - td->length) / 2;
		just = JUST_CENTER;
	}
	for (tsdf = df; tsdf != NULL; tsdf = tsdf->next)
	{
		if (tsdf->style.face_type != MultiPixmap)
		{
			continue;
		}
		has_mp = True;
		acs = tsdf->u.mp.acs;
		pm = tsdf->u.mp.pixmaps;
		sf = tsdf->u.mp.solid_flags;
		add = border_mp_get_length(
			td, pm, acs, sf, TBMP_LEFT_OF_TEXT);
		if (add > left_of_text &&
		    add + left_end + right_of_text + right_end + under_width +
		    2*TBMP_MIN_RL_TITLE_LENGTH <= fw->title_length)
		{
			left_of_text = add;
		}
		add = border_mp_get_length(
			td, pm, acs, sf, TBMP_RIGHT_OF_TEXT);
		if (add > right_of_text &&
		    add + left_end + left_of_text + right_end + under_width +
		    2*TBMP_MIN_RL_TITLE_LENGTH <= fw->title_length)
		{
			right_of_text = add;
		}
		add = border_mp_get_length(
			td, pm, acs, sf, TBMP_LEFT_END);
		if (add > left_end &&
		    add + right_of_text + left_of_text + right_end +
		    under_width + 2*TBMP_MIN_RL_TITLE_LENGTH <= fw->title_length)
		{
			left_end = add;
		}
		add = border_mp_get_length(
			td, pm, acs, sf, TBMP_RIGHT_END);
		if (add > right_end &&
		    add + right_of_text + left_of_text + left_end +
		    under_width + 2*TBMP_MIN_RL_TITLE_LENGTH <= fw->title_length)
		{
			right_end = add;
		}
	}

	if (!has_mp)
	{
		return;
	}

	switch (just)
	{
	case JUST_LEFT:
		is_start = 1;
		/* fall through */
	case JUST_RIGHT:
		if (td->has_an_upsidedown_rotation)
		{
			is_start = !is_start;
		}
		if (is_start)
		{
			if (td->has_an_upsidedown_rotation)
			{
				td->offset = max(
					td->offset, right_of_text + right_end +
					TBMP_MIN_RL_TITLE_LENGTH +
					TBMP_TITLE_PADDING);
			}
			else
			{
				td->offset = max(
					td->offset, left_of_text + left_end +
					TBMP_MIN_RL_TITLE_LENGTH +
					TBMP_TITLE_PADDING);
			}
		}
		else
		{
			if (td->has_an_upsidedown_rotation)
			{
				td->offset = min(
					td->offset, fw->title_length -
					(td->length + left_of_text +
					 left_end + TBMP_MIN_RL_TITLE_LENGTH +
					 TBMP_TITLE_PADDING));
			}
			else
			{
				td->offset = min(
					td->offset, fw->title_length -
					(td->length + right_of_text +
					 right_end + TBMP_MIN_RL_TITLE_LENGTH +
					 TBMP_TITLE_PADDING));
			}
		}
		break;
	case JUST_CENTER:
	default:
		break;
	}

	under_offset = td->offset - (under_width - td->length)/2;
	before_space = under_offset;
	if (td->has_vt)
	{
		after_space =
			td->layout.title_g.height - before_space - under_width;
	}
	else
	{
		after_space =
			td->layout.title_g.width - before_space - under_width;
	}
	if (td->has_an_upsidedown_rotation)
	{
		td->left_end_length = right_end;
		td->left_of_text_length = right_of_text;
		td->right_of_text_length = left_of_text;
		td->right_end_length = left_end;
		tmpi = before_space;
		before_space = after_space;
		after_space = tmpi;
	}
	else
	{
		td->left_end_length = left_end;
		td->left_of_text_length = left_of_text;
		td->right_of_text_length = right_of_text;
		td->right_end_length = right_end;
	}

	if (td->has_vt)
	{
		td->under_text_g.width = td->bar_g.width;
		td->under_text_g.height = under_width;
		td->under_text_g.x = 0;
		td->under_text_g.y = under_offset;
	}
	else
	{
		td->under_text_g.height = td->bar_g.height;
		td->under_text_g.width = under_width;
		td->under_text_g.x = under_offset;
		td->under_text_g.y = 0;
	}

	/* width & height */
	if (td->has_vt)
	{
		/* left */
		td->full_left_main_g.width = td->bar_g.width;
		td->full_left_main_g.height =
			before_space + td->left_buttons_g.height;
		td->left_main_g.width = td->bar_g.width;
		td->left_main_g.height = before_space;
		/* right */
		td->full_right_main_g.width = td->bar_g.width;
		td->full_right_main_g.height = after_space +
			td->right_buttons_g.height;
		td->right_main_g.width = td->bar_g.width;
		td->right_main_g.height = after_space;
	}
	else
	{
		/* left */
		td->full_left_main_g.height = td->bar_g.height;
		td->full_left_main_g.width =
			before_space + td->left_buttons_g.width;
		td->left_main_g.height = td->bar_g.height;
		td->left_main_g.width = before_space;
		/* right */
		td->full_right_main_g.height = td->bar_g.height;
		td->full_right_main_g.width = after_space +
			td->right_buttons_g.width;
		td->right_main_g.height = td->bar_g.height;
		td->right_main_g.width = after_space;
	}

	/* position */
	if (td->has_an_upsidedown_rotation)
	{
		td->full_right_main_g.x = td->bar_g.x;
		td->full_right_main_g.y = td->bar_g.y;
		td->right_main_g.x = 0;
		td->right_main_g.y = 0;
	}
	else
	{
		td->full_left_main_g.x = td->bar_g.x;
		td->full_left_main_g.y = td->bar_g.y;
		td->left_main_g.x = 0;
		td->left_main_g.y = 0;
	}

	if (td->has_vt)
	{
		if (td->has_an_upsidedown_rotation)
		{
			td->full_left_main_g.x = td->bar_g.x;
			td->full_left_main_g.y =
				td->full_right_main_g.height + td->bar_g.y +
				td->under_text_g.height;
			td->left_main_g.y =
				td->under_text_g.y + td->under_text_g.height;
			td->left_main_g.x = 0;
		}
		else
		{
			td->full_right_main_g.x = td->bar_g.x;
			td->full_right_main_g.y =
				td->full_left_main_g.height + td->bar_g.y +
				td->under_text_g.height;
			td->right_main_g.y =
				td->under_text_g.y + td->under_text_g.height;
			td->right_main_g.x = 0;
		}
	}
	else
	{
		if (td->has_an_upsidedown_rotation)
		{
			td->full_left_main_g.x =
				td->full_right_main_g.width + td->bar_g.x +
				td->under_text_g.width;
			td->full_left_main_g.y = td->bar_g.y;
			td->left_main_g.x =
				td->under_text_g.x + td->under_text_g.width;
			td->left_main_g.y = 0;
		}
		else
		{
			td->full_right_main_g.x =
				td->full_left_main_g.width + td->bar_g.x +
				td->under_text_g.width;
			td->full_right_main_g.y = td->bar_g.y;
			td->right_main_g.x =
				td->under_text_g.x + td->under_text_g.width;
			td->right_main_g.y = 0;
		}
	}
}

/* the returned geometries are relative to the titlebar (not the frame) */
static void border_mp_get_extreme_geometry(
	titlebar_descr *td, FvwmPicture **pm, FvwmAcs *acs, unsigned short sf,
	rectangle *left_of_text_g, rectangle *right_of_text_g,
	rectangle *left_end_g, rectangle *right_end_g)
{
	int left_of_text = 0;
	int right_of_text = 0;
	int left_end = 0;
	int right_end = 0;

	left_of_text = border_mp_get_length(
		td, pm, acs, sf, TBMP_LEFT_OF_TEXT);
	left_end = border_mp_get_length(
		td, pm, acs, sf, TBMP_LEFT_END);
	right_of_text = border_mp_get_length(
		td, pm, acs, sf, TBMP_RIGHT_OF_TEXT);
	right_end = border_mp_get_length(
			td, pm, acs, sf, TBMP_RIGHT_END);

	if (left_of_text > 0 && left_of_text <= td->left_of_text_length)
	{
		if (td->has_vt)
		{
			left_of_text_g->y = td->under_text_g.y - left_of_text;
			left_of_text_g->x = 0;
			left_of_text_g->height = left_of_text;
			left_of_text_g->width = td->bar_g.width;
		}
		else
		{
			left_of_text_g->x = td->under_text_g.x - left_of_text;
			left_of_text_g->y = 0;
			left_of_text_g->width = left_of_text;
			left_of_text_g->height = td->bar_g.height;
		}
	}
	else
	{
		left_of_text_g->x = 0;
		left_of_text_g->y = 0;
		left_of_text_g->width = 0;
		left_of_text_g->height = 0;
	}

	if (right_of_text > 0 && right_of_text <= td->right_of_text_length)
	{
		if (td->has_vt)
		{
			right_of_text_g->y =
				td->under_text_g.y + td->under_text_g.height;
			right_of_text_g->x = 0;
			right_of_text_g->height = right_of_text;
			right_of_text_g->width = td->bar_g.width;
		}
		else
		{
			right_of_text_g->x =
				td->under_text_g.x + td->under_text_g.width;
			right_of_text_g->y = 0;
			right_of_text_g->width = right_of_text;
			right_of_text_g->height = td->bar_g.height;
		}
	}
	else
	{
		right_of_text_g->x = 0;
		right_of_text_g->y = 0;
		right_of_text_g->width = 0;
		right_of_text_g->height = 0;
	}

	if (left_end > 0 && left_end <= td->left_end_length)
	{
		if (td->has_vt)
		{
			left_end_g->y = 0;
			left_end_g->x = 0;
			left_end_g->height = left_end;
			left_end_g->width = td->bar_g.width;
		}
		else
		{
			left_end_g->x = 0;
			left_end_g->y = 0;
			left_end_g->width = left_end;
			left_end_g->height = td->bar_g.height;
		}
	}
	else
	{
		left_end_g->x = 0;
		left_end_g->y = 0;
		left_end_g->width = 0;
		left_end_g->height = 0;
	}

	if (right_end > 0 && right_end <= td->right_end_length)
	{
		if (td->has_vt)
		{
			right_end_g->y =
				td->layout.title_g.height - right_end;
			right_end_g->x = 0;
			right_end_g->height = right_end;
			right_end_g->width = td->bar_g.width;
		}
		else
		{
			right_end_g->x =
				td->layout.title_g.width - right_end;
			right_end_g->y = 0;
			right_end_g->width = right_end;
			right_end_g->height = td->bar_g.height;
		}
	}
	else
	{
		right_end_g->x = 0;
		right_end_g->y = 0;
		right_end_g->width = 0;
		right_end_g->height = 0;
	}

	return;
}

static Bool border_mp_get_use_title_style_parts_and_geometry(
	titlebar_descr *td, FvwmPicture **pm, FvwmAcs *acs,
	unsigned short sf, int is_left, rectangle *g, int *part)
{
	rectangle *tmp_g = NULL;
	Bool g_ok = True;

	if (is_left && TBMP_HAS_PART(TBMP_LEFT_BUTTONS, pm, acs, sf))
	{
		*part = TBMP_LEFT_BUTTONS;
		tmp_g = &td->left_buttons_g;
	}
	else if (!is_left && TBMP_HAS_PART(TBMP_RIGHT_BUTTONS, pm, acs, sf))
	{
		*part = TBMP_RIGHT_BUTTONS;
		tmp_g = &td->right_buttons_g;
	}
	else if (is_left && TBMP_HAS_PART(TBMP_LEFT_MAIN, pm, acs, sf))
	{
		*part = TBMP_LEFT_MAIN;
		tmp_g = &td->full_left_main_g;
	}
	else if (!is_left && TBMP_HAS_PART(TBMP_RIGHT_MAIN, pm, acs, sf))
	{
		*part = TBMP_RIGHT_MAIN;
		tmp_g = &td->full_right_main_g;
	}
	else if (TBMP_HAS_PART(TBMP_MAIN, pm, acs, sf))
	{
		*part = TBMP_MAIN;
		tmp_g = &(td->bar_g);
	}
	else
	{
		*part = TBMP_NONE;
	}
	if (g && tmp_g)
	{
		g->x = tmp_g->x;
		g->y = tmp_g->y;
		g->width = tmp_g->width;
		g->height = tmp_g->height;
		g_ok = True;
	}

	return g_ok;
}

/*  Redraws multi-pixmap titlebar (tril@igs.net) */
static void border_mp_draw_mp_titlebar(
	FvwmWindow *fw, titlebar_descr *td, DecorFace *df, Pixmap dest_pix,
	Window w)
{
	FvwmPicture **pm;
	FvwmAcs *acs;
	Pixel *pixels;
	unsigned short solid_flags;
	unsigned short stretch_flags;
	rectangle tmp_g, left_of_text_g,left_end_g,right_of_text_g,right_end_g;
	ButtonState bs;

	bs = td->tbstate.tstate;
	pm = df->u.mp.pixmaps;
	acs = df->u.mp.acs;
	pixels = df->u.mp.pixels;
	stretch_flags = df->u.mp.stretch_flags;
	solid_flags = df->u.mp.solid_flags;
	tmp_g.x = 0;
	tmp_g.y = 0;
	tmp_g.width = td->layout.title_g.width;
	tmp_g.height = td->layout.title_g.height;

	if (TBMP_HAS_PART(TBMP_MAIN, pm, acs, solid_flags))
	{
		border_mp_render_into_pixmap(
			td, td->cd, pm, acs, pixels, solid_flags, stretch_flags,
			TBMP_MAIN, dest_pix, w, &td->bar_g, &td->layout.title_g,
			bs, &tmp_g);
	}
	else if (td->length <= 0)
	{
		border_mp_render_into_pixmap(
			td, td->cd, pm, acs, pixels, solid_flags, stretch_flags,
			TBMP_LEFT_MAIN, dest_pix, w, NULL, &td->layout.title_g,
			bs, &tmp_g);
	}

	border_mp_get_extreme_geometry(
		td, pm, acs, solid_flags, &left_of_text_g, &right_of_text_g,
		&left_end_g, &right_end_g);

	if (td->length > 0)
	{
		if (TBMP_HAS_PART(TBMP_LEFT_MAIN, pm, acs, solid_flags) &&
		    td->left_main_g.width > 0 && td->left_main_g.height > 0)
		{
			border_mp_render_into_pixmap(
				td, td->cd, pm, acs, pixels, solid_flags,
				stretch_flags, TBMP_LEFT_MAIN, dest_pix, w,
				&td->full_left_main_g, &td->layout.title_g, bs,
				&td->left_main_g);
		}
		if (TBMP_HAS_PART(TBMP_RIGHT_MAIN, pm, acs, solid_flags) &&
		    td->right_main_g.width > 0 && td->right_main_g.height > 0)
		{
			border_mp_render_into_pixmap(
				td, td->cd, pm, acs, pixels, solid_flags,
				stretch_flags, TBMP_RIGHT_MAIN, dest_pix, w,
				&td->full_right_main_g, &td->layout.title_g, bs,
				&td->right_main_g);
		}
		if (TBMP_HAS_PART(TBMP_UNDER_TEXT, pm, acs, solid_flags)  &&
		    td->under_text_g.width > 0 && td->under_text_g.height > 0)
		{
			border_mp_render_into_pixmap(
				td, td->cd, pm, acs, pixels, solid_flags,
				stretch_flags, TBMP_UNDER_TEXT, dest_pix, w,
				NULL, &td->layout.title_g, bs,
				&td->under_text_g);
		}
		if (left_of_text_g.width > 0 && left_of_text_g.height > 0)
		{
			border_mp_render_into_pixmap(
				td, td->cd, pm, acs, pixels, solid_flags,
				stretch_flags, TBMP_LEFT_OF_TEXT, dest_pix, w,
				NULL, &td->layout.title_g, bs, &left_of_text_g);
		}
		if (right_of_text_g.width > 0 && right_of_text_g.height > 0)
		{
			border_mp_render_into_pixmap(
				td, td->cd, pm, acs, pixels, solid_flags,
				stretch_flags, TBMP_RIGHT_OF_TEXT, dest_pix, w,
				NULL, &td->layout.title_g, bs, &right_of_text_g);
		}
	}
	if (left_end_g.width > 0 && left_end_g.height > 0)
	{
		border_mp_render_into_pixmap(
			td, td->cd, pm, acs, pixels, solid_flags, stretch_flags,
			TBMP_LEFT_END, dest_pix, w, NULL, &td->layout.title_g,
			bs, &left_end_g);
	}
	if (right_end_g.width > 0 && right_end_g.height > 0)
	{
		border_mp_render_into_pixmap(
			td, td->cd, pm, acs, pixels, solid_flags, stretch_flags,
			TBMP_RIGHT_END, dest_pix, w, NULL, &td->layout.title_g,
			bs, &right_end_g);
	}

	return;
}

/*
 *
 *  draw title bar and buttons
 *
 */
static void border_draw_decor_to_pixmap(
	FvwmWindow *fw, Pixmap dest_pix, Window w,
	pixmap_background_type *solid_bg, rectangle *w_g,
	DecorFace *df, titlebar_descr *td, ButtonState bs,
	int use_title_style, int is_toggled, int left1right0)
{
	register DecorFaceType type = DFS_FACE_TYPE(df->style);
	pixmap_background_type bg;
	rectangle dest_g;
	FvwmPicture *p;
	int width,height;
	int border;
	int lr_just, tb_just;
	common_decorations_type *cd;

	cd = td->cd;
	/* setup some default */
	bg.pixmap.fra.mask = 0;
	bg.pixmap.flags.is_stretched = 0;
	bg.pixmap.flags.is_tiled = 0;
	bg.flags.use_pixmap = 0;
	bg.pixmap.g.x = 0;
	bg.pixmap.g.y = 0;

	if (DFS_BUTTON_RELIEF(df->style) == DFS_BUTTON_IS_FLAT)
	{
		border = 0;
	}
	else
	{
		border = HAS_MWM_BORDER(fw) ? 1 : 2;
	}
	dest_g.width = w_g->width;
	dest_g.height = w_g->height;
	dest_g.x = border;
	dest_g.y = border;

	switch (type)
	{
	case SimpleButton:
		/* do nothing */
		break;
	case SolidButton:
		/* overwrite with the default background */
		dest_g.x = 0;
		dest_g.y = 0;
		border_fill_pixmap_background(dest_pix, &dest_g, solid_bg, cd);
		break;
	case VectorButton:
	case DefaultVectorButton:
		border_draw_vector_to_pixmap(
			dest_pix, cd, is_toggled, &df->u.vector, w_g);
		break;
	case MiniIconButton:
	case PixmapButton:
	case ShrunkPixmapButton:
	case StretchedPixmapButton:
		if (w_g->width - 2*border <= 0 || w_g->height - 2*border <= 0)
		{
			break;
		}
		if (FMiniIconsSupported && type == MiniIconButton)
		{
			if (!fw->mini_icon)
			{
				break;
			}
			p = fw->mini_icon;
			if (cd->cs >= 0)
			{
				bg.pixmap.fra.mask |= FRAM_HAVE_ICON_CSET;
				bg.pixmap.fra.colorset = &Colorset[cd->cs];
			}
		}
		else
		{
			p = df->u.p;
		}
		width = p->width;
		height = p->height;
		if ((type == ShrunkPixmapButton || type == MiniIconButton) &&
		    (p->width > w_g->width - 2*border ||
		     p->height > w_g->height - 2*border))
		{
			/* do so that the picture fit into the destination */
			bg.pixmap.stretch_w = width =
				min(w_g->width - 2*border, p->width);
			bg.pixmap.stretch_h = height =
				min(w_g->height - 2*border, p->height);
			bg.pixmap.flags.is_stretched = 1;
		}
		else if (type == StretchedPixmapButton &&
			 (p->width < w_g->width - 2*border ||
			  p->height < w_g->height - 2*border))
		{
			/* do so that the picture fit into the destination */
			bg.pixmap.stretch_w = width =
				max(w_g->width - 2*border, p->width);
			bg.pixmap.stretch_h = height =
				max(w_g->height - 2*border, p->height);
			bg.pixmap.flags.is_stretched = 1;
		}
		lr_just = DFS_H_JUSTIFICATION(df->style);
		tb_just = DFS_V_JUSTIFICATION(df->style);
		if (!td->td_is_rotated && fw->title_text_rotation != ROTATION_0)
		{
			if (fw->title_text_rotation == ROTATION_180)
			{
				switch (lr_just)
				{
				case JUST_LEFT:
					lr_just = JUST_RIGHT;
					break;
				case JUST_RIGHT:
					lr_just = JUST_LEFT;
					break;
				case JUST_CENTER:
				default:
					break;
				}
				switch (tb_just)
				{
				case JUST_TOP:
					tb_just = JUST_BOTTOM;
					break;
				case JUST_BOTTOM:
					tb_just = JUST_TOP;
					break;
				case JUST_CENTER:
				default:
					break;
				}
			}
			else if (fw->title_text_rotation == ROTATION_90)
			{
				switch (lr_just)
				{
				case JUST_LEFT:
					tb_just = JUST_TOP;
					break;
				case JUST_RIGHT:
					tb_just = JUST_BOTTOM;
					break;
				case JUST_CENTER:
				default:
					tb_just = JUST_CENTER;
					break;
				}
				switch (DFS_V_JUSTIFICATION(df->style))
				{
				case JUST_TOP:
					lr_just = JUST_RIGHT;
					break;
				case JUST_BOTTOM:
					lr_just = JUST_LEFT;
					break;
				case JUST_CENTER:
				default:
					lr_just = JUST_CENTER;
					break;
				}
			}
			else if (fw->title_text_rotation == ROTATION_270)
			{
				switch (lr_just)
				{
				case JUST_LEFT:
					tb_just = JUST_BOTTOM;
					break;
				case JUST_RIGHT:
					tb_just = JUST_TOP;
					break;
				case JUST_CENTER:
				default:
					tb_just = JUST_CENTER;
					break;
				}
				switch (DFS_V_JUSTIFICATION(df->style))
				{
				case JUST_TOP:
					lr_just = JUST_LEFT;
					break;
				case JUST_BOTTOM:
					lr_just = JUST_RIGHT;
					break;
				case JUST_CENTER:
				default:
					lr_just = JUST_CENTER;
					break;
				}
			}
		}
		switch (lr_just)
		{
		case JUST_LEFT:
			dest_g.x = border;
			break;
		case JUST_RIGHT:
			dest_g.x = (int)(w_g->width - width - border);
			break;
		case JUST_CENTER:
		default:
			/* round down */
			dest_g.x = (int)(w_g->width - width) / 2;
			break;
		}
		switch (tb_just)
		{
		case JUST_TOP:
			dest_g.y = border;
			break;
		case JUST_BOTTOM:
			dest_g.y = (int)(w_g->height - height - border);
			break;
		case JUST_CENTER:
		default:
			/* round down */
			dest_g.y = (int)(w_g->height - height) / 2;
			break;
		}
		if (dest_g.x < border)
		{
			dest_g.x = border;
		}
		if (dest_g.y < border)
		{
			dest_g.y = border;
		}
		bg.flags.use_pixmap = 1;
		bg.pixmap.p = p->picture;
		bg.pixmap.shape = p->mask;
		bg.pixmap.alpha = p->alpha;
		bg.pixmap.depth = p->depth;
		bg.pixmap.g.width = p->width;
		bg.pixmap.g.height = p->height;
		border_fill_pixmap_background(dest_pix, &dest_g, &bg, cd);
		break;
	case TiledPixmapButton:
	case AdjustedPixmapButton:
		if (w_g->width - 2*border <= 0 || w_g->height - 2*border <= 0)
		{
			break;
		}
		p = df->u.p;
		if (type == TiledPixmapButton)
		{
			bg.pixmap.flags.is_tiled = 1;
		}
		else
		{
			bg.pixmap.stretch_w = width = w_g->width - 2*dest_g.x;
			bg.pixmap.stretch_h = height = w_g->height - 2*dest_g.y;
			bg.pixmap.flags.is_stretched = 1;
		}
		bg.flags.use_pixmap = 1;
		bg.pixmap.p = p->picture;
		bg.pixmap.shape = p->mask;
		bg.pixmap.alpha = p->alpha;
		bg.pixmap.depth = p->depth;
		bg.pixmap.g.width = p->width;
		bg.pixmap.g.height = p->height;
		border_fill_pixmap_background(dest_pix, &dest_g, &bg, cd);
		break;
	case MultiPixmap: /* for UseTitleStyle only */
	{
		int is_left = left1right0;
		int part = TBMP_NONE;
		int ap, cs;
		unsigned int stretch;
		Pixmap tmp = None;
		FvwmPicture *full_pic = NULL;
		rectangle g;
		dynamic_common_decorations *dcd = &(cd->dynamic_cd);
		FvwmPicture **pm;
		FvwmAcs *acs;
		Pixel *pixels;
		unsigned short sf;

		pm = df->u.mp.pixmaps;
		acs = df->u.mp.acs;
		pixels = df->u.mp.pixels;
		sf = df->u.mp.solid_flags;
		if (!border_mp_get_use_title_style_parts_and_geometry(
			td, pm, acs, sf, is_left, &g, &part))
		{
			g.width = 0;
			g.height = 0;
			g.x = 0;
			g.y = 0;
		}

		if (part == TBMP_NONE)
		{
			break;
		}

		if (sf & (1 << part))
		{
			bg.flags.use_pixmap = 0;
			bg.pixel = pixels[part];
			border_fill_pixmap_background(
				dest_pix, &dest_g, &bg, cd);
			break;
		}
		cs = acs[part].cs;
		ap =  acs[part].alpha_percent;
		if (CSET_IS_TRANSPARENT_PR(cs))
		{
			break;
		}
		if (cs >= 0)
		{
			bg.pixmap.fra.mask = FRAM_HAVE_ADDED_ALPHA;
			bg.pixmap.fra.added_alpha_percent = ap;
		}
		stretch = !!(df->u.mp.stretch_flags & (1 << part));
		bg.flags.use_pixmap = 1;
		dest_g.x = 0;
		dest_g.y = 0;

		if (cs >= 0 && use_title_style && g.width > 0 && g.height > 0 &&
		    !CSET_IS_TRANSPARENT_ROOT(cs) &&
		    (bg.pixmap.p = border_get_bar_pixmaps(
			    dcd, &g, bs, cs, NULL, part, stretch, NULL)) != None)
		{
			bg.pixmap.g.width = g.width;
			bg.pixmap.g.height = g.height;
			bg.pixmap.flags.is_tiled = 1;
			bg.pixmap.g.x = w_g->x - g.x;
			bg.pixmap.g.y = w_g->y - g.y;
			bg.pixmap.shape = None;
			bg.pixmap.alpha = None;
			bg.pixmap.depth = Pdepth;
		}
		else if (CSET_IS_TRANSPARENT_ROOT(cs))
		{
			tmp = border_create_root_transparent_pixmap(
				td, w, w_g->width, w_g->height, cs);
			bg.pixmap.p = tmp;
			bg.pixmap.g.width = w_g->width;
			bg.pixmap.g.height = w_g->height;
			bg.pixmap.shape = None;
			bg.pixmap.alpha = None;
			bg.pixmap.depth = Pdepth;
		}
		else if (cs >= 0)
		{
			int bg_w, bg_h;

			tmp = CreateBackgroundPixmap(
				dpy, w, w_g->width,
				w_g->height, &Colorset[cs],
				Pdepth, Scr.BordersGC, False);
			bg.pixmap.p = tmp;
			GetWindowBackgroundPixmapSize(
				&Colorset[cs], w_g->width,
				w_g->height, &bg_w, &bg_h);
			bg.pixmap.g.width = bg_w;
			bg.pixmap.g.height = bg_h;
			bg.pixmap.shape = None;
			bg.pixmap.alpha = None;
			bg.pixmap.depth = Pdepth;
			bg.pixmap.flags.is_tiled = 1;
		}
		else if (pm[part] && g.width > 0 && g.height > 0 &&
			 border_get_bar_pixmaps(
				 dcd, &g, bs, -1, pm[part], part, stretch,
				 &full_pic) != None && full_pic)
		{
			bg.pixmap.p = full_pic->picture;
			bg.pixmap.shape = full_pic->mask;
			bg.pixmap.alpha = full_pic->alpha;
			bg.pixmap.depth = full_pic->depth;
			bg.pixmap.g.width = full_pic->width;
			bg.pixmap.g.height = full_pic->height;
			bg.pixmap.g.x = w_g->x - g.x;
			bg.pixmap.g.y = w_g->y - g.y;
		}
		else if (pm[part])
		{
			p = pm[part];
			if (df->u.mp.stretch_flags & (1 << part))
			{
				bg.pixmap.flags.is_stretched = 1;
			}
			else
			{
				bg.pixmap.flags.is_tiled = 1;
			}
			bg.pixmap.p = p->picture;
			bg.pixmap.shape = p->mask;
			bg.pixmap.alpha = p->alpha;
			bg.pixmap.depth = p->depth;
			bg.pixmap.g.width = p->width;
			bg.pixmap.g.height = p->height;
			bg.pixmap.stretch_w = dest_g.width - dest_g.x;
			bg.pixmap.stretch_h = dest_g.height - dest_g.y;
		}
		else
		{
			/* should not happen */
			return;
		}
		if (bg.pixmap.p != None)
		{
			border_fill_pixmap_background(
				dest_pix, &dest_g, &bg, cd);
		}
		if (tmp != None)
		{
			XFreePixmap(dpy, tmp);
		}
		break;
	}
	case ColorsetButton:
	{
		colorset_t *cs_t = &Colorset[df->u.acs.cs];
		int cs = df->u.acs.cs;
		Pixmap tmp = None;
		int bg_w, bg_h;

		if (CSET_IS_TRANSPARENT_PR(cs))
		{
			break;
		}
		dest_g.x = 0;
		dest_g.y = 0;
		if (use_title_style &&
		    !CSET_IS_TRANSPARENT_ROOT(cs) &&
		    (bg.pixmap.p = border_get_bar_pixmaps(
			    &(cd->dynamic_cd), &(td->bar_g), bs, cs, NULL,
			    TBMP_NONE, 0, NULL))
		    != None)
		{
			bg.pixmap.g.width = td->bar_g.width;
			bg.pixmap.g.height = td->bar_g.height;
			bg.pixmap.g.x = w_g->x - td->bar_g.x;
			bg.pixmap.g.y = w_g->y - td->bar_g.y;
		}
		else if (CSET_IS_TRANSPARENT_ROOT(cs))
		{
			tmp = border_create_root_transparent_pixmap(
				td, w, w_g->width, w_g->height, cs);
			if (tmp == None)
			{
				break;
			}
			bg.pixmap.p = tmp;
			bg.pixmap.g.width = w_g->width;
			bg.pixmap.g.height = w_g->height;
			bg.pixmap.shape = None;
			bg.pixmap.alpha = None;
			bg.pixmap.depth = Pdepth;
		}
		else
		{
			tmp = CreateBackgroundPixmap(
				dpy, w, w_g->width, w_g->height,
				cs_t, Pdepth, Scr.BordersGC, False);
			if (tmp == None)
			{
				break;
			}
			bg.pixmap.p = tmp;
			GetWindowBackgroundPixmapSize(
				cs_t, w_g->width, w_g->height,
				&bg_w, &bg_h);
			bg.pixmap.g.width = bg_w;
			bg.pixmap.g.height = bg_h;
			bg.pixmap.g.x = 0;
			bg.pixmap.g.y = 0;
		}
		bg.flags.use_pixmap = 1;
		bg.pixmap.shape = None;
		bg.pixmap.alpha = None;
		bg.pixmap.depth = Pdepth;
		bg.pixmap.flags.is_tiled = 1;
		bg.pixmap.fra.mask = FRAM_HAVE_ADDED_ALPHA;
		bg.pixmap.fra.added_alpha_percent = df->u.acs.alpha_percent;
		border_fill_pixmap_background(dest_pix, &dest_g, &bg, cd);
		if (tmp)
		{
			XFreePixmap(dpy, tmp);
		}
		break;
	}
	case GradientButton:
		/* draw the gradient into the pixmap */
		CreateGradientPixmap(
			dpy, dest_pix, Scr.TransMaskGC,
			df->u.grad.gradient_type, 0, 0, df->u.grad.npixels,
			df->u.grad.xcs, df->u.grad.do_dither,
			&df->u.grad.d_pixels, &df->u.grad.d_npixels,
			dest_pix, 0, 0, w_g->width, w_g->height, NULL);

		break;

	default:
		fvwm_msg(ERR, "DrawButton", "unknown button type: %i", type);
		break;
	}

	return;
}

static void border_set_button_pixmap(
	FvwmWindow *fw, titlebar_descr *td, int button, Pixmap *dest_pix,
	Window w)
{
	pixmap_background_type bg;
	unsigned int mask;
	int is_left_button;
	int do_reverse_relief;
	ButtonState bs;
	DecorFace *df;
	rectangle *button_g;
	GC rgc;
	GC sgc;
	Bool free_bg_pixmap = False;
	rectangle pix_g;

	/* prepare variables */
	mask = (1 << button);
	if (td->has_an_upsidedown_rotation)
	{
		is_left_button = (button & 1);
	}
	else
	{
		is_left_button = !(button & 1);
	}
	button_g = &td->layout.button_g[button];
	bs = td->tbstate.bstate[button];
	df = &TB_STATE(GetDecor(fw, buttons[button]))[bs];
	rgc = td->cd->relief_gc;
	sgc = td->cd->shadow_gc;
	/* prepare background, either from the window colour or from the
	 * border style */
	if (!DFS_USE_BORDER_STYLE(df->style))
	{
		/* fill with the button background colour */
		bg.flags.use_pixmap = 0;
		bg.pixel = td->cd->back_color;
		pix_g.x = 0;
		pix_g.y = 0;
		pix_g.width = button_g->width;
		pix_g.height = button_g->height;
		border_fill_pixmap_background(*dest_pix, &pix_g, &bg, td->cd);
	}
	else
	{
		/* draw pixmap background inherited from border style */
		rectangle relative_g;

		relative_g.width = td->frame_g.width;
		relative_g.height = td->frame_g.height;
		relative_g.x = button_g->x;
		relative_g.y = button_g->y;
		border_get_border_background(
			&bg, td->cd, button_g, &relative_g, &free_bg_pixmap, w);
		bg.pixmap.g.x = 0;
		bg.pixmap.g.y = 0;
		/* set the geometry for drawing the Tiled pixmap;
		 * FIXME: maybe add the relief as offset? */
		pix_g.x = 0;
		pix_g.y = 0;
		pix_g.width = button_g->width;
		pix_g.height = button_g->height;
		border_fill_pixmap_background(*dest_pix, &pix_g, &bg, td->cd);
		if (free_bg_pixmap && bg.pixmap.p)
		{
			XFreePixmap(dpy, bg.pixmap.p);
		}
	}

	/* handle title style */
	if (DFS_USE_TITLE_STYLE(df->style))
	{
		/* draw background inherited from title style */
		DecorFace *tsdf;
		Pixmap tmp;

		if (td->draw_rotation != ROTATION_0)
		{
			tmp = CreateRotatedPixmap(
				dpy, *dest_pix,
				td->layout.button_g[button].width,
				td->layout.button_g[button].height,
				Pdepth, Scr.BordersGC, td->restore_rotation);
			XFreePixmap(dpy, *dest_pix);
			*dest_pix = tmp;
			border_rotate_titlebar_descr(fw, td);
			button_g = &td->layout.button_g[button];
			is_left_button = !(button & 1);
		}
		for (tsdf = &TB_STATE(GetDecor(fw, titlebar))[bs]; tsdf != NULL;
		     tsdf = tsdf->next)
		{
			bg.pixel = tsdf->u.back;
			border_draw_decor_to_pixmap(
				fw, *dest_pix, w, &bg, button_g, tsdf, td,
				bs, True, (td->tbstate.toggled_bmask & mask),
				is_left_button);
		}
		if (td->draw_rotation != ROTATION_0)
		{
			tmp = CreateRotatedPixmap(
				dpy, *dest_pix,
				td->layout.button_g[button].width,
				td->layout.button_g[button].height,
				Pdepth, Scr.BordersGC, td->draw_rotation);
			XFreePixmap(dpy, *dest_pix);
			*dest_pix = tmp;
			border_rotate_titlebar_descr(fw, td);
			button_g = &td->layout.button_g[button];
			if (td->has_an_upsidedown_rotation)
			{
				is_left_button = (button & 1);
			}
			else
			{
				is_left_button = !(button & 1);
			}
		}
	}
	/* handle button style */
	for ( ; df; df = df->next)
	{
		/* draw background from button style */
		bg.pixel = df->u.back;
		border_draw_decor_to_pixmap(
			fw, *dest_pix, w, &bg, button_g, df, td, bs, False,
			(td->tbstate.toggled_bmask & mask), is_left_button);
	}
	/* draw the button relief */
	do_reverse_relief = !!(td->tbstate.pressed_bmask & mask);
	switch (DFS_BUTTON_RELIEF(
			TB_STATE(GetDecor(fw, buttons[button]))[bs].style))
	{
	case DFS_BUTTON_IS_SUNK:
		do_reverse_relief ^= 1;
		/* fall through*/
	case DFS_BUTTON_IS_UP:
		do_relieve_rectangle(
			dpy, *dest_pix, 0, 0, button_g->width - 1,
			button_g->height - 1,
			(do_reverse_relief) ? sgc : rgc,
			(do_reverse_relief) ? rgc : sgc,
			td->cd->relief_width, True);
		break;
	default:
		/* flat */
		break;
	}

	return;
}

static void border_draw_one_button(
	FvwmWindow *fw, titlebar_descr *td, int button)
{
	Pixmap p;

	/* make a pixmap */
	if (td->layout.button_g[button].x < 0 ||
	    td->layout.button_g[button].y < 0)
	{
		return;
	}

	p = border_create_decor_pixmap(td->cd, &(td->layout.button_g[button]));
	/* set the background tile */
	border_set_button_pixmap(fw, td, button, &p, FW_W_BUTTON(fw, button));
	/* apply the pixmap and destroy it */
	border_set_part_background(FW_W_BUTTON(fw, button), p);
	XFreePixmap(dpy, p);
	if ((td->tbstate.clear_bmask & (1 << button)) != 0)
	{
		XClearWindow(dpy, FW_W_BUTTON(fw, button));
	}

	return;
}

static void border_draw_title_stick_lines(
	FvwmWindow *fw, titlebar_descr *td, title_draw_descr *tdd,
	Pixmap dest_pix)
{
	int i;
	int num;
	int min;
	int max;
	int left_x;
	int left_w;
	int right_x;
	int right_w;
	int under_text_length = 0;
	int under_text_offset = 0;
	int right_length = 0;
	int left_length = 0;
	rotation_t rotation;

	if (!( (HAS_STICKY_STIPPLED_TITLE(fw) &&
		(IS_STICKY_ACROSS_PAGES(fw) || IS_STICKY_ACROSS_DESKS(fw)))
	       || HAS_STIPPLED_TITLE(fw)))
	{
		return;
	}
	if (td->td_is_rotated)
	{
		rotation = td->restore_rotation;
	}
	else
	{
		rotation = ROTATION_0;
	}
	if (td->has_vt && td->under_text_g.height > 0)
	{
		under_text_length = td->under_text_g.height;
		under_text_offset = td->under_text_g.y;
		left_length = td->left_main_g.height - td->left_of_text_length
			- td->left_end_length;
		right_length = td->right_main_g.height -
			td->right_of_text_length - td->right_end_length;

	}
	else if (!td->has_vt && td->under_text_g.width > 0)
	{
		under_text_length = td->under_text_g.width;
		under_text_offset = td->under_text_g.x;
		left_length = td->left_main_g.width - td->left_of_text_length
			- td->left_end_length;
		right_length = td->right_main_g.width -
			td->right_of_text_length - td->right_end_length;
	}

	/* If the window is sticky either across pages or
	 * desks and it has a stippled title, but nothing for
	 * sticky_stippled_title, then don't bother drawing them, just
	 * return immediately. -- Thomas Adam
	 */
	if ( (IS_STICKY_ACROSS_PAGES(fw) || IS_STICKY_ACROSS_DESKS(fw)) &&
	    (!HAS_STICKY_STIPPLED_TITLE(fw) && HAS_STIPPLED_TITLE(fw)) )
	{
		return;
	}

	num = (int)(fw->title_thickness / WINDOW_TITLE_STICK_VERT_DIST / 2) *
		2 - 1;
	min = fw->title_thickness / 2 - num * 2 + 1;
	max = fw->title_thickness / 2 + num * 2 -
		WINDOW_TITLE_STICK_VERT_DIST + 1;
	left_x = WINDOW_TITLE_STICK_OFFSET + td->left_end_length;
	left_w = ((under_text_length == 0)? td->offset:under_text_offset)
		- left_x - WINDOW_TITLE_TO_STICK_GAP - td->left_of_text_length;
	right_x = ((under_text_length == 0)?
		   td->offset + td->length :
		   under_text_offset + under_text_length)
		+ td->right_of_text_length + WINDOW_TITLE_TO_STICK_GAP - 1;
	right_w = fw->title_length - right_x - WINDOW_TITLE_STICK_OFFSET
		- td->right_end_length;
	/* an odd number of lines every WINDOW_TITLE_STICK_VERT_DIST pixels */
	if (left_w < WINDOW_TITLE_STICK_MIN_WIDTH)
	{
		left_x = td->left_end_length +
			((left_length > WINDOW_TITLE_STICK_MIN_WIDTH)?
			 (left_length - WINDOW_TITLE_STICK_MIN_WIDTH)/2 : 0);
		left_w = WINDOW_TITLE_STICK_MIN_WIDTH;
	}
	if (right_w < WINDOW_TITLE_STICK_MIN_WIDTH)
	{
		right_w = WINDOW_TITLE_STICK_MIN_WIDTH;
		right_x = fw->title_length - WINDOW_TITLE_STICK_MIN_WIDTH - 1
			- td->right_end_length -
			((right_length > WINDOW_TITLE_STICK_MIN_WIDTH)?
			 (right_length -
			  WINDOW_TITLE_STICK_MIN_WIDTH)/2 : 0);
	}
	for (i = min; i <= max; i += WINDOW_TITLE_STICK_VERT_DIST)
	{
		if (left_w > 0)
		{
			do_relieve_rectangle_with_rotation(
				dpy, dest_pix,
				SWAP_ARGS(td->has_vt, left_x, i),
				SWAP_ARGS(td->has_vt, left_w, 1),
				tdd->sgc, tdd->rgc, 1, False, rotation);
		}
		if (right_w > 0)
		{
			do_relieve_rectangle_with_rotation(
				dpy, dest_pix,
				SWAP_ARGS(td->has_vt, right_x, i),
				SWAP_ARGS(td->has_vt, right_w, 1),
				tdd->sgc, tdd->rgc, 1, False, rotation);
		}
	}

	return;
}

static void border_draw_title_mono(
	FvwmWindow *fw, titlebar_descr *td, title_draw_descr *tdd,
	FlocaleWinString *fstr, Pixmap dest_pix)
{
	int has_vt;

	has_vt = HAS_VERTICAL_TITLE(fw);
	XFillRectangle(
		dpy, dest_pix, td->cd->relief_gc,
		td->offset - 2, 0, td->length+4, fw->title_thickness);
	if (fw->visible_name != (char *)NULL)
	{
		FlocaleDrawString(dpy, fw->title_font, fstr, 0);
	}
	/* for mono, we clear an area in the title bar where the window
	 * title goes, so that its more legible. For color, no need */
	do_relieve_rectangle(
		dpy, dest_pix, 0, 0,
		SWAP_ARGS(has_vt, td->offset - 3,
			  fw->title_thickness - 1),
		tdd->rgc, tdd->sgc, td->cd->relief_width, False);
	do_relieve_rectangle(
		dpy, dest_pix,
		SWAP_ARGS(has_vt, td->offset + td->length + 2, 0),
		SWAP_ARGS(has_vt, fw->title_length - td->length -
			  td->offset - 3, fw->title_thickness - 1),
		tdd->rgc, tdd->sgc, td->cd->relief_width, False);
	XDrawLine(
		dpy, dest_pix, tdd->sgc,
		SWAP_ARGS(has_vt, 0, td->offset + td->length + 1),
		SWAP_ARGS(has_vt, td->offset + td->length + 1,
			  fw->title_thickness));

	return;
}

static void border_draw_title_relief(
	FvwmWindow *fw, titlebar_descr *td, title_draw_descr *tdd,
	Pixmap dest_pix)
{
	int reverse = 0;
	rotation_t rotation;

	if (td->td_is_rotated)
	{
		rotation = td->restore_rotation;
	}
	else
	{
		rotation = ROTATION_0;
	}
	/* draw title relief */
	switch (DFS_BUTTON_RELIEF(*tdd->tstyle))
	{
	case DFS_BUTTON_IS_SUNK:
		reverse = 1;
	case DFS_BUTTON_IS_UP:
		do_relieve_rectangle_with_rotation(
			dpy, dest_pix, 0, 0,
			SWAP_ARGS(
				td->has_vt, fw->title_length - 1,
				fw->title_thickness - 1),
			(reverse) ? tdd->sgc : tdd->rgc,
			(reverse) ? tdd->rgc : tdd->sgc, td->cd->relief_width,
			True, rotation);
		break;
	default:
		/* flat */
		break;
	}

	return;
}

static void border_draw_title_deep(
	FvwmWindow *fw, titlebar_descr *td, title_draw_descr *tdd,
	FlocaleWinString *fstr, Pixmap dest_pix, Window w)
{
	DecorFace *df;
	pixmap_background_type bg;

	bg.flags.use_pixmap = 0;
	for (df = tdd->df; df != NULL; df = df->next)
	{
		if (df->style.face_type == MultiPixmap)
		{
			border_mp_draw_mp_titlebar(
				fw, td, df, dest_pix, w);
		}
		else
		{
			bg.pixel = df->u.back;
			border_draw_decor_to_pixmap(
				fw, dest_pix, w, &bg, &td->layout.title_g, df,
				td, td->tbstate.tstate, True, tdd->is_toggled,
				1);
		}
	}
	FlocaleDrawString(dpy, fw->title_font, &tdd->fstr, 0);

	return;
}

static void border_get_titlebar_draw_descr(
	FvwmWindow *fw, titlebar_descr *td, title_draw_descr *tdd,
	Pixmap dest_pix)
{
	memset(tdd, 0, sizeof(*tdd));
	/* prepare the gcs and variables */
	if (td->tbstate.is_title_pressed)
	{
		tdd->rgc = td->cd->shadow_gc;
		tdd->sgc = td->cd->relief_gc;
	}
	else
	{
		tdd->rgc = td->cd->relief_gc;
		tdd->sgc = td->cd->shadow_gc;
	}
	NewFontAndColor(fw->title_font, td->cd->fore_color, td->cd->back_color);
	tdd->tstyle = &TB_STATE(
		GetDecor(fw, titlebar))[td->tbstate.tstate].style;
	tdd->df = &TB_STATE(GetDecor(fw, titlebar))[td->tbstate.tstate];

	/* fetch the title string */
	tdd->fstr.str = fw->visible_name;
	tdd->fstr.win = dest_pix;
	if (td->td_is_rotated)
	{
		tdd->fstr.flags.text_rotation = ROTATION_0;
	}
	else
	{
		tdd->fstr.flags.text_rotation = fw->title_text_rotation;
	}
	if (td->has_vt)
	{
		tdd->fstr.y = td->offset;
		tdd->fstr.x = fw->title_text_offset + 1;
	}
	else
	{
		tdd->fstr.x = td->offset;
		tdd->fstr.y = fw->title_text_offset + 1;
	}
	if (td->cd->cs >= 0)
	{
		tdd->fstr.colorset = &Colorset[td->cd->cs];
		tdd->fstr.flags.has_colorset = 1;
	}
	tdd->fstr.gc = Scr.TitleGC;

	return;
}

static void border_set_title_pixmap(
	FvwmWindow *fw, titlebar_descr *td, Pixmap *dest_pix, Window w)
{
	pixmap_background_type bg;
	title_draw_descr tdd;
	FlocaleWinString fstr;
	Bool free_bg_pixmap = False;
	rectangle pix_g;

	border_get_titlebar_draw_descr(fw, td, &tdd, *dest_pix);
	/* prepare background, either from the window colour or from the
	 * border style */
	if (!DFS_USE_BORDER_STYLE(*tdd.tstyle))
	{
		/* fill with the button background colour */
		bg.flags.use_pixmap = 0;
		bg.pixel = td->cd->back_color;
		pix_g.x = 0;
		pix_g.y = 0;
		pix_g.width = td->layout.title_g.width;
		pix_g.height = td->layout.title_g.height;
		border_fill_pixmap_background(
			*dest_pix, &pix_g, &bg, td->cd);
	}
	else
	{
		/* draw pixmap background inherited from border style */
		rectangle relative_g;
		Pixmap tmp;

		if (td->draw_rotation != ROTATION_0)
		{
			tmp = CreateRotatedPixmap(
				dpy, *dest_pix,
				td->layout.title_g.width,
				td->layout.title_g.height,
				Pdepth, Scr.BordersGC, td->restore_rotation);
			XFreePixmap(dpy, *dest_pix);
			*dest_pix = tmp;
			border_rotate_titlebar_descr(fw, td);
		}
		relative_g.width = td->frame_g.width;
		relative_g.height = td->frame_g.height;
		relative_g.x = td->layout.title_g.x;
		relative_g.y = td->layout.title_g.y;
		border_get_border_background(
			&bg, td->cd, &td->layout.title_g, &relative_g,
			&free_bg_pixmap, w);
		bg.pixmap.g.x = 0;
		bg.pixmap.g.y = 0;
		/* set the geometry for drawing the Tiled pixmap;
		 * FIXME: maybe add the relief as offset? */
		pix_g.x = 0;
		pix_g.y = 0;
		pix_g.width = td->layout.title_g.width;
		pix_g.height = td->layout.title_g.height;
		border_fill_pixmap_background(
			*dest_pix, &pix_g, &bg, td->cd);
		if (free_bg_pixmap && bg.pixmap.p)
		{
			XFreePixmap(dpy, bg.pixmap.p);
		}
		if (td->draw_rotation != ROTATION_0)
		{
			tmp = CreateRotatedPixmap(
				dpy, *dest_pix,
				td->layout.title_g.width,
				td->layout.title_g.height,
				Pdepth, Scr.BordersGC, td->draw_rotation);
			XFreePixmap(dpy, *dest_pix);
			*dest_pix = tmp;
			border_rotate_titlebar_descr(fw, td);
		}
	}

	if (Pdepth < 2)
	{
		border_draw_title_mono(fw, td, &tdd, &fstr, *dest_pix);
	}
	else
	{
		border_draw_title_deep(fw, td, &tdd, &fstr, *dest_pix, w);
	}
	border_draw_title_relief(fw, td, &tdd, *dest_pix);
	border_draw_title_stick_lines(fw, td, &tdd, *dest_pix);

	return;
}

static void border_draw_title(
	FvwmWindow *fw, titlebar_descr *td)
{
	Pixmap p;

	if (td->layout.title_g.x < 0 || td->layout.title_g.y < 0)
	{
		return;
	}
	if (td->draw_rotation != ROTATION_0)
	{
		border_rotate_titlebar_descr(fw, td);
	}
	/* make a pixmap */
	p = border_create_decor_pixmap(td->cd, &(td->layout.title_g));
	/* set the background tile */
#if 0
	fprintf(stderr,"drawing title\n");
#endif
	border_set_title_pixmap(fw, td, &p, FW_W_TITLE(fw));
	if (td->draw_rotation != ROTATION_0)
	{
		Pixmap tmp;

		tmp = CreateRotatedPixmap(
			dpy, p, td->layout.title_g.width,
			td->layout.title_g.height, Pdepth, Scr.BordersGC,
			td->draw_rotation);
		XFreePixmap(dpy, p);
		p = tmp;
		border_rotate_titlebar_descr(fw, td);
	}
	/* apply the pixmap and destroy it */
	border_set_part_background(FW_W_TITLE(fw), p);
	XFreePixmap(dpy, p);
	if (td->tbstate.do_clear_title)
	{
		XClearWindow(dpy, FW_W_TITLE(fw));
	}

	return;
}

static void border_draw_buttons(
	FvwmWindow *fw, titlebar_descr *td)
{
	int i;

	/* draw everything in a big loop */
#if 0
	fprintf(stderr, "drawing buttons 0x%04x\n", td->tbstate.draw_bmask);
#endif
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		unsigned int mask = (1 << i);

		if ((td->tbstate.draw_bmask & mask) != 0)
		{
			border_draw_one_button(fw, td, i);
		}
	}
	/* update the button states */
	fw->decor_state.buttons_drawn |= td->tbstate.draw_bmask;
	fw->decor_state.buttons_inverted = td->tbstate.pressed_bmask;
	fw->decor_state.buttons_lit = td->tbstate.lit_bmask;
	fw->decor_state.buttons_toggled = td->tbstate.toggled_bmask;

	return;
}

static void border_setup_use_title_style(
	FvwmWindow *fw, titlebar_descr *td)
{
	int i;
	DecorFace *df, *tsdf;
	ButtonState bs, tsbs;

	/* use a full bar pixmap (for Colorset) or non window size pixmaps
	 * (for MultiPixmap) under certain condition:
	 * - for the buttons which use title style
	 * - for title which have a button with UseTitle style
	 */
	tsbs = td->tbstate.tstate;
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		bs = td->tbstate.bstate[i];
		df = &TB_STATE(GetDecor(fw, buttons[i]))[bs];
		tsdf = &TB_STATE(GetDecor(fw, buttons[i]))[tsbs];
		if (FW_W_BUTTON(fw, i) != None)
		{
			if (DFS_USE_TITLE_STYLE(df->style))
			{
				border_setup_bar_pixmaps(
					td, &(td->cd->dynamic_cd),
					&TB_STATE(GetDecor(fw, titlebar))[bs],
					bs);
			}
			if (DFS_USE_TITLE_STYLE(tsdf->style))
			{
				border_setup_bar_pixmaps(
					td, &(td->cd->dynamic_cd),
					&TB_STATE(GetDecor(fw, titlebar))[tsbs],
					tsbs);
			}
		}
	}
	return;
}

static void border_rotate_titlebar_descr(
	FvwmWindow *fw, titlebar_descr *td)
{
	rotation_t rotation;
	int i, tmpi;
	static titlebar_descr saved_td;

	if (td->draw_rotation == ROTATION_0)
	{
		return;
	}
	if (!td->has_been_saved)
	{
		td->has_been_saved = True;
		memcpy(&saved_td, td, sizeof(titlebar_descr));
	}
	if (!td->td_is_rotated)
	{
		/* make the bar horizontal */
		switch(td->draw_rotation)
		{
		case ROTATION_90: /* cw */
			rotation = ROTATION_270;
			break;
		case ROTATION_270: /* ccw */
			rotation = ROTATION_90;
			break;
		case ROTATION_180:
			rotation = ROTATION_180;
			break;
		default:
			return;
		}
		td->has_vt = 0;
		td->has_an_upsidedown_rotation = 0;
		td->td_is_rotated = 1;
	}
	else
	{
		/* restore */
		memcpy(td, &saved_td, sizeof(titlebar_descr));
		td->td_is_rotated = 0;
		return;
	}

#define ROTATE_RECTANGLE(rot, r, vs_frame, vs_titlebar, vs_title) \
	{ \
		rectangle tr; \
		tr.x = r->x; \
		tr.y = r->y; \
		tr.width = r->width; \
		tr.height = r->height; \
		switch(rot) \
		{ \
		case ROTATION_270: /* ccw */ \
			tr.x = r->y; \
			if (vs_frame) \
			{ \
				tr.y = td->frame_g.width - (r->x+r->width); \
			} \
			else if (vs_titlebar) \
			{ \
				tr.y = td->bar_g.width - \
					(r->x+r->width); \
			} \
			else if (vs_title) \
			{ \
				tr.y = td->layout.title_g.width - \
					(r->x+r->width); \
			} \
			else \
			{ \
				tr.y = r->x; \
			} \
			tr.width = r->height; \
			tr.height = r->width; \
			break; \
		case ROTATION_90: /* cw */ \
			if (vs_frame) \
			{ \
				tr.x = td->frame_g.height - (r->y+r->height); \
			} \
			else if (vs_titlebar) \
			{ \
				tr.x = td->bar_g.height - \
					(r->y+r->height); \
			} \
			else if (vs_title) \
			{ \
				tr.x = td->layout.title_g.height - \
					(r->y+r->height); \
			} \
			else \
			{ \
				tr.x = r->y; \
			} \
			tr.y = r->x; \
			tr.width = r->height; \
			tr.height = r->width; \
			break; \
		case ROTATION_180: \
			if (vs_frame) \
			{ \
				tr.x = td->frame_g.width - (r->x+r->width); \
			} \
			else if (vs_titlebar) \
			{ \
				tr.x = td->bar_g.width - \
					(r->x + r->width); \
			} \
			else if (vs_title) \
			{ \
				tr.x = td->layout.title_g.width - \
					(r->x + r->width); \
			} \
			else \
			{ \
				tr.x = r->x; \
			} \
			break; \
		case ROTATION_0: \
			break; \
		} \
		r->x = tr.x; \
		r->y = tr.y; \
		r->width = tr.width; \
		r->height = tr.height; \
	}

	switch(rotation)
	{
	case ROTATION_90:
		td->offset =
			td->layout.title_g.height - td->offset - td->length;
		tmpi = td->left_end_length;
		td->left_end_length = td->right_end_length;
		td->right_end_length = tmpi;
		tmpi = td->left_of_text_length;
		td->left_of_text_length = td->right_of_text_length;
		td->right_of_text_length = tmpi;
		break;
	case ROTATION_270:
		break;
	case ROTATION_180:
		td->offset = td->layout.title_g.width - td->offset - td->length;
		tmpi = td->left_end_length;
		td->left_end_length = td->right_end_length;
		td->right_end_length = tmpi;
		tmpi = td->left_of_text_length;
		td->left_of_text_length = td->right_of_text_length;
		td->right_of_text_length = tmpi;
		break;
	case ROTATION_0:
		break;
	}

	ROTATE_RECTANGLE(rotation, (&td->left_buttons_g), True, False, False)
	ROTATE_RECTANGLE(rotation, (&td->right_buttons_g), True, False, False)
	for (i=0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		ROTATE_RECTANGLE(
			rotation, (&td->layout.button_g[i]), True, False, False)
	}
	ROTATE_RECTANGLE(rotation, (&td->under_text_g), False, False, True)
	ROTATE_RECTANGLE(rotation, (&td->left_main_g), False, False, True)
	ROTATE_RECTANGLE(rotation, (&td->right_main_g), False, False, True)
	ROTATE_RECTANGLE(rotation, (&td->full_left_main_g), True, False, False)
	ROTATE_RECTANGLE(rotation, (&td->full_right_main_g), True, False, False)
	ROTATE_RECTANGLE(rotation, (&td->layout.title_g), True, False, False)
	ROTATE_RECTANGLE(rotation, (&td->bar_g), True, False, False)
	ROTATE_RECTANGLE(rotation, (&td->frame_g), False, False, False);

#undef ROTATE_RECTANGLE
}

static void border_get_titlebar_descr_state(
	FvwmWindow *fw, window_parts pressed_parts, int pressed_button,
	clear_window_parts clear_parts, Bool do_hilight,
	border_titlebar_state *tbstate)
{
	int i;

	if ((pressed_parts & PART_BUTTONS) != PART_NONE && pressed_button >= 0)
	{
		tbstate->pressed_bmask = (1 << pressed_button);
	}
	else
	{
		tbstate->pressed_bmask = 0;
	}
	if ((clear_parts & CLEAR_BUTTONS) != CLEAR_NONE)
	{
		tbstate->clear_bmask = 0x3FF;
	}
	else
	{
		tbstate->clear_bmask = 0;
	}
	tbstate->lit_bmask = (do_hilight == True) ? ~0 : 0;
	if ((pressed_parts & PART_TITLE) != PART_NONE)
	{
		tbstate->is_title_pressed = 1;
	}
	else
	{
		tbstate->is_title_pressed = 0;
	}
	if ((clear_parts & CLEAR_TITLE) != CLEAR_NONE)
	{
		tbstate->do_clear_title = 1;
	}
	else
	{
		tbstate->do_clear_title = 0;
	}
	tbstate->is_title_lit = (do_hilight == True) ? 1 : 0;
	tbstate->toggled_bmask = 0;
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		unsigned int mask = (1 << i);

		if (is_button_toggled(fw, i))
		{
			tbstate->toggled_bmask |= mask;
		}
		tbstate->bstate[i] = border_flags_to_button_state(
			tbstate->pressed_bmask & mask,
			tbstate->lit_bmask & mask,
			tbstate->toggled_bmask & mask);
	}
	tbstate->tstate = border_flags_to_button_state(
		tbstate->is_title_pressed, tbstate->is_title_lit, 0);
}

static window_parts border_get_titlebar_descr(
	common_decorations_type *cd, FvwmWindow *fw,
	window_parts pressed_parts, int pressed_button,
	window_parts force_draw_parts, clear_window_parts clear_parts,
	rectangle *old_g, rectangle *new_g, Bool do_hilight,
	titlebar_descr *ret_td)
{
	window_parts draw_parts;
	int i;
	DecorFace *df;
	int is_start = 0;
	JustificationType just;
	int lbl = 0;
	int rbl = 0;

	ret_td->cd = cd;
	ret_td->frame_g = *new_g;
	if (old_g == NULL)
	{
		old_g = &fw->g.frame;
	}
	frame_get_titlebar_dimensions(fw, old_g, NULL, &ret_td->old_layout);
	frame_get_titlebar_dimensions(fw, new_g, NULL, &ret_td->layout);

	ret_td->has_vt = HAS_VERTICAL_TITLE(fw);
	if (USE_TITLE_DECOR_ROTATION(fw))
	{
		ret_td->draw_rotation = fw->title_text_rotation;
		switch(ret_td->draw_rotation)
		{
		case ROTATION_90:
			ret_td->restore_rotation = ROTATION_270;
			break;
		case ROTATION_270: /* ccw */
			ret_td->restore_rotation = ROTATION_90;
			break;
		case ROTATION_180:
			ret_td->restore_rotation = ROTATION_180;
			break;
		default:
			break;
		}
	}
	if (fw->title_text_rotation == ROTATION_270 ||
	    fw->title_text_rotation == ROTATION_180)
	{
		ret_td->has_an_upsidedown_rotation = True;
	}
	/* geometry of the title bar title + buttons */
	if (!ret_td->has_vt)
	{
		ret_td->bar_g.width = new_g->width - 2 * fw->boundary_width;
		ret_td->bar_g.height = ret_td->layout.title_g.height;
		ret_td->bar_g.x = fw->boundary_width;
		ret_td->bar_g.y = ret_td->layout.title_g.y;
	}
	else
	{
		ret_td->bar_g.width = ret_td->layout.title_g.width;
		ret_td->bar_g.height = new_g->height - 2 * fw->boundary_width;
		ret_td->bar_g.y = fw->boundary_width;
		ret_td->bar_g.x = ret_td->layout.title_g.x;
	}

	/* buttons geometries */
	if (ret_td->has_vt)
	{
		ret_td->left_buttons_g.width = ret_td->bar_g.width;
		ret_td->right_buttons_g.width = ret_td->bar_g.width;
	}
	else
	{
		ret_td->left_buttons_g.height = ret_td->bar_g.height;
		ret_td->right_buttons_g.height = ret_td->bar_g.width;
	}

	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		if (ret_td->has_vt)
		{
			if (i & 1)
			{
				rbl += ret_td->layout.button_g[i].height;
			}
			else
			{
				lbl += ret_td->layout.button_g[i].height;
			}
		}
		else
		{
			if (i & 1)
			{
				rbl += ret_td->layout.button_g[i].width;
			}
			else
			{
				lbl += ret_td->layout.button_g[i].width;
			}
		}
	}

	if (ret_td->has_an_upsidedown_rotation)
	{
		if (ret_td->has_vt)
		{
			ret_td->left_buttons_g.height = rbl;
			ret_td->right_buttons_g.height = lbl;
			ret_td->right_buttons_g.y = fw->boundary_width;
			ret_td->right_buttons_g.x = ret_td->bar_g.x;
			ret_td->left_buttons_g.y = ret_td->layout.title_g.y +
				ret_td->layout.title_g.height;
			ret_td->left_buttons_g.x = ret_td->bar_g.x;
		}
		else
		{
			ret_td->left_buttons_g.width = rbl;
			ret_td->right_buttons_g.width = lbl;
			ret_td->right_buttons_g.x = fw->boundary_width;
			ret_td->right_buttons_g.y = ret_td->bar_g.y;
			ret_td->left_buttons_g.x = ret_td->layout.title_g.x +
				ret_td->layout.title_g.width;
			ret_td->left_buttons_g.y = ret_td->bar_g.y;
		}
	}
	else
	{
		if (ret_td->has_vt)
		{
			ret_td->left_buttons_g.height = lbl;
			ret_td->right_buttons_g.height = rbl;
			ret_td->left_buttons_g.y = fw->boundary_width;
			ret_td->left_buttons_g.x = ret_td->bar_g.x;
			ret_td->right_buttons_g.y = ret_td->layout.title_g.y +
				ret_td->layout.title_g.height;
			ret_td->right_buttons_g.x = ret_td->bar_g.x;
		}
		else
		{
			ret_td->left_buttons_g.width = lbl;
			ret_td->right_buttons_g.width = rbl;
			ret_td->left_buttons_g.x = fw->boundary_width;
			ret_td->left_buttons_g.y = ret_td->bar_g.y;
			ret_td->right_buttons_g.x = ret_td->layout.title_g.x +
				ret_td->layout.title_g.width;
			ret_td->right_buttons_g.y = ret_td->bar_g.y;
		}
	}

	/* initialise flags */
	border_get_titlebar_descr_state(
		fw, pressed_parts, pressed_button, clear_parts, do_hilight,
		&(ret_td->tbstate));

	/* get the title string length and position
	 * This is not in "tdd" (titlebar_draw_descr), because these are needed
	 * to draw the buttons with UseTitleStyle */
	just = TB_JUSTIFICATION(GetDecor(fw, titlebar));
	if (fw->visible_name != (char *)NULL)
	{
		ret_td->length = FlocaleTextWidth(
			fw->title_font, fw->visible_name,
			(ret_td->has_vt) ? -strlen(fw->visible_name) :
			strlen(fw->visible_name));
		if (ret_td->length > fw->title_length -
		    2*MIN_WINDOW_TITLE_TEXT_OFFSET)
		{
			ret_td->length = fw->title_length -
				2*MIN_WINDOW_TITLE_TEXT_OFFSET;
			just = JUST_CENTER;
		}
		if (ret_td->length < 0)
		{
			ret_td->length = 0;
		}
	}
	else
	{
		ret_td->length = 0;
	}
	if (ret_td->length == 0)
	{
		just = JUST_CENTER;
	}
	df = &TB_STATE(GetDecor(fw, titlebar))[ret_td->tbstate.tstate];
	switch (just)
	{
	case JUST_LEFT:
		is_start = 1;
		/* fall through */
	case JUST_RIGHT:
		if (ret_td->has_an_upsidedown_rotation)
		{
			is_start = !is_start;
		}
		if (is_start)
		{
			if (WINDOW_TITLE_TEXT_OFFSET + ret_td->length <=
			    fw->title_length)
			{
				ret_td->offset = WINDOW_TITLE_TEXT_OFFSET;
			}
			else
			{
				ret_td->offset =
					fw->title_length - ret_td->length;
			}
		}
		else
		{
			ret_td->offset = fw->title_length - ret_td->length -
				WINDOW_TITLE_TEXT_OFFSET;
		}
		break;
	case JUST_CENTER:
	default:
		ret_td->offset = (fw->title_length - ret_td->length) / 2;
		break;
	}

	if (ret_td->offset < MIN_WINDOW_TITLE_TEXT_OFFSET)
	{
		ret_td->offset = MIN_WINDOW_TITLE_TEXT_OFFSET;
	}

	/* setup MultiPixmap */
	border_mp_get_titlebar_descr(fw, ret_td, df);

	/* determine the parts to draw */
	draw_parts = border_get_tb_parts_to_draw(
		fw, ret_td, old_g, new_g, force_draw_parts);

	return draw_parts;
}


static void border_draw_titlebar(
	common_decorations_type *cd, FvwmWindow *fw,
	window_parts pressed_parts, int pressed_button,
	window_parts force_draw_parts, clear_window_parts clear_parts,
	rectangle *old_g, rectangle *new_g, Bool do_hilight)
{
	window_parts draw_parts;
	titlebar_descr td;

	if (!HAS_TITLE(fw))
	{
		/* just reset border states */
		fw->decor_state.parts_drawn &= ~(PART_TITLE);
		fw->decor_state.parts_lit &= ~(PART_TITLE);
		fw->decor_state.parts_inverted &= ~(PART_TITLE);
		fw->decor_state.buttons_drawn = 0;
		fw->decor_state.buttons_lit = 0;
		fw->decor_state.buttons_inverted = 0;
		fw->decor_state.buttons_toggled = 0;
		return;
	}
	memset(&td, 0, sizeof(td));
	draw_parts = border_get_titlebar_descr(
		cd, fw, pressed_parts, pressed_button, force_draw_parts,
		clear_parts, old_g, new_g, do_hilight, &td);
	if ((draw_parts & PART_TITLE) != PART_NONE ||
	    (draw_parts & PART_BUTTONS) != PART_NONE)
	{
		/* set up UseTitleStyle Colorset */
		border_setup_use_title_style(fw, &td);
	}
	if ((draw_parts & PART_TITLE) != PART_NONE)
	{
		border_draw_title(fw, &td);
	}
	if ((draw_parts & PART_BUTTONS) != PART_NONE)
	{
		border_draw_buttons(fw, &td);
	}
	border_free_bar_pixmaps(&(td.cd->dynamic_cd));

	/* update the decor states */
	fw->decor_state.parts_drawn |= draw_parts;
	if (do_hilight)
	{
		fw->decor_state.parts_lit |= draw_parts;
	}
	else
	{
		fw->decor_state.parts_lit &= ~draw_parts;
	}
	fw->decor_state.parts_inverted &= ~draw_parts;
	fw->decor_state.parts_inverted |= (draw_parts & pressed_parts);
	if (draw_parts & PART_BUTTONS)
	{
		fw->decor_state.buttons_drawn |= td.tbstate.draw_bmask;
		fw->decor_state.parts_lit = (do_hilight) ? ~0 : 0;
		if (td.tbstate.pressed_bmask)
		{
			fw->decor_state.buttons_inverted =
				td.tbstate.pressed_bmask;
		}
		else
		{
			fw->decor_state.buttons_inverted &=
				~td.tbstate.draw_bmask;
		}
		fw->decor_state.buttons_toggled =
			(fw->decor_state.buttons_toggled &
			 ~td.tbstate.max_bmask) | td.tbstate.toggled_bmask;
	}

	return;
}

/*
 *
 * Redraws the windows borders
 *
 */
static void border_draw_border_parts(
	common_decorations_type *cd, FvwmWindow *fw,
	window_parts pressed_parts, window_parts force_draw_parts,
	clear_window_parts clear_parts, rectangle *old_g, rectangle *new_g,
	Bool do_hilight)
{
	border_relief_descr br;
	window_parts draw_parts;
	Bool do_clear;

	if (HAS_NO_BORDER(fw))
	{
		/* just reset border states */
		fw->decor_state.parts_drawn &= ~(PART_FRAME | PART_HANDLES);
		fw->decor_state.parts_lit &= ~(PART_FRAME | PART_HANDLES);
		fw->decor_state.parts_inverted &= ~(PART_FRAME | PART_HANDLES);
		return;
	}
	do_clear = (clear_parts & CLEAR_FRAME) ? True : False;
	/* determine the parts to draw and the position to place them */
	if (HAS_DEPRESSABLE_BORDER(fw))
	{
		pressed_parts &= PART_FRAME;
	}
	else
	{
		pressed_parts = PART_NONE;
	}
	force_draw_parts &= PART_FRAME;
	memset(&br, 0, sizeof(br));
	draw_parts = border_get_parts_and_pos_to_draw(
		cd, fw, pressed_parts, force_draw_parts, old_g, new_g,
		do_hilight, &br);
	if ((draw_parts & PART_FRAME) != PART_NONE)
	{
		border_draw_all_border_parts(
			cd, fw, &br, new_g, draw_parts, pressed_parts,
			do_hilight, do_clear);
	}
	/* update the decor states */
	fw->decor_state.parts_drawn |= draw_parts;
	if (do_hilight)
	{
		fw->decor_state.parts_lit |= draw_parts;
	}
	else
	{
		fw->decor_state.parts_lit &= ~draw_parts;
	}
	fw->decor_state.parts_inverted &= ~draw_parts;
	fw->decor_state.parts_inverted |= (draw_parts & pressed_parts);

	return;
}

/* ---------------------------- interface functions ------------------------ */

DecorFace *border_get_border_style(
	FvwmWindow *fw, Bool has_focus)
{
	DecorFace *df;

	if (has_focus == True)
	{
		df = &(GetDecor(fw, BorderStyle.active));
	}
	else
	{
		df = &(GetDecor(fw, BorderStyle.inactive));
	}

	return df;
}

int border_is_using_border_style(
	FvwmWindow *fw, Bool has_focus)
{
	ButtonState bs;
	int is_pressed;
	int is_toggled;
	int i;

	/* title */
	is_pressed = (FW_W_TITLE(fw) == PressedW);
	bs = border_flags_to_button_state(is_pressed, has_focus, 0);
	if (DFS_USE_BORDER_STYLE(TB_STATE(GetDecor(fw, titlebar))[bs].style))
	{
		return 1;
	}
	for (i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		if (FW_W_BUTTON(fw, i) == None)
		{
			continue;
		}
		is_pressed = (FW_W_BUTTON(fw, i) == PressedW);
		is_toggled = (is_button_toggled(fw, i) == True);
		bs = border_flags_to_button_state(
			is_pressed, (has_focus == True), is_toggled);
		if (DFS_USE_BORDER_STYLE(
			    TB_STATE(GetDecor(fw, buttons[i]))[bs].style))
		{
			return 1;
		}
	}

	return 0;
}

int border_context_to_parts(
	int context)
{
	if (context == C_FRAME || context == C_SIDEBAR ||
	    context == (C_FRAME | C_SIDEBAR))
	{
		return PART_FRAME;
	}
	else if (context == C_F_TOPLEFT)
	{
		return PART_BORDER_NW;
	}
	else if (context == C_F_TOPRIGHT)
	{
		return PART_BORDER_NE;
	}
	else if (context == C_F_BOTTOMLEFT)
	{
		return PART_BORDER_SW;
	}
	else if (context == C_F_BOTTOMRIGHT)
	{
		return PART_BORDER_SE;
	}
	else if (context == C_SB_LEFT)
	{
		return PART_BORDER_W;
	}
	else if (context == C_SB_RIGHT)
	{
		return PART_BORDER_E;
	}
	else if (context == C_SB_TOP)
	{
		return PART_BORDER_N;
	}
	else if (context == C_SB_BOTTOM)
	{
		return PART_BORDER_S;
	}
	else if (context == C_TITLE)
	{
		return PART_TITLE;
	}
	else if (context & (C_LALL | C_RALL))
	{
		return PART_BUTTONS;
	}

	return PART_NONE;
}

void border_get_part_geometry(
	FvwmWindow *fw, window_parts part, rectangle *sidebar_g,
	rectangle *ret_g, Window *ret_w)
{
	int bw;

	bw = fw->boundary_width;
	/* ret_g->x and ret->y is just an offset relatively to the w,
	 * maybe we can take the relief in account? */
	switch (part)
	{
	case PART_BORDER_N:
		ret_g->x = sidebar_g->x;
		ret_g->y = 0;
		*ret_w = FW_W_SIDE(fw, 0);
		break;
	case PART_BORDER_E:
		ret_g->x = 2 * sidebar_g->x + sidebar_g->width - bw;
		ret_g->y = sidebar_g->y;
		*ret_w = FW_W_SIDE(fw, 1);
		break;
	case PART_BORDER_S:
		ret_g->x = sidebar_g->x;
		ret_g->y = 2 * sidebar_g->y + sidebar_g->height - bw;
		*ret_w = FW_W_SIDE(fw, 2);
		break;
	case PART_BORDER_W:
		ret_g->x = 0;
		ret_g->y = sidebar_g->y;
		*ret_w = FW_W_SIDE(fw, 3);
		break;
	case PART_BORDER_NW:
		ret_g->x = 0;
		ret_g->y = 0;
		*ret_w = FW_W_CORNER(fw, 0);
		break;
	case PART_BORDER_NE:
		ret_g->x = sidebar_g->x + sidebar_g->width;
		ret_g->y = 0;
		*ret_w = FW_W_CORNER(fw, 1);
		break;
	case PART_BORDER_SW:
		ret_g->x = 0;
		ret_g->y = sidebar_g->y + sidebar_g->height;
		*ret_w = FW_W_CORNER(fw, 2);
		break;
	case PART_BORDER_SE:
		ret_g->x = sidebar_g->x + sidebar_g->width;
		ret_g->y = sidebar_g->y + sidebar_g->height;
		*ret_w = FW_W_CORNER(fw, 3);
		break;
	default:
		break;
	}

	switch (part)
	{
	case PART_BORDER_N:
	case PART_BORDER_S:
		ret_g->width = sidebar_g->width;
		ret_g->height = bw;
		break;
	case PART_BORDER_E:
	case PART_BORDER_W:
		ret_g->width = bw;
		ret_g->height = sidebar_g->height;
		break;
	case PART_BORDER_NW:
	case PART_BORDER_NE:
	case PART_BORDER_SW:
	case PART_BORDER_SE:
		ret_g->width = sidebar_g->x;
		ret_g->height = sidebar_g->y;
		break;
	default:
		return;
	}

	return;
}

int get_button_number(int context)
{
	int i;

	for (i = 0; (C_L1 << i) & (C_LALL | C_RALL); i++)
	{
		if (context & (C_L1 << i))
		{
			return i;
		}
	}

	return -1;
}

void border_draw_decorations(
	FvwmWindow *fw, window_parts draw_parts, Bool has_focus, Bool do_force,
	clear_window_parts clear_parts, rectangle *old_g, rectangle *new_g)
{
	common_decorations_type cd;
	Bool do_redraw_titlebar = False;
	window_parts pressed_parts;
	window_parts force_parts;
	int context;
	int item;

	if (fw == NULL)
	{
		return;
	}
	if (WAS_NEVER_DRAWN(fw))
	{
		/* force drawing everything */
		do_force = True;
		draw_parts = PART_ALL;
		SET_WAS_NEVER_DRAWN(fw, 0);
	}
	memset(&cd, 0, sizeof(cd));

	/* can't compare with True here, old code calls this with value "2" */
	if (do_force != False)
	{
		force_parts = draw_parts;
	}
	else
	{
		force_parts = PART_NONE;
	}
	if (has_focus)
	{
		/* don't re-draw just for kicks */
		if (Scr.Hilite != fw && Scr.Hilite != NULL)
		{
			FvwmWindow *t = Scr.Hilite;

			Scr.Hilite = NULL;
			/* make sure that the previously highlighted
			 * window got unhighlighted */
			border_draw_decorations(
				t, PART_ALL, False, True, CLEAR_ALL, NULL,
				NULL);
		}
		Scr.Hilite = fw;
	}
	else if (fw == Scr.Hilite)
	{
		Scr.Hilite = NULL;
	}
	if (fw->Desk != Scr.CurrentDesk)
	{
		return;
	}
	if (IS_ICONIFIED(fw))
	{
		DrawIconWindow(fw, True, True, True, False, NULL);
		return;
	}
	/* calculate some values and flags */
	if ((draw_parts & PART_TITLEBAR) && HAS_TITLE(fw))
	{
		do_redraw_titlebar = True;
	}
	get_common_decorations(
		&cd, fw, draw_parts, has_focus, False, do_redraw_titlebar);
	/* redraw */
	context = frame_window_id_to_context(fw, PressedW, &item);
	if ((context & (C_LALL | C_RALL)) == 0)
	{
		item = -1;
	}
	pressed_parts = border_context_to_parts(context);
	if (new_g == NULL)
	{
		new_g = &fw->g.frame;
	}
	if (do_redraw_titlebar)
	{
		border_draw_titlebar(
			&cd, fw, pressed_parts & PART_TITLEBAR, item,
			force_parts & PART_TITLEBAR,
			clear_parts, old_g, new_g, has_focus);
	}
	if (draw_parts & PART_FRAME)
	{
		Pixmap save_pix = cd.dynamic_cd.frame_pixmap;

		memset(&cd, 0, sizeof(cd));
		get_common_decorations(
			&cd, fw, draw_parts, has_focus, True, True);
		cd.dynamic_cd.frame_pixmap = save_pix;
		border_draw_border_parts(
			&cd, fw,
			(pressed_parts & (PART_FRAME | PART_HANDLES)),
			(force_parts & (PART_FRAME | PART_HANDLES)),
			clear_parts, old_g, new_g, has_focus);
	}
	if (cd.dynamic_cd.frame_pixmap != None)
	{
		XFreePixmap(dpy, cd.dynamic_cd.frame_pixmap);
	}
	return;
}

void border_undraw_decorations(
	FvwmWindow *fw)
{
	memset(&fw->decor_state, 0, sizeof(fw->decor_state));

	return;
}

/*
 *
 *  redraw the decoration when style change
 *
 */
void border_redraw_decorations(
	FvwmWindow *fw)
{
	FvwmWindow *u = Scr.Hilite;

	/* domivogt (6-Jun-2000): Don't check if the window is visible here.
	 * If we do, some updates are not applied and when the window becomes
	 * visible again, the X Server may not redraw the window. */
	border_draw_decorations(
		fw, PART_ALL, (Scr.Hilite == fw), True, CLEAR_ALL, NULL, NULL);
	Scr.Hilite = u;

	return;
}

/*
 *
 *  get the the root transparent parts of the decoration
 *
 */
unsigned int border_get_transparent_decorations_part(FvwmWindow *fw)
{
	DecorFace *df,*tdf;
	unsigned int draw_parts = PART_NONE;
	int i;
	window_parts pressed_parts;
	int context;
	int item;
	border_titlebar_state tbstate;
	Bool title_use_borderstyle = False;
	Bool buttons_use_borderstyle = False;
	Bool buttons_use_titlestyle = False;

	context = frame_window_id_to_context(fw, PressedW, &item);
	if ((context & (C_LALL | C_RALL)) == 0)
	{
		item = -1;
	}
	pressed_parts = border_context_to_parts(context);

	memset(&tbstate, 0, sizeof(tbstate));
	border_get_titlebar_descr_state(
		fw, pressed_parts & PART_TITLEBAR, item,
		CLEAR_ALL, (Scr.Hilite == fw), &tbstate);

	for(i = 0; i < NUMBER_OF_TITLE_BUTTONS; i++)
	{
		df = &TB_STATE(GetDecor(fw, buttons[i]))[tbstate.bstate[i]];

		if (DFS_USE_TITLE_STYLE(df->style))
		{
			buttons_use_titlestyle = True;
		}
		if (DFS_USE_BORDER_STYLE(df->style))
		{
			buttons_use_borderstyle = True;
		}
		for(tdf = df; tdf != NULL; tdf = tdf->next)
		{
			if (DFS_FACE_TYPE(tdf->style) == ColorsetButton &&
			    CSET_IS_TRANSPARENT_ROOT(tdf->u.acs.cs))
			{
				draw_parts |= PART_BUTTONS;
				break;
			}
		}
	}

	df = &TB_STATE(GetDecor(fw, titlebar))[tbstate.tstate];
	if (DFS_USE_BORDER_STYLE(df->style))
	{
		title_use_borderstyle = True;
	}
	for(tdf = df; tdf != NULL; tdf = tdf->next)
	{
		if (DFS_FACE_TYPE(tdf->style) == ColorsetButton &&
		    CSET_IS_TRANSPARENT_ROOT(tdf->u.acs.cs))
		{
			draw_parts |= PART_TITLE;
			break;
		}
		else if (DFS_FACE_TYPE(tdf->style) == MultiPixmap)
		{
			int i;

			for (i = 0; i < TBMP_NUM_PIXMAPS; i++)
			{
				if (CSET_IS_TRANSPARENT_ROOT(
					tdf->u.mp.acs[i].cs))
				{
					draw_parts |= PART_TITLE;
					break;
				}
			}
		}
	}

	df = border_get_border_style(fw, (Scr.Hilite == fw));
	if (DFS_FACE_TYPE(df->style) == ColorsetButton &&
	    CSET_IS_TRANSPARENT_ROOT(df->u.acs.cs))
	{
		draw_parts |= PART_FRAME|PART_HANDLES;
	}
	if (draw_parts & PART_FRAME)
	{
		if (title_use_borderstyle)
		{
			draw_parts |= PART_TITLE;
		}
		if (buttons_use_borderstyle)
		{
			draw_parts |= PART_BUTTONS;
		}
	}
	if ((draw_parts & PART_TITLE) && buttons_use_titlestyle)
	{
		draw_parts |= PART_BUTTONS;
	}
#if 0
	fprintf(stderr,"Transparant Part: %u\n", draw_parts);
#endif
	return draw_parts;
}

/* ---------------------------- builtin commands --------------------------- */

/*
 *
 *  Sets the allowed button states
 *
 */
void CMD_ButtonState(F_CMD_ARGS)
{
	char *token;

	while ((token = PeekToken(action, &action)))
	{
		static char first = True;
		if (!token && first)
		{
			Scr.gs.use_active_down_buttons =
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
			Scr.gs.use_inactive_buttons =
				DEFAULT_USE_INACTIVE_BUTTONS;
			Scr.gs.use_inactive_down_buttons =
				DEFAULT_USE_INACTIVE_DOWN_BUTTONS;
			return;
		}
		first = False;
		if (StrEquals("activedown", token))
		{
			Scr.gs.use_active_down_buttons = ParseToggleArgument(
				action, &action,
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS, True);
		}
		else if (StrEquals("inactive", token))
		{
			Scr.gs.use_inactive_buttons = ParseToggleArgument(
				action, &action,
				DEFAULT_USE_INACTIVE_BUTTONS, True);
		}
		else if (StrEquals("inactivedown", token))
		{
			Scr.gs.use_inactive_down_buttons = ParseToggleArgument(
				action, &action,
				DEFAULT_USE_INACTIVE_DOWN_BUTTONS, True);
		}
		else
		{
			Scr.gs.use_active_down_buttons =
				DEFAULT_USE_ACTIVE_DOWN_BUTTONS;
			Scr.gs.use_inactive_buttons =
				DEFAULT_USE_INACTIVE_BUTTONS;
			Scr.gs.use_inactive_down_buttons =
				DEFAULT_USE_INACTIVE_DOWN_BUTTONS;
			fvwm_msg(ERR, "cmd_button_state",
				 "Unknown button state %s", token);
			return;
		}
	}

	return;
}

/*
 *
 *  Sets the border style (veliaa@rpi.edu)
 *
 */
void CMD_BorderStyle(F_CMD_ARGS)
{
	char *parm;
	char *prev;
	FvwmDecor *decor = Scr.cur_decor ? Scr.cur_decor : &Scr.DefaultDecor;

	Scr.flags.do_need_window_update = 1;
	decor->flags.has_changed = 1;
	for (prev = action; (parm = PeekToken(action, &action)); prev = action)
	{
		if (StrEquals(parm, "active") || StrEquals(parm, "inactive"))
		{
			int len;
			char *end, *tmp;
			DecorFace tmpdf, *df;

			memset(&tmpdf.style, 0, sizeof(tmpdf.style));
			DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
			tmpdf.next = NULL;
			if (FMiniIconsSupported)
			{
				tmpdf.u.p = NULL;
			}
			if (StrEquals(parm,"active"))
			{
				df = &decor->BorderStyle.active;
			}
			else
			{
				df = &decor->BorderStyle.inactive;
			}
			df->flags.has_changed = 1;
			while (isspace(*action))
			{
				++action;
			}
			if (*action != '(')
			{
				if (!*action)
				{
					fvwm_msg(
						ERR, "SetBorderStyle",
						"error in %s border"
						" specification", parm);
					return;
				}
				while (isspace(*action))
				{
					++action;
				}
				if (ReadDecorFace(action, &tmpdf,-1,True))
				{
					FreeDecorFace(dpy, df);
					*df = tmpdf;
				}
				break;
			}
			end = strchr(++action, ')');
			if (!end)
			{
				fvwm_msg(
					ERR, "SetBorderStyle",
					"error in %s border specification",
					parm);
				return;
			}
			len = end - action + 1;
			/* TA:  FIXME xasprintf */
			tmp = xmalloc(len);
			strncpy(tmp, action, len - 1);
			tmp[len - 1] = 0;
			ReadDecorFace(tmp, df,-1,True);
			free(tmp);
			action = end + 1;
		}
		else if (strcmp(parm,"--")==0)
		{
			if (ReadDecorFace(
				    prev, &decor->BorderStyle.active,-1,True))
			{
				ReadDecorFace(
					prev, &decor->BorderStyle.inactive, -1,
					False);
			}
			decor->BorderStyle.active.flags.has_changed = 1;
			decor->BorderStyle.inactive.flags.has_changed = 1;
			break;
		}
		else
		{
			DecorFace tmpdf;
			memset(&tmpdf.style, 0, sizeof(tmpdf.style));
			DFS_FACE_TYPE(tmpdf.style) = SimpleButton;
			tmpdf.next = NULL;
			if (FMiniIconsSupported)
			{
				tmpdf.u.p = NULL;
			}
			if (ReadDecorFace(prev, &tmpdf,-1,True))
			{
				FreeDecorFace(dpy,&decor->BorderStyle.active);
				decor->BorderStyle.active = tmpdf;
				ReadDecorFace(
					prev, &decor->BorderStyle.inactive, -1,
					False);
				decor->BorderStyle.active.flags.has_changed = 1;
				decor->BorderStyle.inactive.flags.has_changed =
					1;
			}
			break;
		}
	}

	return;
}
