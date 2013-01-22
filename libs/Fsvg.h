#ifndef FSVG_H
#define FSVG_H

#ifdef HAVE_RSVG
#define USE_SVG 1
#else
#define USE_SVG 0
#endif

#include "PictureBase.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if USE_SVG
#	include <librsvg/rsvg.h>
#	include <librsvg/rsvg-cairo.h>

	typedef RsvgDimensionData		FRsvgDimensionData;
	typedef RsvgHandle			FRsvgHandle;
	typedef cairo_surface_t			Fcairo_surface_t;
	typedef cairo_t				Fcairo_t;

#	define FCAIRO_FORMAT_ARGB32		CAIRO_FORMAT_ARGB32
#	define FCAIRO_STATUS_SUCCESS		CAIRO_STATUS_SUCCESS

#	define FG_OBJECT(a)			G_OBJECT(a)
#	define Fg_object_unref(a)		g_object_unref(a)
#	define Frsvg_handle_get_dimensions(a, b) \
			rsvg_handle_get_dimensions(a, b)
#	define Frsvg_handle_new_from_file(a, b)	rsvg_handle_new_from_file(a, b)
#	define Frsvg_handle_render_cairo(a, b)	rsvg_handle_render_cairo(a, b)

/* TA:  2013-01-22 -- rsvg_init() has been deprecated since version 2.36; but
 * RSVG doesn't define a version of its own to check against.  Since RSVG uses
 * glib, we can check its version instead.
 */
#if !GLIB_CHECK_VERSION (2, 31, 0)
#	define Frsvg_init()			rsvg_init()
#else
#	define Frsvg_init()			g_type_init()
#endif

#	define Fcairo_create(a)			cairo_create(a)
#	define Fcairo_destroy(a)		cairo_destroy(a)
#	define Fcairo_image_surface_create_for_data(a,b,c,d,e) \
 			cairo_image_surface_create_for_data(a,b,c,d,e)
#	define Fcairo_rotate(a, b)		cairo_rotate(a, b)
#	define Fcairo_scale(a, b, c)		cairo_scale(a, b, c)
#	define Fcairo_status(a)			cairo_status(a)
#	define Fcairo_surface_destroy(a)	cairo_surface_destroy(a)
#	define Fcairo_surface_status(a)		cairo_surface_status(a)
#	define Fcairo_translate(a, b, c)	cairo_translate(a, b, c)
#else
	typedef struct {
		int width;
		int height;
		double em;
		double ex;
	} FRsvgDimensionData;
	typedef void FRsvgHandle;
	typedef void Fcairo_surface_t;
	typedef void Fcairo_t;

#	define FCAIRO_FORMAT_ARGB32 0
#	define FCAIRO_STATUS_SUCCESS 0

#	define FG_OBJECT(a)
#	define Fg_object_unref(a)
#	define Frsvg_handle_get_dimensions(a, b)
#	define Frsvg_handle_new_from_file(a, b)	0
#	define Frsvg_handle_render_cairo(a, b)
#	define Frsvg_init()

#	define Fcairo_create(a)	0
#	define Fcairo_destroy(a)
#	define Fcairo_image_surface_create_for_data(a,b,c,d,e) 0
#	define Fcairo_rotate(a, b)
#	define Fcairo_scale(a, b, c)
#	define Fcairo_status(a) 0
#	define Fcairo_surface_destroy(a)
#	define Fcairo_surface_status(a) 0
#	define Fcairo_translate(a, b, c)
#endif

#endif /* FSVG_H */
