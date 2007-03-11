
# $Id$

docdir=@FVWM_DOCDIR@/images
EXTRA_DIST=fvwm.logo.png fvwm.logo2.png m.logo.png

if FVWM_BUILD_HTMLDOC
doc_DATA=$(EXTRA_DIST)
endif

all:
	@for f in $(doc_DATA) ; do \
		[ ! -f $$f ] && cp -p $(srcdir)/$$f $$f; \
	done ; true

