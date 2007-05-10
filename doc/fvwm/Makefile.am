
# $Id$

docdir=@FVWM_DOCDIR@/fvwm
XSLTPROC=@XSLTPROC@

XSL_FILE=$(srcdir)/../fvwm.xsl
XSL_MAN_FILE=$(srcdir)/../fvwm-man.xsl

SECTIONS=authors bidirText bindings bugs builtinBindings colorGradients colorsets commands compilation conditionals configuration copyright cpp description environment ewmh execution expansion focus fonts fvwm.man icccm images initialization m4 menus miscCommands moduleCommands modules multiScreen mwm openLook options quoting scripting session sessionCommands shortcuts styleCommands synopsis userFunctions virtualDesktop virtualDesktopCommands windowAnatomy windowMovement windowState windowStyles xinerama
COMMAND_FILES=$(shell ls $(srcdir)/../commands/*.xml)

# SS: Hmmm, I'm not sure if all versions of make support patsubst.	
HTML_FILES=$(patsubst %,%.html,$(SECTIONS))
XML_FILES=$(patsubst %,$(srcdir)/%.xml,$(SECTIONS))
EXTRA_DIST=$(XML_FILES)
BUILD_FILES=

if FVWM_BUILD_HTMLDOC
doc_DATA=$(HTML_FILES)
BUILD_FILES+=$(HTML_FILES)
endif

if FVWM_BUILD_MANDOC
BUILD_FILES+=fvwm.1
endif

all: $(BUILD_FILES)

QUIET=@
clean:
	rm -f $(HTML_FILES) fvwm.1

# All HTML output is generated from XML files.
%.html : $(srcdir)/%.xml
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude --stringparam html.stylesheet "../style.css" -o $@ $(XSL_FILE) $<

fvwm.man.html: $(XML_FILES) $(XSL_FILE) ../fvwm.ent $(COMMAND_FILES)
	$(QUIET) echo "Generating $@"
	$(QUIET) $(XSLTPROC) --path .. --xinclude --stringparam html.stylesheet "../style.css" --stringparam twdt 1 --stringparam section.autolabel 1 -o $@ $(XSL_FILE) $(srcdir)/fvwm.man.xml

fvwm.1: $(XML_FILES) $(srcdir)/fvwm.man.xml $(XSL_MAN_FILE)
	$(QUIET) echo "Generating $@ man page"
	$(QUIET) $(XSLTPROC) --path .. --xinclude $(XSL_MAN_FILE) $(srcdir)/fvwm.man.xml
