## Process this file with automake to create Makefile.in

PERL = @PERL@
moduledir = @FVWM_MODULEDIR@

SUBDIRS = scripts

bin_PROGRAMS = FvwmCommand
module_PROGRAMS = FvwmCommandS
module_DATA = FvwmCommand.sh FvwmCommand.pm

FvwmCommandS_SOURCES = FvwmCommandS.c FvwmCommand.h
FvwmCommand_SOURCES = FvwmCommand.c FvwmCommand.h

FvwmCommand.sh: findcmd
	$(SHELL) $(srcdir)/findcmd $(moduledir)		\
		< $(top_srcdir)/fvwm/functions.c > $@

FvwmCommand.pm: findcmd.pl
	if test -n "$(PERL)"; then $(PERL) $(srcdir)/findcmd.pl $(moduledir)	\
		< $(top_srcdir)/fvwm/functions.c > $@;				\
	else echo Did not find perl during FVWM install > $@; fi

FvwmCommandS_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
FvwmCommand_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmCommand.1

EXTRA_DIST = $(man_MANS) findcmd findcmd.pl

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

