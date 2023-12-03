docdir = @FVWM_DOCDIR@
MODULE_ADOC_SRCS = \
	fvwm3.adoc \
	fvwm3all.adoc \
	fvwm3commands.adoc \
	fvwm3_manpage_source.adoc \
	fvwm3menus.adoc \
	fvwm3styles.adoc \
	$(wildcard Fvwm*.adoc) \
	$(wildcard fvwm-*.adoc)

# If building FvwmPrompt, don't generate the manpage for FvwmConsole as that
# won't ever be installed, otherwise don't generate FvwmPrompt manpage.
if FVWM_BUILD_GOLANG
MODULE_ADOC = $(filter-out FvwmConsole.adoc, $(MODULE_ADOC_SRCS))
REMOVE_LINK = FvwmConsole.adoc
else
MODULE_ADOC = $(filter-out FvwmPrompt.adoc, $(MODULE_ADOC_SRCS))
REMOVE_LINK = FvwmPrompt.adoc
endif

EXTRA_DIST = $(MODULE_ADOC_SRCS)

nothing:

clean:
	rm -f *.1 *.ad *.html index.adoc

distclean-local: clean

if FVWM_BUILD_DOC
M1M = $(filter-out fvwm3_manpage_source.adoc, $(MODULE_ADOC))
if FVWM_BUILD_MANDOC
man1_MANS = $(patsubst %.adoc,%.1, $(M1M))
endif
if FVWM_BUILD_HTMLDOC
html_MANS = index.html $(patsubst %.adoc,%.html, $(M1M))
doc_DATA = $(html_MANS)
endif

EXTRACT_SECTIONS = \
	commands \
	menus \
	styles
SECTION_FILES = $(patsubst %,fvwm3_%.ad, $(EXTRACT_SECTIONS))

all: docs
docs: $(man1_MANS) $(html_MANS)
$(man1_MANS): $(SECTION_FILES)
$(html_MANS): $(SECTION_FILES)

%.ad: fvwm3_manpage_source.adoc
	SECTION=$(patsubst fvwm3_%.ad,%,$@); \
	cat "$<" | \
	"$(GREP)" -A 1000000 -- "^// BEGIN '$$SECTION'" | \
	"$(GREP)" -B 1000000 -- "^// END '$$SECTION'" | \
	"$(GREP)" -v "^// .* '$$SECTION'" > "$@"

%.1: %.adoc
	"$(ASCIIDOC)" -b manpage -a "$(patsubst %.1,%,$@)" "$<" -o "$@"

index.html:
	"$(SED)" '/$(REMOVE_LINK)/d' index.adoc.in > index.adoc; \
	"$(ASCIIDOC)" -b html5 -a toc -a webfonts! -a index index.adoc -o index.html

%.html: %.adoc
	"$(ASCIIDOC)" -b html5 -a toc -a webfonts! -a "$(patsubst %.html,%,$@)" "$<" -o "$@"

endif
