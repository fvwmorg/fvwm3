dnl Process this file with autoconf to produce a configure script.
dnl
AC_PREREQ(2.60)

dnl should be "yes" only within the released distribution
ISRELEASED="no"
if test $ISRELEASED = "no"; then
	RELDATELONG=""
	RELDATESHORT=""
	RELDATENUM=""
fi

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

AC_INIT([fvwm],
	m4_esyscmd_s([utils/fvwm-version-str.sh]),
	[fvwm-workers@fvwm.org])
AC_CONFIG_AUX_DIR(etc)
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
AM_CONFIG_HEADER(config.h)

VERSIONINFO=""

dnl date of the released version (please zero pad the day in the last 2 dates)
dnl for example: "4 February 2003", "04 Feb 2003", "2003-02-04"
dnl date format strings: "%e %B %Y", "%d-%b-%Y", "%Y-%m-%d"
RELDATELONG="06 November 2016"
RELDATESHORT="06-Nov-2016"
RELDATENUM="2016-11-06"

# constant variable settings
FVWMNAMELONG="F? Virtual Window Manager"
FVWMHOMEPAGE="http://fvwm.org/"
FVWMFTP="ftp.fvwm.org"
FVWMFTPDIR="/pub/ftp"
FVWMALTFTP="metalab.unc.edu"
FVWMALTFTPDIR="/pub/Linux/X11/window-managers/"
FVWMLIST="fvwm@fvwm.org"
FVWMWORKERSLIST="fvwm-workers@fvwm.org"
FVWMWORKERSLISTLONG="fvwm workers list <fvwm-workers@fvwm.org>"
MANPAGE_PREAMBLE='.\" WARNING: This file was automatically generated.  Edit the .in file instead.'

AC_SUBST(ISRELEASED)
AH_TEMPLATE([VERSIONINFO],[Additional version information, like date])
AC_DEFINE_UNQUOTED(VERSIONINFO, "$VERSIONINFO")
AC_SUBST(VERSIONINFO)
AC_SUBST(RELDATELONG)
AC_SUBST(RELDATESHORT)
AC_SUBST(RELDATENUM)
AC_SUBST(FVWMNAMELONG)
AC_SUBST(FVWMHOMEPAGE)
AC_SUBST(FVWMFTP)
AC_SUBST(FVWMFTPDIR)
AC_SUBST(FVWMALTFTP)
AC_SUBST(FVWMALTFTPDIR)
AC_SUBST(FVWMLIST)
AC_SUBST(FVWMWORKERSLIST)
AC_SUBST(FVWMWORKERSLISTLONG)
AC_SUBST(MANPAGE_PREAMBLE)

# check for programs needed to build html docs
AC_CHECK_PROG(SED, sed, sed, "")
AC_SUBST(SED)

# optional python:
AM_PATH_PYTHON([2.5],, [:])

#!!!
PERL=""
REQUIRED_PERL_VERSION=5.004
AC_SUBST(REQUIRED_PERL_VERSION)
if test x"$PERL" = x; then
	AC_PATH_PROG(PERL, perl)
fi
if test ! x"$PERL" = x; then
	AC_MSG_CHECKING([for perl $REQUIRED_PERL_VERSION or better])
	HAVE_PERL_FOR_DOCS=1
	if ("$PERL" -e "require $REQUIRED_PERL_VERSION;") 2>/dev/null; then
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
		AC_MSG_WARN([$PERL not found or too old])
	fi
else
	HAVE_PERL_FOR_DOCS=0
	AC_MSG_WARN([perl is not found, it is required for some scripts and modules])
	AC_MSG_WARN([It is recommended to install perl $REQUIRED_PERL_VERSION or better later])
	PERL=/usr/bin/perl
fi
AC_MSG_RESULT([assuming $PERL as perl location])
AC_SUBST(PERL)

# installation paths
FVWM_MODULESUBDIR=/${PACKAGE}/${VERSION}
FVWM_DATASUBDIR=/${PACKAGE}
FVWM_DOCSUBDIR=/doc/${PACKAGE}

AC_ARG_ENABLE(package-subdirs,
  AS_HELP_STRING([--disable-package-subdirs],
    [do not create subdirs for modules and data]),
  [if test x"$enableval" = xno; then FVWM_MODULESUBDIR=""; FVWM_DATASUBDIR="";
  fi], [])


FVWM_MODULEDIR='${libexecdir}'"$FVWM_MODULESUBDIR"
FVWM_DATADIR='${datadir}'"$FVWM_DATASUBDIR"
FVWM_DOCDIR='${datadir}'"$FVWM_DOCSUBDIR"
FVWM_PERLLIBDIR='${datadir}'"$FVWM_DATASUBDIR/perllib"
FVWM_CONFDIR='${sysconfdir}'dnl used _only_ to search for system.fvwm2rc

AC_SUBST(FVWM_MODULEDIR)
AC_SUBST(FVWM_DATADIR)
AC_SUBST(FVWM_PERLLIBDIR)
AC_SUBST(FVWM_CONFDIR)
AC_SUBST(FVWM_DOCDIR)

AH_TEMPLATE([FVWM_CONFIG],
  [Name of config filenames in FVWM_USERDIR and FVWM_DATADIR])
AC_DEFINE(FVWM_CONFIG, "config")

AH_TEMPLATE([FVWM2RC],[Suffix for old (to be deprecated) config filenames])
AC_DEFINE(FVWM2RC, ".fvwm2rc")

# Various configure-time options

AC_ARG_ENABLE(dmalloc,
  AS_HELP_STRING([--enable-dmalloc],
    [enable support for the dmalloc debugging library]),
  [ac_cv_dmalloc="$enableval"],
  [ac_cv_dmalloc="no"])

AC_ARG_ENABLE(efence,
  AS_HELP_STRING([--enable-efence],
    [enable support for the efence debugging library]),
  [ac_cv_efence="$enableval"],
  [ac_cv_efence="no"])

smr_SWITCH(command-log, command logging, off, FVWM_COMMAND_LOG,, [Produces a log of all executed commands and their times on stderr.])
AH_VERBATIM([_FVWM_COMMAND_LOG],
[#ifdef FVWM_COMMAND_LOG
#   define FVWM_DEBUG_TIME 1
#endif])

smr_SWITCH(debug-msgs, debugging messages, off, FVWM_DEBUG_MSGS,, [if you would like to see lots of debug messages from fvwm, for debugging
   purposes, uncomment the next line])
AH_VERBATIM([_FVWM_DEBUG_MSGS],
[#ifdef FVWM_DEBUG_MSGS
#   define DBUG(x,y) fvwm_msg(DBG,x,y)
#else
#   define DBUG(x,y) /* no messages */
#endif])

dnl dummy: smr_SWITCH(sm, dummy for test script, on, SESSION)

# Need to know where X is, for finding some libraries (e.g. xpm)
no_x=""
AC_PATH_XTRA
if test x"$no_x" = x"yes"; then
  echo
  echo "X11 libraries or header files could not be found.  Please make"
  echo "sure the X11 development package is installed on your system."
  echo "If it is definitely installed, try setting the include and library"
  echo "paths with the --x-include and --x-libraries options of configure."
  echo "Fvwm can not be compiled without the X11 development environment."
  echo
  echo "Aborting."
  echo
  exit 1
fi

# FIXME: default value should be derived from computed path to X
# includes.  Actually, this should probably not appear in configure
# at all: it is settable at runtime, and only confuses the issue to
# have it settable here too.
#
AC_MSG_CHECKING(imagepath)
val="/usr/include/X11/bitmaps:/usr/include/X11/pixmaps"
AC_ARG_WITH(imagepath,
  AS_HELP_STRING([--with-imagepath=PATH],
    [colon-delimited search path for images]),
[ case "$withval" in
  no)
    AC_MSG_ERROR([Can not disable image path.])
    ;;
  yes)
    ;;
  *)
    val="$withval"
    ;;
  esac ])
AH_TEMPLATE([FVWM_IMAGEPATH],[Where to search for images.])
AC_DEFINE_UNQUOTED(FVWM_IMAGEPATH, "$val")

FVWM_IMAGEPATH="$val"
AC_SUBST(FVWM_IMAGEPATH)
AC_MSG_RESULT($val)

# Minimal checks for programs: enough to enable checking for
# optional libraries.
AC_PROG_CC
AC_PROG_CPP

# added -Wall for gcc, what about for others?
if test "x$GCC" = "xyes"; then
  CFLAGS="-Wall -Wno-implicit-int $CFLAGS"
fi

# Help finding POSIX functions on some systems
AC_ISC_POSIX
AC_MINIX

# catch -Werror and similar options when running configure
AC_TRY_COMPILE([#include <stdio.h>],
[int i; static j; int *p; char *c;
  switch (*p = p = *c) { case 0: printf("%Q", c, p); }
  *c = &i; c = p;
  while (1 || (unsigned int)3 >= 0 || ((int)-1) == ((unsigned int)1));
], , AC_MSG_ERROR("
configure is not able to compile programs with warnings.  Please
remove all offending options like -Werror from the CFLAGS and
CPPFLAGS variables and run configure again."))

# check size of some types
ac_save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $X_CFLAGS"
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(Window, , [#include <stdio.h>
#include <X11/X.h>])
AC_CHECK_SIZEOF(Pixel, , [#include <stdio.h>
#include <X11/Intrinsic.h>])
AC_CHECK_SIZEOF(void *)
CFLAGS="$ac_save_CFLAGS"
size_check_failed=""
if test "$ac_cv_sizeof_void_p" -gt "$ac_cv_sizeof_long"; then
  echo "The type void * is bigger than long."
  size_check_failed=1
fi
if test "$ac_cv_sizeof_Window" -gt "$ac_cv_sizeof_long"; then
  echo "The type Window is bigger than long."
  size_check_failed=1
fi
if test "$ac_cv_sizeof_Pixel" -gt "$ac_cv_sizeof_long"; then
  echo "The type Pixel is bigger than long."
  size_check_failed=1
fi
if test x"$size_check_failed" = x1; then
  echo "This is not supported by the module interface yet (make_new_vpacket)."
  echo "Detected type sizes are: int($ac_cv_sizeof_int), long($ac_cv_sizeof_long), void*($ac_cv_sizeof_void_p), Window($ac_cv_sizeof_Window), Pixel($ac_cv_sizeof_Pixel)"
  echo "Please report details of your system and this message to ${FVWMWORKERSLIST}."
  echo
  exit 1
fi

# 'unset' is not portable, but setting to null is not enough to avoid using
# the cached value!  For ancient shells "rm config.cache" is a solution.
UNSET=true
if unset UNSET 2>/dev/null; then UNSET=unset; fi


# ********* multibyte
# FreeBSD has libxpg4, check this and use if found.
AC_CANONICAL_HOST
case $host_os in
freebsd*)
	AC_CHECK_LIB(xpg4, setlocale, [LIBS="$LIBS -lxpg4"]);;
*)
	;;
esac

# *** pkg-config
# unfortunately, we need pkg-config for the detection of certain libs:
# - version of fontconfig without fontconfig-config
# - version of fribidi without fribidi-config
AM_CHECK_PKG_CONFIG


# Building man pages & HTML documentation (from XML source).
# extract command names
if test ! x"$SED" = x; then
	DOC_COMMANDS=`
		sed -n '
		:findhead
			/#.*define.*CMD_ENT/bfindcmd
			n
			bfindhead
		:findcmd
			n
			/CMD_ENT/bfound
			bfindcmd
		:found
			/"#"/bfindcmd
			/"propertychange"/bfindcmd
			/"readwritecolors"/bfindcmd
			/"send_.*"/bfindcmd
			/"set_.*"/bfindcmd
			s/.*CMD_ENT.*CMD_//
			s/,.*//
			p
			n
			bfindcmd
	' < fvwm/functable.c`
	DOC_COMMANDS=`echo $DOC_COMMANDS`
	# with .xml suffix
	DOC_COMMANDS_XML=`for i in $DOC_COMMANDS; do echo ${i}.xml; done`
	DOC_COMMANDS_XML=`echo $DOC_COMMANDS_XML`
	# with .xml suffix and path
	DOC_COMMANDS_XML_PATH=`
		for i in $DOC_COMMANDS; do
			echo ../commands/${i}.xml;
		done
	`
	DOC_COMMANDS_XML_PATH=`echo $DOC_COMMANDS_XML_PATH`
	# with .html suffix
	DOC_COMMANDS_HTML=`for i in $DOC_COMMANDS; do echo ${i}.html; done`
	DOC_COMMANDS_HTML=`echo $DOC_COMMANDS_HTML`
	# extract module names
	DOC_MODULES=""
	DOC_MODULES=`
		for i in modules/*; do echo "$i"; done |
		sed -n '
			:search
			/^modules.Fvwm/bfound
			bnext
		:found
			s/modules.//
			p
		:next
			n
			bsearch
		'
	`
	DOC_MODULES=`echo $DOC_MODULES`
	DOC_MODULES_HTML=`for i in $DOC_MODULES; do echo ${i}.html; done`
	DOC_MODULES_HTML=`echo $DOC_MODULES_HTML`
	# extract man page section names
	DOC_SECTIONS=`cat doc/fvwm/sections`
	DOC_SECTIONS=`echo $DOC_SECTIONS`
	DOC_SECTIONS_XML=`for i in $DOC_SECTIONS; do echo ${i}.xml; done`
	DOC_SECTIONS_XML=`echo $DOC_SECTIONS_XML`
	DOC_SECTIONS_XML_PATH=`for i in $DOC_SECTIONS; do echo ${i}.xml; done`
	DOC_SECTIONS_XML_PATH=`echo $DOC_SECTIONS_XML`
else
	DOC_COMMANDS=""
	DOC_COMMANDS_XML=""
	DOC_COMMANDS_XML_PATH=""
	DOC_COMMANDS_HTML=""
	DOC_MODULES=""
	DOC_MODULES_HTML=""
	DOC_SECTIONS=""
	DOC_SECTIONS_XML=""
	DOC_SECTIONS_XML_PATH=""
fi
AC_SUBST(DOC_COMMANDS)
AC_SUBST(DOC_COMMANDS_XML)
AC_SUBST(DOC_COMMANDS_XML_PATH)
AC_SUBST(DOC_COMMANDS_HTML)
AC_SUBST(DOC_MODULES)
AC_SUBST(DOC_MODULES_HTML)
AC_SUBST(DOC_SECTIONS)
AC_SUBST(DOC_SECTIONS_XML)
AC_SUBST(DOC_SECTIONS_XML_PATH)

problem_mandoc=""
AC_CHECK_PROG(XSLTPROC, xsltproc, xsltproc, "")

AC_ARG_ENABLE(mandoc,
  AS_HELP_STRING([--disable-mandoc],
    [disable generation of man pages]),
  [ if test x"$enableval" = xyes; then
		with_mandoc="yes, check"
	else
		with_mandoc="no"
		problem_mandoc=": Explicitly disabled"
	fi ],
  [ with_mandoc="not specified, check" ]
)

if test ! x"$with_mandoc" = xno; then
	if test x"$XSLTPROC" = x ; then
		with_mandoc="no"
		problem_mandoc=": No xsltproc found in PATH"
	elif test x"$SED" = x ; then
		with_mandoc="no"
		problem_mandoc=": No sed found in PATH"
	elif test x"$HAVE_PERL_FOR_DOCS" = x0 ; then
		with_mandoc="no"
		problem_mandoc=": No perl found in PATH"
	else
		with_mandoc="yes"
	fi
fi
AM_CONDITIONAL([FVWM_BUILD_MANDOC], [test x"$with_mandoc" = xyes])


problem_htmldoc=""
AC_ARG_ENABLE(htmldoc,
  AS_HELP_STRING([--enable-htmldoc],[enable generation of HTML documentation]),
  [ if test x"$enableval" = xyes; then
		with_htmldoc="yes, check"
	else
		with_htmldoc="no"
		problem_htmldoc=": Explicitly disabled"
	fi ],
  [ with_htmldoc="no" ]
)

if test ! x"$with_htmldoc" = xno; then
	if test x"$XSLTPROC" = x ; then
		with_htmldoc="no"
		problem_htmldoc=": No xsltproc found in PATH"
	elif test x"$SED" = x ; then
		with_htmldoc="no"
		problem_htmldoc=": No sed found in PATH"
	elif test x"$HAVE_PERL_FOR_DOCS" = x0 ; then
		with_htmldoc="no"
		problem_htmldoc=": No perl found in PATH"
	else
		with_htmldoc="yes"
	fi
fi
AM_CONDITIONAL([FVWM_BUILD_HTMLDOC], [test x"$with_htmldoc" = xyes])


# ********* session management
# Check the availability of SM; we don't have to add any extra libraries,
# since -lSM -lICE are in X_PRE_LIBS when they exist.
dnl [old check] AC_CHECK_LIB(SM, SmcOpenConnection, AC_DEFINE(SESSION), ,
dnl [old check]            [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])

problem_sm=""

AC_ARG_ENABLE(sm,
  AS_HELP_STRING([--disable-sm],[disable session management support]),
  [ if test x"$enableval" = xyes; then
    with_sm="yes, check"
  else
    with_sm="no"
    problem_sm=": Explicitly disabled"
  fi ],
  [ with_sm="not specified, check" ]
)

if test ! x"$with_sm" = xno; then
  dnl Uncomment the following and comment out AC_CHECK_LIB to get --with-sm-*
  dnl $UNSET ac_cv_lib_SM_SmcOpenConnection
  dnl $UNSET ac_cv_header_X11_SM_SMlib_h
  dnl smr_CHECK_LIB(sm, SM, adds session management support, SmcOpenConnection,
  dnl   X11/SM/SMlib.h,
  dnl   [$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS], $X_CFLAGS)
  dnl test "$sm_LIBS" && AC_DEFINE(SESSION)

  $UNSET ac_cv_lib_SM_SmcOpenConnection
  AH_TEMPLATE([SESSION],[Enables session management functionality.])
  AC_CHECK_LIB(SM, SmcOpenConnection, with_sm=yes; AC_DEFINE(SESSION),
    with_sm=no; problem_sm=": Failed to detect libSM",
    [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])
fi
dnl AC_SUBST(sm_LIBS)
dnl AC_SUBST(sm_CFLAGS)


# Checking for optional libraries
# Default is to use them if found; can be disable using --without
# These are put up front so that if they are requested, but
# configure fails to find them, we fail early.


# ********* shape extension
dnl smr_SWITCH(shape, shape extensions, on, SHAPE)
AC_ARG_ENABLE(shape,
  AS_HELP_STRING([--disable-shape],[disable shaped window support]),
  [ if test x"$enableval" = xyes; then
    with_shape="yes, check"
  else
    with_shape="no"
    problem_shape=": Explicitly disabled"
  fi ],
  [ with_shape="not specified, check" ]
)

AH_TEMPLATE(SHAPE,
[   Define if you want the Shaped window extensions.
   Shaped window extensions seem to increase the window managers RSS
   by about 60 Kbytes. They provide for leaving a title-bar on the window
   without a border.
   If you don't use shaped window extension, you can either make your
   shaped windows undecorated, or live with a border and backdrop around
   all your shaped windows (oclock, xeyes)

   If you normally use a shaped window (xeyes or oclock), you might as
   well compile this extension in, since the memory cost is  minimal in
   this case (The shaped window shared libs will be loaded anyway). If you
   don't normally use a shaped window, you have to decide for yourself.

   Note: if it is compiled in, run time detection is used to make sure that
   the currently running X server supports it.])
if test ! x"$with_shape" = xno; then
  $UNSET ac_cv_lib_Xext_XShapeQueryExtension
  AC_CHECK_LIB(Xext, XShapeQueryExtension,
               with_shape=yes; AC_DEFINE(SHAPE),
               with_shape=no;
               problem_shape=": Failed to detect Shape extension",
               [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])
fi

# ********* MIT Shared Memory Extension
AC_ARG_ENABLE(shm,
  AS_HELP_STRING([--disable-shm],[disable MIT Shared Memory Extension]),
  [ if test x"$enableval" = xyes; then
    with_shm="yes, check"
  else
    with_shm="no"
    problem_shm=": Explicitly disabled"
  fi ],
  [ with_shm="not specified, check" ]
)

AH_TEMPLATE([HAVE_XSHM],[Define if  MIT Shared Memory extension is used.])
if test ! x"$with_shm" = xno; then
  $UNSET ac_cv_lib_Xext_XShmQueryExtension
  AC_CHECK_LIB(Xext, XShmQueryExtension,
               with_shm=yes; AC_DEFINE(HAVE_XSHM),
               with_shm=no;
               problem_shm=": Can't detect MIT Shared Memory ext.",
               [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])
fi

# Silently look for X11/XKBlib.h
AH_TEMPLATE([HAVE_X11_XKBLIB_H],[Define if Xkb extension is used.])
AC_CHECK_HEADER(X11/XKBlib.h, AC_DEFINE(HAVE_X11_XKBLIB_H))

# ********* xineramA
problem_xinerama=""

AC_ARG_ENABLE(xinerama,
  AS_HELP_STRING([--disable-xinerama],[disable Xinerama multi screen support]),
  [ if test x"$enableval" = xyes; then
    with_xinerama="yes, check"
  else
    with_xinerama="no"
    problem_xinerama=": Explicitly disabled"
  fi ],
  [ with_xinerama="not specified, check" ]
)

AH_TEMPLATE([HAVE_XINERAMA],[Define if Xinerama library is used.])
AH_TEMPLATE([HAVE_SOLARIS_XINERAMA],
  [Define if Solaris' Xinerama calls are being used.
  (Solaris 7 11/99 and later)])
AH_TEMPLATE([HAVE_SOLARIS_XINERAMA_H],
  [Define if Solaris' X11/extensions/xinerama.h header is provided.
  (Solaris 9 and later)])
if test ! x"$with_xinerama" = xno; then
  $UNSET ac_cv_lib_Xinerama_XineramaIsActive
  _check_solaris_xinerama=no
  AC_CHECK_LIB(Xinerama, XineramaIsActive,
    with_xinerama=yes; Xinerama_LIBS=-lXinerama; AC_DEFINE(HAVE_XINERAMA),
    _check_solaris_xinerama=yes,
    [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS])
  if test x"$_check_solaris_xinerama" = xyes; then
    AC_CHECK_LIB(Xext, XineramaGetState,
      [ AC_DEFINE(HAVE_XINERAMA) AC_DEFINE(HAVE_SOLARIS_XINERAMA)
      with_xinerama=yes; Xinerama_LIBS=-lXext
      my_CPPFLAGS="$CPPFLAGS"; CPPFLAGS="$X_CPPFLAGS $CPPFLAGS"
      my_hdr="X11/extensions/xinerama.h"
      AC_CHECK_HEADER($my_hdr, AC_DEFINE(HAVE_SOLARIS_XINERAMA_H)
	problem_xinerama=" (Using Solaris Xinerama calls)",
	problem_xinerama=" (Using Solaris 9 prototypes for missing header)")
      CPPFLAGS="$my_CPPFLAGS"],
    with_xinerama=no; Xinerama_LIBS=
    problem_xinerama=": Failed to detect libXinerama",
    [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS])
  fi
  $UNSET _check_solaris_xinerama
fi
AC_SUBST(Xinerama_LIBS)
AC_SUBST(Xinerama_CFLAGS)

# ********* xinerama-emulation
smr_SWITCH(
  xinerama-emulation,
  [Xinerama emulation on one screen (useful only for developers)], off,
  USE_XINERAMA_EMULATION,,
  [Define if Xinerama should be emulated on a single screen.])
if test x"$enable_xinerama_emulation" = xyes; then
  with_xinerama_emulation=yes
else
  with_xinerama_emulation=no
fi

# ********* xrender
problem_xrender=""
AC_ARG_ENABLE(xrender,
  AS_HELP_STRING([--disable-xrender],[disable Xrender alpha-blend rendering]),
  [ if test x"$enableval" = xyes; then
    with_xrender="yes, check"
  else
    with_xrender="no"
    problem_xrender=": Explicitly disabled"
  fi ],
  [ with_xrender="not specified, check" ]
)

AH_TEMPLATE([HAVE_XRENDER],[Define if Xrender library is used.])
if test ! x"$with_xrender" = xno; then
  $UNSET ac_cv_lib_Xrender_XRenderComposite
  AC_CHECK_LIB(Xrender, XRenderComposite,
    with_xrender=yes; Xrender_LIBS=-lXrender;
    AC_DEFINE(HAVE_XRENDER),
    with_xrender=no; Xrender_LIBS=
    problem_xrender=": Failed to detect libXrender",
    [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS])
fi
AC_SUBST(Xrender_LIBS)
AC_SUBST(Xrender_CFLAGS)

# ********* xcursor
AH_TEMPLATE([HAVE_XCURSOR],[Define if Xcursor library is used.])
xcursor_CFLAGS=""
xcursor_LIBS=""
AC_ARG_ENABLE(xcursor,
  AS_HELP_STRING([--disable-xcursor],
    [disable Xcursor ARGB/animated cursor loading]),
  [ if test x"$enableval" = xno; then
       with_xcursor=no
       problem_xcursor=": Explicitly disabled"
    fi ],
)
if test ! x"$with_xcursor" = xno; then
   with_xcursor=no
   if test ! x"$with_xrender" = xno; then
      $UNSET ac_cv_lib_Xrender_XRenderCreateCursor
      AC_CHECK_LIB(Xrender, XRenderCreateCursor, [
         with_xcursor=yes
      ],[
         problem_xcursor=": Your libXrender version is too old"
      ],
      [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS])
   else
      problem_xcursor=": Need Xrender support"
   fi
fi
if test x"$with_xcursor" = xyes ; then
      $UNSET ac_cv_lib_Xcursor_XcursorImageLoadCursor
      AC_CHECK_LIB(Xcursor, XcursorImageLoadCursor, [
         AC_DEFINE(HAVE_XCURSOR)
         Xcursor_LIBS=-lXcursor
         problem_xcursor=""
      ],[
         with_xcursor=no
         problem_xcursor=": Failed to detect libXcursor"
      ],[$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS])
fi
AC_SUBST(Xcursor_CFLAGS)
AC_SUBST(Xcursor_LIBS)

# ********* xft
problem_xft=""

AC_ARG_ENABLE(xft,
  AS_HELP_STRING([--disable-xft],[disable Xft anti-aliased font rendering]),
  [ if test x"$enableval" = xyes; then
    with_xft="yes, check"
  else
    with_xft="no"
    problem_xft=": Explicitly disabled"
  fi ],
  [
    with_xft="not specified, check"
  ]
)

AH_TEMPLATE([HAVE_XFT],[Define if Xft library is used.])
AH_TEMPLATE([HAVE_XFT2],[Define if Xft 2 library is used.])
AH_TEMPLATE([HAVE_XFT_UTF8],[Define if Xft library can handle utf8 encoding])
if test ! x"$with_xft" = xno; then

  # first check for freetype2
  have_freetype=no
  AM_CHECK_FT2(6.1.0)
  if test x"$no_ft" = x; then
    have_freetype=yes
  else
    have_freetype=no
    problem_xft=": Can't detect freetype2 >= 6.1.0/2.0.6"
  fi

  # check for fontconfig for Xft 2
  have_fontconfig=no
  if test ! x"$have_freetype" = xno ; then
    AM_CHECK_FC(1.0.1)
    if test x"$no_fc" = x ; then
      have_fontconfig=yes
      fontconfig_CFLAGS=`$PKG_CONFIG --cflags fontconfig`
      fontconfig_LIBS=`$PKG_CONFIG --libs fontconfig`
      CFLAGS="$CFLAGS $fontconfig_CFLAGS"
      LIBS="$LIBS $fontconfig_LIBS"
    else
      have_fontconfig=no
      problem_xft=": Can't detect fontconfig >= 1.0.1"
    fi
  fi

  # now check for Xft 2
  with_xft=no
  if test ! x"$have_fontconfig" = xno ; then
    # Xft 2
    AM_CHECK_XFT(2.0.0)
    if test x"$no_xft" = x; then
      with_xft=yes
      problem_xft=" (version 2)"
      AC_DEFINE(HAVE_XFT2)
      AC_DEFINE(HAVE_XFT)
      AC_DEFINE(HAVE_XFT_UTF8)
      Xft_LIBS=$XFT_LIBS
      Xft_CFLAGS=$XFT_CFLAGS
    else
      problem_xft=": Can't detect Xft2, detected fontconfig"
    fi
  fi

  # if Xft2 not detected check for Xft1
  if test ! x"$have_freetype" = xno && test ! x"$with_xft" = xyes; then
    #Xft 1
    $UNSET ac_cv_lib_XftConfigSubstitute
    $UNSET ac_cv_lib_Xft_XftFontOpen
    $UNSET ac_cv_lib_Xft_XftDrawStringUtf8
    AC_CHECK_LIB(Xft, XftFontOpen,
      with_xft=yes, with_xft=no,
      [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS $FT2_LIBS $Xrender_LIBS])
    if test x"$with_xft" = xyes ; then
      AC_CHECK_LIB(Xft, XftConfigSubstitute, is_xft1=yes, is_xft1=no,
        [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS $FT2_LIBS $Xrender_LIBS])
      if test x"$is_xft1" = xyes; then
        Xft_LIBS="-lXft $FT2_LIBS"
	Xft_CFLAGS="$Xft_CFLAGS $FT2_CFLAGS"
        problem_xft=" (version 1)"
        AC_DEFINE(HAVE_XFT)
      else
        with_xft=no
        problem_xft=": Can't detect Xft 1 or fontconfig"
      fi
    else
      problem_xft=": Can't detect Xft 1 or 2 and fontconfig"
    fi
    if test x"$with_xft" = xyes; then
      AC_CHECK_LIB(Xft, XftDrawStringUtf8,
        AC_DEFINE(HAVE_XFT_UTF8),,
        [$X_LIBS $X_PRE_LIBS -lXext -lX11 $X_EXTRA_LIBS $FT2_LIBS $Xrender_LIBS])
    fi
  fi

fi
AC_SUBST(Xft_LIBS)
AC_SUBST(Xft_CFLAGS)

# ********* xpm
problem_xpm=": Xpm library or header not found"

$UNSET ac_cv_header_X11_xpm_h
$UNSET ac_cv_lib_Xpm_XpmReadFileToXpmImage
smr_CHECK_LIB(xpm, Xpm, for coloured or shaped icons,
    XpmReadFileToXpmImage, X11/xpm.h,
    [$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS], $X_CFLAGS)

if test ! x"$xpm_LIBS" = x; then
  # Check for proper version of Xpm -- from XEmacs 21.x configure.in
  AC_MSG_CHECKING([for Xpm 3.4g or better])
  my_CPPFLAGS="$CPPFLAGS"
  my_LIBS="$LIBS"
  CPPFLAGS="$CPPFLAGS $xpm_CFLAGS $X_CFLAGS"
  LIBS="$LIBS $xpm_LIBS $X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
  AC_TRY_RUN([#include <X11/xpm.h>
   int main(int c, char **v) {
   return c == 1 ? 0 :
     XpmIncludeVersion != XpmLibraryVersion() ? 1 :
     XpmIncludeVersion < 30407 ? 2 : 0 ;}],
    [./conftest dummy_arg; xpm_status=$?;
     if test x"$xpm_status" = x0; then
       with_xpm=yes
       problem_xpm=""
     else
       with_xpm=no;
       if test x"$xpm_status" = x1; then
	 problem_xpm=": Xpm library and header versions don't match"
       elif test x"$xpm_status" = x2x; then
	 problem_xpm=": Xpm library version is too old"
       else
	 problem_xpm=": Internal xpm detection logic error"
       fi
     fi],
   [with_xpm=no; problem_xpm=": Xpm test error, see config.log"],
   [echo $ac_n "cross compiling; assumed OK... $ac_c"])
  AC_MSG_RESULT($with_xpm)
  CPPFLAGS="$my_CPPFLAGS"
  LIBS="$my_LIBS"

  AH_TEMPLATE([XPM],[Define if Xpm library is used.])
  if test x"$with_xpm" = xyes; then
    AC_DEFINE(XPM)
    # FVWMBANNER=FvwmBanner AC_SUBST(FVWMBANNER)
    # XPMROOT=xpmroot AC_SUBST(XPMROOT)
    # FVWMSCRIPT=FvwmScript AC_SUBST(FVWMSCRIPT)
  else
    xpm_LIBS=
    xpm_CFLAGS=
  fi
fi
AC_SUBST(xpm_LIBS)
AC_SUBST(xpm_CFLAGS)

# ********* png
png_LIBS=""
png_CFLAGS=""

AH_TEMPLATE([HAVE_PNG], [Define PNG])
AC_ARG_ENABLE(png,
    AS_HELP_STRING([--disable-png],
		   [Disables libpng support.]),
		   [with_png="$enableval"],
		   [with_png="yes"])

if test "$with_png" = no; then
      AC_MSG_WARN([***
		   disabling PNG support will mean the default
		   configuration shipped with FVWM will be without icon
		   support.  Be sure this is what you mean to do.
		   ***])
else
	AC_MSG_CHECKING([for libpng...])
	PKG_CHECK_MODULES([png], [libpng >= 1.2],
	[
	 AC_DEFINE(HAVE_PNG)
	],
	[
	 AC_MSG_ERROR([***
		       libpng not found, and --disable-png not given.
		       It is recommended to use libpng so that PNG icons can
		       be rendered by FVWM, which the default configuration
		       file needs, as well as other third-party programs
		       which generate menus, for example.

		       If it is critical that FVWM does not link against
		       libpng (perhaps due to size limitations of the
		       binary), then use --disable-png
		       ***])
	])
	# ** needed by the png support
	AC_C_BIGENDIAN
fi

AC_SUBST(png_LIBS)
AC_SUBST(png_CFLAGS)

# ********* rsvg
rsvg_min_version=2.13.92
AH_TEMPLATE([HAVE_RSVG], [Define if librsvg library is used.])

AC_ARG_ENABLE(rsvg,
  AS_HELP_STRING([--disable-rsvg],
    [disable scalable vector graphics (SVG images)]),
  [ if test x"$enableval" = xno; then
       with_rsvg=no
       problem_rsvg=": Explicitly disabled"
    fi ],
)
if test ! x"$with_rsvg" = xno; then
   with_rsvg=no
   if test ! x"$PKG_CONFIG" = xno ; then
      AC_MSG_CHECKING(for librsvg - version >= $rsvg_min_version)
      if $PKG_CONFIG --exists librsvg-2.0 ; then
         if $PKG_CONFIG --exists "librsvg-2.0 >= $rsvg_min_version" ; then
            AC_MSG_RESULT(yes)

   	    AC_MSG_CHECKING(for cairo svg backend)
            if $PKG_CONFIG --exists cairo-svg ; then
               svg_packages="librsvg-2.0 cairo-svg"
            elif $PKG_CONFIG --exists libsvg-cairo ; then
               svg_packages="librsvg-2.0 libsvg-cairo"
            elif $PKG_CONFIG --exists cairo ; then
               svg_packages="librsvg-2.0 cairo"
            else
               svg_packages=""
            fi
            if test ! x"$svg_packages" = x ; then
               AC_MSG_RESULT(yes)
               rsvg_CFLAGS=`$PKG_CONFIG --cflags $svg_packages`
               rsvg_LIBS=`$PKG_CONFIG --libs $svg_packages`
               with_rsvg=yes
            else
               AC_MSG_RESULT(no)
               AC_MSG_WARN([*** cairo was not found in the pkg-config search])
               AC_MSG_WARN([*** path. Add the directory containing cairo.pc])
               AC_MSG_WARN([*** to the PKG_CONFIG_PATH environment variable.])
               problem_rsvg=": Cannot detect cairo backend"
            fi
         else
            AC_MSG_RESULT(no)
            AC_MSG_WARN([*** Your librsvg version is < $rsvg_min_version])
            problem_rsvg=": Your librsvg version is too old"
         fi
      else
         AC_MSG_RESULT(no)
         AC_MSG_WARN([*** librsvg-2.0 was not found in the pkg-config search])
         AC_MSG_WARN([*** path. Either librsvg is not installed or you need])
         AC_MSG_WARN([*** to add the directory containing librsvg-2.0.pc to])
         AC_MSG_WARN([*** the PKG_CONFIG_PATH environment variable.])
         problem_rsvg=": librsvg library or header not found"
      fi
   else
      problem_rsvg=": pkg-config not found"
   fi
fi
if test x"$with_rsvg" = xyes ; then
   AC_MSG_CHECKING(whether a librsvg program compiles and runs)
   original_CFLAGS="$CFLAGS"
   original_LIBS="$LIBS"
   CFLAGS="$CFLAGS $rsvg_CFLAGS"
   LIBS="$LIBS $rsvg_LIBS"
   AC_TRY_RUN([
      #include <librsvg/rsvg.h>
      #include <librsvg/rsvg-cairo.h>
      int main() {
         RsvgHandle *rsvg;
         g_type_init();
         if(!(rsvg = rsvg_handle_new())) return 1;
         g_object_unref(G_OBJECT(rsvg));
         return 0;
      }
   ], [
      AC_MSG_RESULT(yes)
   ], [
      AC_MSG_RESULT(no)
      AC_MSG_WARN([*** The librsvg test program failed to run. If your system])
      AC_MSG_WARN([*** has shared libraries outside the normal system library])
      AC_MSG_WARN([*** path, you need to make sure that the LD_LIBRARY_PATH])
      AC_MSG_WARN([*** (or the like) environment variable is correctly set.])
      with_rsvg=no
      problem_rsvg=": Failed to run test program"
   ], [echo $ac_n "cross compiling; assumed OK... $ac_c"])
   CFLAGS="$original_CFLAGS"
   LIBS="$original_LIBS"
fi
if test x"$with_rsvg" = xyes ; then
   AC_DEFINE(HAVE_RSVG)
   problem_rsvg=""
else
   rsvg_CFLAGS=""
   rsvg_LIBS=""
fi
AC_SUBST(rsvg_CFLAGS)
AC_SUBST(rsvg_LIBS)

# ********* rplay
$UNSET ac_cv_header_rplay_h
$UNSET ac_cv_lib_rplay_rplay_create
# Add in X_EXTRA_LIBS here to get things like connect().
smr_CHECK_LIB(rplay, , adds audio capability, rplay_create, rplay.h,
	      $X_EXTRA_LIBS)
AH_TEMPLATE([HAVE_RPLAY],[Define if rplay library is used.])
test ! x"$rplay_LIBS" = x && AC_DEFINE(HAVE_RPLAY)
AC_SUBST(rplay_LIBS)
AC_SUBST(rplay_CFLAGS)


# ********* stroke
$UNSET ac_cv_header_stroke_h
$UNSET ac_cv_lib_stroke_stroke_init
dnl Add in X_LIBS for MOUSE_DROPPINGS?
dnl As of 23/Mar/2000 the only libstroke RPM has /usr/X11R6/include/stroke.h
AH_TEMPLATE([HAVE_STROKE],[Define if stroke library is used.])
smr_CHECK_LIB(stroke, , mouse strokes recognition,
	stroke_init, stroke.h,
	[$X_LIBS -lX11], $X_CFLAGS -I/usr/X11R6/include)
test ! x"$stroke_LIBS" = x &&
AC_DEFINE(HAVE_STROKE)
AH_VERBATIM([_HAVE_STROKE],
[#ifdef HAVE_STROKE
#    define STROKE_ARG(x) x,
#    define STROKE_CODE(x) x
#else
#    define STROKE_ARG(x)
#    define STROKE_CODE(x)
#endif])
AC_SUBST(stroke_LIBS)
AC_SUBST(stroke_CFLAGS)


# ********* readline
with_readline=no
problem_readline=": Both termcap and ncurses disabled"

smr_ARG_WITHLIB(termcap, , to accomplish readline)
AH_TEMPLATE([HAVE_READLINE],[Define if readline is available.])
if test ! x"$with_termcap" = xno; then
  # Forget cached values, so user can re-run configure
  $UNSET ac_cv_header_readline_history_h
  $UNSET ac_cv_lib_readline_readline
  AC_MSG_CHECKING([with termcap])
  smr_CHECK_LIB(readline, , line editing/history, readline, readline/history.h,
    $termcap_LIBS)
  if test ! x"$readline_LIBS" = x; then
    with_readline=yes
    readline_LIBS="$readline_LIBS $termcap_LIBS"
    AC_DEFINE(HAVE_READLINE)
  fi
fi
smr_ARG_WITHLIB(ncurses, , to accomplish readline)
if test ! x"$with_ncurses" = xno && test x"$readline_LIBS" = x; then
  # We couldn't use readline with termcap; try with ncurses?
  # Doesn't this seem a hacky way to do this??
  # unset cached values from last check...
  $UNSET ac_cv_header_readline_history_h
  $UNSET ac_cv_lib_readline_readline
  AC_MSG_CHECKING([with ncurses])
  dnl We can not call smr_CHECK_LIB twice here without having the usage twice.
  dnl old_LIBS=$LIBS
  dnl AC_CHECK_LIB(readline, readline, , , $ncurses_LIBS)
  dnl LIBS=$old_LIBS
  dnl if test x"$ac_cv_lib_readline_readline" = xyes; then
  dnl   AC_CHECK_HEADERS(readline/history.h)
  dnl   if test x"$ac_cv_header_readline_history_h" = xyes; then
  dnl     with_readline=yes
  dnl     readline_LIBS="$readline_LIBS $ncurses_LIBS"
  dnl     AC_DEFINE(HAVE_READLINE)
  dnl   fi
  dnl fi
  smr_CHECK_LIB(readline, , line editing/history, readline, readline/history.h,
    $ncurses_LIBS)
  if test ! x"$readline_LIBS" = x; then
    with_readline=yes
    readline_LIBS="$readline_LIBS $ncurses_LIBS"
    AC_DEFINE(HAVE_READLINE)
  fi
fi
dnl We need to check if we have GNU readline or BSD editline
AH_TEMPLATE([HAVE_GNU_READLINE],[Define if readline has full GNU interface])
if test x"$with_readline" = xyes; then
   AC_CHECK_LIB(readline, append_history, AC_DEFINE(HAVE_GNU_READLINE), [], $readline_LIBS)
fi
AC_SUBST(readline_LIBS)
AC_SUBST(readline_CFLAGS)


# Check if Xsetlocale() is available or not.
AH_TEMPLATE([X_LOCALE],[Enables to use setlocale() provided by X])
AC_CHECK_LIB(X11, _Xsetlocale, AC_DEFINE(X_LOCALE),, $X_LIBS -lX11)

# Check if Xsetlocale() is available or not.
AH_TEMPLATE([HAVE_XOUTPUT_METHOD],[Enable X output method])
AC_CHECK_LIB(X11, XOpenOM, AC_DEFINE(HAVE_XOUTPUT_METHOD),, $X_LIBS -lX11)

# ******** iconv
with_iconv=no
with_iconv_type=no
with_lib_iconv=no
with_sys_iconv=no
problem_iconv=": Explicitly disabled"

AC_ARG_ENABLE(iconv,
  AS_HELP_STRING([--disable-iconv],
    [disable support for iconv character conversion]),
  [ac_cv_iconv="$enableval"],
  [ac_cv_iconv="yes"])

AH_TEMPLATE([USE_LIBICONV],
  [define if we use libiconv (not needed in general: for example iconv is
  native with recent glibc)])
AH_TEMPLATE([HAVE_ICONV],
  [Define if iconv (in the libc) or libiconv is available])
if test ! x"$ac_cv_iconv" = xno; then
  # * first check for gnu libiconv
  $UNSET ac_cv_header_libiconv_h
  $UNSET ac_cv_lib_libiconv_libiconv_open
  smr_CHECK_LIB(
  	iconv, , if not found sys iconv is used, libiconv_open, iconv.h)
  if test ! x"$iconv_LIBS" = x; then
     AC_DEFINE(USE_LIBICONV)
     with_lib_iconv=yes
  fi
  AC_SUBST(iconv_LIBS)
  AC_SUBST(iconv_CFLAGS)

  # * check for system iconv

  AC_CHECK_FUNC(iconv_open, with_sys_iconv=yes)

  if test x"$with_lib_iconv" = xyes && test x"$with_sys_iconv" = xyes; then
     AC_MSG_WARN([***  Both system iconv and libiconv found: use libiconv])
     AC_MSG_WARN([***  Use --with-iconv-library=no to use the system iconv])
  fi

  # * libiconv found check for libcharset to get the good iconv charset
  if test x"$with_lib_iconv" = xyes; then
     CHECK_LIBCHARSET
  fi

  test x"$with_lib_iconv" = xyes -o x"$with_sys_iconv" = xyes &&
	 with_iconv=yes && problem_iconv=""
  test x"$with_lib_iconv" = xyes && with_iconv_type="yes (libiconv)"
  test x"$with_sys_iconv" = xyes && with_iconv_type="yes (from C library)"

  if test x"$with_iconv" = xno; then
    AC_MSG_WARN([*** ])
    AC_MSG_WARN([*** No iconv() implementation found in C library or libiconv])
    AC_MSG_WARN([*** Please install libiconv ftp://ftp.gnu.org/pub/gnu/libiconv/])
    AC_MSG_WARN([*** ])
  fi

  if test x"$with_iconv" = xyes; then
     AC_DEFINE(HAVE_ICONV)
     ICONV_SECOND_ARG
  fi
fi
AH_VERBATIM([_HAVE_LIBCHARSET],
[#ifdef USE_LIBICONV
/* define to use locale_charset in the place of nl_langinfog if libiconv
 * is used */
#undef HAVE_LIBCHARSET
#endif])


# ******** nl_langinfo and CODESET
AH_TEMPLATE([HAVE_CODESET],[Have nl_langinfo (CODESET)])
AC_MSG_CHECKING([for nl_langinfo (CODESET)])
AC_TRY_COMPILE([#include <langinfo.h>
#include <locale.h>],
[char *codeset = nl_langinfo(CODESET); setlocale(LC_CTYPE, "");],
   AC_DEFINE(HAVE_CODESET)
   have_codeset=yes,
   have_codeset=no)
AC_MSG_RESULT($have_codeset)


# ********* fribidi
AC_ARG_ENABLE(bidi,
  AS_HELP_STRING([--disable-bidi],[disable bi-directional text support]),
  [ if test x"$enableval" = xyes; then
    with_bidi="yes, check"
  else
    with_bidi="no"
  problem_bidi=": Explicitly disabled"
  fi ],
  [ with_bidi="not specified, check" ]
)

fribidi_min_version=0.19.2
if test ! x"$with_bidi" = xno; then
  AC_ARG_WITH(fribidi-bindir,
    AS_HELP_STRING([--with-fribidi-bindir=DIR],
      [directory of fribidi-config if not in PATH]),
    FRIBIDI_BINDIR="$withval", FRIBIDI_BINDIR=".")
  if test ! x"$PKG_CONFIG" = xno && $PKG_CONFIG --exists "fribidi >= $fribidi_min_version"; then
    FRIBIDI_CONFIG="$PKG_CONFIG fribidi"
  else
    AC_PATH_PROG(FRIBIDI_CONFIG, fribidi-config,, [$FRIBIDI_BINDIR:$PATH])
  fi

  if test x"$FRIBIDI_CONFIG" = x; then
    with_bidi=no
    problem_bidi=": pkgconfig fribidi:  fribidi version >= $fribidi_min_version needed."
  else
    Bidi_CFLAGS=`$FRIBIDI_CONFIG --cflags`
    Bidi_LIBS=`$FRIBIDI_CONFIG --libs`
    AC_CHECK_LIB(fribidi, fribidi_log2vis,
      with_bidi=yes; problem_bidi=,
      with_bidi=no; problem_bidi=": No good libs via $FRIBIDI_CONFIG",
      [$Bidi_LIBS $Bidi_CFLAGS])
  fi
fi

if test ! x"$with_bidi" = xno; then
  fribidi_in_path=yes
  ac_save_LIBS="$LIBS"
  LIBS="$LIBS $Bidi_LIBS"
  AC_TRY_RUN([#include <stdlib.h>
#include <fribidi/fribidi.h>
int main()
{
	FriBidiChar *logical_unicode_str =
		(FriBidiChar *)malloc((4 + 1) * sizeof(FriBidiChar));
	fribidi_charset_to_unicode(
		fribidi_parse_charset("iso8859-8"), "test", 4,
		logical_unicode_str);
	return 0;
}
], [:], [fribidi_in_path=no])
  LIBS="$ac_save_LIBS"
  if test ! x"$fribidi_in_path" = xyes; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $Bidi_CFLAGS"
    LIBS="$LIBS $Bidi_LIBS"
  AC_TRY_RUN([#include <stdlib.h>
#include <fribidi/fribidi.h>
int main()
{
	FriBidiChar *logical_unicode_str =
		(FriBidiChar *)malloc((4 + 1) * sizeof(FriBidiChar));
	fribidi_charset_to_unicode(
		fribidi_parse_charset("iso8859-8"), "test", 4,
		logical_unicode_str);
	return 0;
}
], [:], [with_bidi=no; problem_bidi=": Bad fribidi version, see config.log"])
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi
  if test ! x"$fribidi_in_path" = xno; then
    Bidi_CFLAGS=
    Bidi_LIBS=-lfribidi
  fi
fi

AH_TEMPLATE([HAVE_BIDI],[Define if fribidi library is used.])
AH_TEMPLATE([FRIBIDI_CHARSET_SPELLING],
  [Support fribidi-0.10.4 and older with "CHARSET" spelling.])
if test x"$with_bidi" = xno; then
  Bidi_CFLAGS=
  Bidi_LIBS=
else
  AC_DEFINE(HAVE_BIDI)
  AC_TRY_RUN([#include <fribidi/fribidi.h>
    int main(int c, char **v) {
      return FRIBIDI_CHARSET_NOT_FOUND * 0;
    }
  ], [AC_DEFINE(FRIBIDI_CHARSET_SPELLING)])
fi
AH_VERBATIM([_FRIBIDI_CHARSET_SPELLING],
[#ifdef FRIBIDI_CHARSET_SPELLING
#  define FRIBIDI_CHAR_SET_NOT_FOUND FRIBIDI_CHARSET_NOT_FOUND
#endif])

AC_SUBST(Bidi_LIBS)
AC_SUBST(Bidi_CFLAGS)


# ********* perllib
with_perllib="yes"
problem_perllib=""
FVWM_PERLLIB=perllib
AC_ARG_ENABLE(perllib,
  AS_HELP_STRING([--disable-perllib],[disable installing fvwm perl library]),
  [ if test x"$enableval" = xno; then
    with_perllib="no"
    problem_perllib=": Explicitly disabled"
    FVWM_PERLLIB=""
  fi ],
)
AC_SUBST(FVWM_PERLLIB)

# ******* gettext
ALL_LINGUAS="ar de fr sv_SE zh_CN ru es"
FVWM_DOMAIN="fvwm"
FVWMSCRIPT_DOMAIN="FvwmScript"
ALL_DOMAINS="$FVWM_DOMAIN $FVWMTASKBAR_DOMAIN $FVWMSCRIPT_DOMAIN"

AC_SUBST(ALL_LINGUAS)
AC_SUBST(FVWM_DOMAIN)
AC_SUBST(FVWMTASKBAR_DOMAIN)
AC_SUBST(FVWMSCRIPT_DOMAIN)
AC_SUBST(ALL_DOMAINS)

LOCALEDIR='${datadir}'"/locale"
with_gettext="yes"
problem_gettext=""

AC_ARG_ENABLE(nls,
  AS_HELP_STRING([--disable-nls],[do not use Native Language Support]),
  [ if test x"$enableval" = xno; then
    with_gettext="no"
    problem_gettext=": Explicitly disabled"
  fi ],
)

AH_TEMPLATE([HAVE_NLS],
  [Define to 1 if translation of program messages to the user's native
  language is requested.])
if test ! x"$with_gettext" = xno; then
  AM_GNU_FGETTEXT
  if test x"$USE_NLS" = "xyes"; then
    AC_DEFINE(HAVE_NLS, 1)
  else
    with_gettext="no"
    problem_gettext=": Failed to detected GNU gettext"
  fi
else
  USE_NLS=no
fi

# libs and buil
AC_SUBST(intl_LIBS)
AC_SUBST(intl_CFLAGS)
AC_SUBST(USE_NLS)
AC_SUBST(POSUB)
AC_SUBST(LOCALEDIR)
AC_SUBST(MKINSTALLDIRS)
# programs for devel
AC_SUBST(XGETTEXT)
AC_SUBST(GMSGFMT)
AC_SUBST(MSGMERGE)
AC_SUBST(MSGFMT)
AC_SUBST(MSGUNIQ)
#catalogs
AC_SUBST(INST_LINGUAS)
AC_SUBST(CATALOGS)
AC_SUBST(POFILES)
AC_SUBST(GMOFILES)
AC_SUBST(UPDATEPOFILES)
AC_SUBST(DUMMYPOFILES)
AC_SUBST(CATOBJEXT)

# Check for REQUIRED headers and functions before going any
# further.

# The module interface uses variadic functions for message passing.
AC_CHECK_HEADERS(stdarg.h)
AC_CHECK_FUNCS(vfprintf)
if test x$ac_cv_header_stdarg_h = xno -o \
        $ac_cv_func_vfprintf = no; then
     AC_MSG_ERROR([stdarg.h and vfprintf required])
fi

# For reaping children, fvwm needs either waitpid() or wait3()
# Some extra modules may need wait4() too
AC_CHECK_FUNCS(waitpid)
if test x$ac_cv_func_waitpid = xno; then
    AC_CHECK_FUNCS(wait3 wait4)
    if test x$ac_cv_func_wait3 = xno; then
        AC_MSG_ERROR([Either waitpid or wait3 function is required])
    fi
fi

# C89/C99 signal handling
AC_CHECK_FUNCS(sigsetjmp siglongjmp)

# Look harder for a C preprocessor for FvwmCpp
AC_PATH_PROG(FVWM_CPP, cpp, no, $PATH:/lib:/usr/lib:/usr/ccs/lib)
if test x"$FVWM_CPP" = xno; then
  FVWM_CPP=
  AC_MSG_WARN([cannot locate a C preprocessor: run FvwmCpp with -cppprog])
fi
dnl FIXME: make sure the cpp we found actually works...

# Finish checking for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
dnl lex+yacc now only needed for FvwmScript developers when syntax is changed
dnl AM_PROG_LEX
dnl AC_PROG_YACC

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(stdlib.h fcntl.h limits.h malloc.h string.h memory.h unistd.h)
AC_CHECK_HEADERS(stdint.h inttypes.h)
AC_CHECK_HEADERS(getopt.h sys/select.h sys/systeminfo.h sys/time.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
dnl AC_STRUCT_TM
AC_TYPE_SIGNAL
AC_TYPE_INT16_T
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_STRFTIME
AC_FUNC_SETPGRP
AC_CHECK_FUNCS(gettimeofday mkfifo putenv setvbuf socket waitpid)
AC_CHECK_FUNCS(strdup strstr strtol memmove memcpy strchr sysconf uname div)
AC_CHECK_FUNCS(sigaction siginterrupt getpwuid)
AC_CHECK_FUNCS(setpgrp setpgid)
AC_CHECK_FUNCS(lstat)



pds_CHECK_TYPE(sig_atomic_t, int, [#include <signal.h>], [Specify a type for sig_atomic_t if it's not available.])

if test x$ac_cv_func_setvbuf = xyes; then
  AC_FUNC_SETVBUF_REVERSED
fi

AC_FUNC_SELECT
if test ! x"$ac_cv_func_select" = xyes; then
  AC_MSG_ERROR([select required])
fi

# check for mkstemp, see the discution on this subject on the fvwm workers
# list (2001-02-16 and 2001-02-24)
AM_SAFETY_CHECK_MKSTEMP

# ********* IMLIB, GNOME
# Check the availability of gdk-imlib
AH_TEMPLATE([GDK_IMLIB],[Define if gdk-imlib is used])
AM_PATH_GDK_IMLIB(1.8.0, AC_DEFINE(GDK_IMLIB))
if test x"$no_imlib" = x; then
  with_gdkimlib=yes
  problem_gdkimlib=""
else
  with_gdkimlib=no
  problem_gdkimlib=": Failed on gdk-imlib, see config.log"
fi

GNOME_INIT_HOOK

# Unfortunately we have 2 gnome supports: WM hints and gnome libs.
# The $with_gnomehints below refers to the first, not GNOME_INIT_HOOK.
if test ! x"$enable_gnome_hints" = xno; then
  with_gnomehints=yes
  problem_gnomehints=""
else
  with_gnomehints=no
  problem_gnomehints=": Explicitly disabled"
fi

# Define some compatibility macros needed for config.h.
mg_DEFINE_IF_NOT([#include <X11/keysym.h>],
  [defined XK_Page_Up && defined XK_Page_Down],
  [COMPAT_OLD_KEYSYMDEF], [$X_CFLAGS],
  [Old AIX systems (3.2.5) don't define some common keysyms.])
AH_VERBATIM([_COMPAT_OLD_KEYSYMDEF],
[#ifdef COMPAT_OLD_KEYSYMDEF
#  define XK_Page_Up   XK_Prior
#  define XK_Page_Down XK_Next
#endif])

if test x"$with_stroke" = xyes; then
  mg_DEFINE_IF_NOT([#include <stroke.h>],
    [defined STROKE_MAX_SEQUENCE],
    [COMPAT_OLD_LIBSTROKE], [$stroke_CFLAGS],
    [Old libstroke <= 0.4 does not use STROKE_ prefix for constants.])
fi
AH_VERBATIM([_COMPAT_OLD_LIBSTROKE],
[#ifdef COMPAT_OLD_LIBSTROKE
/* currently we only use one constant */
#  define STROKE_MAX_SEQUENCE MAX_SEQUENCE
#endif])


# Allow building with dmalloc.  Do this last to avoid screwing up any
# other checks above.
case "$ac_cv_dmalloc" in
  yes) AC_CHECK_HEADERS(dmalloc.h)
       AC_CHECK_LIB(dmalloc, dmalloc_shutdown)
       CPPFLAGS="$CPPFLAGS -DDMALLOC_FUNC_CHECK" ;;
esac


# Allow building with efence.
case "$ac_cv_efence" in
  yes) AC_CHECK_LIB(efence, malloc) ;;
esac

# some explicit definitions for config.h file
AH_VERBATIM([_ZEND_EXPLICIT_DEFINITIONS],
[
/**
 * The next few defines are options that are only changed from their values
 * shown here on systems that _don't_ use the configure script.
 **/

/* Enable tests for missing too many XEvents.  Usually you want this. */
#define WORRY_ABOUT_MISSED_XEVENTS 1

/* Define if the X11 ConnectionNumber is actually a file descriptor. */
#define HAVE_X11_FD 1

/* Define if fork() has unix semantics.  On VMS, no child process is created
   until after a successful exec(). */
#define FORK_CREATES_CHILD 1

/* Suffix for executable filenames; NULL if no extension needed. */
#define EXECUTABLE_EXTENSION NULL

/* Define to remove the extension from executable pathnames before calling
   exec(). */
#undef REMOVE_EXECUTABLE_EXTENSION

/* Enables the "MiniIcon" Style option to specify a small pixmap which
 * can be used as one of the title-bar buttons, shown in window list,
 * utilized by modules, etc.  Requires PIXMAP_BUTTONS to be defined
 * (see below). */
/* #undef MINI_ICONS */
/* NOTE: hard coded to 1 */
#if 1
#define FMiniIconsSupported 1
#else
#define FMiniIconsSupported 0
#endif

#if RETSIGTYPE != void
#define SIGNAL_RETURN return 0
#else
#define SIGNAL_RETURN return
#endif

/* Allow GCC extensions to work, if you have GCC. */
#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later. */
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#    define __attribute__(x)
#  endif
/* The __-protected variants of `format' and `printf' attributes
 * are accepted by gcc versions 2.6.4 (effectively 2.7) and later. */
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#    define __format__ format
#    define __printf__ printf
#  endif
#endif

#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif

#if defined (HAVE_MALLOC_H) && !defined (__FreeBSD__) && !defined (__OpenBSD__) && !defined(__NetBSD__)
#  include <malloc.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifndef HAVE_STRCHR
#  define strchr(_s,_c)   index((_s),(_c))
#  define strrchr(_s,_c)  rindex((_s),(_c))
#endif

#ifndef HAVE_MEMCPY
#  define memcpy(_d,_s,_l)  bcopy((_s),(_d),(_l))
#endif
#ifndef HAVE_MEMMOVE
#  define memmove(_d,_s,_l) bcopy((_s),(_d),(_l))
#endif

#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifndef min
#  define min(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a,b) (((a)>(b)) ? (a) : (b))
#endif
#ifndef abs
#  define abs(a) (((a)>=0)?(a):-(a))
#endif

#include "libs/defaults.h"

#ifndef FEVENT_C
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "libs/FEvent.h"
#endif

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#ifdef HAVE_LSTAT
#define DO_USE_LSTAT 1
#define fvwm_lstat(x,y) lstat(x,y)
#else
#define DO_USE_LSTAT 0
#define fvwm_lstat(x,y) -1
#endif

#ifdef HAVE_X11_XKBLIB_H
#include <X11/XKBlib.h>
#define fvwm_KeycodeToKeysym(d, k, l, g) \
	(XkbKeycodeToKeysym((d), (k), (g), (l)))
#else
#define fvwm_KeycodeToKeysym(d, k, x, i) (XKeycodeToKeysym((d), (k), (i)))
#endif

/* A macro that touches a variable in a compiler independent way to suppress
 * warnings. */
#define SUPPRESS_UNUSED_VAR_WARNING(x) \
do { void *p; p = (void *)&x; (void)p; } while (0);
])

# mainly for fvwm-config
AC_SUBST(with_bidi)
AC_SUBST(with_gdkimlib)
AC_SUBST(with_gettext)
AC_SUBST(with_iconv)
AC_SUBST(with_perllib)
AC_SUBST(with_png)
AC_SUBST(with_rsvg)
AC_SUBST(with_readline)
AC_SUBST(with_rplay)
AC_SUBST(with_shape)
AC_SUBST(with_shm)
AC_SUBST(with_sm)
AC_SUBST(with_stroke)
AC_SUBST(with_xcursor)
AC_SUBST(with_xft)
AC_SUBST(with_xinerama)
AC_SUBST(with_xrender)
AC_SUBST(with_xpm)

dnl with autoconf <2.60 this is needed
AC_SUBST(datarootdir)
AC_SUBST(docdir)

LOCAL_BUGADDR=${FVWM_BUGADDR-${USER-${LOGNAME-`whoami`}}}
AC_SUBST(LOCAL_BUGADDR)

AC_OUTPUT(
	Makefile
	libs/Makefile
	fvwm/Makefile
	modules/Makefile
dnl
	bin/fvwm-bug.1
	bin/fvwm-config.1
	bin/fvwm-convert-2.6.1
	bin/fvwm-menu-desktop.1
dnl	bin/fvwm-menu-directory.1
dnl	bin/fvwm-menu-headlines.1
dnl	bin/fvwm-menu-xlock.1
dnl	bin/fvwm-perllib.1
	bin/fvwm-root.1
	modules/FvwmAnimate/FvwmAnimate.1
	modules/FvwmAuto/FvwmAuto.1
	modules/FvwmBacker/FvwmBacker.1
	modules/FvwmBanner/FvwmBanner.1
	modules/FvwmButtons/FvwmButtons.1
	modules/FvwmCommand/FvwmCommand.1
	modules/FvwmConsole/FvwmConsole.1
	modules/FvwmConsole/FvwmConsoleC.pl.1
	modules/FvwmCpp/FvwmCpp.1
	modules/FvwmEvent/FvwmEvent.1
	modules/FvwmForm/FvwmForm.1
	modules/FvwmIconMan/FvwmIconMan.1
	modules/FvwmIdent/FvwmIdent.1
	modules/FvwmM4/FvwmM4.1
	modules/FvwmPager/FvwmPager.1
dnl	modules/FvwmPerl/FvwmPerl.1
	modules/FvwmProxy/FvwmProxy.1
	modules/FvwmRearrange/FvwmRearrange.1
	modules/FvwmScript/FvwmScript.1
dnl	modules/FvwmWindowMenu/FvwmWindowMenu.1
dnl
	bin/Makefile
	bin/fvwm-config
	bin/fvwm-bug
	bin/fvwm-perllib
	bin/fvwm-menu-xlock
	bin/fvwm-menu-directory
	bin/fvwm-menu-desktop
	bin/fvwm-menu-headlines
	bin/fvwm-convert-2.6
dnl
	utils/Makefile
	perllib/Makefile
	perllib/General/Makefile
	perllib/FVWM/Makefile
	perllib/FVWM/Module/Makefile
	perllib/FVWM/Tracker/Makefile
	perllib/FVWM/Module.pm
	default-config/Makefile
	doc/fvwm.ent
	doc/footer.html
	doc/Makefile
	doc/fvwm/Makefile
	doc/commands/Makefile
	doc/docbook-xml/Makefile
	doc/docbook-xml/ent/Makefile
	doc/docbook-xsl/Makefile
	doc/docbook-xsl/common/Makefile
	doc/docbook-xsl/manpages/Makefile
	doc/docbook-xsl/profiling/Makefile
	doc/docbook-xsl/highlighting/Makefile
	doc/docbook-xsl/lib/Makefile
	doc/docbook-xsl/html/Makefile
	doc/images/Makefile
	doc/images/svg_rendering/Makefile
	docs/Makefile
	tests/Makefile
	tests/hints/Makefile
	po/Makefile
dnl
	modules/FvwmAnimate/Makefile
	modules/FvwmAuto/Makefile
	modules/FvwmBacker/Makefile
	modules/FvwmBanner/Makefile
	modules/FvwmButtons/Makefile
	modules/FvwmCommand/Makefile
	modules/FvwmCommand/scripts/Makefile
	modules/FvwmConsole/Makefile
	modules/FvwmConsole/FvwmConsoleC.pl
	modules/FvwmCpp/Makefile
	modules/FvwmEvent/Makefile
	modules/FvwmForm/Makefile
	modules/FvwmIconMan/Makefile
	modules/FvwmIdent/Makefile
	modules/FvwmM4/Makefile
	modules/FvwmPager/Makefile
	modules/FvwmPerl/Makefile
	modules/FvwmPerl/FvwmPerl
	modules/FvwmProxy/Makefile
	modules/FvwmRearrange/Makefile
	modules/FvwmScript/Makefile
	modules/FvwmScript/Scripts/Makefile
	modules/FvwmScript/Widgets/Makefile
)

# ---------------------------------------------------------------------------
# Original config.status is patched to make generated scripts executable.
sed 's/> $ac_file/> $ac_file; test x"`head -1 $ac_file | cut -c1-2`" = x"#!" \&\& chmod a+rx $ac_file/' <config.status >config.status-t && \
mv config.status-t config.status
chmod +x config.status
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
cat >config.status-t <<EOF
#! /bin/sh

# Original config.status is patched to satisfy processing *.in with 'make -j'.
# This bug seems to be partially fixed in autoconf-2.50 (not in all places).
UNIQUE_POSTFIX=\$\$

EOF
sed 's/conftest/conftest\${UNIQUE_POSTFIX}/g' <config.status >>config.status-t && \
mv config.status-t config.status
chmod +x config.status
# ---------------------------------------------------------------------------

eval my_bindir="`eval echo ${bindir}`"
eval my_mandir="`eval echo ${mandir}`"
eval my_moddir="`eval echo ${FVWM_MODULEDIR}`"; eval my_moddir="$my_moddir"
eval my_datdir="`eval echo ${FVWM_DATADIR}`"
eval my_docdir="`eval echo ${FVWM_DOCDIR}`"
eval my_plldir="`eval echo ${FVWM_PERLLIBDIR}`"
test x"$FVWM_PERLLIB" = x && my_plldir="(Not installed) $my_plldir"
eval my_localedir="`eval echo ${LOCALEDIR}`"
test x"$USE_NLS" = xno && my_localedir="(Not installed) $my_localdir"

echo "
Fvwm Configuration:

  Version:     $VERSION$VERSIONINFO

  Executables: $my_bindir
  Man pages:   $my_mandir
  Modules:     $my_moddir
  Data files:  $my_datdir
  Doc files:   $my_docdir
  Perl lib:    $my_plldir
  Locale msg:  $my_localedir $INST_LINGUAS

  With Asian bi-direct. text support? $with_bidi$problem_bidi
  With Gettext Native Lang support?   $with_gettext$problem_gettext
  With Iconv support?                 $with_iconv_type$problem_iconv
  With Mouse strokes (gestures)?      $with_stroke$problem_stroke
  With ReadLine sup. in FvwmConsole?  $with_readline$problem_readline
  With RPlay support in FvwmEvent?    $with_rplay$problem_rplay
  With Shaped window support?         $with_shape$problem_shape
  With Shared memory for XImage?      $with_shm$problem_shm
  With Session Management support?    $with_sm$problem_sm
  With SVG image support?             $with_rsvg$problem_rsvg
  With Xcursor support?               $with_xcursor$problem_xcursor
  With Xinerama multi-head support?   $with_xinerama$problem_xinerama
  With Xft anti-alias font support?   $with_xft$problem_xft
  With XPM image support?             $with_xpm$problem_xpm
  With Xrender image support?         $with_xrender$problem_xrender
  Build man pages?                    $with_mandoc$problem_mandoc
  Build HTML documentation?           $with_htmldoc$problem_htmldoc

"

case "$with_xinerama_emulation" in
  yes) echo "  Xinerama emulation enabled." ;;
esac
case "$ac_cv_dmalloc" in
  yes) echo "  DMALLOC heap debugging library enabled." ;;
esac
case "$ac_cv_efence" in
  yes) echo "  EFENCE debugging library enabled." ;;
esac
