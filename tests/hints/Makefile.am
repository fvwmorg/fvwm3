## Process this file with automake to create Makefile.in

# Don't install this, its just for testing.
noinst_PROGRAMS = hints_test

LDADD = -L$(top_builddir)/libs $(X_LIBS) -lfvwm \
	$(X_PRE_LIBS) -lXext -lX11 -lm $(X_EXTRA_LIBS)
AM_CPPFLAGS = -I$(top_srcdir) $(X_CFLAGS)
