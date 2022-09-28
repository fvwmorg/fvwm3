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
# won't ever be installed.
if FVWM_BUILD_GOLANG
MODULE_ADOC = $(filter-out FvwmConsole.adoc, $(MODULE_ADOC_SRCS))
else
MODULE_ADOC = $(MODULE_ADOC_SRCS)
endif

EXTRA_DIST = $(MODULE_ADOC)

nothing:

clean:
	rm -f *.1 *.ad

distclean-local: clean

if FVWM_BUILD_MANDOC
man1_MANS = $(patsubst %.adoc,%.1, $(MODULE_ADOC))
EXTRACT_SECTIONS = \
	commands \
	menus \
	styles
SECTION_FILES = $(patsubst %,fvwm3_%.ad, $(EXTRACT_SECTIONS))

all: docs
docs: $(man1_MANS)
$(man1_MANS): $(SECTION_FILES)

%.ad: fvwm3_manpage_source.adoc
	SECTION=$(patsubst fvwm3_%.ad,%,$@); \
	cat "$<" | \
	"$(GREP)" -A 1000000 -- "^// BEGIN '$$SECTION'" | \
	"$(GREP)" -B 1000000 -- "^// END '$$SECTION'" | \
	"$(GREP)" -v "^// .* '$$SECTION'" > "$@"

%.1: %.adoc
	"$(ASCIIDOC)" -b manpage -a "$(patsubst %.1,%,$@)" "$<" -o "$@"
endif
