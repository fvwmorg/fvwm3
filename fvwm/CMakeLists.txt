# Declare the minimum required CMake version
# Name the project
project(fvwm3)

#Remove header from SRC_FILES regex:
#' src/[^\n ]*\.h'
# Define the build target for the executable
add_executable(${PROJECT_NAME} menus.c style.c borders.c events.c move_resize.c builtins.c
	add_window.c icons.c fvwm3.c frame.c placement.c virtual.c
	menustyle.c conditional.c ewmh.c stack.c session.c colorset.c
	functions.c ewmh_events.c geometry.c module_list.c focus.c
	windowlist.c functable.c menuitem.c expand.c module_interface.c
	menubindings.c decorations.c ewmh_icons.c update.c bindings.c misc.c
	cursor.c colormaps.c modconf.c  ewmh_conf.c read.c schedule.c
	menucmd.c ewmh_names.c icccm2.c windowshade.c focus_policy.c
	execcontext.c menugeometry.c menudim.c condrc.c infostore.c
	cmdparser_old.c functable_complex.c
)

target_link_libraries(${PROJECT_NAME} libfvwm3 )

#$(Xft_LIBS) $(X_LIBS) $(xpm_LIBS) \
#	$(X_PRE_LIBS) -lXext -lX11 \
#	$(X_EXTRA_LIBS) -lm $(iconv_LIBS) $(Xrender_LIBS) $(Xcursor_LIBS) \
#	$(Bidi_LIBS) $(png_LIBS) $(rsvg_LIBS) $(intl_LIBS) $(XRandR_LIBS)

#Build file
#/usr/bin/gcc -DHAVE_CONFIG_H -I. -I.. -I.. -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-4 -pthread -I/usr/include/libpng16 -I/usr/include/librsvg-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-4 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/cairo -I/usr/include/freetype2 -I/usr/include/harfbuzz -I/usr/include/pixman-1 -pthread -Wall -Wno-implicit-int -fno-common -Wno-error=format-security -g -O2 -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-4 -pthread -MT gravity.o -MD -MP -MF .deps/gravity.Tpo -c -o gravity.o gravity.c

#Link
#/usr/bin/gcc '-DFVWM_MODULEDIR="/usr/local/libexec/fvwm3/1.0.9"' '-DFVWM_DATADIR="/usr/local/share/fvwm3"' '-DFVWM_CONFDIR="/usr/local/etc"' -DFVWM_COLORSET_PRIVATE=1 '-DLOCALEDIR="/usr/local/share/fvwm3/locale"' -Wall -Wno-implicit-int -fno-common -Wno-error=format-security -g -O2 -I/usr/include/freetype2 -I/usr/include/libpng16 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-6 -pthread -o fvwm3 menus.o style.o borders.o events.o move_resize.o builtins.o add_window.o icons.o fvwm3.o frame.o placement.o virtual.o menustyle.o conditional.o ewmh.o stack.o session.o colorset.o functions.o ewmh_events.o geometry.o module_list.o focus.o windowlist.o functable.o menuitem.o expand.o module_interface.o menubindings.o decorations.o ewmh_icons.o update.o bindings.o misc.o cursor.o colormaps.o modconf.o ewmh_conf.o read.o schedule.o menucmd.o ewmh_names.o icccm2.o windowshade.o focus_policy.o execcontext.o menugeometry.o menudim.o condrc.o infostore.o cmdparser_old.o functable_complex.o -L../libs -lfvwm3 -lXft -lXpm -lSM -lICE -lXext -lX11 -lm -lXrender -lXcursor -lfribidi -lpng16 -lrsvg-2 -lm -lgio-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lcairo -lpng16 -lXrandr -lfontconfig -lfreetype
