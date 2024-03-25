cmake_minimum_required(VERSION 3.10)
project(fvwm3)
enable_language(C)

#User options
set(FVWM_DATADIR "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Read-only architecture-independent data [CMAKE_INSTALL_PREFIX/share]")

#Fill in config.h
#configure_file(config.h.in config.h)

#Sets
set(CMAKE_C_STANDARD 11)

#We need some libraries that CMake doesn't have prebuilt 'find_package()'es for, so we use pkg-config for those libraries.
find_package(PkgConfig REQUIRED)

#Libraries that are commented out don't seem to be needed
#Required libraries
find_package(GTK2 REQUIRED gtk #[[gobject gio glib]])
find_package(X11 REQUIRED Xrender Xrandr Xft #[[Xt]] OPTIONAL_COMPONENTS Xpm Xcursor Xext ICE SM #[[Xi]])
#libevent-dev (>= 2.0)

#Optional libraries
#ADD: Make these configurable and make it error if these are configured on and not found.
find_package(Freetype 2.0.0)
find_package(Fontconfig)
find_package(PNG)
pkg_search_module(RSVG IMPORTED_TARGET REQUIRED librsvg-2.0)
pkg_search_module(FRIBIDI IMPORTED_TARGET REQUIRED fribidi)
#asciidoctor
#libncurses5-dev
#libreadline-dev
#sharutils

#message(${X11_LIBRARIES})

#FIX: This is bad. Use pkg-config or something else to check for and add these
link_libraries(X11::Xt X11::Xft X11::Xpm X11::Xext X11::Xrender X11::Xcursor X11::ICE X11::SM PkgConfig::FRIBIDI PNG::PNG PkgConfig::RSVG GTK2::gdk_pixbuf GTK2::cairo X11::Xrandr Fontconfig::Fontconfig Freetype::Freetype)
#link_libraries(-lXft -lXpm X11 X11::Xrender X11::Xcursor -lfribidi -lpng16 -lrsvg-2 -lm -lgio-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lglib-2.0 -lcairo X11::Xrandr -lfontconfig Freetype::Freetype)
include_directories("${CMAKE_SOURCE_DIR}" "/usr/include/freetype2" "/usr/include/libpng16" "/usr/include/harfbuzz" "/usr/include/glib-2.0" "/usr/lib/glib-2.0/include" "/usr/include/sysprof-4" "/usr/include/librsvg-2.0" "/usr/include/libmount" "/usr/include/blkid" "/usr/include/gdk-pixbuf-2.0" "/usr/include/cairo" PRIVATE "/usr/include/pixman-1")

add_subdirectory(${CMAKE_SOURCE_DIR}/libs)

#Link libfvwm3 to everything else
link_libraries(libfvwm3)
add_subdirectory(${CMAKE_SOURCE_DIR}/fvwm)
#add_subdirectory(${CMAKE_SOURCE_DIR}/modules)
