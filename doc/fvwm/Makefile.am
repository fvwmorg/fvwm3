# Author: Scott Smedley

docdir = @FVWM_DOCDIR@/fvwm
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_MAN_FILE = $(srcdir)/../fvwm-man.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl

HTML_FILES = fvwm.man.html
XML_FILES = @DOC_SECTIONS_XML_PATH@
EXTRA_DIST = @DOC_SECTIONS_XML@ $(man_MANS) sections
man_MANS = fvwm.1

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

#%.html : $(srcdir)/%.xml $(srcdir)/../style.css
#	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
#		--xinclude \
#		--stringparam profile.attribute output \
#		--stringparam profile.value html \
#		-o $(<F).p \
#		$(XSL_PROFILE) $<
#	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
#		--xinclude \
#		--stringparam html.stylesheet "$(srcdir)/../style.css" \
#		-o $@ $(XSL_FILE) $(<F).p && rm $(<F).p

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

fvwm.1: $(XML_FILES) $(srcdir)/fvwm.man.xml $(XSL_MAN_FILE) @DOC_COMMANDS_XML_PATH@
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value man \
		-o fvwm.1.tmp \
		$(XSL_PROFILE) "$(srcdir)/fvwm.man.xml"
	$(XSLTPROC) --path "$(top_builddir)/doc":"$(top_srcdir)/doc/commands" \
		--xinclude \
		$(XSL_MAN_FILE) fvwm.1.tmp && rm fvwm.1.tmp
#	mv fvwm.1 fvwm.1.pre
#	tbl fvwm.1.pre | sed '/^\.T[SE]$$/d' > fvwm.1 && rm fvwm.1.pre

install-data-local:
	@rm -f "$(DESTDIR)$(mandir)/man1/fvwm2.1"
	@$(LN_S) fvwm.1 "$(DESTDIR)$(mandir)/man1/fvwm2.1" || \
	echo "Minor warning: $(mandir)/man1/fvwm2.1 symlink was not created"
