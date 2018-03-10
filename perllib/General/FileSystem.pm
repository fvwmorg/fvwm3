# Copyright (C) 1998-2009, Mikhael Goikhman
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
# along with this program; if not, see: <http://www.gnu.org/licenses/>

package General::FileSystem;
require 5.004;
use strict;

use vars qw(@ISA @EXPORT);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(
	load_file save_file append_file remove_file copy_file move_file
	make_dir make_path clean_dir remove_dir copy_dir move_dir
	list_filenames find_file find_executable
	default_dir_perm preserve_stat parse_path get_cwd
);

use vars qw($CACHE_FILE_NUM $cache_counter @prev_filenames @prev_file_content_refs);
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
	$ERROR_HANDLER      ||= "warn";   # may be "die", "warn", "quiet" or CODE
	$DEBUG_ENABLED      ||= 0;
	$LOAD_FILE_DIRS     ||= [ "." ];  # for non fully qualified files only
	$SAVE_FILE_DIR      ||= ".";      # for non fully qualified files only
}


# ----------------------------------------------------------------------------

=head1 NAME

General::FileSystem - file system specific functions

=head1 SYNOPSIS

  use General::FileSystem "-die", "-debug";  # die on errors

  eval {
    make_path("/tmp/my-own/dir");

    my $file_content_ref = load_file("/etc/issue");
    save_file("/tmp/my-own/dir/issue", $file_content_ref);

    # This is equivalent to the previous two lines, but optimized
    copy_file("/etc/issue", "/tmp/my-own/dir/issue");

    make_dir("/tmp/my-own/dir2", 0711);
    copy_file("/etc/issue", "/tmp/my-own/dir2/issue");
    move_file("/tmp/my-own/dir2/issue", "/tmp/my-own/dir2/issue2");
    remove_file("/tmp/my-own/dir2/issue2");
    clean_dir("/tmp/my-own/dir2"); # no effect, it's empty already

    remove_dir("/tmp/my-own");
  };
  if ($@) {
    print "File System Error: $@";
  };

or just:

  use General::FileSystem;
  copy_file("origin.txt", "backup.txt");

=head1 DESCRIPTION

This package contains common file operation functions:

B<load_file>, B<save_file>, B<append_file>, B<remove_file>, B<copy_file>, B<move_file>,
B<make_dir>, B<make_path>, B<clean_dir>, B<remove_dir>, B<copy_dir>, B<move_dir>,
B<list_filenames>, B<find_file>, B<find_executable>,
B<default_dir_perm>, B<preserve_stat>, B<parse_path>, B<get_cwd>.

On fatal file system errors all functions call the error handler, that may
throw exception (die), issue a warning or quietly return undef.
You may control this by passing one of the arguments I<-die>, I<-warn>
or I<-quiet> in B<use> or by setting C<$ERROR_HANDLER> to one of these
values (don't specify a dash in this case).

=head1 REQUIREMENTS

L<Cwd>, L<File::Basename>, L<File::Copy>.

=head1 FUNCTIONS

=cut
# ============================================================================


use Cwd;
use File::Basename;
use File::Copy;


sub import ($;$) {
	my $package = shift;
	while (@_ && $_[0] =~ /^-/) {
		local $_ = shift;
		$ERROR_HANDLER = $1 if /^-(die|warn|quiet)$/i;
		$DEBUG_ENABLED = $1 if /^-(debug)$/i;
	}
	$package->export_to_level(1, @_);
}


# private function
sub call_error_handler ($) {
	my $msg = shift;
	die  "$msg: [$!]\n" if $ERROR_HANDLER eq "die";
	warn "$msg: [$!]\n" if $ERROR_HANDLER eq "warn";
	return undef if $ERROR_HANDLER eq "quiet";
	&$ERROR_HANDLER($msg) if ref($ERROR_HANDLER) eq "CODE";
	return undef;
}


# private function
sub print_log ($) {
	my $msg = shift;
	return unless $DEBUG_ENABLED;
	print STDERR "FileSystem: $msg\n";
}


# ----------------------------------------------------------------------------

=head2 load_file

=over 4

=item usage

  $content_ref = load_file($filename)

=item description

Loads file with given file-name from local filesystem.

=item parameters

  * filename - name of the file to be loaded.

=item returns

Reference to file content string on success, otherwise either dies or warns
and returns undef as configured.

=back

=cut
# ============================================================================


BEGIN {
	$CACHE_FILE_NUM = 6;
	$cache_counter = -1;
	@prev_filenames = ("", "", "", "", "", "");
	@prev_file_content_refs = \("", "", "", "", "", "");
}


sub load_file ($) {
 	my $filename = shift;

	foreach (@$LOAD_FILE_DIRS) {
		if (-f "$_/$filename") { $filename = "$_/$filename"; last; }
	}
	print_log("Loading file $filename") if $DEBUG_ENABLED;

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1) {
			if ($filename eq $prev_filenames[$_] && -r $filename) {
				print_log("getting from file cache") if $DEBUG_ENABLED;
				return $prev_file_content_refs[$_];
			}
		}
	}

	open(FILE, "<$filename") || return call_error_handler("Can't open $filename");
	my $content = join("", <FILE>);
	close(FILE) || return call_error_handler("Can't close $filename");

	if ($ENABLE_CACHE) {
		$cache_counter = ($cache_counter+1) % $CACHE_FILE_NUM;
		$prev_filenames[$cache_counter] = $filename;
		$prev_file_content_refs[$cache_counter] = \$content;
	}
	return \$content;
}


# ----------------------------------------------------------------------------

=head2 save_file

=over 4

=item description

Saves file-content to local filesystem with given file-name.

=item usage

  save_file($filename, \$content);

=item parameters

  * filename - name of the file to be saved into
  * content_ref - reference to file content string
  * create_subdirs - optional flag (default is 0 - don't create subdirs)

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub save_file ($$;$) {
	my ($filename, $content_ref, $create_dirs) = @_;

	if ($filename !~ m=^[/\\]|\w:\\=) {
		$filename = "$SAVE_FILE_DIR/$filename";
	}
	print_log("Saving  file $filename") if $DEBUG_ENABLED;
	die("save_file: No SCALAR ref parameter\n")
		unless ref($content_ref) eq 'SCALAR';

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1) {
			$prev_file_content_refs[$_] = $content_ref
				if $filename eq $prev_filenames[$_];
		}
	}
	if ($create_dirs) {
		my $dirname = dirname($filename);
		make_path($dirname) unless -d $dirname;
	}

	open(FILE, ">$filename") || return call_error_handler("Can't open $filename");
	print FILE $$content_ref;
	close(FILE) || return call_error_handler("Can't close $filename");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 append_file

=over 4

=item description

Appends file-content to local filesystem with given file-name.

=item usage

  append_file($filename, \$appended_content);

=item parameters

  * filename - name of the file to be saved into
  * appended_content_ref - reference to appended-content string

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub append_file ($$) {
	my ($filename, $appended_content_ref) = @_;
	print_log("Append>>file $filename") if $DEBUG_ENABLED;

	if ($ENABLE_CACHE) {
		for (0 .. $CACHE_FILE_NUM-1 && -r $filename) {
			${$prev_file_content_refs[$_]} .= $$appended_content_ref
				if $filename eq $prev_filenames[$_];
		}
	}

	open(FILE, ">>$filename") || return call_error_handler("Can't append to $filename");
	print FILE $$appended_content_ref;
	close(FILE) || return call_error_handler("Can't close $filename");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 remove_file

=over 4

=item description

Removes all files from given directory.

=item usage

  remove_file($filename);

=item parameters

  * filename - name of the file to be deleted

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub remove_file ($;$) {
	my $filename = shift;
	print_log("Removin file $filename") if $DEBUG_ENABLED;
	unlink($filename) || return call_error_handler("Can't unlink $filename");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 make_dir

=over 4

=item description

Removes all files from given directory.

=item usage

  make_dir($PREVIEW_DIR);

=item parameters

  * directory to make
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub make_dir ($;$) {
	my $dirname = shift;
	my $perm = shift || $DEFAULT_DIR_PERM;
	print_log("Creating dir $dirname, " . sprintf("%o", $perm))
		if $DEBUG_ENABLED;
	mkdir($dirname, $perm) || return call_error_handler("Can't mkdir $dirname");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 make_path

=over 4

=item description

Removes all files from given directory.

=item usage

  make_path($PUBLISH_DIR);

=item parameters

  * path to make
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub make_path ($;$) {
	my $dirname = shift;
	my $perm = shift || $DEFAULT_DIR_PERM;
	print_log("Making  path $dirname, " . sprintf("%o", $perm))
		if $DEBUG_ENABLED;

	return 1 if -d $dirname;
	my $parent_dir = dirname($dirname);

	local $DEBUG_ENABLED = 0;
	&make_path($parent_dir, $perm) unless -d $parent_dir;
	make_dir($dirname, $perm);

	return 1;
}


# ----------------------------------------------------------------------------

=head2 copy_file

=over 4

=item description

Copies a file to another location.

=item usage

  copy_file($from, $to);

=item parameters

  * file name to copy from
  * file name to copy to

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub copy_file ($$) {
	my ($src_filename, $dst_filename) = @_;
	print_log("Copying file $src_filename to $dst_filename")
		if $DEBUG_ENABLED;

	# Must manage symbolic links somehow
	# return if -l $src_filename;

	copy($src_filename, $dst_filename)
		or return call_error_handler("Can't copy $src_filename $dst_filename");

	if ($PRESERVED_STAT) {
		my ($device, $inode, $mode) = stat($src_filename);
		chmod($mode, $dst_filename);
	}
	return 1;
}


# ----------------------------------------------------------------------------

=head2 move_file

=over 4

=item description

Moves (or renames) a file to another location.

=item usage

  move_file($from, $to);

=item parameters

  * file name to move from
  * file name to move to

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub move_file ($$) {
	my ($src_filename, $dst_filename) = @_;
	print_log("Moving  file $src_filename to $dst_filename")
		if $DEBUG_ENABLED;

	move($src_filename, $dst_filename)
		or return call_error_handler("Can't move $src_filename $dst_filename");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 clean_dir

=over 4

=item description

Removes all files from given directory.

=item usage

  clean_dir($PREVIEW_DIR);

=item parameters

  * directory to clean
  * optional flag:
    0 - don't go recursively, unlink files in first level only
    1 - recursively clean subdirs (default)
    2 - unlink subdirs
    3 - unlink given directory

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub clean_dir ($;$) {
	my $dirname = shift;
	my $recursive = shift || 1;
	die("clean_dir: Unsupported flag $recursive\n")
		if $recursive > 3 || $recursive < 0;
	print_log(($recursive != 3 ? "Cleaning" : "Removing") . " dir $dirname "
		. ["files only", "recursively files only", "recursively", "completely"]->[$recursive])
		if $DEBUG_ENABLED;

	local $DEBUG_ENABLED = 0;

	my @subdirs = ();
	my $filenames = list_filenames($dirname);

	# process files
	foreach (@$filenames) {
		next if $NEVER_REMOVE_FILES{$_};
		my $filename = "$dirname/$_";
		if (-d $filename) { push @subdirs, $filename; }
		else { unlink("$filename") || return call_error_handler("Can't unlink $filename"); }
	}

	# process subdirs
	map {
		clean_dir($_, $recursive);
		rmdir($_) || return call_error_handler("Can't unlink $_") if $recursive == 2;
	} @subdirs if $recursive;
	rmdir($dirname) || return call_error_handler("Can't unlink $dirname") if $recursive == 3;

	return 1;
}


# ----------------------------------------------------------------------------

=head2 remove_dir

=over 4

=item description

Entirely removes given directory and its content (if any).
This is an alias to C<clean_dir(3)>.

=item usage

  remove_dir($TMP_DIR);

=item parameters

  * directory to clean

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub remove_dir ($) {
	my $dirname = shift;
	return clean_dir($dirname, 3);
}


# ----------------------------------------------------------------------------

=head2 copy_dir

=over 4

=item description

Recursively copies all files and subdirectories inside given directory
to new location.

Destination directory must not exist. Use: C<trap { remove_dir($dest); };>
to remove it before copying.

=item usage

  copy_dir($dir_from, $dir_to);

=item parameters

  * source directory to copy
  * destination directory to copy to (may not exist)
  * optional creating dir permissions (default is $DEFAULT_DIR_PERM)

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub copy_dir ($$) {
	my ($src_dirname, $dst_dirname, $perm) = @_;

	return call_error_handler("Directory $src_dirname does not exist")
		unless -d $src_dirname;
	make_dir($dst_dirname, $perm) unless -d $dst_dirname;

	print_log("Copying  dir $src_dirname to $dst_dirname recursively")
		if $DEBUG_ENABLED;;

	local $DEBUG_ENABLED = 0;

	my $error = 0;
	my @subdirs = ();
	my $filenames = list_filenames($src_dirname);

	# process files
	foreach (@$filenames) {
		next if $NEVER_COPY_FILES{$_};
		my $src_filename = "$src_dirname/$_";
		my $dst_filename = "$dst_dirname/$_";
		if (-d $src_filename) { push @subdirs, $_; }
		elsif (-l $src_filename) { next if "# We ignore links for now! TO FIX!" }
		else { copy_file($src_filename, $dst_filename) or $error = 1; }
	}

	# process subdirs
	foreach (@subdirs) {
		my $src_subdirname = "$src_dirname/$_";
		my $dst_subdirname = "$dst_dirname/$_";
		&copy_dir($src_subdirname, $dst_subdirname) or $error = 1;
	}

	return call_error_handler("Errors while copying some files/subdirs in $src_dirname to $dst_dirname")
		if $error;
	return 1;
}


# ----------------------------------------------------------------------------

=head2 move_dir

=over 4

=item description

Moves (or actually renames) a directory to another location.

Destination directory must not exist. Use: C<trap { remove_dir($dest); };>
to remove it before copying.

=item usage

  move_dir($dir_from, $dir_to);

=item parameters

  * source directory to move from
  * destination directory to move to (must not exist)

=item returns

C<1> on success, otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub move_dir ($$) {
	my ($src_dirname, $dst_dirname) = @_;
	print_log("Moving   dir $src_dirname to $dst_dirname")
		if $DEBUG_ENABLED;

	rename($src_dirname, $dst_dirname)
		or return call_error_handler("Can't rename $src_dirname $dst_dirname");
	return 1;
}


# ----------------------------------------------------------------------------

=head2 list_filenames

=over 4

=item description

Returns the file names in the given directory including all types of files
(regular, directory, link, other), not including '.' and '..' entries.

=item usage

  # mini file lister
  $dir = '/home/ftp';
  foreach my $file (@{list_filenames($dir)}) {
    print "File $file\n" if -f "$dir/$file";
    print "Dir  $file\n" if -d "$dir/$file";
  }

=item parameters

  * directory to list (or array ref of directories)
  * optional flag, 1 means work recursively, the default is 0

=item returns

Array ref of scalars (file names) on success.
Otherwise either dies or warns and returns undef as configured.

=back

=cut
# ============================================================================


sub list_filenames ($;$) {
	my $dirname = shift;
	my $recursive = shift || 0;
	if (ref($dirname) eq "ARRAY") {
		my @files = ();
		foreach (@$dirname) { push @files, &list_filenames($_); }
		return \@files;
	}
	print_log("Listing  dir $dirname") if $DEBUG_ENABLED;

	opendir(DIR, $dirname) || return call_error_handler("Can't opendir $dirname");
	my @files = grep { $_ ne '.' && $_ ne '..' } readdir(DIR);
	closedir(DIR) || return call_error_handler("Can't closedir $dirname");

	if ($recursive) {
		my $i = 0;
		for (; $i < @files; ) {
			my $subdir = $files[$i];
			if (-d "$dirname/$subdir") {
				splice(@files, $i, 1, map { "$subdir/$_" }
					@{&list_filenames("$dirname/$subdir")});
			} else {
				$i++;
			}
		}
	}

	return \@files;
}


# ----------------------------------------------------------------------------

=head2 find_file

=over 4

=item description

Searches for the given file in the given directories.

Returns the fully qualified file name.

=item parameters

  * file name to search for
  * array ref of directories to search in

=item returns

File name with full path if found, or undef if not found.

=back

=cut
# ============================================================================


sub find_file ($$) {
	my $filename = shift;
	my $dirs = shift();
	die "find_file: no dirs given\n" unless ref($dirs) eq "ARRAY";
	foreach (@$dirs) {
		my $file_path = "$_/$filename";
		return $file_path if -f $file_path;
	}
	return undef;
}


# ----------------------------------------------------------------------------

=head2 find_executable

=over 4

=item description

Searches for the given executable file in the directories that are in the
environmebt variable $PATH or in the additional parameter.

Returns the fully qualified file name.

=item usage

  my $gzip_exe = find_executable("gzip", ["/usr/gnu/bin", "/gnu/bin"]);

=item parameters

  * file name to search for (only executables are tested)
  * optional array ref of extra directories to search in

=item returns

File name with full path if found, or undef if not found.

=back

=cut
# ============================================================================


sub find_executable ($;$) {
	my $filename = shift;
	my $extra_dirs = shift;
	my @dirs = split(":", $ENV{"PATH"} || "");
	if (ref($extra_dirs) eq "ARRAY") {
		push @dirs, @$extra_dirs;
	}
	foreach (@dirs) {
		my $file_path = "$_/$filename";
		return $file_path if -x $file_path;
	}
	return undef;
}


# ----------------------------------------------------------------------------

=head2 default_dir_perm

=over 4

=item description

This functions changes default directory permissions, used in
C<make_dir>, C<make_path>, C<copy_dir> and C<move_dir> functions.

The default of this package is 0775.

If no parameters specified, the current value is returned.

=item usage

 default_dir_perm(0700);

=item parameters

  * optional default directory permission (integer)

=item returns

Previous value.

=back

=cut
# ============================================================================


sub default_dir_perm (;$) {
	return if $^O =~ /Win|DOS/;
	my $new_value = shift;
	my $old_value = $DEFAULT_DIR_PERM;

	if (defined $new_value) {
		print_log("default_dir_perm = $new_value") if $DEBUG_ENABLED;
		$DEFAULT_DIR_PERM = $new_value;
	}
	return $old_value;
}


# ----------------------------------------------------------------------------

=head2 preserve_stat

=over 4

=item description

This functions changes behavior of C<copy_file> and C<copy_dir> functions.
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

  preserve_stat(1);

=item parameters

  * optional flag (currently 0 or 1)

=item returns

Previous value.

=back

=cut
# ============================================================================


sub preserve_stat (;$) {
	return if $^O =~ /Win|DOS/;
	my $new_value = shift;
	my $old_value = $PRESERVED_STAT;

	if (defined $new_value) {
		print_log("preserve_stat = $new_value") if $DEBUG_ENABLED;
		$PRESERVED_STAT = $new_value;
	}
	return $old_value;
}


# ----------------------------------------------------------------------------

=head2 parse_path

=over 4

=item usage

  my ($dirname, $basename) = parse_path($filename);

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


sub parse_path ($) {
	my $path = shift;
	if ($path =~ m=^(.*)[/\\]+([^/\\]*)$=) {
		return ($1, $2);
	} else {
		# support even funny dos form c:file
		return $path =~ m=^(\w:)(.*)$=
			? ($1 . ".", $2)
			: (".", $path);
	}
}


# ----------------------------------------------------------------------------

=head2 get_cwd

=over 4

=item usage

  my $cwd = get_cwd();

=item description

Returns the current working directory.

=back

=cut
# ============================================================================


sub get_cwd () {
	$^O eq "MSWin32" ? Win32::GetCwd() : require "getcwd.pl" && getcwd();
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
