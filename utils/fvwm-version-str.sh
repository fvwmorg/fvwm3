#!/bin/sh
#
# fvwm-version-str: emits the version of fvwm which is building.
#		    If this is a release build, then the tag name is chomped
#		    to remove extraneous git information.
#
#		    If it's a developer build, it's left as-is.
#
#
#
# Intended to be called from configure.ac (via autogen.sh)

VERSION="released"

[ -d ".git" ] || { echo "$VERSION" && exit 0 ; }

if grep -q -i '^ISRELEASED="yes"' ./configure.ac; then
	echo "$VERSION"
else
	git describe --always --long --dirty --tags
fi
