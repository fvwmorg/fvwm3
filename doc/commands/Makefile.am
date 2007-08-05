docdir = @FVWM_DOCDIR@/commands
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl

include $(top_srcdir)/doc/commands.mak

EXTRA_DIST = $(XML_COMMANDS)

if FVWM_BUILD_HTMLDOC
doc_DATA = $(HTML_COMMANDS)
all: $(HTML_COMMANDS)
endif

clean:
	rm -f x $(HTML_COMMANDS)

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
