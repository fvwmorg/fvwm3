#ifndef FVWMLIB_FSVG_H
#define FVWMLIB_FSVG_H

#include "fvwm_x11.h"
#include "PictureBase.h"

#ifdef HAVE_RSVG
#define USE_SVG 1
#include <librsvg/rsvg.h>
#include <cairo.h>
#else
#define USE_SVG 0
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if USE_SVG
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
#endif

#if !GLIB_CHECK_VERSION(2,35,0)
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

#endif /* FVWMLIB_FSVG_H */
