#!/bin/sh

# options:
#
#  -R   increase release number after building (2.3.29 -> 3.0.0)
#  -M   increase major number after building (2.3.29 -> 2.4.0)
#  -r   build and commit a release (tags the sources and updates version info)
#
# environment variables:
#
#   FVWMRELNAME
#     The name that will show up in the ChangeLog (only with -r).
#   FVWMRELEMAIL
#     The email address that will show up in the ChangeLog (only with -r).
#   FVWMRELPRECVSCOMMAND
#     A command that will be executed before the first cvs command. Can be used
#     to bring up the network prior to network access. (only with -r)
#   FVWMRELPOSTCVSCOMMAND
#     Same as above, but executed after the last cvs command.

CHECK_FILE=AUTHORS
CHECK_STRING1=fvwm
CHECK_STRING2="Robert Nation"
CHECK_VERSION_STRING="AM_INIT_AUTOMAKE"
VERSION_PRE="fvwm-"
VERSION_POST=".tar.gz"
READY_STRING=" is ready for distribution"
CFLAGS="-g -O2 -Wall -Werror"

# parse options
IS_RELEASE=0
IS_MINOR=1
IS_MAJOR=0
while [ ! x$1 = x ] ; do
  case "$1" in
    -r)
      IS_RELEASE=1
      echo "Your name and email address will show up in the ChangeLog."
      if [ -z "$FVWMRELNAME" ] ; then
        echo "Please enter your name (or set FVWMRELNAME variable)"
        read FVWMRELNAME
      else
        echo "Name: $FVWMRELNAME"
      fi
      if [ -z "$FVWMRELEMAIL" ] ; then
        echo "Please enter your name (or set FVWMRELEMAIL variable)"
        read FVWMRELEMAIL
      else
        echo "Email: $FVWMRELEMAIL"
      fi
      ;;
    -R) IS_MINOR=0; IS_MAJOR=0 ;;
    -M) IS_MINOR=0; IS_MAJOR=1 ;;
  esac
  shift
done

wrong_dir=1
if [ -r "$CHECK_FILE" ] ; then
  if grep "$CHECK_STRING1" "$CHECK_FILE" ; then
    if grep "$CHECK_STRING2" "$CHECK_FILE" ; then
      wrong_dir=0
    fi > /dev/null 2> /dev/null
  fi > /dev/null 2> /dev/null
fi > /dev/null 2> /dev/null

if [ $wrong_dir = 1 ] ; then
  echo "The fvwm sources are not present in the current directory."
  exit 11;
fi

# get release numbers
VERSION=`grep $CHECK_VERSION_STRING configure.in 2>&1 |
         cut -f 2 -d "," |
         perl -pe 's/[[:space:]]*([0-9]+\.[0-9]+\.[0-9]+).$/$1/g'`
VRELEASE=`echo $VERSION | cut -f 1 -d "."`
VMAJOR=`echo $VERSION | cut -f 2 -d "."`
VMINOR=`echo $VERSION | cut -f 3 -d "."`
if [ -z "$VRELEASE" -o -z "$VMAJOR" ] ; then
  echo "Failed to fetch version number from configure.in."
  exit 3;
fi
if [ x$VMINOR = x ] ; then
  VMINOR=0
fi
if [ "$IS_MINOR" = 1 ] ; then
  # minor release
  VRELEASEP=$VRELEASE
  VMAJORP=$VMAJOR
  VMINORP=`echo $VMINOR | perl -pe 's/(.+)/@{[$1+1]}/g'`
elif [ "$IS_MAJOR" = 1 ] ; then
  # major release
  VRELEASEP=$VRELEASE
  VMAJORP=`echo $VMAJOR | perl -pe 's/(.+)/@{[$1+1]}/g'`
  VMINORP=0
else
  VRELEASEP=`echo $VRELEASE | perl -pe 's/(.+)/@{[$1+1]}/g'`
  VMAJORP=0
  VMINORP=0
fi
VRELNUM=$VRELEASE.$VMAJOR.$VMINOR
VRELNUMP=$VRELEASEP.$VMAJORP.$VMINORP
VERSION_STRING=$VERSION_PRE$VRELNUM$VERSION_POST
echo "***** building $VERSION_STRING *****"

# find GNU make
MAKE=
for i in gnumake gmake make; do
  VER=`(echo 'all:;@echo $(MAKE_VERSION)' | $i -f -) 2>/dev/null`
  case $VER in
    3.*) MAKE=$i; break 2 ;;
  esac
done
case $MAKE in
  ?*) : OK, found one ;;
  *)  echo "Can't find GNU make version 3 on the PATH!"; exit 12 ;;
esac

# find compiler (prefer gcc)
CC=
VER=
VER=`(gcc --version) 2>/dev/null`
if [ x$VER = x ] ; then
  CC=cc
else
  CC=gcc
fi

# clean up
echo removing old configure files ...
if [ -f configure ] ; then
  rm configure || exit 21
fi
if [ -f config.cache ] ; then
  rm config.cache || exit 22
fi
if [ -f config.log ] ; then
  rm config.log || exit 23
fi
if [ -f config.status ] ; then
  rm config.status || exit 24
fi

# build the distribution
echo running automake ...
automake --add-missing || exit 31
echo running autoreconf ...
autoreconf || exit 32
echo running configure ...
./configure --enable-gnome || exit 33
echo running make clean ...
$MAKE clean || exit 34
echo running make ...
$MAKE CC="$CC" CFLAGS="$CFLAGS" || exit 35
echo running make distcheck2 ...
$MAKE CC="$CC" distcheck2 2>&1 | grep "$VERSION_STRING$READY_STRING" || exit 36
echo
echo "distribution file is ready"
echo

# update some files and commit changes
if [ $IS_RELEASE = 0 ] ; then
  echo "If this is to be an official release:"
  echo " . Tag the source tree:"
  echo "     cvs tag version-x_y_z"
  echo " . Increase the version number in configure.in and commit this change"
  echo " . Create entries in ChangeLog and NEWS files indicating the release"
else
  echo updating NEWS file
  NNEWS="new-NEWS"
  perl -pe 's/^(.*) '$VRELNUM' (\(not released yet\))$/$1 '$VRELNUMP' $2\n\n$1 '$VRELNUM' (@{[substr(`date +%Y-%m-%d`,0,10)]})/' \
    < NEWS > $NNEWS || exit 41
  mv $NNEWS NEWS || exit 42
  echo tagging CVS source
  if [ ! "$FVWMRELPRECVSCOMMAND" = "" ] ; then
    $FVWMRELPRECVSCOMMAND
  fi
  cvs tag version-${VRELEASE}_${VMAJOR}_${VMINOR} || exit 43
  echo increasing version number in configure.in
  NCFG="new-configure.in"
  touch $NCFG || exit 44
  cat configure.in |
  perl -pe 's/'$VRELNUM'/'$VRELNUMP'/g' \
    > $NCFG || exit 45
  mv $NCFG configure.in || exit 46
  echo generating ChangeLog entry ...
  NCLOG="new-ChangeLog"
  touch $NCLOG || exit 47
  echo `date +%Y-%m-%d`"  $FVWMRELNAME  <$FVWMRELEMAIL>" > $NCLOG
  echo >> $NCLOG
  echo "	* NEWS, configure.in:" >> $NCLOG
  echo "	changed version to $VRELNUMP" >> $NCLOG
  echo >> $NCLOG
  cat ChangeLog >> $NCLOG
  mv $NCLOG ChangeLog || exit 48
  echo committing configure.in and ChangeLog
  cvs commit -m \
    "* Set development version to $VRELNUMP." \
    NEWS configure.in ChangeLog || exit 49
  if [ ! "$FVWMRELPOSTCVSCOMMAND" = "" ] ; then
    $FVWMRELPOSTCVSCOMMAND
  fi
  echo
  echo Then
fi
echo " . Upload the distribution to ftp://ftp.fvwm.org/pub/incoming/fvwm"
echo " . Notify fvwm-owner@fvwm.org of the upload"
echo " . Update the version numbers in fvwm-web/download.html"
