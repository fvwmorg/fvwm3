# Copyright (c) 2003-2009 Mikhael Goikhman
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
# along with this program; if not, see: <http://www.gnu.org/licenses/>

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
	$self->add_handler(M_NEW_PAGE, sub {
		my $event = $_[1];
		$self->calculate_internals($event->args);
	});

	$self->request_windowlist_events;

	my $result = $self->SUPER::start;

	$self->delete_handlers;

	$self->add_handler(M_NEW_PAGE | M_NEW_DESK, sub {
		my $event = $_[1];
		if ($event->type == M_NEW_DESK) {
			my $old_desk_n = $self->{data}->{desk_n};
			my $new_desk_n = $event->args->{desk_n};
			$self->{data}->{desk_n} = $new_desk_n;
			my $really_changed = $old_desk_n != $new_desk_n;
			$self->notify("desk only changed", $really_changed);
			return unless $really_changed;
		} else {
			$self->calculate_internals($event->args);
			$self->notify("page only changed");
		}
		$self->notify("desk/page changed");
	});

	return $result;
}

sub calculate_internals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	@$data{keys %$args} = values %$args;
	$data->{page_nx} = int($data->{vp_x} / $data->{vp_width});
	$data->{page_ny} = int($data->{vp_y} / $data->{vp_height});
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

This B<FVWM::Tracker> subclass provides an information about the current
fvwm page and desk and screen dimensions. Like with all trackers, this
information is automatically brought up to the date for the entire tracker
object life and may be retrieved by its C<data> method.

This tracker defines the following observables that enable additional way
of work:

    "desk/page changed",
    "desk only changed",
    "page only changed",

=head1 SYNOPSYS

Using B<FVWM::Module> $module object:

    my $page_tracker = $module->track("PageInfo");
    my $page_hash = $page_tracker->data;
    my $curr_desk = $page_hash->{'desk_n'};

=head1 OVERRIDDEN METHODS

=over 4

=item B<data>

Returns hash ref representing the current page/desk, with the following keys
(the fvwm variable equivalents are shown on the right):

    desk_n           $[desk.n]
    page_nx          $[page.nx]
    page_ny          $[page.ny]
    desk_pages_x     $[desk.pagesx]
    desk_pages_y     $[desk.pagesy]
    vp_width         $[vp.width]
    vp_height        $[vp.height]
    vp_x             $[vp.x]
    vp_y             $[vp.y]

=item B<dump>

Returns 2 debug lines representing the current page data (as described in
C<data>) in the human readable format.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
