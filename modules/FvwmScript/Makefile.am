## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

SUBDIRS = Scripts Widgets

moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = @FVWMSCRIPT@
EXTRA_PROGRAMS = FvwmScript

FvwmScript_SOURCES = FvwmScript.c Instructions.c libyywrap.c \
	types.h scanner.c script.c script.h
FvwmScript_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a Widgets/libWidgets.a

YFLAGS = -d

man_MANS = FvwmScript.1
EXTRA_DIST = $(man_MANS) scanner.l script.y TODO README

LDADD = Widgets/libWidgets.a -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(xpm_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS) -lm
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

.PHONY: syntax

syntax:
	$(LEX) $(AM_LFLAGS) $(LFLAGS) scanner.l && mv lex.yy.c scanner.c
	$(YACC) $(AM_YFLAGS) $(YFLAGS) script.y && mv y.tab.c script.c
	if test -f y.tab.h; then \
	if cmp -s y.tab.h script.h; then rm -f y.tab.h; else mv y.tab.h script.h; fi; \
	else :; fi
