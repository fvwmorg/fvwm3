## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

EXTRA_DIST = docs dirs postinst postrm fvwm.menu fvwm.menu-method

version = @VERSION@
release = 0.`date +%Y%m%d`
cparams = --without-gnome --without-rplay-library
mparams = CFLAGS="-O2"
datum = `date "+%a, %d %b %Y %T %z"`
fullname = FVWM Workers
email = fvwm-workers@fvwm.org
arch ?= $(shell dpkg-architecture -qDEB_BUILD_ARCH)
distdir = $(PACKAGE)-$(version)
tarball = $(distdir).tar.gz
instdir = inst-$(version)

this:
	@if [ ! -d debian ]; then echo "Wrong working dir `pwd`"; exit -1; fi
	@if [ ! -f $(tarball) ]; then echo "No $(tarball)"; exit -1; fi
	-rm -rf $(distdir)
	$(AMTAR) -zxf $(tarball)
	@cd $(distdir); \
	if [ ! -d debian ]; then \
		cp -r $(top_srcdir)/debian ./; \
		echo "deb-inplace:" >>Makefile.in; \
		echo '	$$==(MAKE) -f debian/Makefile $$==(AM_MAKEFLAGS) inplace' | tr -d = >>Makefile.in; \
		ls `cat debian/docs` >debian/docs-tmp 2>/dev/null && \
		rm -f debian/docs-tmp || mv debian/docs-tmp debian/docs; \
	fi; \
	echo ""; \
	echo "==== Creating deb from $(tarball), release $(release) ===="; \
	echo ""; \
	sleep 3; \
	./configure $(cparams); \
	$(MAKE) $(AM_MAKEFLAGS) deb-inplace
	-rm -rf $(distdir)

inplace:
	@if [ ! -d debian ]; then echo "Wrong working dir `pwd`"; exit -1; fi
	$(MAKE) clean
	echo "fvwm ($(version)-$(release)) unstable; urgency=low" > debian/changelog
	echo "" >> debian/changelog
	echo "  * new upstream release" >> debian/changelog
	echo "" >> debian/changelog
	echo " -- $(fullname) <$(email)>  $(datum)" >> debian/changelog
	./configure --prefix=/usr --mandir=/usr/share/man --libexecdir=/usr/lib --sysconfdir=/etc/X11/fvwm $(cparams)
	fakeroot dh_testdir
	fakeroot dh_testroot
	fakeroot dh_clean
	(cd .. && dpkg-source -b $(CURDIR))
	fakeroot dh_testdir
	fakeroot dh_testroot
	$(MAKE) $(mparams)
	$(MAKE) DESTDIR=`cd $(top_srcdir) && pwd`/$(instdir) install
	-mkdir $(top_srcdir)/$(instdir)/etc
	-mkdir $(top_srcdir)/$(instdir)/etc/menu-methods
	cp debian/fvwm.menu-method $(top_srcdir)/$(instdir)/etc/menu-methods/fvwm
	-mkdir $(top_srcdir)/$(instdir)/usr/lib/menu
	cp debian/fvwm.menu $(top_srcdir)/$(instdir)/usr/lib/menu/fvwm
	fakeroot dh_installchangelogs
	fakeroot dh_installdocs
	fakeroot dh_installmenu
	fakeroot dh_link
	fakeroot dh_strip
	fakeroot dh_compress
	fakeroot dh_fixperms
	fakeroot dh_perl
	fakeroot dh_installdeb
	fakeroot dh_shlibdeps
	fakeroot dh_gencontrol
	fakeroot dh_md5sums
	fakeroot dh_builddeb
	-@dpkg-genchanges > ../fvwm_$(version)-$(release)_$(arch).changes
	-@debsign ../fvwm_$(version)-$(release)_$(arch).changes
	fakeroot dh_clean
	-rm -rf $(top_srcdir)/$(instdir)
	(cd .. && ls fvwm_$(version)-$(release)*)
