
# $Id$

# Author: Scott Smedley

docdir=@FVWM_DOCDIR@/modules
XSLTPROC=@XSLTPROC@

XSL_FILE=$(srcdir)/../fvwm.xsl
XSL_MAN_FILE=$(srcdir)/../fvwm-man.xsl
XSL_PROFILE=$(srcdir)/../docbook-xsl/profiling/profile.xsl


MODULES=FvwmAnimate FvwmAuto FvwmBacker FvwmBanner FvwmButtons FvwmCommand FvwmConsole FvwmCpp FvwmDebug FvwmDragWell FvwmEvent FvwmForm FvwmGtk FvwmIconBox FvwmIconMan FvwmIdent FvwmM4 FvwmPager FvwmPerl FvwmProxy FvwmRearrange FvwmSave FvwmSaveDesk FvwmScript FvwmScroll FvwmTabs FvwmTaskBar FvwmTheme FvwmWharf FvwmWindowMenu FvwmWinList
HTML_FILES=$(patsubst %,%.html,$(MODULES))
XML_FILES=todo.xml
EXTRA_DIST=$(XML_FILES)

if FVWM_BUILD_HTMLDOC
doc_DATA=$(HTML_FILES)
all: $(HTML_FILES) imagedir
endif

QUIET=@
clean:
	rm -f $(HTML_FILES)

FvwmTabs.html: $(srcdir)/FvwmTabs.xml ../fvwm.ent
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o $<.p \
		$(XSL_PROFILE) $(srcdir)/FvwmTabs.xml
	$(QUIET) $(XSLTPROC) --path .. --xinclude \
		--stringparam html.stylesheet "../style.css" \
		--stringparam section.autolabel 1 \
		--stringparam header.file $(srcdir)/../header.html \
		-o $@ $(XSL_FILE) $<.p && rm $<.p

FvwmTabs.1: $(srcdir)/FvwmTabs.xml $(XSL_MAN_FILE)
	$(QUIET) echo "Generating $@ man page"
	$(QUIET) $(XSLTPROC) --path .. --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value man \
		-o $<.p \
		$(XSL_PROFILE) $(srcdir)/FvwmTabs.xml
	$(QUIET) $(XSLTPROC) --path .. --xinclude \
		$(XSL_MAN_FILE) $<.p && rm $<.p

imagedir:
	@ [ ! -d images ] && cp -rp $(srcdir)/images . || true

install: install-am
	@ cp -rp images $(docdir)


# All HTML output is generated from XML files.
# %.html : $(srcdir)/%.xml
%.html: $(srcdir)/todo.xml
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude --stringparam html.stylesheet "../style.css" -o $@ $(XSL_FILE) $<

