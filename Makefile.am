## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

## Make sure we have a current version of automake
AUTOMAKE_OPTIONS = 1.4

SUBDIRS = \
	default-config \
	libs \
	fvwm \
	modules \
	bin \
	utils \
	$(FVWM_PERLLIB) \
	doc \
	$(POSUB)

XSESSIONFILE = contrib/xsession/fvwm3.desktop
xsessionsdir = $(datarootdir)/xsessions
xsessions_DATA = $(XSESSIONFILE)

CLEANFILES = $(XSESSIONFILE)

EXTRA_DIST = CHANGELOG.md \
    $(XSESSIONFILE).in \
    meson.build \
    meson.options \
    meson-scripts \
    config_defines.h

## ---------------------------------------------------------------------------
## Manage bzip2 archive together with gzip archive
#  Usage:
#    make dist2       # instead of make dist
#    make distcheck2  # instead of make distcheck
#    make distclean2  # instead of make distclean

DISTCHECK_CONFIGURE_FLAGS = --enable-mandoc

distcheck2: distcheck
	@banner="$(distdir).tar.gz - ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$banner"; \
	echo "$$dashes"
	@if test "x$(ISRELEASED)" != xyes; then \
		echo 'Warning: ISRELEASED is not set to "yes".'; \
		echo '	So these can not be the official tarballs.'; \
		exit 1; \
	fi

distclean2: distclean

uninstall-hook:
	-rmdir @FVWM_DATADIR@

$(XSESSIONFILE): $(XSESSIONFILE).in
	${SED} -e 's|[@]bindir@|$(bindir)|g' \
		-e 's|[@]datadir@|$(datadir)|g' < "$@.in" > "$@"
