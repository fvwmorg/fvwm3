## Process this file with automake to create Makefile.in

program_transform_name =

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmCommandS

FvwmCommandS_SOURCES = FvwmCommandS.c FvwmCommand.h fifos.c

FvwmCommandS_DEPENDENCIES = $(top_builddir)/libs/libfvwm3.a

LDADD = -L$(top_builddir)/libs -lfvwm3

# FIXME:
# Despite not using X functions explicitly, the code includes
# fvwmlib.h, which *does* include X headers and xpm.h!
AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS)
