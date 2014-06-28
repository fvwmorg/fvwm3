## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

SUBDIRS = Scripts Widgets

moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = FvwmScript

FvwmScript_SOURCES = FvwmScript.c Instructions.c libyywrap.c \
	types.h scanner.c script.c script.h
FvwmScript_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a Widgets/libWidgets.a

YFLAGS = -d

man_MANS = FvwmScript.1
EXTRA_DIST = $(man_MANS) scanner.l script.y TODO

LDADD = Widgets/libWidgets.a -L$(top_builddir)/libs -lfvwm $(Xft_LIBS) \
	$(X_LIBS) $(xpm_LIBS) $(Xinerama_LIBS) $(X_PRE_LIBS) -lXext -lX11 \
	$(X_EXTRA_LIBS) -lm $(Xrender_LIBS) $(Xcursor_LIBS) $(iconv_LIBS) \
	$(Bidi_LIBS) $(png_LIBS) $(rsvg_LIBS) $(intl_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS) \
	$(Xrender_CFLAGS) $(iconv_CFLAGS) $(Bidi_CFLAGS) $(png_CFLAGS)

# Need this to get datadir into FvwmScript.c:
AM_CFLAGS = -DFVWM_DATADIR=\"$(FVWM_DATADIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

.PHONY: syntax

# We don't autoconfigure lex/yacc, since these tools only needed for
# developers that want to regenerate the FvwmScript syntax.
# To override the default values use: make YACC="bison -y" syntax
LEX = flex
YACC = yacc

syntax:
	rm -f lex.yy.c y.tab.c y.tab.h script.tab.c script.tab.h || true
	if $(LEX) --nounput --help 2>/dev/null > /dev/null; then \
		$(LEX) $(AM_LFLAGS) $(LFLAGS) --nounput scanner.l; \
	else \
		$(LEX) $(AM_LFLAGS) $(LFLAGS) scanner.l; \
	fi && mv lex.yy.c scanner.c

	@# detect and prefer bison; bison without -y uses file.tab.[ch] names
	YACC=$(YACC); if which bison >/dev/null 2>&1; then YACC="bison -y"; fi; \
	$$YACC $(AM_YFLAGS) $(YFLAGS) script.y && mv *.tab.c script.c

	@# don't touch unmodified header to reduce rebuilds
	if test -f y.tab.h -o -f script.tab.h; then \
		if cmp -s *.tab.h script.h; then \
			rm -f y.tab.h; \
		else \
			mv *.tab.h script.h; \
		fi; \
	else :; fi
