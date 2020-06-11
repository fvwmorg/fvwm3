#!/usr/bin/perl

use strict;
use warnings;

use IO::Socket::UNIX;

use constant SOCK_PATH => "/tmp/fvwm_fmd.sock";

my $socket = IO::Socket::UNIX->new(
   Type => SOCK_STREAM,
   Peer => SOCK_PATH,
) or die "Couldn't connect to fmd: $!\n";

print $socket "Exec xterm\n";
