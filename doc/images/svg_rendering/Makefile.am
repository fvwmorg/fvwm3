
# $Id$

# Author: Scott Smedley

docdir=@FVWM_DOCDIR@/images/svg_rendering
EXTRA_DIST=affine_combined.png all_combined.png plain.png position.png prevent_cropping.png rotation.png scale_down.png scale_down_uniform.png scale_down_y.png scale_mirroring.png scale_up_uniform.png size_keep_ratio_x.png size_keep_ratio_y.png size_mirroring.png size_mirroring_y.png size.png transpose.png


if FVWM_BUILD_HTMLDOC
doc_DATA=$(EXTRA_DIST)
endif

all: $(doc_DATA)

%.png: $(srcdir)/$@
	@ cp $(srcdir)/$@ $@

