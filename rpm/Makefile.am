## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

EXTRA_DIST = @PACKAGE@.spec

version = @VERSION@
release = 0.`date +%Y%m%d`

rpm: @PACKAGE@.spec
	@\
	version=$(version); \
	release=$(release); \
	tarball=@PACKAGE@-$$version.tar.gz; \
	tarball_full=`cd $(top_srcdir) && pwd`/$$tarball; \
        rpm_ver=`rpm --version | cut -d' ' -f3`; \
	rh_srcdir=`awk 'BEGIN { print ('$$rpm_ver' >= 4.0? \
		ENVIRON["HOME"]: "/usr/src") "/redhat/SOURCES" }'`; \
	if [ ! -f $$tarball_full ]; then echo "No $$tarball"; exit -1; fi; \
	echo ""; \
	echo "==== Creating rpm from $$tarball, release $$release ===="; \
	echo ""; \
	if [ ! -d $$rh_srcdir ]; then mkdir -p $$rh_srcdir; fi; \
	ln -sf $$tarball_full $$rh_srcdir || exit -1; \
	sed < @PACKAGE@.spec > tmp-@PACKAGE@.spec \
		-e "s/%define ver /&$$version  %define old_ver /" \
		-e "s/%define rel /&$$release  %define old_rel /"
	rpm -bb tmp-@PACKAGE@.spec
	rm tmp-@PACKAGE@.spec
