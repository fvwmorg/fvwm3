#!/usr/local/bin/tkperl
# console.pl - combination FvwmTalk/FvwmDebug using fvwmmod.pl

require 'fvwmmod.pl';

($winid, $contxt, @CmdArgs) = &InitModule;

require 'getopts.pl';
{
    local (@ARGV) = @CmdArgs;
    &Getopts('p');
}

@history = ();
# Initial 500 line limit (arbitrary)
$epacketlimit = $packetlimit = 500;
$defFont = '*helvetica*m*o*n*14*';

use Tk;

$MW = MainWindow->new;

$cmd = $MW->Entry(
    -width   => 80,
);
$cmd->bind('<Return>' => sub {
    $line = $cmd->get;
    unshift(@history, $line);
    pop(@history) if (@history > 20);
    &SendInfo(0, $line);
    $cmd->delete(0,end);
    $histindx = @history;
});

$tf = $MW->Frame();
#$tf->pack(-fill => 'x', -before => $pf);
$tf->pack(-side => 'top', -fill => 'x');

# Extras for packet tracing
if (defined($opt_p)) {
    ($pf, $packetlog, $pl) = &ScrollBox($MW, "Packets:", 'fixed', 20, 'yes');

    $buflbl = $tf->Label(
	-text => 'Packet buffer limit (lines):',
	-font => $defFont,
    );

    $bufentry = $tf->Entry(
	-width => 5,
	-textvariable => \$epacketlimit,
	-justify => right,
    );
    $bufentry->bind('<Return>' => sub {
	local($size, $lines);
	if ($epacketlimit > 0) {
	    $packetlimit = $epacketlimit;
	}
	$bufentry->delete(0,end);
	$bufentry->insert(0,$packetlimit);
	$size = $packetlog->index(end) - 1;
	$lines = ($size - $packetlimit);
	$packetlog->delete(0, $lines) if ($size > $packetlimit);
	if ($packetlimit < 20) {
	    $packetlog->configure(-height => $packetlimit);
	} else {
	    $packetlog->configure(-height => 20);
	}
    });


    $buflbl->pack(-side => 'left');
    $bufentry->pack(-side => 'left', -anchor => 'e');
    $bufentry->bind('<Control-u>' => sub {$bufentry->delete(0,end)});

    $tf->bind('<Enter>' => sub {$bufentry->focus});

    &AddHandler($M_NEW_PAGE, 'ListNewPage');
    &AddHandler($M_NEW_DESK, 'ListNewDesk');
    &AddHandler($M_ADD_WINDOW | $M_CONFIGURE_WINDOW, 'ListConfigWin');
    &AddHandler($M_LOWER_WINDOW | $M_RAISE_WINDOW | $M_DESTROY_WINDOW |
	$M_DEICONIFY | $M_MAP,
	'ListWindow');
    &AddHandler($M_WINDOWSHADE | $M_DEWINDOWSHADE, 'ListWindowShade');
    &AddHandler($M_FOCUS_CHANGE, 'ListFocus');
    &AddHandler($M_ICONIFY | $M_ICON_LOCATION, 'ListIcon');
    &AddHandler($M_WINDOW_NAME | $M_ICON_NAME | $M_RES_CLASS | $M_RES_NAME,
	'ListName');
    &AddHandler($M_CONFIG_INFO | $M_ERROR | $M_STRING, 'ListConfigInfo');
    &AddHandler($M_END_WINDOWLIST | $M_END_CONFIG_INFO, 'ListEndConfigInfo');
    &AddHandler($M_ICON_FILE | $M_DEFAULTICON | $M_MINI_ICON, 'ListUnknown');
}

&AddHandler($M_ERROR | $M_STRING, 'handleError');

$cmd->bind('<Control-p>' => \&histprev);
$cmd->bind('<Up>' => \&histprev);
$cmd->bind('<Control-n>' => \&histnext);
$cmd->bind('<Down>' => \&histnext);

$cmd->bind('<Control-c>' => \&clearErrors);

($ef, $errlog, $el) = &ScrollBox($MW, "Errors:", 'variable', 5, 'no');

$quitbut = $tf->Button(
    -text => "Quit",
    -font => $defFont,
    -foreground => 'red',
    -command => sub { &EndModule ; exit },
);
$quitbut->pack(-side => right, -anchor => 'e');

$clearbut = $tf->Button(
    -text => "Clear",
    -font => $defFont,
    -foreground => 'red',
    -command => sub { &clearErrors },
);
$clearbut->pack(-side => right, -anchor => 'e');

$lbl = $MW->Label(
    -font => $defFont,
    -foreground => 'blue',
    -text => "Commands:",
);
$lbl->pack(-anchor => 'sw');

sub histprev {
    $cmd->delete(0,end);
    $histindx = 0 if (++$histindx > @history);
    $cmd->insert(0, "$history[$histindx]");
};
sub histnext {
    $cmd->delete(0,end);
    $histindx = @history if (--$histindx < 0);
    $cmd->insert(0, "$history[$histindx]") if ($histindx >= 0);
};

$cmd->bind('<Control-u>' => sub {$cmd->delete(0,end)});
$cmd->pack(-fill => 'x');

$MW->focusFollowsMouse;
$MW->title("TkPerlConsole.pl");

&EventLoop;

&EndModule;
exit;

########################################################################
# Subroutines
########################################################################

sub ScrollBox {
    local ($parent, $label, $fontname, $initheight, $expand) = @_;

    $frame = $parent->Frame();
    $frame->bind('<Enter>' => sub {$cmd->focus});
    $frame->pack(-side => 'top', -expand => $expand, -fill => 'both');
    $lbl = $frame->Label(
	-font => $defFont,
	-foreground => 'blue',
	-text => $label,
    );
    $lbl->pack(-anchor => 'sw');
    $sbar = $frame->Scrollbar;
    $sbar->pack(-side => 'right', -fill => 'y');
    $lbox = $frame->Listbox(
	-yscrollcommand => [$sbar => 'set'],
	-font => $fontname,
	-height => $initheight,
	-selectmode => 'single',
    );
    $sbar->configure(-command => [$lbox => 'yview']);
    $lbox->pack(-expand => $expand, -fill => 'both',);
    ($frame,$lbox,$lbl,$sbar);
}

# tohex - convert a list of values from decimal to hex
sub tohex {
    foreach (@_) {
	$_ = sprintf("%lx", $_);
	tr/a-z/A-Z/;
    }
}

sub Quit {
    &showline( "$0 exiting\n");
    &EndModule;
    exit;
}

sub ListNewPage { 
    local($type, $x, $y, $desk) = @_;
    &showline( "new page\n     x $x\n     y $y\n     desk $desk\n");
    1;
}
sub ListNewDesk {
    local($type, $desk) = @_;
    &showline( "new desk\n     desk $desk\n");
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
    &showline( <<"END");
$stype
     ID $id
     frame ID $fid
     fvwm ptr $ptr
     frame x $x
     frame y $y
     frame w $w
     frame h $h
     desk $desk
     flags $flags
     title height $th
     border width $bw
     window base width $wbw
     window base height $wbh
     window resize width increment $wrwi
     window resize height increment $wrhi
     window min width $minw
     window min height $minh
     window max width $maxw
     window max height $maxh
     icon label window $lblid
     icon pixmap window $pmid
     window gravity $grav
     text color pixel value $tc
     border color pixel value $bc
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

    &showline( "$stype\n     ID $id\n     frame ID $fid\n     fvwm ptr $ptr\n");
    1;
}

sub ListFocus { 
    ($type, $id, $fid, $ptr, $tc, $bc) = @_;
    &tohex($id, $fix, $ptr);
    &showline( "focus\n     ID $id\n     frame ID $fid\n     fvwm ptr $ptr\n");
    &showline( "     text color pixel value $tc\n     border color pixel value $bc\n");
    1;
}

sub ListWindowShade { 
    ($type, $id) = @_;
    &tohex($id);
    $stype = "???";
    $stype = "windowshade" if ($type == $M_WINDOWSHADE);
    $stype = "de-windowshade" if ($type == $M_DEWINDOWSHADE);
    &showline( "$stype\n     ID $id\n");
    1;
}

sub ListIcon {
    ($type, $id, $fid, $ptr, $x, $y, $w, $h) = @_;
    $stype = "iconify" if ($type == $M_ICONIFY);
    $stype = "icon location" if ($type == $M_ICON_LOCATION);
    &tohex($id, $fix, $ptr);
    &showline( "$stype\n     ID $id\n     frame ID $fid\n     fvwm ptr $ptr\n");
    &showline( "     icon x $x\n     icon y $y\n     icon w $w\n     icon h $h\n");
    1;
}

sub ListName {
    ($type, $id, $fid, $ptr, $value) = @_;
    $stype = "window name" if ($type == $M_WINDOW_NAME);
    $stype = "icon name" if ($type == $M_ICON_NAME);
    $stype = "window class" if ($type == $M_RES_CLASS);
    $stype = "class resource name" if ($type == $M_RES_NAME);
    &tohex($id, $fid, $ptr);
    &showline( "$stype\n     ID $id\n     frame ID $fid\n     fvwm ptr $ptr\n");
    &showline( "     $stype $value\n");
    1;
}

sub ListConfigInfo {
    ($type, $txt) = @_;
    $stype = "config_info" if ($type == $M_CONFIG_INFO);
    $stype = "error" if ($type == $M_ERROR);
    $stype = "string" if ($type == $M_STRING);
    &showline( "$stype\n    $txt\n");
    1;
}

sub ListEndConfigInfo {
    &showline( "end_config_info\n");
    1;
}

sub ListUnknown {
    &showline( "miscellaneous packet\n    $type\n");
    1;
}

sub clearErrors {
    $errlog->delete(0, end);
    $packetlog->delete(0, end) if (defined($opt_p));
}

sub handleError {
    $errlimit = 20;
    local($size, $lines);

    $errlog->insert(end,"$_[1]\n");
    $size = $errlog->index(end) - 1;
    $lines = ($size - $errlimit);
    $errlog->delete(0, $lines) if ($size > $errlimit);
    $errlog->see(end);
    1;
};

sub showline {
    local($size, $lines);

    for $line (split(/\n/,$_[0])) {
	$packetlog->insert(end,"$line");
    }
    $size = $packetlog->index(end) - 1;
    $lines = ($size - $packetlimit);
    $packetlog->delete(0, $lines) if ($size > $packetlimit);
    $packetlog->see(end);
}
