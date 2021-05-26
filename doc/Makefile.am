docdir = @FVWM_DOCDIR@
MODULE_ADOCS = $(wildcard fvwm3/fvwm3.adoc bin/*.adoc modules/*.adoc)

EXTRA_DIST = $(MODULE_ADOCS)

if FVWM_BUILD_MANDOC
BUILD_MANS = $(patsubst %.adoc,%, $(MODULE_ADOCS))
else
BUILD_MANS =
endif

all: docs
docs: $(BUILD_MANS)

clean:
	rm -fr man1/

distclean-local: clean

if FVWM_BUILD_MANDOC
QUIET_ASCIIDOC = @echo '  ' DOC '    ' $@'.1';

%: %.adoc
	$(QUIET_ASCIIDOC) \
	NAME=`basename "$@" | "$(SED)" -e "${transform}"`; \
	$(ASCIIDOC) -b manpage $< -o "man1/$$NAME.1"

install-data-local:
	install -d -m 755 $(DESTDIR)$(mandir)/man1/
	@for i in $(notdir $(BUILD_MANS)); do \
		NAME=`basename "$$i" | "$(SED)" -e "${transform}"`; \
		install -m 644 "man1/$$NAME.1" $(DESTDIR)$(mandir)/man1/; \
	done

uninstall-local:
	@for i in $(notdir $(BUILD_MANS)); do \
		NAME=`basename "$$i" | "$(SED)" -e "${transform}"`; \
		rm -f "$(DESTDIR)$(mandir)/man1/$$NAME.1"; \
	done
endif
