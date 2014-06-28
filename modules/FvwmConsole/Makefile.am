## Process this file with automake to create Makefile.in

PERL = @PERL@
moduledir = @FVWM_MODULEDIR@

module_PROGRAMS = FvwmConsole FvwmConsoleC
module_SCRIPTS = FvwmConsoleC.pl

FvwmConsole_SOURCES = FvwmConsole.c FvwmConsole.h
FvwmConsoleC_SOURCES = FvwmConsoleC.c FvwmConsole.h getline.c

FvwmConsole_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a
FvwmConsoleC_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmConsole.1 FvwmConsoleC.pl.1
EXTRA_DIST = FvwmConsoleC.pl.in $(man_MANS) Changes .fvwm2rc.sample

# Use X_EXTRA_LIBS to get socket(), etc.
LDADD = -L$(top_builddir)/libs -lfvwm $(readline_LIBS) $(X_EXTRA_LIBS)

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
AM_CPPFLAGS = -I$(top_srcdir) $(readline_CFLAGS) $(xpm_CFLAGS) $(X_CFLAGS)
