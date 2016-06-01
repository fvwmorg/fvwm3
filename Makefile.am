## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

## Make sure we have a current version of automake
AUTOMAKE_OPTIONS = 1.4

SUBDIRS = \
	libs \
	fvwm \
	modules \
	bin \
	utils \
	$(FVWM_PERLLIB) \
	doc \
	docs \
	$(POSUB) \
	tests

EXTRA_DIST = INSTALL.fvwm

## ---------------------------------------------------------------------------
## Manage bzip2 archive together with gzip archive
#  Usage:
#    make dist2       # instead of make dist
#    make distcheck2  # instead of make distcheck
#    make distclean2  # instead of make distclean

DISTCHECK_CONFIGURE_FLAGS = --enable-htmldoc

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
