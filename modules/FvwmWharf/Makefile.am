## Process this file with automake to create Makefile.in

## Notes: The Imakefile had compiled this only if Xpm was enabled;
## however, it appears to be #ifdef'd properly to compile without XPM.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmWharf
FvwmWharf_SOURCES = FvwmWharf.c Wharf.h icons.c stepgfx.c stepgfx.h
FvwmWharf_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmWharf.1
EXTRA_DIST = $(man_MANS) sample.style \
	   ASSound/ASSound.c \
	   OffiX/DragAndDrop.h OffiX/DragAndDropTypes.h

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(xpm_LIBS) $(png_LIBS) \
	$(X_PRE_LIBS) $(Xinerama_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS) \
	$(rsvg_LIBS) $(Xrender_LIBS)

INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS) $(png_CFLAGS) \
	$(Xrender_CFLAGS)
