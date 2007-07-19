
# $Id$

# Author: Scott Smedley

docdir=@FVWM_DOCDIR@/images
EXTRA_DIST=fvwm.logo.png fvwm.logo2.png m.logo.png

if FVWM_BUILD_HTMLDOC
doc_DATA=$(EXTRA_DIST)
endif

SUBDIRS=svg_rendering

all: $(doc_DATA)

%.png: $(srcdir)/$@
	test x"$(top_srcdir)" = x"$(top_builddir)" || \
	cp $(srcdir)/$@ $(top_builddir)/doc/images/$@

distclean-local:
	test x"$(top_srcdir)" = x"$(top_builddir)" || \
	rm -f $(top_builddir)/doc/images/*.png
