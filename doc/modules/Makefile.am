# Author: Scott Smedley

SUBDIRS = images

docdir = @FVWM_DOCDIR@/modules
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_MAN_FILE = $(srcdir)/../fvwm-man.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl


MODULES = \
	FvwmAnimate FvwmAuto FvwmBacker FvwmBanner FvwmButtons FvwmCommand \
	FvwmConsole FvwmCpp FvwmDebug FvwmDragWell FvwmEvent FvwmForm \
	FvwmGtk FvwmIconBox FvwmIconMan FvwmIdent FvwmM4 FvwmPager FvwmPerl \
	FvwmProxy FvwmRearrange FvwmSave FvwmSaveDesk FvwmScript FvwmScroll \
	FvwmTabs FvwmTaskBar FvwmTheme FvwmWharf FvwmWindowMenu FvwmWinList
# one day we should have:
#MODULES_XML = $(patsubst %,%.xml,$(MODULES))
MODULES_XML = FvwmTabs.xml
HTML_FILES = $(patsubst %,%.html,$(MODULES))
XML_FILES = $(MODULES_XML) todo.xml
EXTRA_DIST = $(XML_FILES)

if FVWM_BUILD_HTMLDOC
doc_DATA = $(HTML_FILES)
endif

clean:
	rm -f $(HTML_FILES)

FvwmTabs.html: $(srcdir)/FvwmTabs.xml $(top_builddir)/doc/fvwm.ent
	$(XSLTPROC) --path $(top_builddir)/doc:$(top_srcdir)/doc --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o FvwmTabs.xml.p \
		$(XSL_PROFILE) $(srcdir)/FvwmTabs.xml
	$(XSLTPROC) --path $(top_builddir)/doc:$(top_srcdir)/doc --xinclude \
		--stringparam html.stylesheet "$(srcdir)/../style.css" \
		--stringparam section.autolabel 1 \
		--stringparam header.file $(srcdir)/../header.html \
		-o $@ $(XSL_FILE) FvwmTabs.xml.p && rm FvwmTabs.xml.p

FvwmTabs.1: $(srcdir)/FvwmTabs.xml $(XSL_MAN_FILE)
	$(XSLTPROC) --path $(top_builddir)/doc:$(top_srcdir)/doc --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value man \
		-o FvwmTabs.xml.p \
		$(XSL_PROFILE) $(srcdir)/FvwmTabs.xml
	$(XSLTPROC) --path $(top_builddir)/doc:$(top_srcdir)/doc --xinclude \
		$(XSL_MAN_FILE) FvwmTabs.xml.p && rm FvwmTabs.xml.p

# All HTML output is generated from XML files.
# %.html : $(srcdir)/%.xml
%.html: $(srcdir)/todo.xml
	$(XSLTPROC) --path $(top_builddir)/doc:$(top_srcdir)/doc \
		--xinclude --stringparam \
		html.stylesheet "$(srcdir)/../style.css" -o $@ $(XSL_FILE) $<
