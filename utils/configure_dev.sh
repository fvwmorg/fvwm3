#!/bin/sh

# reconfigure CVS source code
# must be called from main fvwm directory
echo automake
automake || exit 1
echo autoreconf
autoreconf || exit 2
echo ./configure
./configure || exit 3
