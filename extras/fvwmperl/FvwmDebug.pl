#!/usr/bin/perl
# Set the above to wherever your perl interpreter is.  (Duh.)

# FvwmDebug - Perl implementation
# (c)1996 Dan Astoorian <djast@utopia.csas.com>

# Note: This requires fvwmmod.pl installed in your @INC path:
require 'fvwmmod.pl';

########################################################################
# Mainline start
########################################################################

($winid, $contxt, @CmdArgs) = &InitModule;

require 'getopts.pl';

# Options:
@ARGV = @CmdArgs;
&Getopts('hs:f:m:M:cw');

#for debugging:
#$opt_f="/dev/ttyp2" unless ($opt_f);

if (defined $opt_f) {
    close(STDOUT);
    open(STDOUT, "> $opt_f");
    select(STDOUT);
    $| = 1;
}

if (defined $opt_h) {
    print <<"END";
-h:        This help message
-s <secs>: Exit after <secs> seconds
-f <file>: Send output to <file> instead of FVWM's stdout
-m <mask>: Show only packets with types in <mask>
-M <mask>: Show only packets with types NOT in <mask>
-c:        Send an initial Send_ConfigInfo packet to FVWM
-w:        Send an initial Send_WindowList packet to FVWM
END
    &EndModule;
    exit;
}

if (defined $opt_s) {
    $SIG{"ALRM"} = Quit;
    alarm $opt_s;
}
print "$0 started:\n\t window id $winid\n\t context $contxt\n\t args ";
foreach (@CmdArgs) {
    print "\"$_\" ";
}
print "\n";

&AddHandler($M_NEW_PAGE, 'ListNewPage');
&AddHandler($M_NEW_DESK, 'ListNewDesk');
&AddHandler($M_ADD_WINDOW | $M_CONFIGURE_WINDOW, 'ListConfigWin');
&AddHandler($M_LOWER_WINOW | $M_RAISE_WINDOW | $M_DESTROY_WINDOW |
    $M_DEICONIFY |$M_MAP, 'ListWindow');
&AddHandler($M_FOCUS_CHANGE, 'ListFocus');
&AddHandler($M_ICONIFY | $M_ICON_LOCATION, 'ListIcon');
&AddHandler($M_WINDOW_NAME | $M_ICON_NAME | $M_RES_CLASS | $M_RES_NAME,
    'ListName');
&AddHandler($M_CONFIG_INFO | $M_ERROR, 'ListConfigInfo');
&AddHandler($M_END_WINDOWLIST | $M_END_CONFIG_INFO, 'ListEndConfigInfo');

if (defined $opt_m || defined $opt_M) {
    local($mask) = ($MAX_MASK);

    if ($opt_m) {
	$mask &= oct($opt_m);
    }
    if ($opt_M) {
	$mask &= (oct($opt_M) ^ $MAX_MASK);
    }
    &SendInfo(0, "Set_Mask $mask");
}

&SendInfo(0, "Send_ConfigInfo") if (defined $opt_c);
&SendInfo(0, "Send_WindowList") if (defined $opt_w);

&EventLoop;

&Quit;

########################################################################
# Subroutines
########################################################################

# tohex - convert a list of values from decimal to hex
sub tohex {
    foreach (@_) {
	$_ = sprintf("%lx", $_);
    }
}

sub Quit {
    print "$0 exiting\n";
    &EndModule;
    exit;
}

sub ListNewPage { 
    local($type, $x, $y, $desk) = @_;
    print "new page\n\t x $x\n\t y $y\n\t desk $desk\n";
    1;
}
sub ListNewDesk {
    local($type, $desk) = @_;
    print "new desk\n\t desk $desk\n";
    1;
}

sub ListConfigWin { 
    local($type, $id, $fid, $ptr, $x, $y, $w, $h,
	$desk, $flags, $th, $bw, $wbw, $wbh,
	$wrwi, $wrhi, $minw, $minh, $maxw, $maxh,
	$lblid, $pmid, $grav, $tc, $bc) = @_;
    $stype = "Add Window" if ($type == $M_ADD_WINDOW);
    $stype = "Config Window" if ($type == $M_CONFIGURE_WINDOW);
    &tohex($id, $fid, $ptr, $flags, $lblid, $pmid, $grav, $tc, $bc);
    print <<"END";
$stype
\t ID $id
\t frame ID $fid
\t fvwm ptr $ptr
\t frame x $x
\t frame y $y
\t frame w $w
\t frame h $h
\t desk $desk
\t flags $flags
\t title height $th
\t border width $bw
\t window base width $wbw
\t window base height $wbh
\t window resize width increment $wrwi
\t window resize height increment $wrhi
\t window min width $minw
\t window min height $minh
\t window max width $maxw
\t window max height $maxh
\t icon label window $lblid
\t icon pixmap window $pmid
\t window gravity $grav
\t text color pixel value $tc
\t border color pixel value $bc
END
    1;
}

sub ListWindow {
    ($type, $id, $fid, $ptr) = @_;
    $stype = "raise" if ($type == $M_RAISE_WINDOW);
    $stype = "lower" if ($type == $M_LOWER_WINDOW);
    $stype = "destroy" if ($type == $M_DESTROY_WINDOW);
    $stype = "map" if ($type == $M_MAP);
    $stype = "de-iconify" if ($type == $M_DEICONIFY);
    &tohex($id, $fid, $ptr);

    print "$stype\n\t ID $id\n\t frame ID $fid\n\t fvwm ptr $ptr\n";
    1;
}

sub ListFocus { 
    ($type, $id, $fid, $ptr, $tc, $bc) = @_;
    &tohex($id, $fix, $ptr);
    print "focus\n\t ID $id\n\t frame ID $fid\n\t fvwm ptr $ptr\n";
    print "\t text color pixel value $tc\n\t border color pixel value $bc\n";
    1;
}

sub ListIcon {
    ($type, $id, $fid, $ptr, $x, $y, $w, $h) = @_;
    $stype = "iconify" if ($type == $M_ICONIFY);
    $stype = "icon location" if ($type == $M_ICON_LOCATION);
    &tohex($id, $fix, $ptr);
    print "$stype\n\t ID $id\n\t frame ID $fid\n\t fvwm ptr $ptr\n";
    print "\t icon x $x\n\t icon y $y\n\t icon w $w\n\t icon h $h\n";
    1;
}

sub ListName {
    ($type, $id, $fid, $ptr, $value) = @_;
    $stype = "window name" if ($type == $M_WINDOW_NAME);
    $stype = "icon name" if ($type == $M_ICON_NAME);
    $stype = "window class" if ($type == $M_RES_CLASS);
    $stype = "class resource name" if ($type == $M_RES_NAME);
    &tohex($id, $fid, $ptr);
    print "$stype\n\t ID $id\n\t frame ID $fid\n\t fvwm ptr $ptr\n";
    print "\t $stype $value\n";
    1;
}

sub ListConfigInfo {
    ($type, $txt) = @_;
    $stype = "config_info" if ($type == $M_CONFIG_INFO);
    $stype = "error" if ($type == $M_ERROR);
    print "$stype\n\t$txt\n";
    1;
}

sub ListEndConfigInfo {
    print "end_config_info\n";
    1;
}
