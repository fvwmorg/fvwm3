## Process this file with automake to create Makefile.in

PERL = @PERL@

SUBDIRS = scripts

bin_PROGRAMS = FvwmCommand
moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmCommandS
module_DATA = FvwmCommand.sh FvwmCommand.pm

FvwmCommandS_SOURCES = FvwmCommandS.c FvwmCommand.h
FvwmCommand_SOURCES = FvwmCommand.c FvwmCommand.h

FvwmCommand.sh: findcmd.pl $(top_srcdir)/fvwm/functions.c
	if test -n "$(PERL)" -a -x "$(PERL)"; then \
		$(PERL) $(srcdir)/findcmd.pl $(bindir) -sh \
		< $(top_srcdir)/fvwm/functions.c > $@; \
	else echo '# Did not find perl during FVWM install' > $@; fi

FvwmCommand.pm: findcmd.pl $(top_srcdir)/fvwm/functions.c
	if test -n "$(PERL)" -a -x "$(PERL)"; then \
		$(PERL) $(srcdir)/findcmd.pl $(bindir) \
		< $(top_srcdir)/fvwm/functions.c > $@; \
	else echo '# Did not find perl during FVWM install' > $@; fi

CLEANFILES = FvwmCommand.sh FvwmCommand.pm

FvwmCommandS_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
FvwmCommand_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmCommand.1

EXTRA_DIST = $(man_MANS) findcmd.pl

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir) $(xpm_CFLAGS) $(X_CFLAGS)

