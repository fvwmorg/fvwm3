#!/bin/sh

# configure and build CVS source code from scratch
# must be called from main fvwm directory
utils/configure_dev.sh || exit 1
utils/build_dev.sh || exit 2
