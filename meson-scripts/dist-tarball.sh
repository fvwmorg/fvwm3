#!/bin/bash
#
# Script to sanitise the tarball when calling "meson dist".  By default,
# "meson dist" calls git-archive, which includes more files than the autotools
# version of "make dist".

# TODO: ".*" as a glob might be too much in the future; expand this out...
FILES_TO_IGNORE=".clang-format
.disabled-travis.yml
.editorconfig
.git
.github
.gitignore
.mailmap
dev-docs"

(
	cd "$MESON_DIST_ROOT" && {
    		IFS=$'\n'
    		for f in $FILES_TO_IGNORE
    		do
        		echo "Removing $f from tarball..."
        		rm -r "./$f"
    		done
	}
)
