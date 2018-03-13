dnl Convenience macros


dnl new version of FUNC_SELECT
dnl - submitted to autoconf maintainer; expected to appear in next version

AC_DEFUN([AC_FUNC_SELECT],
[AC_CHECK_FUNCS(select)
if test "$ac_cv_func_select" = yes; then
  AC_CHECK_HEADERS(unistd.h sys/types.h sys/time.h sys/select.h sys/socket.h)
  AC_MSG_CHECKING([argument types of select()])
  AC_CACHE_VAL(ac_cv_type_fd_set_size_t,dnl
    [AC_CACHE_VAL(ac_cv_type_fd_set,dnl
      [for ac_cv_type_fd_set in 'fd_set' 'int' 'void'; do
        for ac_cv_type_fd_set_size_t in 'int' 'size_t' 'unsigned long' 'unsigned'; do
	  for ac_type_timeval in 'struct timeval' 'const struct timeval'; do
            AC_TRY_COMPILE(dnl
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif],
[extern select ($ac_cv_type_fd_set_size_t,
 $ac_cv_type_fd_set *,	$ac_cv_type_fd_set *, $ac_cv_type_fd_set *,
 $ac_type_timeval *);],
[ac_found=yes ; break 3],ac_found=no)
          done
        done
      done
    ])dnl AC_CACHE_VAL
  ])dnl AC_CACHE_VAL
  if test "$ac_found" = no; then
    AC_MSG_ERROR([can not determine argument types])
  fi

  AC_MSG_RESULT([select($ac_cv_type_fd_set_size_t,$ac_cv_type_fd_set *,...)])
  AH_TEMPLATE([fd_set_size_t],
  [Define to the type used in argument 1 `select'.  Usually this is an `int'.])
  AC_DEFINE_UNQUOTED(fd_set_size_t, $ac_cv_type_fd_set_size_t)
  ac_cast=
  if test "$ac_cv_type_fd_set" != fd_set; then
    # Arguments 2-4 are not fd_set.  Some weirdo systems use fd_set type for
    # FD_SET macros, but insist that you cast the argument to select.  I don't
    # understand why that might be, but it means we cannot define fd_set.
    AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])fd_set[^a-zA-Z_0-9]>>dnl
changequote([,]),dnl
[#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif],dnl
    # We found fd_set type in a header, need special cast
    ac_cast="($ac_cv_type_fd_set *)",dnl
    # No fd_set type; it is safe to define it
    AH_TEMPLATE([fd_set],
  [Define to the type used in arguments 2-4 of `select', if not set by system
  headers.])
    AC_DEFINE_UNQUOTED(fd_set,$ac_cv_type_fd_set))
  fi
  AH_TEMPLATE([SELECT_FD_SET_CAST],
  [Define a suitable cast for arguments 2-4 of `select'.  On most systems,
   this will be the empty string, as select usually takes pointers to fd_set.])
  AC_DEFINE_UNQUOTED(SELECT_FD_SET_CAST,$ac_cast)
fi
])



dnl Checking for typedefs, with extra headers


dnl pds_CHECK_TYPE(TYPE, DEFAULT, [HEADERS], [comment])
AC_DEFUN([pds_CHECK_TYPE],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(ac_cv_type_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
$3], ac_cv_type_[$1]=yes, ac_cv_type_[$]1=no)])dnl
AC_MSG_RESULT($ac_cv_type_[$1])
if test $ac_cv_type_[$1] = no; then
  AH_TEMPLATE([$1],[$4])
  AC_DEFINE_UNQUOTED($1, $2)
fi
])


dnl Configure-time switch with default
dnl
dnl Each switch defines an --enable-FOO and --disable-FOO option in
dnl the resulting configure script.
dnl
dnl Usage:
dnl smr_SWITCH(name, description, default, pos-def, neg-def,
dnl            pos-def-comment, neg-def-comment)
dnl
dnl where:
dnl
dnl name        name of switch; generates --enable-name & --disable-name
dnl             options
dnl description help string is set to this prefixed by "enable" or
dnl             "disable", whichever is the non-default value
dnl default     either "on" or "off"; specifies default if neither
dnl             --enable-name nor --disable-name is specified
dnl pos-def     a symbol to AC_DEFINE if switch is on (optional)
dnl neg-def     a symbol to AC_DEFINE if switch is off (optional)
dnl
AC_DEFUN([smr_SWITCH], [
    AC_MSG_CHECKING(whether to enable [$2])
    AC_ARG_ENABLE(
        $1,
	ifelse($3, on,
		[AC_HELP_STRING([--disable-$1], [disable $2])],
		[AC_HELP_STRING([--enable-$1], [enable $2])]
	),
        ifelse([$4], , , [AH_TEMPLATE([$4],[$6])])
        ifelse([$5], , , [AH_TEMPLATE([$5],[$7])])
        [ if test "$enableval" = yes; then
            AC_MSG_RESULT(yes)
            ifelse($4, , , [AC_DEFINE($4)])
        else
            AC_MSG_RESULT(no)
            ifelse($5, , , [AC_DEFINE($5)])
        fi ],
        ifelse($3, on,
           [ AC_MSG_RESULT(yes)
            ifelse($4, , , [AC_DEFINE($4)]) ],
           [ AC_MSG_RESULT(no)
            ifelse($5, , , [AC_DEFINE($5)])])
	)
])


dnl Allow argument for optional libraries; wraps AC_ARG_WITH, to
dnl provide a "--with-foo-library" option in the configure script, where foo
dnl is presumed to be a library name.  The argument given by the user
dnl (i.e. "bar" in ./configure --with-foo-library=bar) may be one of three
dnl things:
dnl     * boolean (no, yes or blank): whether to use library or not
dnl     * file: assumed to be the name of the library
dnl     * directory: assumed to *contain* the library
dnl
dnl The argument is sanity-checked.  If all is well, two variables are
dnl set: "with_foo" (value is yes, no, or maybe), and "foo_LIBS" (value
dnl is either blank, a file, -lfoo, or '-L/some/dir -lfoo').  The idea
dnl is: the first tells you whether the library is to be used or not
dnl (or the user didn't specify one way or the other) and the second
dnl to put on the command line for linking with the library.
dnl
dnl Usage:
dnl smr_ARG_WITHLIB(name, libname, description)
dnl
dnl name                name for --with argument ("foo" for libfoo)
dnl libname             (optional) actual name of library,
dnl                     if different from name
dnl description         (optional) used to construct help string
dnl
AC_DEFUN([smr_ARG_WITHLIB], [

ifelse($2, , smr_lib=[$1], smr_lib=[$2])

AC_ARG_WITH([$1]-library,
ifelse($3, ,
AS_HELP_STRING([--with-$1-library[=PATH]],[use $1 library]),
AS_HELP_STRING([--with-$1-library[=PATH]],[use $1 library ($3)])),
[
    if test "$withval" = yes; then
        with_[$1]=yes
        [$1]_LIBS="-l${smr_lib}"
    elif test "$withval" = no; then
        with_[$1]=no
        [$1]_LIBS=
    else
        with_[$1]=yes
        if test -f "$withval"; then
            [$1]_LIBS=$withval
        elif test -d "$withval"; then
            [$1]_LIBS="-L$withval -l${smr_lib}"
        else
            AC_MSG_ERROR([--with-$1-library argument must be yes/no, file or directory])
        fi
    fi
], [
    with_[$1]=maybe
    [$1]_LIBS="-l${smr_lib}"
])])


dnl Check if the include files for a library are accessible, and
dnl define the variable "name_CFLAGS" with the proper "-I" flag for
dnl the compiler.  The user has a chance to specify the includes
dnl location, using "--with-foo-includes".
dnl
dnl This should be used *after* smr_ARG_WITHLIB *and* AC_CHECK_LIB are
dnl successful.
dnl
dnl Usage:
dnl smr_ARG_WITHINCLUDES(name, header, extra-flags)
dnl
dnl name                library name, MUST same as used with smr_ARG_WITHLIB
dnl header              a header file required for using the lib
dnl extra-flags         (optional) flags required when compiling the
dnl                     header, typically more includes; for ex. X_CFLAGS
dnl
AC_DEFUN([smr_ARG_WITHINCLUDES], [

AC_ARG_WITH([$1]-includes,
AS_HELP_STRING([--with-$1-includes=DIR],[set directory for $1 headers]),
[
    if test -d "$withval"; then
        [$1]_CFLAGS="-I${withval}"
    else
        AC_MSG_ERROR([argument must be a directory])
    fi])

    dnl We need to put the given include directory into CPPFLAGS temporarily, but
    dnl then restore CPPFLAGS to its old value.
    dnl
    smr_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS ${[$1]_CFLAGS}"
    ifelse($3, , , CPPFLAGS="$CPPFLAGS [$3]")

    AC_CHECK_HEADERS($2)

    CPPFLAGS=$smr_save_CPPFLAGS
])


dnl Probe for an optional library.  This macro creates both
dnl --with-foo-library and --with-foo-includes options for the configure
dnl script.  If --with-foo-library is *not* specified, the default is to
dnl probe for the library, and use it if found.
dnl
dnl Usage:
dnl smr_CHECK_LIB(name, libname, desc, func, header, x-libs, x-flags)
dnl
dnl name        name for --with options
dnl libname     (optional) real name of library, if different from
dnl             above
dnl desc        (optional) short descr. of library, for help string
dnl func        function of library, to probe for
dnl header      (optional) header required for using library
dnl x-libs      (optional) extra libraries, if needed to link with lib
dnl x-flags     (optional) extra flags, if needed to include header files
dnl
AC_DEFUN([smr_CHECK_LIB],
[
ifelse($2, , smr_lib=[$1], smr_lib=[$2])
ifelse($5, , , smr_header=[$5])
smr_ARG_WITHLIB($1,$2,$3)
if test "$with_$1" != no; then
    AC_CHECK_LIB(ifelse($2, , $1, $2), $4,
        smr_havelib=yes,
        smr_havelib=no; problem_$1=": Can't find working lib$smr_lib",
        ifelse($6, , ${$1_LIBS}, [${$1_LIBS} $6]))
    if test "$smr_havelib" = yes -a "$smr_header" != ""; then
        smr_ARG_WITHINCLUDES($1, $smr_header, $7)
        smr_safe=`echo "$smr_header" | sed 'y%./+-%__p_%'`
        if eval "test \"`echo '$ac_cv_header_'$smr_safe`\" != yes"; then
            smr_havelib=no
            problem_$1=": Can't find working $smr_header"
        fi
    fi
    if test "$smr_havelib" = yes; then
        with_$1=yes
        problem_$1=
    else
        $1_LIBS=
        $1_CFLAGS=
        with_$1=no
    fi
else
    problem_$1=": Explicitly disabled"
fi])


dnl Defines a boolean variable for config.h depending on a condition.
dnl
dnl Usage:
dnl mg_DEFINE_IF_NOT(c-code, cpp-if-cond, var-name, extra-flags, var-comment)
dnl
dnl c-code       the first code part inside main()
dnl cpp-if-cond  boolean preprocessor condition
dnl var-name     this variable will be defined if the given condition is false
dnl extra-flags  (optional) extra flags for compiling, typically more -I glags
dnl
dnl Example:
dnl mg_DEFINE_IF_NOT([#include <features.h>], [defined __USE_BSD], [NON_BSD])
dnl
AC_DEFUN([mg_DEFINE_IF_NOT], [
mg_save_CPPFLAGS="$CPPFLAGS"
ifelse($4, , , CPPFLAGS="$CPPFLAGS [$4]")

AH_TEMPLATE([$3],[$5])
AC_TRY_RUN([
#include <stdio.h>
int main(int c, char **v) {
$1
#if $2
  return 0;
#else
  return 1;
#endif
}
], [:], [AC_DEFINE($3)])

CPPFLAGS="$mg_save_CPPFLAGS"
])


# Check for gdk-imlib
AC_DEFUN([AM_PATH_GDK_IMLIB],
[dnl
dnl Get the cflags and libraries from the imlib-config script
dnl
AC_ARG_WITH(imlib-prefix,AS_HELP_STRING([--with-imlib-prefix=PFX],[prefix for IMLIB files (optional)]),
            imlib_prefix="$withval", imlib_prefix="")
AC_ARG_WITH(imlib-exec-prefix,AS_HELP_STRING([--with-imlib-exec-prefix=PFX],[exec prefix for IMLIB files (optional)]),
            imlib_exec_prefix="$withval", imlib_exec_prefix="")
AC_ARG_ENABLE(imlibtest,AS_HELP_STRING([--disable-imlibtest],[do not try to compile and run a test IMLIB program]),
            , enable_imlibtest=yes)

  if test x$imlib_exec_prefix != x ; then
     imlib_args="$imlib_args --exec-prefix=$imlib_exec_prefix"
     if test x${IMLIBCONF+set} != xset ; then
        IMLIBCONF=$imlib_exec_prefix/bin/imlib-config
     fi
  fi
  if test x$imlib_prefix != x ; then
     imlib_args="$imlib_args --prefix=$imlib_prefix"
     if test x${IMLIBCONF+set} != xset ; then
        IMLIBCONF=$imlib_prefix/bin/imlib-config
     fi
  fi

  AC_PATH_PROG(IMLIBCONF, imlib-config, no)
  min_imlib_version=ifelse([$1], ,1.8.1,$1)
  AC_MSG_CHECKING(for GDK IMLIB - version >= $min_imlib_version)
  no_imlib=""
  if test "$IMLIBCONF" = "no" ; then
    no_imlib=yes
  else
    GDK_IMLIB_CFLAGS=`$IMLIBCONF $imlibconf_args --cflags-gdk`
    GDK_IMLIB_LIBS=`$IMLIBCONF $imlibconf_args --libs-gdk`

    imlib_major_version=`$IMLIBCONF $imlib_args --version | \
           sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\1/'`
    imlib_minor_version=`$IMLIBCONF $imlib_args --version | \
           sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\2/'`
    if test "x$enable_imlibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
      LIBS="$LIBS $GDK_IMLIB_LIBS"
dnl
dnl Now check if the installed IMLIB is sufficiently new. (Also sanity
dnl checks the results of imlib-config to some extent
dnl
      rm -f conf.imlibtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <gdk_imlib.h>

/* migo: originally it was GdkImLibColor with incorrect spelling */
GdkImlibImage testimage;

int main ()
{
  int major, minor;
  char *tmp_version;

  system ("touch conf.gdkimlibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_imlib_version");
  if (sscanf(tmp_version, "%d.%d", &major, &minor) != 2) {
     printf("%s, bad version string\n", "$min_imlib_version");
     exit(1);
   }

    if (($imlib_major_version > major) ||
        (($imlib_major_version == major) && ($imlib_minor_version > minor)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'imlib-config --version' returned %d.%d, but the minimum version\n", $imlib_major_version, $imlib_minor_version);
      printf("*** of IMLIB required is %d.%d. If imlib-config is correct, then it is\n", major, minor);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If imlib-config was wrong, set the environment variable IMLIBCONF\n");
      printf("*** to point to the correct copy of imlib-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_imlib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_imlib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$IMLIBCONF" = "no" ; then
                       (echo "*** The imlib-config script installed by IMLIB could not be found" >&5) 2>/dev/null || \
       echo "*** The imlib-config script installed by IMLIB could not be found"
                       (echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in" >&5) 2>/dev/null || \
       echo "*** If IMLIB was installed in PREFIX, make sure PREFIX/bin is in"
                       (echo "*** your path, or set the IMLIBCONF environment variable to the" >&5) 2>/dev/null || \
       echo "*** your path, or set the IMLIBCONF environment variable to the"
                       (echo "*** full path to imlib-config." >&5) 2>/dev/null || \
       echo "*** full path to imlib-config."
     else
       if test -f conf.gdkimlibtest ; then
        :
       else
                          (echo "*** Could not run IMLIB test program, checking why..." >&5) 2>/dev/null || \
          echo "*** Could not run IMLIB test program, checking why..."
          CFLAGS="$CFLAGS $GDK_IMLIB_CFLAGS"
          LIBS="$LIBS $GDK_IMLIB_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <gdk_imlib.h>
],      [ return 0; ],
        [                 (echo "*** The test program compiled, but did not run. This usually means" >&5) 2>/dev/null || \
          echo "*** The test program compiled, but did not run. This usually means"
                          (echo "*** that the run-time linker is not finding IMLIB or finding the wrong" >&5) 2>/dev/null || \
          echo "*** that the run-time linker is not finding IMLIB or finding the wrong"
                          (echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your" >&5) 2>/dev/null || \
          echo "*** version of IMLIB. If it is not finding IMLIB, you'll need to set your"
                          (echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point" >&5) 2>/dev/null || \
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
                          (echo "*** to the installed location  Also, make sure you have run ldconfig if that" >&5) 2>/dev/null || \
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
                          (echo "*** is required on your system" >&5) 2>/dev/null || \
          echo "*** is required on your system"
                          (echo "***" >&5) 2>/dev/null || \
          echo "***"
                          (echo "*** If you have an old version installed, it is best to remove it, although" >&5) 2>/dev/null || \
          echo "*** If you have an old version installed, it is best to remove it, although"
                          (echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" >&5) 2>/dev/null || \
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [                 (echo "*** The test program failed to compile or link. See the file config.log for the" >&5) 2>/dev/null || \
          echo "*** The test program failed to compile or link. See the file config.log for the"
                          (echo "*** exact error that occured. This usually means IMLIB was incorrectly installed" >&5) 2>/dev/null || \
          echo "*** exact error that occured. This usually means IMLIB was incorrectly installed"
                          (echo "*** or that you have moved IMLIB since it was installed. In the latter case, you" >&5) 2>/dev/null || \
          echo "*** or that you have moved IMLIB since it was installed. In the latter case, you"
                          (echo "*** may want to edit the imlib-config script: $IMLIBCONF" >&5) 2>/dev/null || \
          echo "*** may want to edit the imlib-config script: $IMLIBCONF"])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GDK_IMLIB_CFLAGS=""
     GDK_IMLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GDK_IMLIB_CFLAGS)
  AC_SUBST(GDK_IMLIB_LIBS)
  rm -f conf.gdkimlibtest
])

#
# check if iconv second argument use const char.
#
AC_DEFUN([ICONV_SECOND_ARG],[
	AC_MSG_CHECKING(check if second arg of iconv is const)
	AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#if defined(__STDC__)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], use_const=no, use_const=yes)
	AC_MSG_RESULT($use_const)
        AH_TEMPLATE([ICONV_ARG_CONST],
                    [define if second arg of iconv use const])
	if test "x$use_const" = "xyes"; then
		AC_DEFINE(ICONV_ARG_CONST, const)
	else
		AC_DEFINE(ICONV_ARG_CONST, )
	fi
])

#
# check for  locale_charset if libiconv is used
#
AC_DEFUN([CHECK_LIBCHARSET],[
	AC_MSG_CHECKING(check for libcharset)
	ac_save_CFLAGS="$CFLAGS"
      	ac_save_LIBS="$LIBS"
      	CFLAGS="$CFLAGS $iconv_CFLAGS"
      	LIBS="$LIBS $iconv_LIBS"
	AC_TRY_LINK([
#include <libcharset.h>],
[const char *c;
c = locale_charset ();
], r=yes, r=no)
	AC_MSG_RESULT($r)
	if test "x$r" = "xyes"; then
       		AC_DEFINE(HAVE_LIBCHARSET)
	fi
	CFLAGS="$ac_save_CFLAGS"
        LIBS="$ac_save_LIBS"
])

#-----------------------------------------------------------------------------
# pkg-config

dnl
dnl
AC_DEFUN([AM_CHECK_PKG_CONFIG],
[dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
AC_ARG_WITH(pkgconfig-prefix,
AS_HELP_STRING([--with-pkgconfig-prefix=PFX],[prefix where pkg-config is installed]),
            pkgconfig_config_prefix="$withval", pkgconfig_config_prefix="")
AC_ARG_WITH(pkgconfig-exec-prefix,
AS_HELP_STRING([--with-pkgconfig-exec-prefix=PFX],[exec prefix where pkg-config is installed]),
            pkgconfig_config_exec_prefix="$withval",pkgconfig_config_exec_prefix="")

if test x$pkgconfig_config_exec_prefix != x ; then
  pkgconfig_config_args="$pkgconfig_config_args --exec-prefix=$pkgconfig_config_exec_prefix"
  if test x${PKG_CONFIG+set} != xset ; then
    PKG_CONFIG=$pkgconfig_config_exec_prefix/bin/pkg-config
  fi
fi
if test x$pkgconfig_config_prefix != x ; then
  pkgconfig_config_args="$pkgconfig_config_args --prefix=$pkgconfig_config_prefix"
  if test x${PKG_CONFIG+set} != xset ; then
    PKG_CONFIG=$pkgconfig_config_prefix/bin/pkg-config
  fi
fi
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
])

#-----------------------------------------------------------------------------
# Configure paths for fontconfig
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor
# modified by olicha for fontconfig

dnl AM_CHECK_FC([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for fontconfig, and define FC_CFLAGS and FC_LIBS
dnl
AC_DEFUN([AM_CHECK_FC],
[dnl
dnl Get the cflags and libraries from the fontconfig-config script
dnl
AC_ARG_WITH(fontconfig-prefix,
AS_HELP_STRING([--with-fontconfig-prefix=PFX],[prefix where fontconfig is installed (for Xft2)]),
            fc_config_prefix="$withval", fc_config_prefix="")
AC_ARG_WITH(fontconfig-exec-prefix,
AS_HELP_STRING([--with-fontconfig-exec-prefix=PFX],[exec prefix where fontconfig is installed]),
            fc_config_exec_prefix="$withval", fc_config_exec_prefix="")
AC_ARG_ENABLE(fontconfigtest,
AS_HELP_STRING([--disable-fontconfigtest],[do not try to compile and run a test fontconfig program]),
            [], enable_fctest=yes)

if test x$fc_config_exec_prefix != x ; then
  fc_config_args="$fc_config_args --exec-prefix=$fc_config_exec_prefix"
  if test x${FC_CONFIG+set} != xset ; then
    FC_CONFIG=$fc_config_exec_prefix/bin/fontconfig-config
  fi
fi
if test x$fc_config_prefix != x ; then
  fc_config_args="$fc_config_args --prefix=$fc_config_prefix"
  if test x${FC_CONFIG+set} != xset ; then
    FC_CONFIG=$fc_config_prefix/bin/fontconfig-config
  fi
fi
AC_PATH_PROG(FC_CONFIG, fontconfig-config, no)

min_fc_version=ifelse([$1], ,1.0.1,$1)
AC_MSG_CHECKING(for Fontconfig - version >= $min_fc_version)
no_fc=""
pkg_config_fontconfig_exists=""

if test "$FC_CONFIG" = "no" ; then
  if test "x$PKG_CONFIG" != "xno" ; then
    if $PKG_CONFIG --exists 'fontconfig' ; then
      if $PKG_CONFIG --exists 'fontconfig >= $1' ; then
        FC_CFLAGS=`$PKG_CONFIG --cflags fontconfig`
        FC_LIBS=`$PKG_CONFIG --libs fontconfig`
      else
        no_fc=yes
        fc_config_is_lt=yes
      fi
    else
      pkg_config_fontconfig_exists="maybe"
      no_fc=yes
    fi
  else
    no_fc=yes
  fi
else
  FC_CFLAGS=`$FC_CONFIG $fc_config_args --cflags`
  FC_LIBS=`$FC_CONFIG $fc_config_args --libs`
  fc_config_major_version=`$FC_CONFIG $fc_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\1/'`
  fc_config_minor_version=`$FC_CONFIG $fc_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\2/'`
  fc_config_micro_version=`$FC_CONFIG $fc_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\3/'`
  fc_min_major_version=`echo $min_fc_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\1/'`
  fc_min_minor_version=`echo $min_fc_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\2/'`
  fc_min_micro_version=`echo $min_fc_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\3/'`
  fc_config_is_lt=no
  if test $fc_config_major_version -lt $fc_min_major_version ; then
    fc_config_is_lt=yes
  else
    if test $fc_config_major_version -eq $fc_min_major_version ; then
      if test $fc_config_minor_version -lt $fc_min_minor_version ; then
        fc_config_is_lt=yes
      else
        if test $fc_config_minor_version -eq $fc_min_minor_version ; then
          if test $fc_config_micro_version -lt $fc_min_micro_version ; then
            fc_config_is_lt=yes
          fi
        fi
      fi
    fi
  fi
  if test "x$fc_config_is_lt" = "xyes" ; then
    no_fc=yes
  fi
fi

if test "x$no_fc" = x ; then
  if test "x$enable_fctest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $FC_CFLAGS $FT2_CFLAGS"
    LIBS="$FC_LIBS $LIBS $FT2_LIBS"
dnl
dnl Sanity checks for the results of fontconfig-config/pkg-config to some extent
dnl
      AC_TRY_RUN([
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  FcBool result;

  result = FcInit();

  if (result)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
],, no_fc=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi
fi

if test "x$no_fc" = x; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
  if test "x$fc_config_is_lt" = "xyes"; then
    echo "*** Your Fontconfig package version is < $1"
  elif test "x$pkg_config_fontconfig_exists" = "xmaybe"; then
    echo "*** fontconfig was not found in the pkg-config search path."
    echo "*** either fontconfig is not installed or perhaps you should"
    echo "*** add the directory containing fontconfig.pc to the "
    echo "*** PKG_CONFIG_PATH environment variable."
  elif test "$FC_CONFIG" != "no"; then
    echo "*** The Fontconfig test program failed to run.  If your system uses"
    echo "*** shared libraries and they are installed outside the normal"
    echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
    echo "*** (or whatever is appropiate for your system) is correctly set."
  fi
  FC_CFLAGS=""
  FC_LIBS=""
  ifelse([$3], , :, [$3])
fi
AC_SUBST(FC_CFLAGS)
AC_SUBST(FC_LIBS)
])

#-----------------------------------------------------------------------------
# Configure paths for xft 2
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor
# modified by olicha for xft

dnl AM_CHECK_XFT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for xft, and define XFT_CFLAGS and XFT_LIBS
dnl
AC_DEFUN([AM_CHECK_XFT],
[dnl
dnl Get the cflags and libraries from the xft-config script
dnl
AC_ARG_WITH(xft-prefix,
AS_HELP_STRING([--with-xft-prefix=PFX],[prefix where Xft2 is installed (optional)]),
            xft_config_prefix="$withval", xft_config_prefix="")
AC_ARG_WITH(xft-exec-prefix,
AS_HELP_STRING([--with-xft-exec-prefix=PFX],[exec prefix where Xft2 is installed]),
            xft_config_exec_prefix="$withval", xft_config_exec_prefix="")
AC_ARG_ENABLE(xfttest,
AS_HELP_STRING([--disable-xfttest],[do not try to compile and run a test Xft program]),
            [], enable_xfttest=yes)

if test x$xft_config_exec_prefix != x ; then
  xft_config_args="$xft_config_args --exec-prefix=$xft_config_exec_prefix"
  if test x${XFT_CONFIG+set} != xset ; then
    XFT_CONFIG=$xft_config_exec_prefix/bin/xft-config
  fi
fi
if test x$xft_config_prefix != x ; then
  xft_config_args="$xft_config_args --prefix=$xft_config_prefix"
  if test x${XFT_CONFIG+set} != xset ; then
    XFT_CONFIG=$xft_config_prefix/bin/xft-config
  fi
fi
AC_PATH_PROG(XFT_CONFIG, xft-config, no)

min_xft_version=ifelse([$1], ,2.0.0,$1)
AC_MSG_CHECKING(for Xft - version >= $min_xft_version)
no_xft=""
pkg_config_xft_exists=""

if test "$XFT_CONFIG" = "no" ; then
  if test "x$PKG_CONFIG" != "xno" ; then
    if $PKG_CONFIG --exists 'xft' ; then
      if $PKG_CONFIG --exists 'xft >= $1' ; then
        XFT_CFLAGS=`$PKG_CONFIG --cflags xft`
        XFT_LIBS=`$PKG_CONFIG --libs xft`
      else
        no_xft=yes
        xft_config_is_lt=yes
      fi
    else
      pkg_config_xft_exists="maybe"
      no_xft=yes
    fi
  else
    no_xft=yes
  fi
else
  XFT_CFLAGS=`$XFT_CONFIG $xft_config_args --cflags`
  XFT_LIBS=`$XFT_CONFIG $xft_config_args --libs`
  xft_config_major_version=`$XFT_CONFIG $xft_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\1/'`
  xft_config_minor_version=`$XFT_CONFIG $xft_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\2/'`
  xft_config_micro_version=`$XFT_CONFIG $xft_config_args --version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\3/'`
  xft_min_major_version=`echo $min_xft_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\1/'`
  xft_min_minor_version=`echo $min_xft_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\2/'`
  xft_min_micro_version=`echo $min_xft_version | \
         sed 's/^[[^0-9.]]*\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*$/\3/'`
  xft_config_is_lt=no
  if test $xft_config_major_version -lt $xft_min_major_version ; then
    xft_config_is_lt=yes
  else
    if test $xft_config_major_version -eq $xft_min_major_version ; then
      if test $xft_config_minor_version -lt $xft_min_minor_version ; then
        xft_config_is_lt=yes
      else
        if test $xft_config_minor_version -eq $xft_min_minor_version ; then
          if test $xft_config_micro_version -lt $xft_min_micro_version ; then
            xft_config_is_lt=yes
          fi
        fi
      fi
    fi
  fi
  if test "x$xft_config_is_lt" = "xyes" ; then
    ifelse([$3], , :, [$3])
  fi
fi

if test "x$no_xft" = x ; then
  if test "x$enable_xfttest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$XFT_CFLAGS $CFLAGS"
    LIBS="$XFT_LIBS $LIBS"
dnl
dnl Sanity checks for the results of xft-config/pkg-config to some extent
dnl
      AC_TRY_RUN([
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  FcBool result = 1;

  result = XftInit(NULL);

  if (result)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
],, no_xft=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"
  fi
fi

if test "x$no_xft" = x; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
  if test "x$xft_config_is_lt" = "xyes"; then
    echo "*** Your xft2 package version is < $1"
  elif test "x$pkg_config_fontconfig_exists" = "xmaybe" ; then
    echo "*** xft2 was not found in the pkg-config search path."
    echo "*** either xft is not installed or perhaps you should"
    echo "*** add the directory containing xft.pc to the "
    echo "*** PKG_CONFIG_PATH environment variable."
  elif test "$XFT_CONFIG" = "no"; then
    echo "*** The xft-config script installed by Xft 2 could not be found."
    echo "*** If Xft 2 was installed in PREFIX, make sure PREFIX/bin is in"
    echo "*** your path, or set the XFT_CONFIG environment variable to the"
    echo "*** full path to xft-config."
  else
    echo "*** The Xft test program failed to run.  If your system uses"
    echo "*** shared libraries and they are installed outside the normal"
    echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
    echo "*** (or whatever is appropiate for your system) is correctly set."
  fi
  XFT_CFLAGS=""
  XFT_LIBS=""
  ifelse([$3], , :, [$3])
fi
AC_SUBST(XFT_CFLAGS)
AC_SUBST(XFT_LIBS)
])

#-----------------------------------------------------------------------------
# gettext stuff from the gettext package
#
# Authors: Ulrich Drepper <drepper@cygnus.com>, 1996.
# modified by the fvwm workers
#

AC_DEFUN([AM_GNU_FGETTEXT],
[
  AC_REQUIRE([AM_PO_SUBDIRS])dnl

  intl_LIBS=
  intl_CFLAGS=
  POSUB=

  found_gettext=yes

  dnl check for the necessary stuff in the libc
  dnl the pbs is that we can detect this stuff but in fact the included
  dnl libintl.h is from gettext
  dnl Moreover, we do not try to use other implementation, but we may try
  dnl one day
  $UNSET ac_cv_header_intl_h
  $UNSET ac_cv_func_gettext
  $UNSET ac_cv_func_bindtextdomain
  $UNSET ac_cv_func_textdomain
  dnl a "gnu extension"
  $UNSET ac_cv_func_dgettext
  #bind_textdomain_codeset
  AC_CHECK_HEADER(libintl.h,
    [AC_CHECK_FUNCS(gettext bindtextdomain textdomain dgettext,,
      found_gettext=no)], found_gettext=no)

  AC_MSG_CHECKING([for gnu gettext in libc])
  if test x"$found_gettext" = "xyes"; then
    problem_gettext=" (libc)"
    AC_MSG_RESULT([yes])
    AC_MSG_CHECKING(if a simple gettext program link)
    AC_TRY_LINK([
      #include <libintl.h>
      ],
      [const char *c; c = gettext("foo");],
      found_gettext=yes;problem_gettext=" (libc)", found_gettext=no)
    AC_MSG_RESULT($found_gettext)
  else
    AC_MSG_RESULT([no])
  fi

  if test x"$found_gettext" = xno; then
    dnl not found, check for libintl
    $UNSET ac_cv_header_intl_h
    $UNSET ac_cv_lib_intl_bindtextdomain
    $UNSET ac_cv_lib_intl_textdomain
    $UNSET ac_cv_lib_intl_dgettext
    smr_CHECK_LIB(intl, intl, for Native Language Support,
      bindtextdomain, libintl.h)
    if test x"$intl_LIBS" != x; then
      no_textdomain=no
      no_dgettext=no
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      AC_CHECK_LIB(intl, textdomain,, no_textdomain=yes,
        [$intl_LIBS $iconv_LIBS])
      if test "$no_textdomain" != "yes"; then
        AC_CHECK_LIB(intl, dgettext,, no_dgettext=yes, [$intl_LIBS $iconv_LIBS])
        if test "$no_dgettext" != "yes"; then
          CFLAGS="$CFLAGS $intl_LIBS $iconv_LIBS"
          LIBS="$LIBS $intl_LIBS $iconv_LIBS"
          AC_MSG_CHECKING(if a simple gettext program link)
          AC_TRY_LINK([
          #include <libintl.h>
          ],
          [const char *c; c = gettext("foo");],
          found_gettext=yes;problem_gettext=" (intl library)", found_gettext=no)
          AC_MSG_RESULT($found_gettext)
        fi
      fi
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi
  fi

  if test "$found_gettext" = "yes"; then
    dnl Mark actions to use GNU gettext tools.
    CATOBJEXT=.gmo
    USE_NLS=yes
    dnl We need to process the po/ directory.
    POSUB=po
  else
    USE_NLS=no
  fi

  dnl Make the po/ variables we use known to autoconf
])


dnl Checks for all prerequisites of the po subdirectory,
dnl except for USE_NLS.
AC_DEFUN([AM_PO_SUBDIRS],
[
  AC_REQUIRE([AC_PROG_MAKE_SET])dnl
  AC_REQUIRE([AC_PROG_INSTALL])dnl
  AC_REQUIRE([AM_MKINSTALLDIRS])dnl

  dnl Perform the following tests also if --disable-nls has been given,
  dnl because they are needed for "make dist" to work.

  dnl Search for GNU msgfmt in the PATH.
  dnl The first test excludes Solaris msgfmt and early GNU msgfmt versions.
  dnl The second test excludes FreeBSD msgfmt.
  AM_PATH_PROG_WITH_TEST(MSGFMT, msgfmt,
    [$ac_dir/$ac_word --statistics /dev/null >/dev/null 2>&1 &&
     (if $ac_dir/$ac_word --statistics /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi)],
    :)
  AC_PATH_PROG(GMSGFMT, gmsgfmt, $MSGFMT)

  dnl Search for GNU xgettext 0.11 or newer in the PATH.
  dnl The first test excludes Solaris xgettext and early GNU xgettext versions.
  dnl The second test excludes FreeBSD xgettext.
  AM_PATH_PROG_WITH_TEST(XGETTEXT, xgettext,
    [$ac_dir/$ac_word --omit-header --copyright-holder= /dev/null >/dev/null 2>&1 &&
     (if $ac_dir/$ac_word --omit-header --copyright-holder= /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi)],
    :)
  dnl Remove leftover from FreeBSD xgettext call.
  rm -f messages.po

  dnl Search for GNU msgmerge 0.11 or newer in the PATH.
  AM_PATH_PROG_WITH_TEST(MSGMERGE, msgmerge,
    [$ac_dir/$ac_word --update -q /dev/null /dev/null >/dev/null 2>&1], :)

  dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
  dnl Test whether we really found GNU msgfmt.
  if test "$GMSGFMT" != ":"; then
    dnl If it is no GNU msgfmt we define it as : so that the
    dnl Makefiles still can work.
    if $GMSGFMT --statistics /dev/null >/dev/null 2>&1 &&
       (if $GMSGFMT --statistics /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi); then
      : ;
    else
      GMSGFMT=`echo "$GMSGFMT" | sed -e 's,^.*/,,'`
      AC_MSG_RESULT(
        [found $GMSGFMT program is not GNU msgfmt; ignore it])
      GMSGFMT=":"
    fi
  fi

  dnl This could go away some day; the PATH_PROG_WITH_TEST already does it.
  dnl Test whether we really found GNU xgettext.
  if test "$XGETTEXT" != ":"; then
    dnl If it is no GNU xgettext we define it as : so that the
    dnl Makefiles still can work.
    if $XGETTEXT --omit-header --copyright-holder= /dev/null >/dev/null 2>&1 &&
       (if $XGETTEXT --omit-header --copyright-holder= /dev/null 2>&1 >/dev/null | grep usage >/dev/null; then exit 1; else exit 0; fi); then
      : ;
    else
      AC_MSG_RESULT(
        [found xgettext program is not GNU xgettext; ignore it])
      XGETTEXT=":"
    fi
    dnl Remove leftover from FreeBSD xgettext call.
    rm -f messages.po
  fi

  AC_PATH_PROG(MSGUNIQ, msguniq, $MSGUNIQ)

  AC_MSG_CHECKING([for NLS fvwm messages catalogs])
  AC_MSG_RESULT([$ALL_LINGUAS])
  POFILES=
  GMOFILES=
  UPDATEPOFILES=
  DUMMYPOFILES=
  for lang in $ALL_LINGUAS; do
    for dom in $ALL_DOMAINS; do
      POFILES="$POFILES $dom.$lang.po"
      GMOFILES="$GMOFILES $dom.$lang.gmo"
      UPDATEPOFILES="$UPDATEPOFILES $dom.$lang.po-update"
      DUMMYPOFILES="$DUMMYPOFILES $dom.$lang.nop"
    done
  done
  # CATALOGS depends on both $ac_dir and the user's LINGUAS environment variable.
  INST_LINGUAS=
  AC_MSG_CHECKING([for NLS desired catalogs to be installed])
  #if test "%UNSET%" != "$LINGUAS"; then
  # FIXME: How to check if LINGUAS has been *set* to ""
  if test -n "$LINGUAS"; then
    AC_MSG_RESULT([$LINGUAS])
  else
    AC_MSG_RESULT([all])
  fi
  AC_MSG_CHECKING([for NLS messages catalogs to be installed])
  if test -n "$ALL_LINGUAS"; then
    for presentlang in $ALL_LINGUAS; do
      useit=no
      #if test "%UNSET%" != "$LINGUAS"; then
      if test -n "$LINGUAS"; then
        desiredlanguages="$LINGUAS"
      else
        desiredlanguages="$ALL_LINGUAS"
      fi
      for desiredlang in $desiredlanguages; do
        # Use the presentlang catalog if desiredlang is
        #   a. equal to presentlang, or
        #   b. a variant of presentlang (because in this case,
        #      presentlang can be used as a fallback for messages
        #      which are not translated in the desiredlang catalog).
        case "$desiredlang" in
          "$presentlang"*) useit=yes;;
        esac
      done
      if test $useit = yes; then
        INST_LINGUAS="$INST_LINGUAS $presentlang"
      fi
    done
  fi
  AC_MSG_RESULT([$INST_LINGUAS])
  CATALOGS=
  if test -n "$INST_LINGUAS"; then
    for lang in $INST_LINGUAS; do
      CATALOGS="$CATALOGS $lang.gmo"
    done
  fi

])


AC_DEFUN([AM_MKINSTALLDIRS],
[
  dnl If the AC_CONFIG_AUX_DIR macro for autoconf is used we possibly
  dnl find the mkinstalldirs script in another subdir but $(top_srcdir).
  dnl Try to locate is.
  MKINSTALLDIRS=
  if test -n "$ac_aux_dir"; then
    MKINSTALLDIRS="$ac_aux_dir/mkinstalldirs"
  fi
  if test -z "$MKINSTALLDIRS"; then
    MKINSTALLDIRS="\$(top_srcdir)/mkinstalldirs"
  fi
])

# Search path for a program which passes the given test.

dnl AM_PATH_PROG_WITH_TEST(VARIABLE, PROG-TO-CHECK-FOR,
dnl   TEST-PERFORMED-ON-FOUND_PROGRAM [, VALUE-IF-NOT-FOUND [, PATH]])
AC_DEFUN([AM_PATH_PROG_WITH_TEST],
[# Extract the first word of "$2", so it can be a program name with args.
set dummy $2; ac_word=[$]2
AC_MSG_CHECKING([for $ac_word])
AC_CACHE_VAL(ac_cv_path_$1,
[case "[$]$1" in
  /*)
  ac_cv_path_$1="[$]$1" # Let the user override the test with a path.
  ;;
  *)
  IFS="${IFS=   }"; ac_save_ifs="$IFS"; IFS="${IFS}:"
  for ac_dir in ifelse([$5], , $PATH, [$5]); do
    test -z "$ac_dir" && ac_dir=.
    if test -f $ac_dir/$ac_word; then
      if [$3]; then
        ac_cv_path_$1="$ac_dir/$ac_word"
        break
      fi
    fi
  done
  IFS="$ac_save_ifs"
dnl If no 4th arg is given, leave the cache variable unset,
dnl so AC_PATH_PROGS will keep looking.
ifelse([$4], , , [  test -z "[$]ac_cv_path_$1" && ac_cv_path_$1="$4"
])dnl
  ;;
esac])dnl
$1="$ac_cv_path_$1"
if test ifelse([$4], , [-n "[$]$1"], ["[$]$1" != "$4"]); then
  AC_MSG_RESULT([$]$1)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST($1)dnl
])


dnl Usage: AM_GNU_GETTEXT_VERSION([gettext-version])
AC_DEFUN([AM_GNU_GETTEXT_VERSION], [])


#-----------------------------------------------------------------------------
# Safety check for mkstemp
#
AC_DEFUN([AM_SAFETY_CHECK_MKSTEMP],[
  AC_CHECK_FUNCS(mkstemp)
  has_safety_mkstemp=no
  AC_MSG_CHECKING(if mkstemp is safe)
  if test x$ac_cv_func_mkstemp != xno; then
    AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
  char template[128];
  char template_orig[128];
  int fd;

  sprintf(template, "configure-mkstemp-test.XXXXXX");
  strcpy(template_orig, template);
  fd = mkstemp(template);
  if (fd == -1)
  {
    /* could not create temp file */
    return 1;
  }
  if (strcmp(template, template_orig) == 0)
  {
    /* mkstemp broken */
    return 2;
  }
  if (close(fd) != 0)
  {
    /* doh! */
    return 3;
  }
  if (unlink(template))
  {
     return 4;
  }
  /* mkstemp works properly */
  return 0;
}
    ],
    [has_safety_mkstemp=yes], [has_safety_mkstemp=no])
  fi
  AH_TEMPLATE([HAVE_SAFETY_MKSTEMP],[Enable the use of mkstemp])
  if test x$has_safety_mkstemp = xno; then
    AC_MSG_RESULT(no, use our mkstemp)
  else
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_SAFETY_MKSTEMP)
  fi
])
