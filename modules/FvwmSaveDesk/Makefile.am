## Process this file with automake to create Makefile.in

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmSaveDesk
man_MANS = FvwmSaveDesk.1
EXTRA_DIST = $(man_MANS) example/fvwm2rc example/xinitrc

FvwmSaveDesk_SOURCES = FvwmSaveDesk.c FvwmSaveDesk.h
FvwmSaveDesk_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm  \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(Xft_CFLAGS) $(X_CFLAGS)

