docdir=@FVWM_DOCDIR@

HTML_FILES=index.html allCommands.html groupedCommands.html modules.html style.css
EXTRA_DIST=fvwm.xsl fvwm-man.xsl header.html $(HTML_FILES)
EXTRA_DIST+=$(shell find $(srcdir)/docbook-x?l -type f)

if FVWM_BUILD_HTMLDOC
doc_DATA=$(HTML_FILES)
endif


SUBDIRS=fvwm commands images modules

# [ `pwd` != `cd $(srcdir) && pwd` ] && ...
all:
	@for f in $(doc_DATA) ; do \
		[ ! -f $$f ] && cp -p $(srcdir)/$$f $$f; \
	done ; true

# This isn't portable (ie. doesn't work with all versions of make)
#all: $(doc_DATA)
#
#%.html: $(srcdir)/$@
#	@ cp $(srcdir)/$@ $@
