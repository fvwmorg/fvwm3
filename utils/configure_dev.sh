#!/bin/sh

# reconfigure CVS source code
# must be called from main fvwm directory
echo aclocal
aclocal || exit 1
echo autoheader
autoheader || exit 2
echo automake
automake --add-missing || exit 3
echo autoreconf
autoreconf || exit 4
echo ./configure "$@"
./configure "$@" || exit 5
