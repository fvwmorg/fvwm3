## Process this file with automake to create Makefile.in

## Conversion notes: Imakefile talks about FvwmCommand.sh and
## FvwmCommand.pm, but they are not to be found.  The perl example
## scripts were not installed.

bin_PROGRAMS = FvwmCommand

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmCommandS

FvwmCommandS_SOURCES = FvwmCommandS.c FvwmCommand.h
FvwmCommand_SOURCES = FvwmCommand.c FvwmCommand.h

FvwmCommandS_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
FvwmCommand_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmCommand.1

examples = ex-auto.pl ex-cascade.pl ex-grpmv.pl \
	focus-Netscape.pl push-away.pl test1.pl

EXTRA_DIST = $(man_MANS) $(examples) \
	   findcmd findcmd.pl test2.sh

LDADD = -L$(top_builddir)/libs -lfvwm

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

