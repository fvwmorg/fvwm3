## Process this file with automake to create Makefile.in

SUBDIRS = Scripts Widgets

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmScript

FvwmScript_SOURCES = FvwmScript.c Instructions.c lex.yy.c libyywrap.c \
	types.h y.tab.c y.tab.h
FvwmScript_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmScript.1
EXTRA_DIST = $(man_MANS) BUGS \
	   Compiler/bisonin Compiler/compilation Compiler/lexin

LDADD = Widgets/libWidgets.a -L$(top_builddir)/libs -lfvwm \
	$(xpm_LIBS) $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

