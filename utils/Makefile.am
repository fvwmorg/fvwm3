## Process this file with automake to create Makefile.in

noinst_PROGRAMS = @XPMREDUCE@
EXTRA_PROGRAMS = xpm-reduce

EXTRA_DIST = BuildXLockMenu fvwmrc_convert quantize_pixmaps \
	xpm-reduced-rgb.txt

LDADD = $(xpm_LIBS) $(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)
INCLUDES = $(X_CFLAGS) $(xpm_CFLAGS)
