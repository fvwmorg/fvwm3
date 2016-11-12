## Process this file with automake to create Makefile.in

PERL = @PERL@

SUBDIRS = scripts

bin_PROGRAMS = FvwmCommand
moduledir = @FVWM_MODULEDIR@
module_DATA = FvwmCommand.sh FvwmCommand.pm

FvwmCommand_SOURCES = FvwmCommand.c FvwmCommand.h fifos.c

FvwmCommand.sh: findcmd.pl $(top_srcdir)/fvwm/functable.c
	if test -n "$(PERL)" -a -x "$(PERL)"; then \
		$(PERL) $(srcdir)/findcmd.pl $(bindir) -sh \
		< $(top_srcdir)/fvwm/functable.c > $@; \
	else echo '# Did not find perl during fvwm install' > $@; fi

FvwmCommand.pm: findcmd.pl $(top_srcdir)/fvwm/functable.c
	if test -n "$(PERL)" -a -x "$(PERL)"; then \
		$(PERL) $(srcdir)/findcmd.pl $(bindir) \
		< $(top_srcdir)/fvwm/functable.c > $@; \
	else echo '# Did not find perl during fvwm install' > $@; fi

CLEANFILES = FvwmCommand.sh FvwmCommand.pm

FvwmCommand_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

man_MANS = FvwmCommand.1

EXTRA_DIST = $(man_MANS) findcmd.pl

LDADD = -L$(top_builddir)/libs -lfvwm3

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS)
