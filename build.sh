#!/bin/bash

# Released under the same licence as fvwm3.

fflag=0
iflag=0
vflag=0
Dargs=""
buildDir="compile"
tempFile="$(mktemp)"
prefix=""

# Portably determine number of CPU cores on different systems (MacOS, BSD,
# Linux).
nproc() {
	NPROCCMD="nproc"

	command "$NPROCCMD" >/dev/null 2>&1 || {
		NPROCCMD="sysctl -n hw.ncpu"
	}

	result="$(eval command $NPROCCMD)"
	[ -z "$result" -o "$result" -le 0 ] && result="1"

	echo "$result"
}

die() {
	echo "$@" >&2
	[ -s "$tempFile" -a "$vflag" = 0 ] && {
		cat $tempFile >&2
		echo >&2
		echo "Build output available here: $tempFile" >&2
	}
	exit 1
}

run_cmd() {
	[ "$vflag" = 0 ] && {
		"$@" >>"$tempFile" 2>&1
		return $?
	} || {
		"$@" | tee -a "$tempFile"
		return $?
	}
}

usage() {
	die "$(cat <<EOF
$(basename $0) [-b builddir] [-f] [-v] [-i] [-m] [-p] [-w] [-D opts]

Where:

-b: Directory to use to build fvwm3, defaults to: 'compile'
-d: Compile all docs (manpages and HTML), implies options '-m' and '-w'
-f: Forces 'meson setup' to rerun, with --reconfigure --wipe
-h: Shows this output
-i: Installs compiled files
-m: Build manual pages
-p: Set the prefix for where the installation path will be
-v: Verbose mode - does not hide output from meson
-w: Build HTML documentation
-D: Can be specified multiple times to pass options to 'meson setup'
EOF
)"
}

# Getopts
while getopts ":dfb:D:himp:vw" o; do
	case "$o" in
		b)
			buildDir="$OPTARG"
			;;
		d)
			Dargs="$Dargs -Dmandoc=true -Dhtmldoc=true"
			;;
		f)
			fflag=1
			;;
		i)
			iflag=1
			;;
		m)
			Dargs="$Dargs -Dmandoc=true"
			;;
		p)
			Dargs="$Dargs -Dprefix=$OPTARG"
			prefix="$OPTARG"
			;;
		v)
			vflag=1
			;;
		w)
			Dargs="$Dargs -Dhtmldoc=true"
			;;
		D)
			Dargs="$Dargs -D$OPTARG"
			;;
		h | * | \?)
			usage
			;;
	esac
done
shift $((OPTIND-1))

# Check to see whether meson is installed.
command -v meson >/dev/null 2>&1 || die "meson not found..."

echo "Tempfile is: $tempFile"
echo "Using build directory: $buildDir"
[ -n "$prefix" ] && {
	echo "Prefix is: $prefix"
}

{ [ ! -d "$buildDir" ] || [ "$fflag" = 1 ] ; } && {
	echo "Running meson setup..."
	run_cmd meson setup --reconfigure --wipe "$buildDir" $Dargs || {
		die "Command failed..."
	}
}

echo "Compiling..."
run_cmd ninja -j $(nproc) -C "$buildDir" || die "Command failed..."

[ "$iflag" = 1 ] && {
	echo "Installing..."
	run_cmd meson install -C "$buildDir"
}

exit 0
