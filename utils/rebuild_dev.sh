#!/bin/sh

# configure and build CVS source code from scratch
# must be called from main fvwm directory
utils/configure_dev.sh || exit 1
make clean
utils/build_dev.sh || exit 2
