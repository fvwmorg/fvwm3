## Process this file with automake to create Makefile.in

## Notes: two files present in the directory, but not mentioned in the
## Imakefile were dropped: goodies.c and signal.c.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmTaskBar

FvwmTaskBar_SOURCES =  FvwmTaskBar.c List.c ButtonArray.c Mallocs.c \
	Colors.c Start.c FvwmTaskBar.h ButtonArray.h List.h \
	Colors.h Mallocs.h Start.h Goodies.c Goodies.h
FvwmTaskBar_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmTaskBar.1
EXTRA_DIST = $(man_MANS) fvwm2.README minimail.xbm

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

