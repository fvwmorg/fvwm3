## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmPipe
FvwmPipe_SOURCES = FvwmPipe.c

LDADD = -L$(top_builddir)/libs -lfvwm 
INCLUDES = -I$(top_srcdir)/libs $(X_CFLAGS)

