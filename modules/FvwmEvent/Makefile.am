## Process this file with automake to create Makefile.in

moduleexecdir = @FVWM_MODULEDIR@
moduleexec_PROGRAMS = FvwmEvent
man_MANS = FvwmEvent.1
EXTRA_DIST = $(man_MANS)

FvwmEvent_SOURCES = FvwmEvent.c
FvwmEvent_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

# This module behaves like the old FvwmAudio when invoked with that name.
install-exec-local: ../../config.h Makefile
	$(mkinstalldirs) $(DESTDIR)$(moduleexecdir)
	rm -f $(DESTDIR)$(moduleexecdir)/FvwmAudio
	@_fe=`echo FvwmEvent|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	 _fa=`echo FvwmAudio|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	 echo "  $(LN_S) $$_fe $(DESTDIR)$(moduleexecdir)/$$_fa"; \
	 $(LN_S) "$$_fe" $(DESTDIR)$(moduleexecdir)/"$$_fa" || true

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm $(rplay_LIBS) \
	$(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

AM_CPPFLAGS = -I$(top_srcdir) $(rplay_CFLAGS) $(X_CFLAGS)
