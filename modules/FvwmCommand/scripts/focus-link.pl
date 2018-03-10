#! xPERLx

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

####
# NAME
#    focus-link.pl - perl FvwmCommand script
####
# SYNOPSIS
#    focus-link.pl [-v]
####
# OPTION
#    -v  show version number and exit.
####
# DESCRIPTION
#    This is a user programmable window focus script.
#    It requires FvwmCommand version 1.5 or later.
#    FvwmCommandS must be invoked from fvwm prior to this command.
#
#    This script can be invoked from a shell or from .fvwm2rc. For example.
#
#   AddToFunc "InitFunction" "I" Module FvwmBanner
#   + "I" Module	FvwmPager 0 8
#   + "I" Exec    xcb -n 4 -l vertical -g 240x180-0+530 &
#   + "I" Exec sh -c "sleep 2;$HOME/scripts/focus-link.pl & "
#
#    Sleep is used in order to avoid un-necessary reaction during initial
#    window creation. A shell is invoked to avoid fvwm itself sleeps for
#    2 seconds.
#
#    Default behavior is listed below.
#    In order to change the behavior, modify user_function using user
#    functions.
#     1. When a window is opened up, focus the window and move the pointer
#        to it. The parent window regains focus when a window is closed.
#        Parenthood is determined when a window is opened. It is the last
#        focused window with the same X class.
#     2. #1 would not occur to AcroRead opening window.
#     3. #1 would not occur when SkipMapping is set and the window is the
#        only window of its class.
#     4. For Netscape find dialog window, addition to #1, resize the window
#        to 300x150 pixels and move it to East edge of the screen.
#        Download/upload windows will not be focused nor be in focus link
#        list.
#     5. Move appletviewer to NorthWest corner.
#     6. Xterm won't focus back to its parent after closed.
#     7. When a window is de-iconified, focus it and move the pointer.
#
####
# USER FUNCTIONS
#    These are collection of functions a user can call from programmable
#    section.

#  user function description are comments that start with ##

# change path if necessary
$FVWMCOMMAND = "/usr/X11/lib/X11/fvwm/FvwmCommand";
# if not there, try this
if (! -x $FVWMCOMMAND) {
  $FVWMCOMMAND = "$ENV{HOME}/usr/X11/lib/X11/fvwm/FvwmCommand";
}

#********** user configurable function **************************
sub user_function {
  if (action_was ("add")) {
    # don't do anything to opening window of acrobat reader
    return if class_matches ("AcroRead", "splashScreen_popup");

    # if skipmapping is specified with 'style' command and
    # if the window is the first of its class, then don't focus
    if (window_flag('SkipMapping') && no_parent_window()) {
      return();
    }

    # don't focus download/upload window. do not put it in focus link list
    if (class_matches ("Netscape", "(Download|upload)")) {
      delete_from_list();
      return();
    }

    focus_window();

    # move Netscape find dialog to edge
    if (class_matches ("Netscape", "find")) {
      resize_window ('300p','150p'); # resize before move to be on the edge
      move_window ("East");
      # move_window ('-0p', '-100p'); # just an example
    }
    # move appletviewer to corner
    move_window ("Northwest") if class_matches ("VendorShell", "AWTapp");

    # if you don't want move the pointer, comment out the next line
    warp_to_window (50, 50); # center of the window

  }elsif (action_was ("destroy")) {
    #don't focus back to parent if its xterm
    return if (class_matches ("XTerm"));

    focus_window (get_parent_window());
    # if you don't want move the pointer, comment out the next line
    warp_to_window (get_parent_window(), 50, 50);  #center of window

  }elsif (action_was ("deiconify")) {
    focus_window();

  }elsif (action_was ("iconify")) {
    #    focus_window (get_parent_window());
  }
}

#********** end of user configurable function **************************


init();


#package FvwmFocusLink;
#require Exporter;
#@ISA = qw(Exporter);
#
#@EXPORT = qw( &move_window
#	      &resize_window
#	      &focus_window
#	      &warp_to_window
#	      &class_matches
#	      &window_flag
#	      &resource_matches
#	      &action_was
#	      &get_parent_window
#	      &no_parent_window
#	      &delete_from_list
#              &init );
#


#
# user callable functions
#

###
# move_window [<id>] <direction>
#  or
###
# move_window [<id>] <x> <y>
#
#    If <id> is prensent in hex format, then move <id> window.
#    Otherwise, move the window in question.
#
#    If <y> is present, move window to <x> <y> in percentage of screen.
#
#    If 'p' is appended to <width> or <height>, it specifies in
#    pixel count. And, if <width'p'> or <height'p'> is lead with '-',
#    it signifies that pixel count from right or bottom edge.
#
#    If <y> does not exist, <dir> must be one of North Northeast East
#    Southeast South Southwest West Northwest to move window to edge.

sub move_window {
  my (@a) = @_;
  my ($height, $width, $x, $y, $w, $dir);

  ($id) = @a;
  if ($id =~ /^$HEX$/) {
    shift @a;
  }else{
    $id = $W->{id};
  }
  return undef if ($id eq $NULLWINDOW || $id eq ''
		   || !defined $Window{$id}
		   || defined $Window{$id}{destroy} );

  ($dir,$y) = @a;

  if (defined $y) {
    $x = $dir;
    if ($x =~ s/^-(.*)p$/$1/) {
      ($width) = ($Window{$id}{frame} =~ /width (\d+)/);
      $x = $SW - $width - $x . "p";
    }
    if ($y =~ s/^-(.*)p$/$1/) {
      ($height) = ($Window{$id}{frame} =~ /height (\d+)/);
      $y = $SH - $height - $y . "p";
    }
    send_cmd("windowid $id move $x $y", $id, "^$id $ACTPAIR{frame}");

  }else{
    ($x,$y,$width,$height) =
      ($Window{$id}{frame}
       =~ /x (-?\d+), y (-?\d+), width (\d+), height (\d+)/);

    if ($dir =~ /[Ee]ast/) {
      $x = $SW - $width;
    }elsif ($dir =~ /[Ww]est/) {
      $x = 0;
    }
    if ($dir =~ /[Nn]orth/) {
      $y = 0;
    }elsif ($dir =~ /[Ss]outh/) {
      $y = $SH - $height;
    }

    send_cmd("windowid $id move ${x}p ${y}p\n", $id, "^$id $ACTPAIR{frame}");
  }
  !defined $Window{$id}{'destroy'};
}

###
# resize_window [<id>] <width> <height>
#
#    Resize window to <width> and <height> in percentage of screen size.
#
#    If <id> is not null, resize <id>. Otherwise resize the
#    window in question.
#
#    Letter 'p' can be appended to <width> and <height> to specify in
#    pixel count.

sub resize_window {
  my ($id,$wd,$ht) = @_;
  if (!defined $ht) {
    $ht = $wd;
    $wd = $id;
    $id = $W->{id};
  }
  return undef if ($id eq $NULLWINDOW || $id eq ''
		   || !defined $Window{$id}
		   || defined $Window{$id}{destroy} );
  send_cmd("windowid $id resize $wd $ht\n", $id, "^$id $ACTPAIR{frame}");
  !defined $Window{$id}{'destroy'};
}

###
# focus_window [<id>]
#
#    If <id> is not null, focus on <id>.
#    Otherwise, focus on the window in question.

sub focus_window {
  my ($id) = @_;
  my ($l);

  if (!defined $id) {
    $id = $W->{id};
  }
  return undef if ($id eq $NULLWINDOW || $id eq ''
		   || !defined $Window{$id}
		   || defined $Window{$id}{destroy} );

  send_cmd("windowid $id focus\n");
  keep_last_focused ($id);
  !defined $Window{$id}{'destroy'};
}

###
# warp_to_window [<id>] [<x> <y>]
#
#    Move pointer to window.
#
#    If <id> is a window id, warp to <id>.
#    Otherwise, warp to the window in question.
#
#    If <x> and <y> are present, warp to <x> and <y> percentage of window
#    size down and in from the upper left hand corner.
#
#    Letter 'p' can be appended to <width> and <height> to specify in pixel
#    count.

sub warp_to_window {
  my (@a) = @_;

  my ($id) = @a;
  if ($id !~ /^$HEX$/) {
    $id = $W->{id};
  }else{
    shift @a;
  }
  return undef if ($id eq $NULLWINDOW || $id eq ''
		   || !defined $Window{$id}
		   || defined $Window{$id}{destroy} );

  my ($x, $y) = @a;

  # ensure both exists or none
  if (!defined $x || !defined $y) {
    $x = $y = '';
  }
  send_cmd ("windowid $id WarpToWindow $x $y");
  !defined $Window{$id}{'destroy'};
}

###
# class_matches <class> [<resource>]
#
#    Check if window class and optional resource match.
#
#    If arg1 is present, and if class matches with <class> and resource
#    matches with <resource>, then return 1.
#
#    If arg1 is not present, and if class matches with <class> then
#    return 1.
#    Otherwise, return null.
sub class_matches {
  my($c,$r) = @_;
  if (defined $r) {
    return $W->{class} =~ /$c/ && $W->{resource} =~ /$r/;
  }
  return $W->{class} =~ /$c/;
}

###
# window_flag [<id>] <flag>
#
#    Return 1 if <flag> is true in the window in question.
#    If <id> is not null, check on <id>. Otherwise check on the
#    window in question.
#    <flag> must be a exact match to one of these:
#
#  StartIconic
#  OnTop
#  Sticky
#  WindowListSkip
#  SuppressIcon
#  NoiconTitle
#  Lenience
#  StickyIcon
#  CirculateSkipIcon
#  CirculateSkip
#  ClickToFocus
#  SloppyFocus
#  SkipMapping
#  Handles
#  Title
#  Mapped
#  Iconified
#  Transient
#  Visible
#  IconOurs
#  PixmapOurs
#  ShapedIcon
#  Maximized
#  WmTakeFocus
#  WmDeleteWindow
#  IconMoved
#  IconUnmapped
#  MapPending
#  HintOverride
#  MWMButtons
#  MWMBorders
sub window_flag {
  my ($id, $f) = @_;
  if (!defined $f) {
    $f = $id;
    $id = $W->{id};
  }

  return undef if ($id eq $NULLWINDOW || $id eq '');
  return $Window{$id}{$f} eq 'yes';
}

###
# resource_matches <resource>
#    Check if window resource matches pattern <resource>.
#    If it matches, return 1.
#    Otherwise return null.
sub resource_matches {
  my($r) = @_;
  return $W->{resource} =~ /$r/;
}

###
# action_was <action>
#    Check if <action> was taken place.
#
#    <action> must be a exact match to one of these:
#
#  new page
#  new desk
#  add
#  raise
#  lower
#  focus change
#  destroy
#  iconify
#  deiconify
#  windowshade
#  dewindowshade
#  end windowlist
#  icon location
#  end configinfo
#  string
sub action_was {
  my ($act) = @_;
  return $W->{act} eq $act;
}

###
# get_parent_window [<id>]
#
#    Return parent window id.
#
#    If <id> is not null, check on <id>. Otherwise check on the
#    window in question.
sub get_parent_window {
  my ($id) = @_;
  if (!defined $id) {
    $id = $W->{id};
  }
  return $NULLWINDOW if ($id eq $NULLWINDOW || $id eq ''
			 || !defined $Window{$id} );

  if (defined $Window{$id}{parent}) {
    $Window{$id}{parent};
  }else{
    $NULLWINDOW;  # must be an orphan
  }
}

###
# no_parent_window [<id>]
#
#    Return 1 if no parent window exits.
#
#    If <id> is not null, check on <id>. Otherwise check on the
#    window in question.
sub no_parent_window {
  return get_parent_window(@_) eq $NULLWINDOW;
}

###
# delete_from_list
#
#    Delete the window from link list
sub delete_from_list {
  my ($id);
  my ($f, $i);

  return if $W->{id} eq $NULLWINDOW || $W->{id} eq '';
  $id = $W->{id};
  $f = $Focus{$W->{class}};
  foreach $i (0..1) {
    if ($f->[$i] eq $id) {
      $f->[$i] = $NULLWINDOW;
    }
  }
  # adopt orphans
  foreach $i (keys %Window) {
    if ($Window{$i}{parent} eq $id) {
      # avoid being parent of itself
      if ($Window{$id}{parent} ne $i) {
	$Window{$i}{parent} = $Window{$id}{parent};
      }else{
	$Window{$i}{parent} = $NULLWINDOW;
      }
    }
  }
  undef %{$Window{$id}};
  delete $Window{$id};
}


#
# Supporting routines
#

# add_to_list
#    Add the window to link list.
#    Link points back to the last focused window among the same class
sub add_to_list {
  return if ( $W->{id} eq '' || $W->{id} eq $NULLWINDOW);
  if (!defined @{$Focus{$W->{class}}}) {
    # no parent listed
    $W->{parent} = $NULLWINDOW;
    $Focus{$W->{class}}[0] = $NULLWINDOW;
  }else{
    $W->{parent} = $Focus{$W->{class}}[0];
    # can't be parent of itself
    if ($W->{parent} eq $W->{id}) {
      $W->{parent} = $Focus{$W->{class}}[1];
      if ($W->{parent} eq $W->{id}) {
	$W->{parent} = $NULLWINDOW;
      }
    }
  }
}


# Send command
# Optionally wait for a keyword to come back
sub send_cmd {
  my($cmd,$id,$key)=@_;
  my($cid);

  $cmd =~ s/\n*$/\n/;		# ensure 1 cr at eol
  # ensure not to send command for window already destroyed
  if ($cmd =~ /^windowid ($HEX)/) {
    $cid = $1;
    return if ($cid eq $NULLWINDOW || $cid eq '' ||
	       !defined $Window{$cid} ||
	       defined $Window{$cid}{destory} );
  }

  print FCC	$cmd;
  print STDERR	$cmd if $Debug & 2;

  # if specified wait  for the keyword
  if (defined $key) {
    read_message($id,$key);
  }
}

# Wait until action defined in $STATUS occure, then call process.
# If a window focused, keep in focus list as the last window
# among the class.
# Create current window info from lines leaded with the same id
# number.
sub next_action {
  my ($act, $id);

  read_message();

  undef ($id);
  ($act) = /^\s+(\w+(\s\w+)?)/;  # such as "new page"
  if (!$act) {
    ($id, $act) = (/^($HEX) ($STATUS)/);
  }

  #undef $W;
  if ($id ne '') {
    if ($act eq 'add') {
      # create new window info
      $Window{"$id"} = {id=>"$id"};
    }
    $W = $Window{"$id"};
    if (defined $ACTPAIR{$act}) {
      # multi line action
      read_message($id, "$id $ACTPAIR{$act}");
    }
  } else {
    $W = {};
  }

  $W->{act} = $act;
}

# read message fifo or stack
# if $id is defined and the message is not for $id then
# push it on to the stack and read it again
# if $key is also defined, loop until $key is matched
sub read_message {
  my ($id,$key) = @_;
  my ($sk);
  $sk = 0;
  LOOP_ID: {
    do {
      if ($sk==0 && $#Message >= 0) {
	$_ = shift (@Message);
      }else{
	$_ = <FCM>;
	if (/^$/ && eof(FCM)) {
	  eof_quit()
	}
      }
    } while (/^\s*$/);
    if (/^($HEX) ($STATUS)\s*(.*)/ && $1 ne $NULLWINDOW) {
      $Window{$1}{$2} = $3;
      if ($2 eq 'class') {
	$Window{$1}{id} = $1;
	$Window{$1}{parent} = $NULLWINDOW;
	push (@{$Focus{$3}}, $1);
      }
    }

    # for $id only
    if ($id ne '') {
      if (!/^$id/) {
	push (@Message,$_);
	$sk = 1;  # don't read from @Message
	redo LOOP_ID;
      }
      return undef if !defined $Window{$id};
      return undef if defined $Window{$id}{destroy};
    }

    # wait for keyword
    if (defined $key && !/$key/) {
      $sk = 1;
      redo LOOP_ID;
    }
  }
}

# Keep the last 2 focused windows of each class.
# This allows focus_window and add_to_list to be any order
sub keep_last_focused {
  my ($id) = @_;
  my ($l);

  return undef if ($id eq ''
		   || $id eq $NULLWINDOW
		   || !defined $Window{$id}
		   || defined $Window{$id}{destroy} );


  # consider for cases that Focus is not defined yet
  if (!defined $Focus{$Window{$id}{class}}) {
    @{$Focus{$Window{$id}{class}}} = ($NULLWINDOW);
  }
  $l = $Focus{$Window{$id}{class}};
  if ($l->[0] ne $id) {
    # only if changed
    unshift (@$l, $id);
    splice (@$l,2);
  }
}

sub kill_server {
  send_cmd ("killme"); # kill FvwmCommandS
  close (FCM);
  close (FCC);
}

# error. quit.
sub eof_quit {
  print STDERR "FCM EOF ";
  exit 1;
}

# signal handler
sub sig_quit {
  my($sig) = @_;
  print STDERR "signal $sig\n";
  exit 2;
}

#
# debug
#

# print window list
sub print_list {
  my($k, $w);
  foreach $k (keys %Window) {
    $w = $Window{$k};
    print (STDERR "id $w->{id} parent $w->{parent}  " ,
	   "class $w->{class} \tresource $w->{resource}\n");
  }
  print STDERR "\n";
}

# print the current list of parents for each class
sub print_focused_list {
  my ($k);
  local($,);
  $, = ";";
  foreach $k (keys %Focus) {
    print (STDERR "class $k parents @{$Focus{$k}}\n");
  }
  print STDERR "\n";
}

#
# initialize
#   setup new fifos
#   get screen size - to move
#   get window list
#
#   debug option -d<N>
#         1 to print window info
#         2 to print command sent
#         4 to print focused window list
#         8 to print action
sub init {
  my ($count, $type);

  $Debug = 0;
  if ($ARGV[0] =~ /^-v/) {
    print STDERR "focus-link.pl version 2.0\n";
    exit;
  }
  if ($ARGV[0] =~ /^-d(\d+)/) {
    $Debug = $1;
    shift @ARGV;
  }

  $SIG{'TTIN'} = "IGNORE";
  $SIG{'TTOUT'} = "IGNORE";
  $SIG{'QUIT'} = "sig_quit";
  $SIG{'INT'} = "sig_quit";
  $SIG{'HUP'} = "sig_quit";
  $SIG{'PIPE'} = "sig_quit";

  $STATUS = '(?:\w+(?:\s\w+)?)';   #status to be captured
  # matching pair for some action
  %ACTPAIR = ('add'=>'map',
	      'deiconify'=>'map',
	      'iconify'=>'lower',
	      'frame'=>'pixel' );

  $HEX = '0x[\da-f]+'; #for regex
  $NULLWINDOW = '0x00000000';   # root or invalid window id

  $FIFO = "$ENV{'HOME'}/.FCMfocus";

  # screen width
  if( `xwininfo -root` =~ /Width: (\d+)\s+Height: (\d+)/ ) {
    $SW = $1;
    $SH = $2;
  }else{
    # some resonable number  if xwininfo doesn't work
    $SW = 1024;
    $SH = 786;
  }

  # start a dedicated server
  # start a client monitoring (-m option ) all fvwm transaction (-i3 option )
  # unlinking and verifying M FIFO ensures that new FvwmCommand is running
  unlink( "${FIFO}M" );
  open( FCM, "$main::FVWMCOMMAND -S $FIFO -f $FIFO  -m -i3 </dev/null|" )
    || die "FCM $FIFO";
  while (! -p "${FIFO}M") {
    sleep(1);
    if ($count++ > 5) {
      die "Can't open ${FIFO}M";
    }
  }

  # send command through the new fifo which is "$FIFO" + "C"
  open( FCC, ">${FIFO}C" ) || die "FCC $FIFO" ;

  # appearantly, it has to be unbuffered
  select( FCC ); $| = 1;
  select( STDOUT ); $| = 1;

  %Focus = (); # last focused windows for each class
  @Message = (); # message queue

  %Window = ();

  #get current screen info
  send_cmd("Send_WindowList\n");

#  while(<FCM>) {
#    last if /^end windowlist/;
#
#    # create window list
#    if (/^($HEX) ($STATUS)\s*(.*)/) {
#      if ($1 ne $NULLWINDOW) {
#	$Window{$1}{$2} = $3;
#	# make focused window list
#	if ($2 eq 'class') {
#	  $Window{$1}{id} = $1;
#	  $Window{$1}{parent} = $NULLWINDOW;
#	  push (@{$Focus{$3}}, $1);
#	}
#      }
#    }
#  }

  print_list() if ($Debug & 1);
  print_focused_list() if ($Debug & 4);
  main_loop();
}

sub main_loop {
   while (1) {
     next_action ();
     $Window{$W->{id}} = $W if $W->{id} ne '';
     add_to_list() if action_was("add");
     print STDERR "Action $W->{act} $W->{id}\n" if $Debug & 8;
     main::user_function();
     if ($W->{act} eq 'destroy') {
       delete_from_list();
     }
     if ($W->{act} eq 'focus change') {
       keep_last_focused ($W->{id});
     }
     print_list() if ($Debug & 1 && !action_was("focus change")) ;
     print_focused_list() if ($Debug & 4 && !action_was("focus change"));
   }
}

END {
  kill_server();
  print STDERR "end\n";
}

1;

####
# SEE ALSO
#    FvwmCommand
####
# AUTHOR
#    Toshi Isogai  isogai@ucsub.colorado.edu
