## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

EXTRA_DIST = fvwm.spec

version = @VERSION@
release = 0.`date +%Y%m%d`

rpm: fvwm.spec
	@\
	version=$(version); \
	release=$(release); \
	tarball=fvwm-$$version.tar.gz; \
	tarball_full=`cd $(top_srcdir) && pwd`/$$tarball; \
	rh_srcdir=$$HOME/redhat/SOURCES; \
	if [ ! -f $$tarball_full ]; then echo "No $$tarball"; exit -1; fi; \
	echo ""; \
	echo "==== Creating rpm from $$tarball, release $$release ===="; \
	echo ""; \
	if [ ! -d $$rh_srcdir ]; then mkdir -p $$rh_srcdir; fi; \
	ln -sf $$tarball_full $$rh_srcdir/; \
	sed < fvwm.spec > tmp-fvwm.spec \
		-e "s/%define ver /&$$version  %define old_ver /" \
		-e "s/%define rel /&$$release  %define old_rel /"
	rpm -bb tmp-fvwm.spec
	rm tmp-fvwm.spec
