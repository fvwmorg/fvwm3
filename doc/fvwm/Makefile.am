# Author: Scott Smedley

docdir = @FVWM_DOCDIR@/fvwm3
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_MAN_FILE = $(srcdir)/../fvwm-man.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl

HTML_FILES = fvwm3.man.html
XML_FILES = @DOC_SECTIONS_XML_PATH@
EXTRA_DIST = @DOC_SECTIONS_XML@ $(man_MANS) sections
man_MANS = fvwm3.1

if FVWM_BUILD_HTMLDOC
doc_DATA = $(HTML_FILES)
BUILD_HTML_FILES = $(HTML_FILES)
else
BUILD_HTML_FILES =
endif

if FVWM_BUILD_MANDOC
BUILD_MANS = $(man_MANS)
else
BUILD_MANS =
endif

all: $(BUILD_HTML_FILES) $(BUILD_MANS)

clean:
	rm -f $(BUILD_HTML_FILES) $(BUILD_MANS)

distclean-local: clean

if FVWM_BUILD_MANDOC
fvwm.man.html: $(XML_FILES) $(XSL_FILE) $(top_builddir)/doc/fvwm.ent @DOC_COMMANDS_XML_PATH@ $(srcdir)/../header.html
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o fvwm.man.html.tmp \
		$(XSL_PROFILE) "$(srcdir)/fvwm.man.xml"
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		--stringparam html.stylesheet "../style.css" \
		--stringparam header.file "$(srcdir)/../header.html" \
		--stringparam twdt 1 \
		--stringparam section.autolabel 1 \
		-o $@ $(XSL_FILE) fvwm.man.html.tmp && rm fvwm.man.html.tmp

fvwm3.1: $(XML_FILES) $(srcdir)/fvwm.man.xml $(XSL_MAN_FILE) @DOC_COMMANDS_XML_PATH@
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value man \
		-o fvwm3.1.tmp \
		$(XSL_PROFILE) "$(srcdir)/fvwm.man.xml"
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		$(XSL_MAN_FILE) fvwm3.1.tmp && rm fvwm3.1.tmp
#	mv fvwm.1 fvwm.1.pre
#	tbl fvwm.1.pre | sed '/^\.T[SE]$$/d' > fvwm.1 && rm fvwm.1.pre
endif
