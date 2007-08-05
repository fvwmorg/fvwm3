# Author: Scott Smedley

docdir = @FVWM_DOCDIR@/fvwm
XSLTPROC = @XSLTPROC@

XSL_FILE = $(srcdir)/../fvwm.xsl
XSL_MAN_FILE = $(srcdir)/../fvwm-man.xsl
XSL_PROFILE = $(srcdir)/../docbook-xsl/profiling/profile.xsl


SECTIONS = \
	authors bidirText bindings bugs builtinBindings colorGradients \
	colorsets commands compilation conditionals configuration copyright \
	cpp description environment ewmh execution expansion focus fonts \
	fvwm.man icccm images initialization m4 menus miscCommands \
	moduleCommands modules multiScreen mwm openLook options quoting \
	scripting session sessionCommands shortcuts styleCommands synopsis \
	userFunctions virtualDesktop virtualDesktopCommands windowAnatomy \
	windowMovement windowState windowStyles xinerama

include $(top_srcdir)/doc/commands.mak
COMMAND_FILES = $(patsubst %,$(top_srcdir)/doc/commands/%.xml,$(COMMANDS))

# SS: Hmmm, I'm not sure if all versions of make support patsubst.
# HTML_FILES = $(patsubst %,%.html,$(SECTIONS))
HTML_FILES = fvwm.man.html
XML_FILES = $(patsubst %,$(srcdir)/%.xml,$(SECTIONS))
EXTRA_DIST = $(XML_FILES) $(man_MANS)
BUILD_FILES =
man_MANS = fvwm.1

if FVWM_BUILD_HTMLDOC
doc_DATA = $(HTML_FILES)
BUILD_FILES += $(HTML_FILES)
endif

if FVWM_BUILD_MANDOC
BUILD_FILES += $(man_MANS)
endif

all: $(BUILD_FILES)

clean:
	rm -f x $(HTML_FILES) $(man_MANS)

distclean-local: clean

#%.html : $(srcdir)/%.xml $(srcdir)/../style.css
#	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
#		--stringparam profile.attribute output \
#		--stringparam profile.value html \
#		-o $(<F).p \
#		$(XSL_PROFILE) $<
#	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
#		--stringparam html.stylesheet "$(srcdir)/../style.css" \
#		-o $@ $(XSL_FILE) $(<F).p && rm $(<F).p

fvwm.man.html: $(XML_FILES) $(XSL_FILE) $(top_builddir)/doc/fvwm.ent $(COMMAND_FILES) $(srcdir)/../header.html $(srcdir)/../style.css
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value html \
		-o fvwm.man.xml.p \
		$(XSL_PROFILE) $(srcdir)/fvwm.man.xml
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		--stringparam html.stylesheet "$(srcdir)/../style.css" \
		--stringparam header.file $(srcdir)/../header.html \
		--stringparam twdt 1 \
		--stringparam section.autolabel 1 \
		-o $@ $(XSL_FILE) fvwm.man.xml.p && rm fvwm.man.xml.p

fvwm.1: $(XML_FILES) $(srcdir)/fvwm.man.xml $(XSL_MAN_FILE)
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		--stringparam profile.attribute output \
		--stringparam profile.value man \
		-o fvwm.man.xml.p \
		$(XSL_PROFILE) $(srcdir)/fvwm.man.xml
	$(XSLTPROC) --path $(top_builddir)/doc --xinclude \
		$(XSL_MAN_FILE) fvwm.man.xml.p && rm fvwm.man.xml.p

distclean-local:
	rm -f $(top_builddir)/doc/fvwm/fvwm.1

install-data-local:
	@rm -f $(DESTDIR)$(mandir)/man1/fvwm2.1
	@$(LN_S) fvwm.1 $(DESTDIR)$(mandir)/man1/fvwm2.1 || \
	echo "Minor warning: $(mandir)/man1/fvwm2.1 symlink was not created"
