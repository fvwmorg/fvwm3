## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

EXTRA_DIST = @PACKAGE@.spec

version = @VERSION@
release = 0.`date +%Y%m%d`
cparams = --without-gnome --without-rplay-library --sysconfdir=/etc
mparams = CFLAGS="-O2"

rpm: @PACKAGE@.spec
	@\
	version=$(version); \
	release=$(release); \
	cparams='$(cparams)'; \
	mparams='$(mparams)'; \
	tarball=@PACKAGE@-$$version.tar.gz; \
	tarball_full=`cd $(top_srcdir) && pwd`/$$tarball; \
	if [ ! -f $$tarball_full ]; then echo "No $$tarball"; exit -1; fi; \
	ln -sf $$tarball_full /tmp/$$tarball || exit -1; \
	echo ""; \
	echo "==== Creating rpm from /tmp/$$tarball, release $$release ===="; \
	echo ""; \
	sleep 3; \
	sed < @PACKAGE@.spec > tmp-@PACKAGE@.spec \
		-e "s|%define ver |&$$version  %define old_ver |" \
		-e "s|%define rel |&$$release  %define old_rel |" \
		-e "s|%define c_p |&$$cparams  %define old_c_p |" \
		-e "s|%define m_p |&$$mparams  %define old_m_p |" \
	; \
	rpmbuild -ba tmp-@PACKAGE@.spec --clean; \
	rm /tmp/$$tarball; \
	rm tmp-@PACKAGE@.spec
