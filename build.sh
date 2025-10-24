#!/bin/bash

# Released under the same licence as fvwm3.

fflag=0
iflag=0
vflag=0
Dargs=""
buildDir="compile"
tempFile="$(mktemp)"

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
$(basename $0) [-b builddir] [-f] [-v] [-D opts]

Where:

-b: directory to use to build fvwm3, defaults to: 'compile'
-f: Forces 'meson setup' to rerun, with --reconfigure --wipe
-h: Shows this output
-i: Installs compiled files
-v: Verbose mode - does not hide output from meson
-D: Can be specified multiple times to pass options to 'meson setup'
EOF
)"
}

# Getopts
while getopts ":fb:D:hiv" o; do
	case "$o" in
		b)
			buildDir="$OPTARG"
			;;
		f)
			fflag=1
			;;
		i)
			iflag=1
			;;
		v)
			vflag=1
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

{ [ ! -d "$buildDir" ] || [ "$fflag" = 1 ] ; } && {
	echo "Running meson setup..."
	run_cmd meson setup --reconfigure --wipe "$buildDir" "$Dargs" || {
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
