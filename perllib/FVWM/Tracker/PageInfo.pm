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

package FVWM::Tracker::PageInfo;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"desk/page changed",
		"desk only changed",
		"page only changed",
	];
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};
	$self->addHandler(M_NEW_PAGE, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	$self->requestWindowListEvents;

	my $result = $self->SUPER::start;

	$self->deleteHandlers;

	$self->addHandler(M_NEW_PAGE | M_NEW_DESK, sub {
		my $event = $_[1];
		if ($event->type == M_NEW_DESK) {
			my $oldDeskN = $self->{data}->{desk_n};
			my $newDeskN = $event->args->{desk_n};
			$self->{data}->{desk_n} = $newDeskN;
			my $reallyChanged = $oldDeskN != $newDeskN;
			$self->notify("desk only changed", $reallyChanged);
			return unless $reallyChanged;
		} else {
			$self->calculateInternals($event->args);
			$self->notify("page only changed");
		}
		$self->notify("desk/page changed");
	});

	return $result;
}

sub calculateInternals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	while (my ($name, $value) = each %$args) {
		$data->{$name} = $value;
	}
	$data->{page_nx} = $data->{vp_x} / $data->{vp_width};
	$data->{page_ny} = $data->{vp_y} / $data->{vp_height};
}

sub dump ($) {
	my $self = shift;
	my $data = $self->{data};
	my $string = join(', ', map { "$_=$data->{$_}" } sort keys %$data) . "\n";
	$string =~ s/^(.*?)(, d.*?)(, p.*?), (v.*)$/$1$3$2\n$4/s;
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the current
page information.

This tracker defines the following observables:

    "desk/page changed",
    "desk only changed",
    "page only changed",

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    my $pageTracker = $module->track("PageInfo");
    my $pageHash = $pageTracker->data;
    my $currDesk = $pageHash->{'desk_n'};

=head1 OVERRIDDEN METHODS

=over 4

=item B<data>

Returns hash ref representing the current page/desk with the following keys:

    desk_n
    page_nx
    page_ny
    vp_x
    vp_y
    vp_width
    vp_height

=item B<dump>

Works similarly to B<data>, but returns 2 debug lines representing
the current page data in the human readable format.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
