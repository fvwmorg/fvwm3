## This is a -*- Makefile -*-
## Process this file with automake to create Makefile.in

EXTRA_DIST = conffiles copyright dirs docs postinst postrm prerm \
	fvwm.menu fvwm.menu-method

version = @VERSION@
release = 0.`date +%Y%m%d`
cparams = --without-gnome --without-rplay-library
mparams = CFLAGS="-O2"
datum = `date "+%a, %d %b %Y %T %z"`
fullname = FVWM Workers
email = fvwm-workers@fvwm.org
arch = `dpkg-architecture -qDEB_BUILD_ARCH`
distdir = $(PACKAGE)-$(version)
tarball = $(distdir).tar.gz
instdir = inst-$(version)
fullinstdir = `cd $(top_srcdir) && pwd`/$(instdir)

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
	LANG=en_UK LC_ALL=en_UK echo " -- $(fullname) <$(email)>  $(datum)" >> debian/changelog
	./configure --prefix=/usr --mandir=/usr/share/man --libexecdir=/usr/lib --sysconfdir=/etc/X11/fvwm $(cparams)
	fakeroot dh_testdir -P$(fullinstdir)
	fakeroot dh_testroot -P$(fullinstdir)
	fakeroot dh_clean -P$(fullinstdir)
	(cd .. && dpkg-source -b $(CURDIR))
	fakeroot dh_testdir -P$(fullinstdir)
	fakeroot dh_testroot -P$(fullinstdir)
	$(MAKE) $(mparams)
	$(MAKE) DESTDIR=$(fullinstdir) install
	-mkdir $(fullinstdir)/etc
	-mkdir $(fullinstdir)/etc/menu-methods
	cp debian/fvwm.menu-method $(fullinstdir)/etc/menu-methods/fvwm
	-mkdir $(fullinstdir)/usr/lib/menu
	cp debian/fvwm.menu $(fullinstdir)/usr/lib/menu/fvwm
	fakeroot dh_installchangelogs -P$(fullinstdir)
	fakeroot dh_installdocs -P$(fullinstdir)
	fakeroot dh_installmenu -P$(fullinstdir)
	fakeroot dh_link -P$(fullinstdir)
	fakeroot dh_strip -P$(fullinstdir)
	fakeroot dh_compress -P$(fullinstdir)
	fakeroot dh_fixperms -P$(fullinstdir)
	fakeroot dh_perl -P$(fullinstdir)
	fakeroot dh_installdeb -P$(fullinstdir)
	fakeroot dh_shlibdeps -P$(fullinstdir)
	fakeroot dh_gencontrol -P$(fullinstdir)
	fakeroot dh_md5sums -P$(fullinstdir)
	fakeroot dh_builddeb -P$(fullinstdir)
	-@dpkg-genchanges > ../fvwm_$(version)-$(release)_$(arch).changes
	-@debsign ../fvwm_$(version)-$(release)_$(arch).changes
	fakeroot dh_clean -P$(fullinstdir)
	-rm -rf $(fullinstdir)
	(cd .. && ls fvwm_$(version)-$(release)*)
