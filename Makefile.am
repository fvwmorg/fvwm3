## -*- Makefile -*- mode
## Process this file with automake to create Makefile.in

## Make sure we have a current version of automake
AUTOMAKE_OPTIONS = 1.4

SUBDIRS = libs \
	fvwm \
	modules \
	bin \
	utils \
	$(FVWM_PERLLIB) \
	docs \
	rpm \
	sample.fvwmrc \
	$(POSUB) \
	tests

EXTRA_DIST = config.rpath  \
	INSTALL.fvwm \
	vms/README \
	vms/config.h \
	vms/fvwmrc.dat \
	vms/make_fvwm.mms \
	vms/vms.c \
	vms/vms.h \
	vms/vms_shareables.opt

## ---------------------------------------------------------------------------
## Manage bzip2 archive together with gzip archive
#  Usage:
#    make dist2       # instead of make dist
#    make distcheck2  # instead of make distcheck
#    make distclean2  # instead of make distclean

dist2: dist
	gzip -dc $(distdir).tar.gz | bzip2 -9 >$(distdir).tar.bz2

distcheck2: distcheck dist2
	@banner="$(distdir).tar.bz2 - ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$banner"; \
	echo "$$dashes"
	@if test "x$(ISRELEASED)" != xyes; then \
		echo 'Warning: ISRELEASED is not set to "yes".'; \
		echo '	So these can not be the official tarballs.'; \
	fi

distclean2: distclean

## ---------------------------------------------------------------------------
## Produce an rpm package using dist or from the given tarball
#  Usage:
#    make rpm-dist
#    make release=1 rpm-dist
#    make rpm-this
#    make version=2.3.22 release=2 rpm-this
#    make rpm-dist cparams='--enable-multibyte --quiet' mparams='CFLAGS="-O2 -g"'

rpm-dist: dist rpm-this

rpm-this:
	(cd rpm && $(MAKE) $(AM_MAKEFLAGS) rpm) || exit 1

