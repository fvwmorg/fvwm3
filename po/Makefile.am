
localedir = @LOCALEDIR@

FVWM_POT_FILES = \
     ../fvwm/fvwm.c \
     ../fvwm/expand.c \
     ../fvwm/windowlist.c \
     ../fvwm/virtual.c

FVWMRC_POT_FILES = \
     ../fvwm/ConfigFvwmSetup \
     ../modules/FvwmForm/FvwmForm-Setup.in

FVWMTASKBAR_POT_FILES = \
	../modules/FvwmTaskBar/Goodies.c \
	../modules/FvwmTaskBar/FvwmTaskBar.c

EXTRA_DIST = \
	@POFILES@ @GMOFILES@ fvwm.pot FvwmTaskBar.pot remove-potcdate.sed \
	remove-potcdate.sin

COPYRIGHT_HOLDER = fvwm workers

# These options get passed to xgettext.
XGETTEXT_OPTIONS = --keyword=_ --keyword=N_

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

MSGMERGE_UPDATE = @MSGMERGE@ --update

SUFFIXES = .po .gmo .mo .sed .sin .nop .po-update

.po.mo:
	@echo "$(MSGFMT) -c -o $@ $<"; \
	$(MSGFMT) -c -o t-$@ $< && mv t-$@ $@

.po.gmo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics -o $${lang}.gmo $${lang}.po"; \
	cd $(srcdir) && rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics -o t-$${lang}.gmo $${lang}.po && mv t-$${lang}.gmo $${lang}.gmo

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all-local: all-local-@USE_NLS@

# make [all] should do nothing ...
#all-local-yes: $(CATALOGS)
all-local-yes:
all-local-no:

catalogs: $(CATALOGS)

maintainer-clean-local:
	rm -f $(GMOFILES)

$(FVWM_DOMAIN).pot-update: $(FVWM_POT_FILES) \
	$(FVWMRC_POT_FILES) \
	remove-potcdate.sed
	rm -f fvwmrc.pot; \
	rcpotfiles='$(FVWMRC_POT_FILES)'; \
	rm -f duplicate; \
	for file in $$rcpotfiles; do \
	  perl -ne 's/\[gt\.((\\.|.)+?)\]/ print \
	  "\#$$ARGV: line $$.\n"."msgid \"$$1\"\n"."msgstr \"\"\n\n"/ge' \
	  $$file >> duplicate; \
	done; \
	msguniq duplicate > fvwmrc.pot;
	rm -f duplicate; \
	potfiles='$(FVWM_POT_FILES)'; \
	$(XGETTEXT) \
	  --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS) -o $(FVWM_DOMAIN).po \
	  --copyright-holder='$(COPYRIGHT_HOLDER)' $$potfiles; \
	test ! -f $(FVWM_DOMAIN).po || { \
	  if test -f $(srcdir)/$(FVWM_DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(FVWM_DOMAIN).pot > $(FVWM_DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(FVWM_DOMAIN).po > $(FVWM_DOMAIN).2po && \
	    if cmp $(FVWM_DOMAIN).1po $(FVWM_DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(FVWM_DOMAIN).1po $(FVWM_DOMAIN).2po $(FVWM_DOMAIN).po; \
	    else \
	      rm -f $(FVWM_DOMAIN).1po $(FVWM_DOMAIN).2po $(srcdir)/$(FVWM_DOMAIN).pot && \
	      mv $(FVWM_DOMAIN).po $(srcdir)/$(FVWM_DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(FVWM_DOMAIN).po $(srcdir)/$(FVWM_DOMAIN).pot; \
	  fi; \
	  mv $(FVWM_DOMAIN).pot duplicate.pot; \
	  cat fvwmrc.pot >> duplicate.pot; \
	  msguniq duplicate.pot  > $(FVWM_DOMAIN).pot; \
	  rm -f duplicate.pot fvwmrc.pot; \
	}

$(srcdir)/$(FVWM_DOMAIN).pot:
	$(MAKE) $(FVWM_DOMAIN).pot-update

$(FVWMTASKBAR_DOMAIN).pot-update: $(FVWMTASKBAR_POT_FILES) \
	remove-potcdate.sed
	potfiles='$(FVWMTASKBAR_POT_FILES)'; \
	$(XGETTEXT) \
	  --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS) \
	  -o $(FVWMTASKBAR_DOMAIN).po \
	  --copyright-holder='$(COPYRIGHT_HOLDER)' $$potfiles; \
	test ! -f $(FVWMTASKBAR_DOMAIN).po || { \
	  if test -f $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot \
	    > $(FVWMTASKBAR_DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(FVWMTASKBAR_DOMAIN).po \
	    > $(FVWMTASKBAR_DOMAIN).2po && \
	    if cmp $(FVWMTASKBAR_DOMAIN).1po $(FVWMTASKBAR_DOMAIN).2po \
	    >/dev/null 2>&1; then \
	      rm -f $(FVWMTASKBAR_DOMAIN).1po $(FVWMTASKBAR_DOMAIN).2po \
	      $(FVWMTASKBAR_DOMAIN).po; \
	    else \
	      rm -f $(FVWMTASKBAR_DOMAIN).1po $(FVWMTASKBAR_DOMAIN).2po \
	      $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot && \
	      mv $(FVWMTASKBAR_DOMAIN).po $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(FVWMTASKBAR_DOMAIN).po $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot; \
	  fi; \
	}

$(srcdir)/$(FVWMTASKBAR_DOMAIN).pot:
	$(MAKE) $(FVWMTASKBAR_DOMAIN).pot-update

$(POFILES): $(srcdir)/$(FVWM_DOMAIN).pot $(srcdir)/$(FVWMTASKBAR_DOMAIN).pot
	dlang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	dom=`echo $$dlang | sed -e 's/\..*$$//'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE_UPDATE) $${dlang}.po $${dom}.pot"; \
	cd $(srcdir) && $(MSGMERGE_UPDATE) $${dlang}.po $${dom}.pot

#	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
#	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
#	echo "$${cdcmd}$(MSGMERGE_UPDATE) $${lang}.po $(FVWM_DOMAIN).pot"; \
#	cd $(srcdir) && $(MSGMERGE_UPDATE) $${lang}.po $(FVWM_DOMAIN).pot

install-data-local: install-data-local-@USE_NLS@
install-data-local-no: all
install-data-local-yes: all
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	all_domains='$(ALL_DOMAINS)'; \
	for dom in $$all_domains; do \
	  for cat in $$catalogs; do \
	    cat=`basename $$cat`; \
	    lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	    cat="$$dom.$$cat"; \
	    dir=$(localedir)/$$lang/LC_MESSAGES; \
	    $(mkinstalldirs) $(DESTDIR)$$dir; \
	    if test -r $$cat; \
	      then realcat=$$cat; \
	    else realcat=$(srcdir)/$$cat; \
	    fi; \
	    $(INSTALL_DATA) $$realcat $(DESTDIR)$$dir/$$dom.mo; \
	    echo "installing $$realcat as $(DESTDIR)$$dir/$$dom.mo"; \
	    for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	      if test -n "$$lc"; then \
	        if (cd $(DESTDIR)$(localedir)/$$lang && \
	            LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' \
                    >/dev/null; then \
	          link=`cd $(DESTDIR)$(localedir)/$$lang && \
                       LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	          mv $(DESTDIR)$(localedir)/$$lang/$$lc \
                     $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	           for file in *; do \
	             if test -f $$file; then \
	               ln -s ../$$link/$$file \
                             $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	             fi; \
	           done); \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        else \
	          if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	            :; \
	          else \
	            rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	            mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          fi; \
	        fi; \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo; \
	        ln -s ../LC_MESSAGES/$$dom.mo \
	           $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo 2>/dev/null || \
	        ln $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$$dom.mo \
	           $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo 2>/dev/null || \
	        cp -p $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$$dom.mo \
	           $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo; \
	        echo "installing $$realcat link as \
	        $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo"; \
	      fi; \
	    done; \
	  done; \
	done


installdirs-data: installdirs-data-local-@USE_NLS@
installdirs-data-no:
installdirs-data-local-yes:
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	@all_domain='$(ALL_DOMAINS)' \
	for dom in $$all_domains; do \
	  for cat in $$catalogs; do \
	    cat=`basename $$cat`; \
	    lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	    dir=$(localedir)/$$lang/LC_MESSAGES; \
	    $(mkinstalldirs) $(DESTDIR)$$dir; \
	    for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	      if test -n "$$lc"; then \
	        if (cd $(DESTDIR)$(localedir)/$$lang && \
                    LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' \
                    >/dev/null; then \
	          link=`cd $(DESTDIR)$(localedir)/$$lang && \
                        LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	          mv $(DESTDIR)$(localedir)/$$lang/$$lc \
                     $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	           for file in *; do \
	             if test -f $$file; then \
	               ln -s ../$$link/$$file \
                             $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	             fi; \
	           done); \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        else \
	          if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	            :; \
	          else \
	            rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	            mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          fi; \
	        fi; \
	      fi; \
	    done; \
	  done; \
	done


uninstall-local: uninstall-data-local-@USE_NLS@
uninstall-local-no:
uninstall-local-yes:
	catalogs='$(CATALOGS)'; \
	@all_domain='$(ALL_DOMAIN)'
	for dom in $$all_domain; do
	  for cat in $$catalogs; do \
	    cat=`basename $$cat`; \
	    lang=`echo $@ | sed -e 's/\.gmo$$//'`; \
	    for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo;
	    done; \
	  done; \
	done

update-po: Makefile
	$(MAKE) $(FVWM_DOMAIN).pot-update
	$(MAKE) $(FVWMTASKBAR_DOMAIN).pot-update
	$(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-gmo

# General rule for updating PO files.

.nop.po-update:
	dlang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po-update$$//'`; \
	dom=`echo $$dlang | sed -e 's/\..*$$//'`; \
	tmpdir=`pwd`; \
	echo "$$dlang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $$dlang.po $$dom.pot -o $$dlang.new.po"; \
	cd $(srcdir); \
	if $(MSGMERGE) $$dlang.po $$dom.pot -o $$tmpdir/$$dlang.new.po; then \
	  if cmp $$dlang.po $$tmpdir/$$dlang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$dlang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$dlang.new.po $$dlang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$dlang.po failed: cannot move \
	      $$tmpdir/$$dlang.new.po to $$dlang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$dlang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$dlang.new.po; \
	fi

$(DUMMYPOFILES):

update-gmo: Makefile $(GMOFILES)
	@:
