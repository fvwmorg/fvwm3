
# $Id$

docdir=@FVWM_DOCDIR@

if FVWM_BUILD_HTMLDOC
doc_DATA=index.html allCommands.html groupedCommands.html style.css
EXTRA_DIST=fvwm.xsl fvwm-man.xsl header.html scott.notes $(doc_DATA)
EXTRA_DIST+=$(shell find $(srcdir)/docbook-x?l -type f)
endif

SUBDIRS=fvwm commands images modules

# [ `pwd` != $(srcdir) ] && ...
all:
	@for f in $(doc_DATA) ; do \
		[ ! -f $$f ] && cp -p $(srcdir)/$$f $$f; \
	done ; true
