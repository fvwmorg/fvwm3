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
	rpm \
	debian \
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

## ---------------------------------------------------------------------------
## Produce an rpm package using dist or from the given tarball
## (requires rpmbuild in your system)
#  Usage:
#    make rpm-dist
#    make release=1 rpm-dist
#    make rpm-this
#    make version=2.3.22 release=2 rpm-this
#    make rpm-dist cparams='--enable-multibyte --quiet' mparams='CFLAGS="-O2 -g"'

# automatical regeneration is missing for other dirs, so do it explicitly
rpm-regenerate:
	(cd rpm && $(MAKE) $(AM_MAKEFLAGS) Makefile *.spec) || exit 1

rpm-dist: dist rpm-this

rpm-this: rpm-regenerate
	(cd rpm && $(MAKE) $(AM_MAKEFLAGS) this) || exit 1

## -----------------------------------------------------------------
## Produce a deb package using dist or from the given tarball
## (requires fakeroot, dpkg and debhelper in your system)
#  Usage:
#    make deb-dist
#    make release=1 deb-dist
#    make deb-this
#    make version=2.3.22 release=2 deb-this
#    make deb-dist cparams='--enable-gnome --quiet' mparams='CFLAGS="-O2 -g"'
#    make deb-inplace release=1.mg fullname=migo # works on sources directly

deb-regenerate:
	(cd debian && $(MAKE) $(AM_MAKEFLAGS) Makefile control) || exit 1

deb-dist: dist deb-this

deb-this: deb-regenerate
	$(MAKE) -f debian/Makefile $(AM_MAKEFLAGS) this || exit 1

deb-inplace: deb-regenerate
	$(MAKE) -f debian/Makefile $(AM_MAKEFLAGS) inplace || exit 1
