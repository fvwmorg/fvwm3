#!/bin/bash

set -xv

# mkrelease -- script to help ease the burden of creating a fvwm3 release.
#
# Originally published December 2020.  Released under the same licence as
# fvwm3 itself.

function die()
{
	echo "$@" >&2
	exit 1
}

[ -z "$1" ] && die "No version given"
next_version="$1"

current_ver="$(git describe --tags --abbrev=0)"
current_tag="$(git describe --tags --abbrev=0)"
pre_tag="${current_tag%.*}"
current_tag="${current_tag##*.}"

next_release_branch="release/$next_version"

echo "*** updating configure.ac:"
echo "***     new version ($next_version)..."
sed -i -e "/AC_INIT/ s/$current_ver/$next_version/" configure.ac || \
	die "Unable to update configure.ac to next version"

echo "***     released to yes"
sed -i -e 's/ISRELEASED="no"/ISRELEASED="yes"/' configure.ac || \
	die "Unable to update configure.ac to released"

echo "***     updating release dates"
reldatelong="$(date -d "now" +"%e %B %y")"
reldateshort="$(date -d "now" +"%d-%b-%y")"
reldatenum="$(date -d "now" +"%y-%m-%d")"

sed -i -e "/^RELDATELONG=/cRELDATELONG=\"$reldatelong\"" configure.ac
sed -i -e "/^RELDATESHORT=/cRELDATESHORT=\"$reldateshort\"" configure.ac
sed -i -e "/^RELDATENUM=/cRELDATENUM=\"$reldatenum\"" configure.ac

exit

echo
echo "*** generating release tarball"
make dist || die "Couldn't generate dist tarball"

echo "*** test compiling release tarball"
cp ./fvwm3-${next_version}.tar.gz /tmp && {
	(cd /tmp && {
		tar xzf ./fvwm3-${next_version}.tar.gz && \
		(cd fvwm3-${next_version} && \
		   ./configure --enable-mandoc &>/dev/null && \
		   make -j$(nproc) &>/dev/null || \
		   	die "Unable to compile fvwm3 dist")
	})
}
fvwm3_ver="$(/tmp/fvwm3-$next_version/fvwm/fvwm3 --version | head -n 1)"
[ -z "$fvwm3_ver" ] && die "Couldn't determine fvwm3 version"

echo "***     version reports as: $fvwm3_ver"
rm -fr /tmp/fvwm3-${next_version} && rm -f /tmp/fvwm3-${next_version}*.tar.gz
