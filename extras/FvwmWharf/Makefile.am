## Process this file with automake to create Makefile.in

## Notes: The Imakefile had compiled this only if Xpm was enabled;
## however, it appears to be #ifdef'd properly to compile without XPM.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmWharf
FvwmWharf_SOURCES = FvwmWharf.c Wharf.h icons.c stepgfx.c stepgfx.h
FvwmWharf_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmWharf.1
EXTRA_DIST = $(man_MANS) sample.style \
	   ASSound/ASSound.c ASSound/README \
	   OffiX/DragAndDrop.h OffiX/DragAndDropTypes.h

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

