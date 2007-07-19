# Author: Scott Smedley

docdir = @FVWM_DOCDIR@

HTML_FILES = \
	index.html allCommands.html groupedCommands.html modules.html style.css
EXTRA_DIST = \
	fvwm.xsl fvwm-man.xsl header.html $(HTML_FILES)
EXTRA_DIST += \
	$(shell find $(srcdir)/docbook-x?l -type f)

if FVWM_BUILD_HTMLDOC
doc_DATA = \
	$(HTML_FILES)
endif


SUBDIRS=fvwm commands images modules

all: $(doc_DATA)

%.html: $(srcdir)/$@
	test x"$(top_srcdir)" = x"$(top_builddir)" || \
	cp $(srcdir)/$@ $(top_builddir)/doc/$@

distclean-local:
	test x"$(top_srcdir)" = x"$(top_builddir)" || \
	rm -f $(top_builddir)/doc/*.html
