## Process this file with automake to create Makefile.in

## Notes: The Imakefile had compiled this only if Xpm was enabled;
## however, it appears to be #ifdef'd properly to compile without XPM.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmDragWell
man_MANS = FvwmDragWell.1
FvwmDragWell_SOURCES = fvwmDragWell.c xdndDragSource.c cursorStuff.c
FvwmDragWell_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
EXTRA_DIST = $(man_MANS) README


LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 -lXmu $(X_EXTRA_LIBS) -lm
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

