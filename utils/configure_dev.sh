#!/bin/sh

# reconfigure CVS source code
# must be called from main fvwm directory
echo aclocal
aclocal || exit 1
echo automake
automake --add-missing || exit 2
echo autoreconf
autoreconf || exit 3
echo ./configure "$@"
./configure "$@" || exit 4
