#!/bin/sh

CHECK_FILE=AUTHORS
CHECK_STRING1=fvwm
CHECK_STRING2="Robert Nation"
CHECK_VERSION_STRING="AM_INIT_AUTOMAKE"
VERSION_PRE="fvwm-"
VERSION_POST=".tar.gz"
READY_STRING=" is ready for distribution"

wrong_dir=0

if [ "$1" = -r ] ; then
  IS_RELEASE=1
else
  IS_RELEASE=0
fi

err_exit ()
{
  echo
  echo "an error occured while builting the distribution. Error code = $1"
  exit $1
}

if [ ! -r "$CHECK_FILE" ] ; then
  wrong_dir=1
elif ! grep "$CHECK_STRING1" "$CHECK_FILE" ; then
  wrong_dir=1
elif ! grep "$CHECK_STRING2" "$CHECK_FILE" ; then
  wrong_dir=1
fi > /dev/null 2> /dev/null

if [ $wrong_dir = 1 ] ; then
  echo "The fvwm sources are not present in the current directory."
  err_exit 1;
fi

VERSION=`grep $CHECK_VERSION_STRING configure.in 2>&1 |
         cut -f 2 -d "," |
         sed -e "s/[[:space:]]//g" -e "s/)//g"`
VRELEASE=`echo $VERSION | cut -f 1 -d "."`
VMAJOR=`echo $VERSION | cut -f 2 -d "."`
VMINOR=`echo $VERSION | cut -f 3 -d "."`
if [ -z "$VRELEASE" -o -z "$VMAJOR" -o -z "$VMINOR" ] ; then
  echo "Failed to fetch version number from configure.in."
  err_exit 12;
fi
VERSION_STRING=$VERSION_PRE$VRELEASE.$VMAJOR.$VMINOR$VERSION_POST
echo "***** building $VERSION_STRING *****"

# clean up
echo removing old configure files ...
if [ -f configure ] ; then
  rm configure || err_exit 2
fi
if [ -f config.cache ] ; then
  rm config.cache || err_exit 3
fi
if [ -f config.log ] ; then
  rm config.log || err_exit 4
fi
if [ -f config.status ] ; then
  rm config.status || err_exit 5
fi

echo running automake ...
automake --add-missing || err_exit 6
echo running autoreconf ...
autoreconf || err_exit 7
echo running configure ...
./configure --enable-gnome || err_exit 8
echo running make clean ...
make clean || err_exit 9
echo running make ...
make CFLAGS="-g -O2 -Wall -Werror" || err_exit 10
echo running make distcheck ...
make distcheck 2>&1 | grep "$VERSION_STRING$READY_STRING" || err_exit 11
echo
echo "distribution file is ready"
echo
if [ $IS_RELEASE = 0] ; then
  echo "If this is to be an official release:"
  echo " . Tag the source tree:"
  echo "     cvs tag version-x_y_z"
  echo " . Increase the version number in configure.in and commit this change"
  echo " . Create entries in ChangeLog and NEWS files indicating the release"
  echo " . Upload the distribution to ftp://ftp.fvwm.org/pub/incoming/fvwm"
  echo " . Notify fvwm-owner@fvwm.org of the upload"
else
  echo tagging CVS source
  cvs tag version-$VRELEASE_$VMAJOR_VMINOR || err_exit 13
  echo increasing version number in configure.in
  NCFG="configure.in.$RANDOM"
  cat configure.in |
  sed -e "s/$VRELEASE\.$VMAJOR\.$VMINOR/$VRELEASE\.$VMAJOR\.$[$VMINOR+1]/g" >
  $NCFG |
  err_exit 14
  mv $NCFG configure.in
  echo committing configure.in
  cvs commit -m "* Set development version to $VRELEASE.$VMAJOR.$VMINOR." \
      configure.in || err_exit 15
  echo
  echo
  echo "Please put this in the ChangeLog and create an entry in the NEWS file."
  echo "------------------------------ snip ---------------------------------"
  echo "yyyy-mm-dd  Your name  <your@email.address>"
  echo
  echo "	* configure.in: changed version to $VRELEASE.$VMAJOR.$VMINOR"
  echo
  echo "------------------------------ snip ---------------------------------"
  echo
  echo Then
  echo " . Upload the distribution to ftp://ftp.fvwm.org/pub/incoming/fvwm"
  echo " . Notify fvwm-owner@fvwm.org of the upload"
  echo " . Update the version numbers in fvwm-web/download.html"
fi
