docdir = @FVWM_DOCDIR@/commands
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl

include $(top_srcdir)/doc/commands.mak

HTML_FILES = $(patsubst %,%.html,$(COMMANDS))
XML_FILES = $(patsubst %,%.xml,$(COMMANDS))
EXTRA_DIST = $(XML_FILES)

if FVWM_BUILD_HTMLDOC
doc_DATA = $(HTML_FILES)
all: $(HTML_FILES)
endif

clean:
	rm -f x $(HTML_FILES)

distclean-local: clean

# All HTML output is generated from XML files.
%.html : $(srcdir)/%.xml $(srcdir)/../header.html $(srcdir)/../style.css
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o $(<F).p \
		$(XSL_PROFILE) $<
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		--stringparam html.stylesheet "$(srcdir)/../style.css" \
		--stringparam header.file $(srcdir)/../header.html \
		-o $@ $(XSL_FILE) $(<F).p && rm $(<F).p
