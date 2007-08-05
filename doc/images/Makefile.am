# Author: Scott Smedley

docdir = @FVWM_DOCDIR@/images
EXTRA_DIST = fvwm.logo.png fvwm.logo2.png m.logo.png

if FVWM_BUILD_HTMLDOC
doc_DATA = $(EXTRA_DIST)
endif

SUBDIRS = svg_rendering

all: $(doc_DATA)
