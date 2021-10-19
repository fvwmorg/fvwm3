#!/usr/bin/env perl
#
# A very crude example how to turn status output from CWM in to something
# that can be interpreted by lemonbar.
#
# https://github.com/LemonBoy/bar
#
# Patches improving this script is welcome; I threw it together in about
# fifteen minutes, there's no support for SIGPIPE or anything like that yet.
#
# -- Thomas Adam

use strict;
use warnings;

use JSON::XS;
use IO::Select;

use Data::Dumper;

$| = 1;
binmode STDOUT, ':encoding(UTF-8)';

my $pipe = $ENV{'FVWM_STATUS_PIPE'};
# If there are no pipes, that's OK.
unless (-e $pipe) {
	warn "No pipe found ($pipe) - exiting.\n";
	exit;
}

my %scr_map;
my $last_clock_line = '';
my %reply_map;

sub query_xrandr
{
	my %lb = (
		'global_monitor' => {
			screen => '',
			data => undef,
		}
	);

	open(my $fh, '-|', 'xrandr -q') or die $!;
	while (my $line = <$fh>) {
		if ($line =~ /(.*?)\s+connected|connected primary\s*(\d+x\d+\d+\+\d+)\s+/) {
			my $output = $1;
			$lb{$output} = {
				screen  => $output,
			}
		}
	}
	close($fh);

	return (\%lb);
}

sub format_output
{
	my ($data) = @_;
        my $skip_all_but_global = 0;

        if (exists $data->{'screens'}->{'global_monitor'} and
            (exists $scr_map{'global_monitor'} and scalar keys %scr_map >= 1)) {

	    delete $data->{'screens'}->{'global_monitor'};
            $scr_map{'global_monitor'}->{'screen'} = 0;
        }

	foreach my $screen  (keys %{ $data->{'screens'} }) {
		my $extra_msg = '';
		my $extra_urgent = '';
		my $msg = "%{Sn$screen}";

		my $scr_h = $data->{'screens'}->{$screen};

		# For the list of desktops, we maintain the sort order based on the
		# group names being 0 -> 9.
		foreach my $deskname (
			sort {
				$scr_h->{'desktops'}->{$a}->{'number'} <=>
				$scr_h->{'desktops'}->{$b}->{'number'}
			} keys %{$scr_h->{'desktops'}})
		{
			my $desk_name  = $scr_h->{'desktops'}->{$deskname};
			my $sym_name   = $desk_name->{'number'};
			my $is_current = $desk_name->{'is_current'};
			my $desk_count = $desk_name->{'number_of_clients'};
			my $is_urgent  = $desk_name->{'is_urgent'} ||= 0;
			my $is_active  = $desk_name->{'is_active'} ||= 0;

			# If the window is active, give it a different colour.
			if ($is_current) {
					$msg .= "|%{B#39c488} $sym_name %{B-}";

					$extra_msg .= "%{B#D7C72F}[Scr:$screen][A:$desk_count]%{B-}";
					# Gather any other bits of information for the _CURRENT_
					# group we might want.
					if ($is_urgent) {
						$extra_urgent = "%{B#FF0000}[U]%{B-}";
					}
			} else {
				if ($is_urgent) {
						$msg .= "|%{B#b82e2e} $sym_name %{B-}";
				} elsif ($is_active) {
					$msg .= "|%{B#007FFF} $sym_name %{B-}";

					# If the deskname is in the active desktops lists then
					# mark it as being viewed in addition to the currently
					# active group.
				} elsif ($desk_count > 0) {
					# Highlight groups with clients on them.
					$msg .= "|%{B#004C98} $sym_name %{B-}";
				} elsif ($desk_count == 0) {
					# Don't show groups which have no clients.
					next;
				}
			}
		}

		$msg .= "%{F#FF00FF}|%{F-}$extra_msg$extra_urgent";

		if (defined $scr_h->{'current_client'}) {
			my $cc = $scr_h->{'current_client'};
			$msg .= "%{c}%{U#00FF00}%{+u}%{+o}%{B#AC59FF}%{F-}" .
				"        " . $cc . "        " .  "%{-u}%{-o}%{B-}";
		}
		$reply_map{$screen} = $msg;
	}
}

sub process_line
{
	my ($fifo) = @_;
	my $msg;

	open (my $pipe_fh, '<', $fifo) or die "Cannot open $fifo: $!";

	my $fifo_local = \*$pipe_fh;
	my $select = IO::Select->new($fifo_local);

	while (my @ready = $select->can_read()) {
		foreach my $fd (@ready) {
			while (my $line = <$pipe_fh>) {
				chomp $line;
				if ($line =~ s/^clock://) {
					$last_clock_line = $line // '';
				} else {
					my $json;
					eval {
						$json = decode_json($line);
					};
					if ($@) {
						warn "Couldn't parse: <<$line>>\n";
						next;
					}
					format_output(decode_json($line));
				}
				foreach my $scr_key (keys %reply_map) {
					print "$reply_map{$scr_key}$last_clock_line\n";
				}
			}
		}
	}
}

%scr_map = %{ query_xrandr() };
process_line($pipe);
