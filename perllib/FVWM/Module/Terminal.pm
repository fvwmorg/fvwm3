# Copyright (c) 2003, Mikhael Goikhman
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

BEGIN {
	if ($ENV{FVWM_MODULE_TERMINAL_CLIENT}) {
		my ($outFd, $inFd, $name, $prompt, $ornaments) = @ARGV;
		$name ||= "FVWM::Module::Terminal";
		$prompt ||= "";
		$ornaments ||= 0;
		$name .= " Client";

		eval "use IO::File; use Term::ReadLine;";
		$SIG{__DIE__} = sub {
			my $msg = shift;
			$msg =~ /(.*?)^BEGIN failed/sm and die $1;
			print "$name: $msg";
			sleep(2);
			die "$name: Exiting due to errors\n";
		};
		die "$name: $@" if $@;

		my $ostream = new IO::File ">&$outFd"
			or die "Can't write to file descriptor (&$outFd)\n";
		my $istream = new IO::File "<&$inFd"
			or die "Can't read from file descriptor (&$inFd)\n";
		$ostream->autoflush(1);
		$istream->autoflush(1);
# DEBUG
$ostream->print("Abra ka Dabra\n");
print "($ostream) ($istream)\n";

		my $term = Term::ReadLine->new($name);
		$term->ornaments($ornaments);

		my $line;
		while (defined($line = $term->readline($prompt))) {
			$ostream->print("$line\n");
		}
		print "Closed\n";
		sleep(1);
		exit(0);
	}
}

package FVWM::Module::Terminal;

use 5.004;
use strict;

use FVWM::Module::Toolkit qw(base IO::Select);
use General::Parse;

sub new ($@) {
	my $class = shift;
	my %params = @_;

	my $xterm = delete $params{XTerm} || "xterm -name FvwmPerlTerminal -e";
	my $prompt = delete $params{Prompt} || "";
	my $ornaments = delete $params{Ornaments} || 0;
	my $self = $class->SUPER::new(%params);

	pipe(PARENT_IN, CHILD_OUT) || $self->internalDie("Can't pipe");
	pipe(CHILD_IN, PARENT_OUT) || $self->internalDie("Can't pipe");
	CHILD_OUT->autoflush;
	PARENT_OUT->autoflush;

	my $pid = fork;
	$self->internalDie("Can't fork: $!") unless defined $pid;

	if ($pid == 0) {
		close PARENT_IN; close PARENT_OUT;

		my $co = fileno(CHILD_OUT);
		my $ci = fileno(CHILD_IN);
		my $q = sub { local $_ = shift; s/'/\\'/g; $_ };
		$ENV{FVWM_MODULE_TERMINAL_CLIENT} = 1;
		my @cmd = ($^X, "-w", __FILE__, $co, $ci, $self->name, $prompt, $ornaments);

		unshift @cmd, getTokens($xterm);
# DEBUG
my $ostream = new IO::File ">&$co"
	or die "Can't write to file descriptor (&$co)\n";
$ostream->print("Asta la Vista\n");
print join(", ", @cmd), "\n";

		{ exec {$cmd[0]} @cmd }
		$self->internalDie("Can't fork $cmd[0]");
		close CHILD_IN; close CHILD_OUT;
		exit 0;
	}

	close CHILD_IN; close CHILD_OUT;

	$self->{termSelect} = new IO::Select($self->{istream}, \*PARENT_IN);
	$self->{termOutput} = \*PARENT_OUT;

	return $self;
}

sub waitPacket ($) {
	my $self = shift;

	while (1) {
		my @handles = $self->{termSelect}->can_read();
		my ($coreInput, $termInput);
		foreach (@handles) {
			($_ == $self->{istream}? $coreInput: $termInput) = $_;
		}
		if ($termInput) {
			my $line = $termInput->getline;
			unless (defined $line) {
				$self->debug("EOF from terminal client, terminating", 3);
				$self->terminate;
			}
			chomp($line);
			$self->debug("Got [$line] from terminal client", 4);
			$self->processTermLine($line);
		}
		last if $coreInput;
	}
}

sub processTermLine ($$) {
	my $self = shift;
	my $line = shift;

	$self->showMessage("I got $line!");
}

1;

__END__

=head1 NAME

FVWM::Module::Terminal - FVWM::Module with X terminal based solutions

=head1 SYNOPSIS

NOTE: This class is not functional yet.

Name this module TestModuleTerminal, make it executable and place in ModulePath:

    #!/usr/bin/perl -w

    use lib `fvwm-perllib dir`;
    use FVWM::Module::Terminal;

    my $module = new FVWM::Module::Terminal(
        XTerm => 'rxvt', Debug => 2, Name => "TestModuleTerminal",
    );

    my $id = undef;
    $module->send('Next (TestModuleTerminal) SendToModule myid $[w.id]');

    $module->addDefaultErrorHandler;
    $module->addHandler(M_STRING, sub {
        $[1]->_text =~ /^myid (.*)$/ && $id = eval $1;
    };
    $module->addHandler(M_ICONIFY, sub {
        return unless defined $id;
        my $id0 = $_[1]->_win_id;
        $module->send("WindowId $id Iconify off") if $id0 == $id;
    });
    $module->track('Scheduler')->schedule(60, sub {
        $module->showMessage("You run this module for 1 minute")
    });

    $module->eventLoop;

=head1 DESCRIPTION

NOTE: This class is not functional yet.

The B<FVWM::Module::Terminal> class is a sub-class of
B<FVWM::Module::Toolkit> that overloads the methods B<waitPacket>,
B<showError>, B<showMessage> and B<showDebug> to manage terminal
functionality.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only overloaded or new methods are covered here:

=over 8

=item B<waitPacket>

Listen to the terminal read-line while waiting for the packet from fvwm.

=item B<showError> I<msg> [I<title>]

Shows the error message in terminal.

Useful for diagnostics of a Terminal based module.

=item B<showMessage> I<msg> [I<title>]

Shows the message in terminal.

Useful for notices by a Terminal based module.

=item B<showDebug> I<msg> [I<title>]

Shows the debug info in terminal.

Useful for debugging a Terminal based module.

=head1 BUGS

Awaiting for your reporting.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module> and L<Term::ReadLine>.

=cut
