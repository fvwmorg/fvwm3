## Process this file with automake to create Makefile.in

## Notes: Imakefile used "changepath.sh" script to install proper path
## to perl interpreter in script.  This has been dropped.

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmConsole FvwmConsoleC
module_SCRIPTS = FvwmConsoleC.pl

FvwmConsole_SOURCES = FvwmConsole.c FvwmConsole.h
FvwmConsoleC_SOURCES = FvwmConsoleC.c FvwmConsole.h getline.c

man_MANS = FvwmConsole.1 FvwmConsoleC.pl.1
EXTRA_DIST = $(man_MANS) $(module_SCRIPTS) Changes .fvwm2rc.sample

LDADD = -L$(top_builddir)/libs -lfvwm $(readline_LIBS) $(socket_LIBS)

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
INCLUDES = -I$(top_srcdir)/libs $(readline_CFLAGS) \
	 $(X_CFLAGS) $(xpm_CFLAGS)
