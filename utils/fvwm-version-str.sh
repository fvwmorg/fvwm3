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
# Intended to be called from meson.
set -e

VERSION="released"
RS=".release-status"

[ -d ".git" ] || { echo "$VERSION" && exit 0 ; }

. "./$RS"

[ -z "$ISRELEASED" ] && { echo "UNKNOWN" && exit 0 ; }
[ "$ISRELEASED" = "yes" ] && { echo "$VERSION" && exit 0 ; }

git describe --always --long --dirty --tags

