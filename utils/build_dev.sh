#!/bin/sh

# call make with -Wall -Werror
# must be called from main fvwm directory
echo 'make CFLAGS="-O2 -g -Wall -Werror"'
make CFLAGS="-O2 -g -Wall -Werror" || exit 1
