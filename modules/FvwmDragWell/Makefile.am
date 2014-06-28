## Process this file with automake to create Makefile.in

## Notes: The Imakefile had compiled this only if Xpm was enabled;
## however, it appears to be #ifdef'd properly to compile without XPM.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmDragWell
man_MANS = FvwmDragWell.1
FvwmDragWell_SOURCES = fvwmDragWell.c xdndDragSource.c cursorStuff.c \
	commonStuff.h cursorStuff.h dragSource.h fvwmDragWell.h
FvwmDragWell_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
EXTRA_DIST = $(man_MANS)


LDADD = -L$(top_builddir)/libs -lfvwm \
	$(X_LIBS) $(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) \
	-lm $(Xrender_LIBS) $(rsvg_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS) $(Xrender_CFLAGS)

