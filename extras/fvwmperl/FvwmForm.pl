#!/usr/local/bin/tkperl

# This implements most of FvwmForm.
# The good news: unlike FvwmForm, this version permits cut and paste.
# The bad news: it doesn't implement it faithfully (eg. recursive variable,
#  substitutions, server grabs, pointer warps, etc. are not implemented).

# On the plus side, though, this module is only about 200 lines,
# compared with 1600 for the C implementation; with some work, this
# module could be made fully compatible with FvwmForm, although it's not
# clear that this would be a useful thing to do; this module was
# written more as a demonstration of fvwmmod.pl than as a practical
# tool.

push @INC, "/h/a/djast/X11/fvwmlib";
push @INC, "/home/djast/.fvwmlib/bin";
require 'fvwmmod.pl';

($winid, $contxt, @CmdArgs) = &InitModule;

require 'getopts.pl';
{
    local (@ARGV) = @CmdArgs;
    &Getopts('p');
    $modname = $ARGV[0];
}

use Tk;

%anchor = (
    '', 'center',
    'center', 'center',
    'left', 'w',
    'right', 'e',
    'expand', 'center',
);

$MW = MainWindow->new;
$MW->bind(Tk::Entry, "<Control-u>" => sub {($this) = @_ ; $this->delete(0,end)});

($winid, $contxt, @CmdArgs) = &InitModule;

&AddHandler($M_CONFIG_INFO, 'ConfigInfo');
&AddHandler($M_END_CONFIG_INFO, 'PostForm');

$curframe = $MW->Frame;

sub ConfigInfo {
    local ($type,$line) = @_;
    if ($line !~ /^\s*\*$modname([^\s]*)\s*(.*)/) {
	return 1;
    }

    ($name,$args) = ($1, $2);
    @args = &parseline($args);

    if ($name eq "GrabServer") {
	# Note: not implemented yet
	++$GrabServer;
    } elsif ($name eq "WarpPointer") {
	# Note: not implemented yet
	++$WarpPointer;
    } elsif ($name eq "Position") {
	if ($args =~ /^(\d+)\D+(\d+)/) {
	    ($gx, $gy) = ($1, $2);
	    $pos = "+$gx+$gy";
	    $MW->wm(geometry => $pos);
	}
    } elsif ($name eq "Fore") {
	$ColorFore = $args;
    } elsif ($name eq "Back") {
	$ColorBack = $args;
    } elsif ($name eq "ItemFore") {
	$ColorItemFore = $args;
    } elsif ($name eq "ItemBack") {
	$ColorItemBack = $args;
	$MW->configure(
	    -bg => $ColorBack
	    );
	$curframe->configure(
	    -bg => $ColorBack
	    );
    } elsif ($name eq "Font") {
	$Font = $args;
    } elsif ($name eq "ButtonFont") {
	$ButtonFont = $args;
    } elsif ($name eq "InputFont") {
	$InputFont = $args;
    } elsif ($name eq "Line") {
	($just) = @args;
	$anchor = $anchor{$just} || 'center';
	$curframe = $MW->Frame(
	    -bg => $ColorBack,
	    );
	if ($just eq 'expand') {
	    $curframe->pack(-anchor => $anchor, -expand => x);
	} else {
	    $curframe->pack(-anchor => $anchor);
	}
    } elsif ($name eq "Text") {
	$text = $curframe->Label(
	    -text => $args[0],
	    -font => $Font,
	    -fg => $ColorFore,
	    -bg => $ColorBack,
	    );
	$text->pack(-side => left, -expand => x);
    } elsif ($name eq "Input") {
	($var, $width, $init) = @args;
	$variables{$var} = $init;
	$input = $curframe->Entry(
	    -width => $width,
	    -fg => $ColorItemFore,
	    -bg => $ColorItemBack,
	    -font => $InputFont,
	    -textvariable => \$variables{$var},
	    );
	$input->pack;
    } elsif ($name eq "Selection") {
	($itemname, $type) = @args;
	$selecttype{$itemname} = $type;
	$curselection = $itemname;
    } elsif ($name eq "Choice") {
	($itemname, $value, $state, $label) = @args;
	$isrb = ($selecttype{$itemname} ne 'multiple');
	if ($isrb) {
	    $method = Radiobutton;
	} else {
	    $method = Checkbutton;
	}

	$choice = $curframe->$method(
	    -text => $label,
	    -fg => $ColorItemFore,
	    -selectcolor => $ColorItemFore,
	    -bg => $ColorItemBack,
	    -font => $ButtonFont,
	    );
	if ($isrb) {
	    $choice->configure (
		-variable => \$choices{$curselection},
		-value => $itemname,
	    );
	} else {
	    $choice->configure (
		-variable => \$choices{$curselection},
	    );
	}
	$choice->select if ($state eq "on");
	$choice->pack(-side => left);
    } elsif ($name eq "Button") {
	($type, $lbl, $key) = @args;
	$curbuttontype = $type;
	$curbutton = $curframe->Button(
	    -text => $lbl,
	    -fg => $ColorItemFore,
	    -bg => $ColorItemBack,
	    -font => $ButtonFont,
	    );
	$curbutton->pack(-side => left, -fill => x);
	if (defined $key) {
	    if ($key eq "^M") {
		$keytxt = "<Return>";
	    } elsif ($key eq "^[") {
		$keytxt = "<Escape>";
	    } elsif ($key !~ /^\^/) {
		$keytxt = "<$key>";
	    } else {
		# control characters seem to be a problem...
		undef $keytxt ;
	    }
	    # ??? Bad values for $keytxt kill some tkperls
	    if (defined($keytxt)) {
		$MW->bind("$keytxt" => [\&Binding, $key, $curbutton]);
	    }
	}
    } elsif ($name eq "Command") {
	$cmd = $args;
	# remind: commands should be queued instead of only
	# remembering the last one
	$curbutton->configure(-command => [\&CmdButton, $curbuttontype, $cmd]);
    }

    1;
}

sub Binding {
    local ($this,$key,$what) = @_;
    $what->flash;
    $what->invoke;
}

sub CmdButton {
    local ($type,$cmd) = @_;

    foreach $i (keys %choices) {
	# define variables for each selected choice
	$variables{$choices{$i}} = "";
    }

    # do variable substitution on $cmd
    $cmd =~ s/\$\(([^\)\?]*)\?([^\)]*)\)/defined($variables{$1})?$2:""/ge;
    $cmd =~ s/\$\(([^\)\?]*)\!([^\)]*)\)/defined($variables{$1})?"":$2/ge;
    $cmd =~ s/\$\(([^\)]*)\)/$variables{$1}/g;
    &SendInfo(0, $cmd) ;
    if ($type eq "quit") { exit };

    # need to handle type 'restart'
};

sub PostForm {
    1;
}

sub parseline {
    local ($_) = @_;
    local @v = ();

    #@v = split(/[\s"]+/,$v);

    # Split into words, minding the quotes
    @v = /("[^"]*"|[^\s]+)\s*/g;

    # Trim leading and trailing quotes
    grep(s/^"(.*)"$/\1/,@v);

    return @v;
}

&SendInfo(0, "Send_ConfigInfo");

&EventLoop;
