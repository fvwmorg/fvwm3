#!/usr/bin/perl -w

#  Modification History

#  Created on 01/12/03 by Dan Espen (dane):

# This test script is designed to be driven with FvwmCommandS:
# All the functionality related to windowshading is tested,
# based on my reading of the man page.

my $W1='window1';
my $W2='window2';
# xterm is chosen because it doesn't like being resized by pixel
my $CMD1="xterm -geometry 10x5+0+0 -T $W1 -n $W1 -e sleep 1000";
# xv is OK being resized to almost any size
# Note that some copies of xv put <unregistered> after the name
# so wildcards are used the the commands that match the window name.
my $CMD2="xv -geometry 400x400+200+0 -name $W2";

# Just in case, set default conditions:
&Reset();

# Create some windows to windowshade:
my $child1 = fork;
if ($child1 == 0) {
  exec $CMD1;
  # Never gets here
  exit 0;
}
my $child2 = fork;
if ($child2 == 0) {
  exec $CMD2;
  # Never gets here
  exit 0;
}
sleep 1;			# give windows time to appear

if ( "$ARGV[0]" ne "debug" ) {
&ShadeDir("","");		# defaults to titlebar direction
&ShadeDir("West","W");

&ShadeDir("South","S");
&ShadeDir("East","E");
&ShadeDir("North","N");

&ShadeDir("NorthEast","NE");
&ShadeDir("SouthEast","SE");
&ShadeDir("NorthWest","NW");
&ShadeDir("SouthWest","SW");

&SetStyle("TitleAtBottom");
&ShadeDir("South","S");
&ShadeDir("East","E");
&ShadeDir("North","N");

&SetStyle("TitleAtRight");
# Just go for the default:
&ShadeDir("","");

&SetStyle("TitleAtLeft");
# Just go for the default:
&ShadeDir("","");

&SetStyle("NoTitle");
&ShadeDir("","");		# This goes up
&ShadeDir("West","W");
&ShadeDir("South","S");
&ShadeDir("East","E");
&ShadeDir("North","N");

# Restore normality:
&SetStyle("TitleAtTop,Title");
&ShadeDir("South","S","stay");
&ShadeDir("ShadeAgain North","ShadeAgain N","stay");
&ShadeDir("ShadeAgain East","ShadeAgain E","stay");
&ShadeDir("ShadeAgain West","ShadeAgain W");

# Now some animation:
&SetStyle("WindowShadeSteps 1p");
&ShadeDir("South","S");
&ShadeDir("North","N");
&ShadeDir("East","E");
&ShadeDir("West","W");
&SetStyle("WindowShadeSteps 0");

print "WindowShadeShrinks\n";
&SetStyle("WindowShadeSteps 2p"); # Need some steps to see anything
&SetStyle("WindowShadeShrinks");
&ShadeDir("South","S");
&ShadeDir("North","N");
&ShadeDir("East","E");
&ShadeDir("West","W");

# I just kept moving this down as I added tests...
} else {
  print "Debug switch set, bypassing some tests\n";
}

print "WindowBusy/AlwaysLazy/Lazy\n";
&SetStyle("WindowShadeSteps 10"); # Need some steps to see anything
&SetStyle("WindowShadeBusy");
&ShadeDir("South","S");
&ShadeDir("North","NW");
&SetStyle("WindowShadeAlwaysLazy");
&ShadeDir("South","S");
&ShadeDir("North","NW");
&SetStyle("WindowShadeLazy");
&ShadeDir("South","S");
&ShadeDir("North","NW");

&Reset();

print "Close both windows\n";
`FvwmCommand "Next ($W1*) Close"`;
`FvwmCommand "Next ($W2*) Close"`;
print "Done\n";
exit 0;

# When I wrote this I used fork because I thought I might
# have to resort to killing the windows.
# This would do the job:
#kill (9, $child2,$child1);

# Subroutines:

# Set styles, both windows:
sub SetStyle {
  my ($style)=@_;
  print "Style set to $style\n";
  `FvwmCommand "Style $W1* $style"`;
  `FvwmCommand "Style $W2* $style"`;
}

# Shade both windows, pause, then unshade
sub ShadeDir {
  my ($dir,$dirshort,$stay)=@_;
  print "Shade $dir both windows\n";
  `FvwmCommand "Next ($W1*) WindowShade $dir"`;
  `FvwmCommand "Next ($W2*) WindowShade $dirshort"`;
  sleep 1;
  if ( $#_ > 2 ) {
    if ( "$stay" ne "stay") {
      &unShade();
    }
  }
}

# Unshade both windows:
sub unShade {
  print "UnShade both windows\n";
  `FvwmCommand "Next ($W1*) WindowShade"`;
  `FvwmCommand "Next ($W2*) WindowShade"`;
  sleep 1;
}

# Reset Styles back to "starting condition":
sub Reset {
  &SetStyle("TitleAtTop,Title,BorderWidth 6,HandleWidth 6");
  &SetStyle("WindowShadeSteps 0,WindowShadeScrolls");
  &SetStyle("WindowShadeLazy");
}

