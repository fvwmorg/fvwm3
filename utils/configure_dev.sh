#!/bin/sh

# reconfigure CVS source code
# must be called from main fvwm directory
set -x
aclocal || exit 1
autoheader || exit 2
automake --add-missing || exit 3
autoreconf || exit 4
./configure ${1+"$@"} || exit 5
