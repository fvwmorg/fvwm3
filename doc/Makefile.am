docdir = @FVWM_DOCDIR@
MODULE_ADOCS = $(wildcard fvwm3/fvwm3.adoc bin/*.adoc modules/*.adoc)

man_MANS = $(patsubst %.adoc,%.1, $(MODULE_ADOCS))
EXTRA_DISTS = $(man_MANS)

if FVWM_BUILD_MANDOC
BUILD_MANS = $(man_MANS)
else
BUILD_MANS =
endif

all: docs
docs: $(BUILD_MANS)

clean:
	rm -f $(BUILD_MANS)

distclean-local: clean

if FVWM_BUILD_MANDOC
QUIET_ASCIIDOC = @echo '  ' DOC '    ' $@;

%.1: %.adoc
	$(QUIET_ASCIIDOC)$(ASCIIDOC) -b manpage $< -o $@

install-data-local:
	install -d -m 755 $(DESTDIR)$(mandir)/man1/
	install -m 644 $(BUILD_MANS) $(DESTDIR)$(mandir)/man1/

uninstall-local:
	@for i in $(notdir $(BUILD_MANS)); do \
		rm -f "$(DESTDIR)$(mandir)/man1/$$i"; \
	done
endif
