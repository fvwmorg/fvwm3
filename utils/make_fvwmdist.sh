#!/bin/sh

CHECK_FILE=AUTHORS
CHECK_STRING1=fvwm
CHECK_STRING2="Robert Nation"

wrong_dir=0

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
make distcheck 2>&1 || err_exit 11
echo
echo "distribution file is ready"
echo 
echo "If this is to be an official release:"
echo " . Tag the source tree:"
echo "     cvs tag version-x_y_z"
echo " . Increase the version number in configure.in and commit this change"
echo " . Upload the distribution to ftp://ftp.fvwm.org/pub/incoming/fvwm"
echo " . Notify fvwm-owner@fvwm.org of the upload"

