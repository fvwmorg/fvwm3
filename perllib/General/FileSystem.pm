# Copyright (C) 1998-2002, Mikhael Goikhman
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package General::FileSystem;
require 5.004;
use strict;

use vars qw(@ISA @EXPORT);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(
	loadFile saveFile appendFile removeFile copyFile moveFile
	makeDir makePath cleanDir removeDir copyDir moveDir
	listFileNames findExecutable
	defaultDirPerm preserveStat parsePath getCwd
);

use vars qw($CACHE_FILE_NUM $cacheCounter @prevFileNames @prevFileContentRefs);
use vars qw($ENABLE_CACHE %NEVER_COPY_FILES %NEVER_REMOVE_FILES);
use vars qw($DEFAULT_DIR_PERM $PRESERVED_STAT);
use vars qw($DEBUG_ENABLED $ERROR_HANDLER $LOAD_FILE_DIRS $SAVE_FILE_DIR);

BEGIN {
	$ENABLE_CACHE       = 0;  # this is risky for dynamical files
	%NEVER_COPY_FILES   = ( 'CVS' => 1, 'core' => 1 );
	%NEVER_REMOVE_FILES = ( 'CVS' => 1 );
	$DEFAULT_DIR_PERM   = 0775;
	$PRESERVED_STAT     = 0;

	# allow these constants to be set directly from outside
	$ERROR_HANDLER      ||= "warn";   # may be "die", "none", "warn" or CODE
	$DEBUG_ENABLED      ||= 0;
	$LOAD_FILE_DIRS     ||= [ "." ];  # for non fully qualified files only
	$SAVE_FILE_DIR      ||= ".";      # for non fully qualified files only
}


# ----------------------------------------------------------------------------

=head1 NAME

FileSystem - file system specific functions

=head1 SYNOPSIS

  use General::FileSystem "-die", "-debug";  # die on errors

  eval {
    makePath("/tmp/my-own/dir");

    my $fileContentRef = loadFile("/etc/issue");
    saveFile("/tmp/my-own/dir/issue", $fileContentRef);

    # This is equivalent to the previous two lines, but optimized
    copyFile("/etc/issue", "/tmp/my-own/dir/issue");

    makeDir("/tmp/my-own/dir2", 0711);
    copyFile("/etc/issue", "/tmp/my-own/dir2/issue");
    moveFile("/tmp/my-own/dir2/issue", "/tmp/my-own/dir2/issue2");
    removeFile("/tmp/my-own/dir2/issue2");
    cleanDir("/tmp/my-own/dir2"); # no effect, it's empty already

    removeDir("/tmp/my-own");
  };
  if ($@) {
    print "File System Error: $@";
  };

or just:

  use General::FileSystem;
  copyFile("origin.txt", "backup.txt");

=head1 DESCRIPTION

This package contains common file operation functions.

On fatal errors all functions call the error handler (see C<$ERROR_HANDLER>),
that may throw exeption (die) or issue a warning or quietly return undef.

=head1 REQUIREMENTS

L<Cwd>, L<File::Basename>, L<File::Copy>.

=head1 METHODS

=cut
# ============================================================================


use Cwd;
use File::Basename;
use File::Copy;


sub import ($;$) {
	my $package = shift;
	while (@_ && $_[0] =~ /^-/) {
		local $_ = shift;
		$ERROR_HANDLER = $1 if /^-(die|warn|none)$/i;
		$DEBUG_ENABLED = $1 if /^-(debug)$/i;
	}
	$package->export_to_level(1, @_);
}


# private function
sub callErrorHandler ($) {
	my $msg = shift;
	die  "$msg: [$!]\n" if $ERROR_HANDLER eq "die";
	warn "$msg: [$!]\n" if $ERROR_HANDLER eq "warn";
	return undef if $ERROR_HANDLER eq "none";
	&$ERROR_HANDLER($msg) if ref($ERROR_HANDLER) eq "CODE";
	return undef;
}


# private function
sub printLog ($) {
	my $msg = shift;
	return unless $DEBUG_ENABLED;
	print STDERR "FileSystem: $msg\n";
}


# ----------------------------------------------------------------------------

=head2 loadFile

=over 4

=item usage

  $contentRef = loadFile($fileName)

=item description

Loads file with given file-name from local filesystem.

=item parameters

  * fileName - name of the file to be loaded.

=item returns

Reference to file content string on success, otherwise throws exception.

=back

=cut
# ============================================================================


BEGIN {
	$CACHE_FILE_NUM = 6;
	$cacheCounter = -1;
	@prevFileNames = ("", "", "", "", "", "");
	@prevFileContentRefs = \("", "", "", "", "", "");
}


sub loadFile ($) {
 	my $fileName = shift;
	printLog("Loading file $fileName") if $DEBUG_ENABLED;

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1) {
			if ($fileName eq $prevFileNames[$_] && -r $fileName) {
				printLog("getting from file cache") if $DEBUG_ENABLED;
				return $prevFileContentRefs[$_];
			}
		}
	}

	open(FILE, "<$fileName") || return callErrorHandler("Can't open $fileName");
	my $fileContent = join("", <FILE>);
	close(FILE) || return callErrorHandler("Can't close $fileName");

	if ($ENABLE_CACHE) {
		$cacheCounter = ($cacheCounter+1) % $CACHE_FILE_NUM;
		$prevFileNames[$cacheCounter] = $fileName;
		$prevFileContentRefs[$cacheCounter] = \$fileContent;
	}
	return \$fileContent;
}


# ----------------------------------------------------------------------------

=head2 saveFile

=over 4

=item description

Saves file-content to local filesystem with given file-name.

=item usage

  saveFile($fileName, \$fileContent);

=item parameters

  * fileName - name of the file to be saved into
  * fileContentRef - reference to file-content string
  * createSubdirs - optional flag (default is 0 - don't create subdirs)

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub saveFile ($$;$) {
	my ($fileName, $fileContentRef, $createDirs) = @_;
	printLog("Saving  file $fileName") if $DEBUG_ENABLED;
	die("saveFile: No SCALAR ref parameter\n")
		unless ref($fileContentRef) eq 'SCALAR';

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1) {
			$prevFileContentRefs[$_] = $fileContentRef
				if $fileName eq $prevFileNames[$_];
		}
	}
	if ($createDirs) {
		my $dirName = dirname($fileName);
		makePath($dirName) unless -d $dirName;
	}

	open(FILE, ">$fileName") || return callErrorHandler("Can't open $fileName");
	print FILE $$fileContentRef;
	close(FILE) || return callErrorHandler("Can't close $fileName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 appendFile

=over 4

=item description

Appends file-append-content to local filesystem with given file-name.

=item usage

  appendFile($fileName, \$fileAppendContent);

=item parameters

  * fileName - name of the file to be saved into
  * fileAppendContentRef - reference to file-append-content string

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub appendFile ($$) {
	my ($fileName, $fileAppendRef) = @_;
	printLog("Append>>file $fileName") if $DEBUG_ENABLED;

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1 && -r $fileName) {
			${$prevFileContentRefs[$_]} .= $$fileAppendRef
				if $fileName eq $prevFileNames[$_];
		}
	}

	open(FILE, ">>$fileName") || return callErrorHandler("Can't append to $fileName");
	print FILE $$fileAppendRef;
	close(FILE) || return callErrorHandler("Can't close $fileName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 removeFile

=over 4

=item description

Removes all files from given directory.

=item usage

  removeFile($fileName);

=item parameters

  * fileName - name of the file to be deleted

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub removeFile ($;$) {
	my $fileName = shift;
	printLog("Removin file $fileName") if $DEBUG_ENABLED;
	unlink($fileName) || return callErrorHandler("Can't unlink $fileName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 makeDir

=over 4

=item description

Removes all files from given directory.

=item usage

  makeDir($PREVIEW_DIR);

=item parameters

  * directory to make
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub makeDir ($;$) {
	my $dirName = shift;
	my $perm = shift || $DEFAULT_DIR_PERM;
	printLog("Creating dir $dirName, " . sprintf("%o", $perm))
		if $DEBUG_ENABLED;
	mkdir($dirName, $perm) || return callErrorHandler("Can't mkdir $dirName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 makePath

=over 4

=item description

Removes all files from given directory.

=item usage

  makePath($PUBLISH_DIR);

=item parameters

  * path to make
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub makePath ($;$) {
	my $dirName = shift;
	my $perm = shift || $DEFAULT_DIR_PERM;
	printLog("Making  path $dirName, " . sprintf("%o", $perm))
		if $DEBUG_ENABLED;

	return 1 if -d $dirName;
	my $parentDir = dirname($dirName);

	local $DEBUG_ENABLED = 0;
	&makePath($parentDir, $perm) unless -d $parentDir;
	makeDir($dirName, $perm);

	return 1;
}


# ----------------------------------------------------------------------------

=head2 copyFile

=over 4

=item description

Copies a file to another location.

=item usage

  copyFile($from, $to);

=item parameters

  * file name to copy from
  * file name to copy to

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub copyFile ($$) {
	my ($srcFileName, $dstFileName) = @_;
	printLog("Copying file $srcFileName to $dstFileName")
		if $DEBUG_ENABLED;

	# Must manage symbolic links somehow
	# return if -l $srcFileName;

	copy($srcFileName, $dstFileName)
		or return callErrorHandler("Can't copy $srcFileName $dstFileName");

	if ($PRESERVED_STAT) {
		my ($device, $inode, $mode) = stat($srcFileName);
		chmod($mode, $dstFileName);
	}
	return 1;
}


# ----------------------------------------------------------------------------

=head2 moveFile

=over 4

=item description

Moves (or renames) a file to another location.

=item usage

  moveFile($from, $to);

=item parameters

  * file name to move from
  * file name to move to

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub moveFile ($$) {
	my ($srcFileName, $dstFileName) = @_;
	printLog("Moving  file $srcFileName to $dstFileName")
		if $DEBUG_ENABLED;

	move($srcFileName, $dstFileName)
		or return callErrorHandler("Can't move $srcFileName $dstFileName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 cleanDir

=over 4

=item description

Removes all files from given directory.

=item usage

  cleanDir($PREVIEW_DIR);

=item parameters

  * directory to clean
  * optional flag:
    0 - don't go recursively, unlink files in first level only
    1 - recursively clean subdirs (default)
    2 - unlink subdirs
    3 - unlink given directory

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub cleanDir ($;$) {
	my $dirName = shift;
	my $recursive = shift || 1;
	die("cleanDir: Unsupported flag $recursive\n")
		if $recursive > 3 || $recursive < 0;
	printLog(($recursive != 3? "Cleaning": "Removing") . " dir $dirName "
		. ["files only", "recursively files only", "recursively", "completely"]->[$recursive])
		if $DEBUG_ENABLED;

	local $DEBUG_ENABLED = 0;

	my @subdirs = ();
	my $fileNames = listFileNames($dirName);

	# process files
	foreach (@$fileNames) {
		next if $NEVER_REMOVE_FILES{$_};
		my $fileName = "$dirName/$_";
		if (-d $fileName) { push @subdirs, $fileName; }
		else { unlink("$fileName") || return callErrorHandler("Can't unlink $fileName"); }
	}

	# process subdirs
	map {
		cleanDir($_, $recursive);
		rmdir($_) || return callErrorHandler("Can't unlink $_") if $recursive == 2;
	} @subdirs if $recursive;
	rmdir($dirName) || return callErrorHandler("Can't unlink $dirName") if $recursive == 3;

	return 1;
}


# ----------------------------------------------------------------------------

=head2 removeDir

=over 4

=item description

Entirely removes given directory and its content (if any).
This is an alias to C<cleanDir(3)>.

=item usage

  removeDir($TMP_DIR);

=item parameters

  * directory to clean

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub removeDir ($) {
	my $dirName = shift;
	return cleanDir($dirName, 3);
}


# ----------------------------------------------------------------------------

=head2 copyDir

=over 4

=item description

Recursively copies all files and subdirectories inside given directory
to new location.

Destination directory must not exist. Use: C<trap { removeDir($dest); };>
to remove it before copying.

=item usage

  copyDir($dirFrom, $dirTo);

=item parameters

  * source directory to copy
  * destination directory to copy to (may not exist)
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub copyDir ($$) {
	my ($srcDirName, $dstDirName, $perm) = @_;

	return callErrorHandler("Directory $srcDirName does not exist")
		unless -d $srcDirName;
	makeDir($dstDirName, $perm) unless -d $dstDirName;

	printLog("Copying  dir $srcDirName to $dstDirName recursively")
		if $DEBUG_ENABLED;;

	local $DEBUG_ENABLED = 0;

	my @subdirs = ();
	my $fileNames = listFileNames($srcDirName);

	# process files
	foreach (@$fileNames) {
		next if $NEVER_COPY_FILES{$_};
		my $srcFileName = "$srcDirName/$_";
		my $dstFileName = "$dstDirName/$_";
		if (-d $srcFileName) { push @subdirs, $_; }
		elsif (-l $srcFileName) { next if "# We ignore links for now! TO FIX!" }
		else { copyFile($srcFileName, $dstFileName); }
	}

	# process subdirs
	foreach (@subdirs) {
		my $srcSubDirName = "$srcDirName/$_";
		my $dstSubDirName = "$dstDirName/$_";
		&copyDir($srcSubDirName, $dstSubDirName);
	}

	return 1;
}


# ----------------------------------------------------------------------------

=head2 moveDir

=over 4

=item description

Moves (or actually renames) a directory to another location.

Destination directory must not exist. Use: C<trap { removeDir($dest); };>
to remove it before copying.

=item usage

  moveDir($dirFrom, $dirTo);

=item parameters

  * source directory to move from
  * destination directory to move to (must not exist)

=item returns

C<1> on success, otherwise throws exception.

=back

=cut
# ============================================================================


sub moveDir ($$) {
	my ($srcDirName, $dstDirName) = @_;
	printLog("Moving   dir $srcDirName to $dstDirName")
		if $DEBUG_ENABLED;

	rename($srcDirName, $dstDirName)
		or return callErrorHandler("Can't rename $srcDirName $dstDirName");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 listFileNames

=over 4

=item description

Returns the file names in the given directory including all types of files
(regular, directory, link, other), not including '.' and '..' entries.

=item usage

  # mini file lister
  foreach my $file (@{listFileNames('/home/ftp')}) {
    print "File $file\n" if -f $file;
    print "Dir  $file\n" if -d $file;
  }

=item parameters

 * directory to list (or array ref of directories)

=item returns

Array ref of scalars (file names). Throws exception on error.

=back

=cut
# ============================================================================


sub listFileNames ($) {
	my $dirName = shift;
	if (ref($dirName) eq "ARRAY") {
		my @files = ();
		foreach (@$dirName) { push @files, &listFileNames($_); }
		return \@files;
	}
	printLog("Listing  dir $dirName") if $DEBUG_ENABLED;

	opendir(DIR, $dirName) || return callErrorHandler("Can't opendir $dirName");
	my @files = grep { $_ ne '.' && $_ ne '..' } readdir(DIR);
	closedir(DIR) || return callErrorHandler("Can't closedir $dirName");

	return \@files;
}


# ----------------------------------------------------------------------------

=head2 findFile

=over 4

=item description

Searches for the given file in the given directories.

Returns the fully qualified file name.

=item usage

  my $gtkrc = findFile(".gtkrc", [$home, "$home/.gnome"]);

=item parameters

  * file name to search for
  * array ref of directories to search in

=item returns

File name with full path if found, or undef if not found.

=back

=cut
# ============================================================================


sub findFile ($$) {
	my $fileName = shift;
	my $dirs = shift();
	die "findFile: no dirs given\n" unless ref($dirs) eq "ARRAY";
	foreach (@$dirs) {
		my $filePath = "$_/$fileName";
		return $filePath if -f $filePath;
	}
	return undef;
}


# ----------------------------------------------------------------------------

=head2 findExecutable

=over 4

=item description

Searches for the given executable file in the directories that are in the
environmebt variable $PATH or in the additional parameter.

Returns the fully qualified file name.

=item usage

  my $gzipExe = findExecutable("gzip", ["/usr/gnu/bin", "/gnu/bin"]);

=item parameters

  * file name to search for (only executables are tested)
  * optional array ref of directories to search in

=item returns

File name with full path if found, or undef if not found.

=back

=cut
# ============================================================================


sub findExecutable ($;$) {
	my $fileName = shift;
	my $addDirs = shift;
	my @dirs = split(":", $ENV{"PATH"} || "");
	if (ref($addDirs) eq "ARRAY") {
		push @dirs, @$addDirs;
	}
	foreach (@dirs) {
		my $filePath = "$_/$fileName";
		return $filePath if -x $filePath;
	}
	return undef;
}


# ----------------------------------------------------------------------------

=head2 defaultDirPerm

=over 4

=item description

This functions changes default directory permissions, used in
C<makeDir>, C<makePath>, C<copyDir> and C<moveDir> functions.

The default of this package is 0775.

If no parameters specified, nothing is set (only current value is returned).

=item usage

 defaultDirPerm(0700);

=item parameters

 * optional default directory permission (integer)

=item returns

Previous value.

=back

=cut
# ============================================================================


sub defaultDirPerm (;$) {
	return if $^O =~ /Win|DOS/;
	my $newValue = shift;
	my $oldValue = $DEFAULT_DIR_PERM;

	if (defined $newValue) {
		printLog("defaultDirPerm = $newValue") if $DEBUG_ENABLED;
		$DEFAULT_DIR_PERM = $newValue;
	}
	return $oldValue;
}


# ----------------------------------------------------------------------------

=head2 preserveStat

=over 4

=item description

This functions changes behavior of C<copyFile> and C<copyDir> functions.
If 0 is given as a parameter stats will not be preserved.

TODO: specify values for diferent preserves:

  0 nothing
  1 mode   file mode  (type and permissions)
  2 uid    numeric user ID of file's owner
  4 gid    numeric group ID of file's owner
  8 atime  last access time since the epoch
 16 mtime  last modify time since the epoch
 32 ctime  inode change time (NOT creation time!) since the epo

The default of this package is 0.

If no parameters specified, nothing is set (only current value is returned).

=item usage

 preserveStat(1);

=item parameters

 * optional flag (currently 0 or 1)

=item returns

Previous value.

=back

=cut
# ============================================================================


sub preserveStat (;$) {
	return if $^O =~ /Win|DOS/;
	my $newValue = shift;
	my $oldValue = $PRESERVED_STAT;

	if (defined $newValue) {
		printLog("preserveStat = $newValue") if $DEBUG_ENABLED;
		$PRESERVED_STAT = $newValue;
	}
	return $oldValue;
}


# ----------------------------------------------------------------------------

=head2 parsePath

=over 4

=item usage

  my ($dirName, $baseName) = parsePath($fileName);

=item examples

  # in: "/data/projects/magazine"  out: ("/data/projects", "magazine")
  # in: "/magazine"                out: ("", "magazine")
  # in: "dir/"                     out: (dir", "")
  # in: "magazine"                 out: (".", "magazine")

  # in: "c:\projects\magazine"     out: ("c:\projects", "magazine")
  # in: "c:\magazine"              out: ("c:", "magazine")
  # in: "c:magazine"               out: ("c:.", "magazine")

=item description

Returns a list of 2 scalars: directory name and base name. All unix and dos
file names supported.

Note, the rule is this: you can join both scalars using a directory delimiter
(slash or backslash) and you will always get the the original (logical)
file name.

=back

=cut
# ============================================================================


sub parsePath ($) {
	my $path = shift;
	if ($path =~ m=^(.*)[/\\]+([^/\\]*)$=) {
		return ($1, $2);
	} else {
		# support even funny dos form c:file
		return $path =~ m=^(\w:)(.*)$=?
			($1 . ".", $2):
			(".", $path);
	}
}


# ----------------------------------------------------------------------------

=head2 getCwd

=over 4

=item usage

  my $cwd = getCwd();

=item description

Returns the current working directory.

=back

=cut
# ============================================================================


sub getCwd () {
	$^O eq "MSWin32"? Win32::GetCwd(): require "getcwd.pl" && getcwd();
}


# ----------------------------------------------------------------------------

=head1 BUGS

All global functions and constants in this package should probably be
instantiated into a class object. As usual there are pros and cons.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>

=cut
# ============================================================================


1;
