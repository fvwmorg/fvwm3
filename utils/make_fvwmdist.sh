#!/bin/sh

# options:
#
#  -r   Build and commit a release (tags the sources and updates version info).
#  -w   Disable -Werror in compiler flags.
#  -R   Increase release number after building (2.3.29 -> 3.0.0).
#  -M   Increase major number after building (2.3.29 -> 2.4.0).
#  -N   Your name for the ChangeLog.
#
# environment variables:
#
#   FVWMRELNAME
#     The name that will show up in the ChangeLog (only with -r).
#     If omitted a your gcos name is used.  Can also be set with -N.
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
CFLAGSE="-g -O2 -Wall -Werror"
CFLAGSW="-g -O2 -Wall"
CFLAGS="$CFLAGSE"

export LANG=en_US
export LC_ALL=en_US

# parse options
IS_RELEASE=0
IS_MINOR=1
IS_MAJOR=0
while [ ! x$1 = x ] ; do
  case "$1" in
    -r)
      IS_RELEASE=1 ;;
    -R) IS_MINOR=0; IS_MAJOR=0 ;;
    -M) IS_MINOR=0; IS_MAJOR=1 ;;
    -w) CFLAGS="$CFLAGSW" ;;
    -N) FVWMRELNAME=$2;shift;;
  esac
  shift
done

if [ $IS_RELEASE = 1 ] ; then
  echo "Your name and email address will show up in the ChangeLog."
  if [ -z "$FVWMRELNAME" ] ; then
    FVWMRELNAME=`perl -e 'print ((split(/,/, ((getpwnam(((getpwuid($<))[0])))[6])))[0]);'`
    echo "Please enter your name or press return to use \"$FVWMRELNAME\""
    read ANSWER
    if [ ! "$ANSWER" = "" ] ; then
      FVWMRELNAME=$ANSWER
    fi
  else
    echo "Name: $FVWMRELNAME"
  fi
  if [ -z "$FVWMRELEMAIL" ] ; then
    echo "Please enter your emailaddress (or set FVWMRELEMAIL variable)"
    read FVWMRELEMAIL
  else
    echo "Email: $FVWMRELEMAIL"
  fi
  echo "Your name will show up in the ChangeLog as $FVWMRELNAME"
  echo "Your email address will show up in the ChangeLog as $FVWMRELEMAIL"
fi

wrong_dir=1
if [ -r "$CHECK_FILE" ] ; then
  if grep "$CHECK_STRING1" "$CHECK_FILE" > /dev/null 2> /dev/null ; then
    if grep "$CHECK_STRING2" "$CHECK_FILE" > /dev/null 2> /dev/null ; then
      wrong_dir=0
    fi
  fi
fi

if [ $wrong_dir = 1 ] ; then
  echo "The fvwm sources are not present in the current directory."
  echo "Looked for "$CHECK_FILE" containing \""$CHECK_STRING1"\" and \""$CHECK_STRING2"\". exit."
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
if [ -f config.h.in ]; then
  echo running autoreconf ...
  autoreconf || exit 32
else
  echo running autoreconf ...
  autoreconf || exit 33
  echo running automake again ...
  automake --add-missing || exit 34
  echo running autoconf again ...
  autoconf || exit 35
fi
echo running configure ...
./configure || exit 35
echo running make clean ...
$MAKE clean || exit 37
echo running make ...
$MAKE CC="$CC" CFLAGS="$CFLAGS" || exit 38
echo running make distcheck2 ...
$MAKE CC="$CC" distcheck2 2>&1 | grep "$VERSION_STRING$READY_STRING" || exit 39
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
  perl -pe 's/^(.*) '$VRELNUM' (\(not released yet\))$/$1 '$VRELNUMP' $2\n\n$1 '$VRELNUM' (@{[substr(`date "+%d-%b-%Y"`,0,12)]})/' \
      < NEWS > $NNEWS || exit 41
  mv $NNEWS NEWS || exit 42
  echo updating FAQ file
  NFAQ="new-FAQ"
  if [ $IS_MINOR = 1 ]; then
    perl -pe 's/(Last updated).*(for beta release) '$VRELNUM' (and official)$/$1 @{[substr(`date "+%b %d, %Y"`,0,12)]} $2 '$VRELNUMP' $3/' \
      < docs/FAQ > docs/$NFAQ || exit 43
  else
    perl -pe 's/(Last updated).*(for beta release .* and official release) [0-9]*\.[0-9*]\.[0-9*]\.$/$1 @{[substr(`date "+%b %d, %Y"`,0,12)]} $2 '$VRELNUMP'\./' \
      < docs/FAQ > docs/$NFAQ || exit 44
  fi
  mv docs/$NFAQ docs/FAQ || exit 45
  echo tagging CVS source
  if [ ! "$FVWMRELPRECVSCOMMAND" = "" ] ; then
    $FVWMRELPRECVSCOMMAND
  fi
  cvs tag version-${VRELEASE}_${VMAJOR}_${VMINOR} || exit 46
  echo increasing version number in configure.in
  NCFG="new-configure.in"
  touch $NCFG || exit 47
  cat configure.in |
  perl -pe 's/'$VRELNUM'/'$VRELNUMP'/g' \
    > $NCFG || exit 48
  mv $NCFG configure.in || exit 49
  echo generating ChangeLog entry ...
  NCLOG="new-ChangeLog"
  touch $NCLOG || exit 50
  echo `date +%Y-%m-%d`"  $FVWMRELNAME  <$FVWMRELEMAIL>" > $NCLOG
  echo >> $NCLOG
  echo "	* NEWS, configure.in:" >> $NCLOG
  echo "	changed version to $VRELNUMP" >> $NCLOG
  echo >> $NCLOG
  cat ChangeLog >> $NCLOG
  mv $NCLOG ChangeLog || exit 51
  echo committing configure.in and ChangeLog
  cvs commit -m \
    "* Set development version to $VRELNUMP." \
    NEWS configure.in ChangeLog || exit 52
  if [ ! "$FVWMRELPOSTCVSCOMMAND" = "" ] ; then
    $FVWMRELPOSTCVSCOMMAND
  fi
  echo
  echo Then
fi
echo " . Upload the distribution to ftp://ftp.fvwm.org/pub/incoming/fvwm"
echo " . Notify fvwm-owner@fvwm.org of the upload"
echo " . Update the version numbers in fvwm-web/download.html and"
echo "   fvwm-web/index.html."
echo " . If releasing the stable branch, update NEWS in the beta branch to"
echo "   identify the latest stable release and describe the changes."
echo " . Use fvwm-web generated/txt2html.sh to update the NEWS file:"
echo "   $ cd fvwm-web/generated && ./txt2html.sh ../../fvwm/NEWS"
