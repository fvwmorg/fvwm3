#!/usr/bin/perl
# Set the above to wherever your perl interpreter is.  (Duh.)

# MiniPerl.pl - small, instructive example of a fvwm module implemented
# in Perl.
# (c)1996 Dan Astoorian <djast@utopia.csas.com>

# Note: This requires fvwmmod.pl installed in your @INC path:
require 'fvwmmod.pl';

($winId, $context, @args) = &InitModule;

# Register event handlers:
&AddHandler($M_WINDOW_NAME, 'showwin');
&AddHandler($M_END_WINDOWLIST, 'cleanup');

print "---start of window list---\n";
# Ask FVWM to send us its list of windows
&SendInfo(0, "Send_WindowList");

&EventLoop;
&EndModule;

sub showwin {
    ($type, $id, $frameid, $ptr, $name) = @_;
    printf("Window ID %8lx: %s\n", $id, $name);
    1;
}
sub cleanup {
    print "--- end of window list ---\n";
    0;
}
