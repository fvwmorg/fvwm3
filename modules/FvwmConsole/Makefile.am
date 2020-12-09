## Process this file with automake to create Makefile.in
program_transform_name =

PERL = @PERL@
moduledir = @FVWM_MODULEDIR@

# If FVWM_BUILD_GOLANG has been defined via --enable-golang, then FvwmPrompt
# is preferred.  In this case, disable compiling/installing FvwmConsole --
# eventually, FvwmConsole will go away completely, but for compatibility
# reasons, remains here only under this condition.
if !FVWM_BUILD_GOLANG

module_PROGRAMS = FvwmConsole FvwmConsoleC
module_SCRIPTS = FvwmConsoleC.pl

FvwmConsole_SOURCES = FvwmConsole.c FvwmConsole.h
FvwmConsoleC_SOURCES = FvwmConsoleC.c FvwmConsole.h getline.c

FvwmConsole_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a
FvwmConsoleC_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

EXTRA_DIST = FvwmConsoleC.pl.in Changes .fvwm2rc.sample

# Use X_EXTRA_LIBS to get socket(), etc.
LDADD = -L$(top_builddir)/libs -lfvwm3 $(readline_LIBS) $(freetype_LIBS) \
	$(X_EXTRA_LIBS)

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
AM_CPPFLAGS = -I$(top_srcdir) $(readline_CFLAGS) $(xpm_CFLAGS) \
	      $(freetype_CFLAGS) $(X_CFLAGS)
endif !FVWM_BUILD_GOLANG
