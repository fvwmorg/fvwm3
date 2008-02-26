docdir = @FVWM_DOCDIR@/commands
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl

EXTRA_DIST = @DOC_COMMANDS_XML@ WindowsDesk.xml

if FVWM_BUILD_HTMLDOC
doc_DATA = @DOC_COMMANDS_HTML@ WindowsDesk.html
all: @DOC_COMMANDS_HTML@  WindowsDesk.html
endif

clean:
	rm -f @DOC_COMMANDS_HTML@ WindowsDesk.html

distclean-local: clean

# All HTML output is generated from XML files.
%.html : $(srcdir)/%.xml $(srcdir)/../header.html $(srcdir)/../style.css
	$(XSLTPROC) --path "$(top_builddir)/doc" --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o $(<F).p \
		$(XSL_PROFILE) $<
	$(XSLTPROC) --path "$(top_builddir)/doc" --xinclude \
		--stringparam html.stylesheet "$(srcdir)/../style.css" \
		--stringparam header.file "$(srcdir)/../header.html" \
		-o $@ $(XSL_FILE) $(<F).p && rm $(<F).p
