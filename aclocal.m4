dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl Convenience macros


dnl Checking for typedefs, with extra headers


dnl pds_CHECK_TYPE(TYPE, DEFAULT, [HEADERS])
AC_DEFUN(pds_CHECK_TYPE,
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
$3], ac_cv_type_$1=yes, ac_cv_type_$1=no)])dnl
AC_MSG_RESULT($ac_cv_type_$1)
if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1, $2)
fi
])


dnl Configure-time switch with default
dnl
dnl Each switch defines an --enable-FOO and --disable-FOO option in
dnl the resulting configure script.
dnl
dnl Usage:
dnl smr_SWITCH(name, description, default, pos-def, neg-def)
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
AC_DEFUN(smr_SWITCH, [
    AC_MSG_CHECKING(whether to enable $2)
    AC_ARG_ENABLE(
        $1,
        ifelse($3, on,
            [  --disable-[$1]    disable [$2]],
            [  --enable-[$1]     enable [$2]]),
        [ if test "$enableval" = yes; then
            AC_MSG_RESULT(yes)
            ifelse($4, , , AC_DEFINE($4))
        else
            AC_MSG_RESULT(no)
            ifelse($5, , , AC_DEFINE($5))
        fi ],
        ifelse($3, on,
           [ AC_MSG_RESULT(yes)
             ifelse($4, , , AC_DEFINE($4)) ],
           [ AC_MSG_RESULT(no)
            ifelse($5, , , AC_DEFINE($5))]))])


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
AC_DEFUN(smr_ARG_WITHLIB, [

ifelse($2, , smr_lib=[$1], smr_lib=[$2])

AC_ARG_WITH([$1]-library,
ifelse($3, ,
[  --with-$1-library[=PATH]   use $1 library],
[  --with-$1-library[=PATH]   use $1 library ($3)]),
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
            AC_MSG_ERROR(argument must be boolean, file, or directory)
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
AC_DEFUN(smr_ARG_WITHINCLUDES, [

AC_ARG_WITH([$1]-includes,
[  --with-$1-includes=DIR  set directory for $1 headers],
[
    if test -d "$withval"; then
        [$1]_CFLAGS="-I${withval}"
    else
        AC_MSG_ERROR(argument must be a directory)
    fi])

dnl This bit of logic comes from autoconf's AC_PROG_CC macro.  We need
dnl to put the given include directory into CPPFLAGS temporarily, but
dnl then restore CPPFLAGS to its old value.
dnl
smr_test_CPPFLAGS="${CPPFLAGS+set}"
smr_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS ${[$1]_CFLAGS}"

    ifelse($3, , , CPPFLAGS="$CPPFLAGS [$3]")
    AC_CHECK_HEADERS($2)

if test "$smr_test_CPPFLAGS" = set; then
    CPPFLAGS=$smr_save_CPPFLAGS
else
    unset CPPFLAGS
fi
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
AC_DEFUN(smr_CHECK_LIB,
[
ifelse($2, , smr_lib=[$1], smr_lib=[$2])
ifelse($5, , , smr_header=[$5])
smr_ARG_WITHLIB($1,$2,$3)
if test "$with_$1" != no; then
    AC_CHECK_LIB($smr_lib, $4,
        smr_havelib=yes, smr_havelib=no,
        ifelse($6, , ${$1_LIBS}, [${$1_LIBS} $6]))
    if test "$smr_havelib" = yes -a "$smr_header" != ""; then
        smr_ARG_WITHINCLUDES($1, $smr_header, $7)
        smr_safe=`echo "$smr_header" | sed 'y%./+-%__p_%'`
        if eval "test \"`echo '$ac_cv_header_'$smr_safe`\" != yes"; then
            smr_havelib=no
        fi
    fi
    if test "$smr_havelib" = yes; then
        with_$1=yes
    else
        $1_LIBS=
        $1_CFLAGS=
        with_$1=no
    fi
fi])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])


dnl AM_PROG_LEX
dnl Look for flex, lex or missing, then run AC_PROG_LEX and AC_DECL_YYTEXT
AC_DEFUN(AM_PROG_LEX,
[missing_dir=ifelse([$1],,`cd $ac_aux_dir && pwd`,$1)
AC_CHECK_PROGS(LEX, flex lex, "$missing_dir/missing flex")
AC_PROG_LEX
AC_DECL_YYTEXT])

