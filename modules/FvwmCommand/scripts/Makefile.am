## Process this file with automake to create Makefile.in

## The script "focus-link" has a manpage, so perhaps it is intended to be
## installed?  (it is not installed currently)

PERL = @PERL@

noinst_SCRIPTS = ex-auto ex-cascade ex-grpmv focus-Netscape	\
	focus-link push-away test1

EXTRA_DIST = ex-auto.pl ex-cascade.pl ex-grpmv.pl focus-Netscape.pl	\
	focus-link.pl push-away.pl test1.pl focus-link.1 test2.sh

SUFFIXES = .pl
.pl:
	rm -f $@
	sed -e 's,xPERLx,$(PERL),' $< > $@-t
	mv $@-t $@
	chmod +x $@
