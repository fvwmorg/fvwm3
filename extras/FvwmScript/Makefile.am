## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

SUBDIRS = Scripts Widgets

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmScript

FvwmScript_SOURCES = FvwmScript.c Instructions.c libyywrap.c \
	script.y scanner.l types.h
FvwmScript_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

YFLAGS = -d

man_MANS = FvwmScript.1
EXTRA_DIST = $(man_MANS) BUGS script.h

LDADD = Widgets/libWidgets.a -L$(top_builddir)/libs -lfvwm \
	$(xpm_LIBS) $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

