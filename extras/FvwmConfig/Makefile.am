## Process this file with automake to create Makefile.in

## Notes: this needs both Xpm and C++

moduledir = @FVWM_MODULEDIR@
module_PROGRAMS = FvwmConfig

FvwmConfig_SOURCES = FvwmConfig.C WinBase.C WinBase.h WinButton.C \
	WinButton.h WinColorSelector.C WinColorSelector.h WinInput.C \
	WinInput.h WinRadioButton.C WinRadioButton.h WinSlider.C \
	WinSlider.h WinText.C WinText.h dead.c

FvwmConfig_DEPENDENCIES = $(top_builddir)/libs/libfvwm.a

man_MANS = FvwmConfig.1
EXTRA_DIST = $(man_MANS)

LDADD = -L$(top_builddir)/libs -lfvwm $(xpm_LIBS) \
	$(X_LIBS) $(X_PRE_LIBS) -lXext -lX11 $(X_EXTRA_LIBS)

INCLUDES = -I$(top_srcdir)/libs $(xpm_CFLAGS) $(X_CFLAGS)

