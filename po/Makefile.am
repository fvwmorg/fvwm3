
localedir = @LOCALEDIR@

EXTRA_DIST = \
	@POFILES@ @GMOFILES@ fvwm3.pot meson.build

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

all-local: all-local-@USE_NLS@

# make [all] should do nothing ...
all-local-yes:
all-local-no:

catalogs: $(CATALOGS)

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
	    if test -f $$realcat; then \
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
	    fi; \
	  done; \
	done

installdirs-data: installdirs-data-local-@USE_NLS@
installdirs-data-local-no:
installdirs-data-local-yes:
	$(mkinstalldirs) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	all_domains='$(ALL_DOMAINS)'; \
	for dom in $$all_domainss; do \
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

uninstall-local: uninstall-local-@USE_NLS@
uninstall-local-no:
uninstall-local-yes:
	catalogs='$(CATALOGS)'; \
	all_domains='$(ALL_DOMAINS)'; \
	for dom in $$all_domains; do \
	  for cat in $$catalogs; do \
	    cat=`basename $$cat`; \
	    lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	    for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$$dom.mo; \
	    done; \
	  done; \
	done
	if test -d @FVWM_DATADIR@"/locale"; then rm -fr @FVWM_DATADIR@"/locale"; fi

