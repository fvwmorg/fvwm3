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

package FVWM::Tracker::WindowList;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"window added",
		"window deleted",
		"window properties updated",
	];
}

sub new ($$;$) {
	my $class = shift;
	my $module = shift;
	my @options = split(/ /, shift || "");

	my $self = $class->FVWM::Tracker::new($module);

	$self->{options} = [ @options ];

	return $self;
}

sub addRequestedInfoHandlers ($$$$) {
	my $self = shift;
	my $handlerC = shift;
	my $handlerD = shift;
	my $handlerP = shift;

	my $useWInfo = 1;
	my $useNames = 1;
	my $useStack = 0;
	my $useIcons = 0;
	foreach my $option (@{$self->{options}}) {  
		/^(\!?)winfo$/ and $useWInfo = $1 ne '!';
		/^(\!?)names$/ and $useNames = $1 ne '!';
		/^(\!?)stack$/ and $useStack = $1 ne '!';
		/^(\!?)icons$/ and $useIcons = $1 ne '!';
	}
	my $mask1  = 0;
	my $mask2  = 0;
	my $xmask2 = 0;
	$mask1 |= M_ADD_WINDOW | M_CONFIGURE_WINDOW if $useWInfo;
	$mask2 |= M_RES_NAME | M_RES_CLASS | M_WINDOW_NAME | M_VISIBLE_NAME | M_ICON_NAME
		if $useNames;
	$mask2 |= M_RESTACK | M_RAISE_WINDOW | M_LOWER_WINDOW if $useStack;  
	$mask2 |= M_ICON_LOCATION | M_ICON_FILE | M_DEFAULTICON | M_MINI_ICON
		if $useIcons;
	$xmask2 |= MX_VISIBLE_ICON_NAME if $useNames;

	$self->addHandler($mask1, $handlerC) if $mask1;
	$self->addHandler(M_DESTROY_WINDOW, $handlerD);
	$self->addHandler($mask2, $handlerP) if $mask2;
	$self->addHandler($xmask2, $handlerP) if $xmask2;
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};

	### TODO
	$self->addRequestedInfoHandlers(sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	}, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	}, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	$self->requestWindowListEvents;

	### temporary
	$self->deleteHandlers;

	my $result = $self->SUPER::start;

	$self->deleteHandlers;

	### TODO
	$self->addRequestedInfoHandlers(sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	}, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	}, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	return $result;
}

sub calculateInternals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	### TODO
}

sub data ($;$) { 
	my $self = shift;
	my $id = shift;
	my $data = $self->{data};
	return $data unless defined $id;
	return $data->{$id};
}

sub dump ($;$) { 
	my $self = shift;
	my $id = shift;
	my $data = $self->{data};
	my @ids = defined $id? ($id): sort { $a <=> $b } keys %$data;

	my $string = "";
	foreach (@ids) {
		my $winData = $data->{$_};
		$string .= "Window $_\n";
		while (my ($prop, $value) = %$winData) {
			$string .= "\t$prop:\t[$value]\n";
		}
	}
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the window
information.

This tracker defines the following observables:

    "window added",
    "window deleted",
    "window properties updated",

NOT USABLE YET.

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object (preferably):

    my $windowsTracker = $module->track("ModuleConfig");
    my $windows = $windowsTracker->data;
    my $windowSizeX = $windows->{$winId}->{'x'};

or:

    my $windowsTracker = $module->track("WindowList");
    my $windowSizeX = $windowsTracker->data($winId)->{'x'};

=head1 OVERRIDDEN METHODS

=over 4

=item B<new> I<module> I<params>

It is possible the kind of window list.

To be written.

=item B<data> [I<window-id>]

Returns an array ref of window hashes of one hash ref (if
I<window-id> is given).

=item B<dump> [I<window-id>]

Works similarly like B<data>, but returns one or several debug lines.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
