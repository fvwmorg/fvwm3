/* Fvwm colorset technology is Copyright (C) 1999 Joey Shutup
     http://www.streetmap.co.uk/streetmap.dll?Postcode2Map?BS24+9TZ
     You may use this code for any purpose, as long as the original copyright
     and this notice remains in the source code and all documentation
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIBS_COLORSETS_H
#define LIBS_COLORSETS_H

typedef struct {
	Pixel fg;
	Pixel bg;
	Pixel hilite;
	Pixel shadow;
	Pixel fgsh;
	Pixel tint;
	Pixel icon_tint;
	Pixmap pixmap;
	Pixmap shape_mask;
	unsigned int fg_alpha : 7;
	unsigned int width : 12;
	unsigned int height : 12;
	unsigned int pixmap_type: 3;
	unsigned int shape_width : 12;
	unsigned int shape_height : 12;
	unsigned int shape_type : 2;
	unsigned int tint_percent : 12;
	unsigned int do_dither_icon : 1;
	unsigned int icon_tint_percent : 12;
	unsigned int icon_alpha : 12;
#ifdef FVWM_COLORSET_PRIVATE
	/* fvwm/colorset.c use only */
	Pixel fg_tint;
	Pixel fg_saved;
	Pixel bg_tint;
	Pixel bg_saved;
	Pixmap mask;
	Pixmap alpha_pixmap;
	char *pixmap_args;
	char *gradient_args;
	char gradient_type;
	unsigned int color_flags;
	FvwmPicture *picture;
	Pixel *pixels;
	int nalloc_pixels;
	int fg_tint_percent;
	int bg_tint_percent;
	short image_alpha_percent;
	Bool dither;
	Bool allows_buffered_transparency;
	Bool is_maybe_root_transparent;
#endif
} colorset_struct;

#define PIXMAP_TILED 0
#define PIXMAP_STRETCH_X 1
#define PIXMAP_STRETCH_Y 2
#define PIXMAP_STRETCH 3
#define PIXMAP_STRETCH_ASPECT 4
#define PIXMAP_ROOT_PIXMAP_PURE 5
#define PIXMAP_ROOT_PIXMAP_TRAN 6

#define SHAPE_TILED 0
#define SHAPE_STRETCH 1
#define SHAPE_STRETCH_ASPECT 2

#ifdef FVWM_COLORSET_PRIVATE
#define FG_SUPPLIED 0x1
#define BG_SUPPLIED 0x2
#define HI_SUPPLIED 0x4
#define SH_SUPPLIED 0x8
#define FGSH_SUPPLIED 0x10
#define FG_CONTRAST 0x20
#define BG_AVERAGE  0x40
#define TINT_SUPPLIED  0x80
#define FG_TINT_SUPPLIED  0x100
#define BG_TINT_SUPPLIED  0x200
#define ICON_TINT_SUPPLIED 0x400
#endif

/* colorsets are stored as an array of structs to permit fast dereferencing */
extern colorset_struct *Colorset;


/* some macro for transparency */
#define CSET_IS_TRANSPARENT(cset) \
    (cset >= 0 && (Colorset[cset].pixmap == ParentRelative ||                \
		   (Colorset[cset].pixmap != None &&                         \
		    (Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN ||\
		     Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_PURE))))
#define CSET_IS_TRANSPARENT_PR(cset) \
    (cset >= 0 && Colorset[cset].pixmap == ParentRelative)
#define CSET_IS_TRANSPARENT_ROOT(cset) \
    (cset >= 0 && Colorset[cset].pixmap != None &&                         \
     (Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN ||\
      Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_PURE))
#define CSET_IS_TRANSPARENT_PR_PURE(cset) \
    (cset >= 0 && Colorset[cset].pixmap == ParentRelative && \
     Colorset[cset].tint_percent == 0)
#define CSET_IS_TRANSPARENT_ROOT_PURE(cset) \
    (cset >= 0 && Colorset[cset].pixmap != None &&                         \
     Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_PURE)
#define CSET_IS_TRANSPARENT_ROOT_TRAN(cset) \
    (cset >= 0 && Colorset[cset].pixmap != None &&                         \
     Colorset[cset].pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN)
#define CSET_IS_TRANSPARENT_PR_TINT(cset) \
    (cset >= 0 && Colorset[cset].pixmap == ParentRelative && \
     Colorset[cset].tint_percent > 0)


/* some macro for transparency */
#define CSETS_IS_TRANSPARENT(cset) \
    (cset >= 0 && (cset->pixmap == ParentRelative ||                \
		   (cset->pixmap != None &&                         \
		    (cset->pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN ||\
		     cset->pixmap_type == PIXMAP_ROOT_PIXMAP_PURE))))
#define CSETS_IS_TRANSPARENT_ROOT(cset) \
    (cset >= 0 && cset->pixmap != None &&                         \
     (cset->pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN ||\
      cset->pixmap_type == PIXMAP_ROOT_PIXMAP_PURE))
#define CSETS_IS_TRANSPARENT_PR_PURE(cset) \
    (cset >= 0 && cset->pixmap == ParentRelative && \
     cset->tint_percent == 0)
#define CSETS_IS_TRANSPARENT_ROOT_PURE(cset) \
    (cset >= 0 && cset->pixmap != None &&                         \
     cset->pixmap_type == PIXMAP_ROOT_PIXMAP_PURE)
#define CSETS_IS_TRANSPARENT_ROOT_TRAN(cset) \
    (cset >= 0 && cset->pixmap != None &&                         \
     cset->pixmap_type == PIXMAP_ROOT_PIXMAP_TRAN)
#define CSETS_IS_TRANSPARENT_PR_TINT(cset) \
    (cset >= 0 && cset->pixmap == ParentRelative && \
     cset->tint_percent > 0)

#ifndef FVWM_COLORSET_PRIVATE
/* Create n new colorsets, fvwm/colorset.c does its own thing (different size)
 */
void AllocColorset(int n);
#endif

/* dump one */
char *DumpColorset(int n, colorset_struct *colorset);
/* load one */
int LoadColorset(char *line);

Pixmap CreateBackgroundPixmap(Display *dpy, Window win, int width, int height,
			      colorset_struct *colorset, unsigned int depth,
			      GC gc, Bool is_mask);
Pixmap ScrollPixmap(
	Display *dpy, Pixmap p, GC gc, int x_off, int y_off, int width,
	int height, unsigned int depth);
void SetWindowBackgroundWithOffset(
	Display *dpy, Window win, int x_off, int y_off, int width, int height,
	colorset_struct *colorset, unsigned int depth, GC gc, Bool clear_area);
void SetWindowBackground(Display *dpy, Window win, int width, int height,
			 colorset_struct *colorset, unsigned int depth, GC gc,
			 Bool clear_area);
void UpdateBackgroundTransparency(
	Display *dpy, Window win, int width, int height,
	colorset_struct *colorset, unsigned int depth, GC gc, Bool clear_area);
void SetRectangleBackground(
  Display *dpy, Window win, int x, int y, int width, int height,
  colorset_struct *colorset, unsigned int depth, GC gc);

#endif
