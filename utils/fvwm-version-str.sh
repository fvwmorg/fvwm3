#!/bin/sh
#
# fvwm-version-str: emits git describe variables for configure.ac
#		    If this is a release build, then the tag name is chomped
#		    to remove extraneous git information.
#
#		    If it's a developer build, it's left as-is.
#
#
#
# Intended to be called from configure.ac (via autogen.sh)

# Modified from gitdescribe project (https://github.com/pete4abw/gitdescribe)
# Peter Hyman, pete@peterhyman.com

usage() {
cat >&2  <<EOF
$(basename $0) command [-r]
all - entire git describe
commit - commit, omitting g
tagrev - tag revision count
major - major release version
minor - minor release version
micro - micro release version - may include commit info if not release tag
reldatel - Release tag date long Monthname format (DD MMMMMMM YYYY)
reldates - Release tag date short Monthname format (DD-Mmm-YYYY)
reldaten - Release tag date number format only (YYYY-m#-DD)
version - M.m.c
version_info - released or null
-r -- get release tag only
EOF
exit 1
}

# showw message and usage
die() {
	echo "$1" >&2
	usage
}

# return variables
# everything, with leading `v' and leading `g' for commits
describe_tag=

# abbreviated commit
commit=

# count of commits from last tag
tagrev=

# major version
major=

# minor version
minor=

# micro version
micro=

# release date long format
releasedatel=

# release date short format
releasedates=

# release date number format
releasedaten=

# version
version=

# version info, release or null
version_info=

# get release or tag?
# if -r option is used, tagopt will be null
tagopt="--tags"

# how long shhow commit string be? 7 is default
commit_length=7

# get whole commit and parse
# describe_tag variable will hold
# TAG-R-C
# with leading v and commit leadint g removed
# tag commit dates obtained with git show

init() {
	local r_describe
	# git describe raw format
	describe_tag=$(git describe $tagopt --long --abbrev=$commit_length)

	# assign commit, tag revision, and version to variables using `-' separator
	# reverse describe to count dashes from back
	# in case tag has a dash in it ( e.g. 1.2.3-rc0 )
	# then reverse it back again
	r_describe_tag=$(echo $describe_tag | rev)
	commit=$(echo $r_describe_tag | cut -d- -f1 | rev)
	tagrev=$(echo $r_describe_tag | cut -d- -f2 | rev)
	version=$(echo $r_describe_tag | cut -d- -f3 | rev)

	# set micro version or full micro version if tag revision > 0
	micro=$(echo $version | cut -d. -f3)
	if [ $tagrev -gt 0 ]; then
		micro=$micro-$tagrev-$commit
		version_info="not released"
	elif [ -z $tagopt ]; then
		# if a release tag and HEAD offset = 0, then
		version_info="released"
		# get dates using git show
		releasedatel=$(git show -n1 -s --date=format:"%d %B %Y" --format="%cd" $version)
		releasedates=$(git show -n1 -s --date=format:"%d %b %Y" --format="%cd" $version)
		releasedaten=$(git show -n1 -s --date=format:"%Y-%m-%d" --format="%cd" $version)
	fi

	# assign minor version
	minor=$(echo $version | cut -d. -f2)

	# assign major version
	major=$(echo $version | cut -d. -f1)
}

type -p git >/dev/null || die "Something very wrong: git not found."

[ $# -eq 0 ] && die "Must provide a command and optional argument."

# are we getting a release only?
if [ $# -eq 2 ]; then
	if [ $2 = "-r" ]; then
		tagopt=""
	else
		die "Invalid option. Must be -r or nothing."
	fi
fi

init

case "$1" in
	"all" )
		retval=$describe_tag
		;;
	"commit" )
		retval=$commit
		;;
	"tagrev" )
		retval=$tagrev
		;;
	"version" )
		retval=$version
		;;
	"major" )
		retval=$major
		;;
	"minor" )
		retval=$minor
		;;
	"micro" )
		retval=$micro
		;;
	"reldatel" )
		retval=$releasedatel
		;;
	"reldates" )
		retval=$releasedates
		;;
	"reldaten" )
		retval=$releasedaten
		;;
	"version" )
		retval=$version
		;;
	"version_info" )
		retval=$version_info
		;;
	* )
		die "Invalid command."
		;;
esac

echo $retval

exit 0
