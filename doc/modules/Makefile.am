
# $Id$

docdir=@FVWM_DOCDIR@/modules
XSLTPROC=@XSLTPROC@

XSL_FILE=$(srcdir)/../fvwm.xsl

MODULES=FvwmAnimate FvwmAuto FvwmBacker FvwmBanner FvwmButtons FvwmCommand FvwmConsole FvwmCpp FvwmDebug FvwmDragWell FvwmEvent FvwmForm FvwmGtk FvwmIconBox FvwmIconMan FvwmIdent FvwmM4 FvwmPager FvwmPerl FvwmProxy FvwmRearrange FvwmSave FvwmSaveDesk FvwmScript FvwmScroll FvwmTabs FvwmTaskBar FvwmTheme FvwmWharf FvwmWindowMenu FvwmWinList
HTML_FILES=$(patsubst %,%.html,$(MODULES))
XML_FILES=todo.xml
EXTRA_DIST=modules.html $(XML_FILES)

if FVWM_BUILD_HTMLDOC
doc_DATA=$(HTML_FILES) modules.html
all: $(HTML_FILES)
endif


QUIET=@
clean:
	rm -f $(HTML_FILES)

# All HTML output is generated from XML files.
# %.html : $(srcdir)/%.xml
%.html: $(srcdir)/todo.xml
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude --stringparam html.stylesheet "../style.css" -o $@ $(XSL_FILE) $<
