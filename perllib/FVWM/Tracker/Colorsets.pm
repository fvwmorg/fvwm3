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

package FVWM::Tracker::Colorsets;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"colorset changed",
	];
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};
	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	$self->requestConfigInfoEvents;

	my $result = $self->SUPER::start;

	$self->deleteHandlers;

	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		my $num = $self->calculateInternals($event->args);
		return unless defined $num;
		$self->notify("colorset changed", $num);
	});

	return $result;
}

sub calculateInternals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	my $text = $args->{text};
	$self->internalDie("No 'text' arg in M_CONFIG_INFO")
		unless defined $text;
	return undef unless $text =~ /^colorset (\d+) (.*)$/i;

	my $num = $1;
	my $rest = $2;
	$self->{data}->{$num} = $rest;

	return $num;
}

sub data ($;$) { 
	my $self = shift;
	my $num = shift;
	my $data = $self->{data};
	return $data unless defined $num;
	return $data->{$num};
}

sub dump ($;$) { 
	my $self = shift;
	my $num = shift;
	my $data = $self->{data};
	my @nums = defined $num? ($num):
		sort { "$a$b" =~ /^\d+$/? $a <=> $b: $a cmp $b } keys %$data;

	my $string = "";
	foreach (@nums) {
		my $csData = $data->{$_};
		$string .= "Colorset $_ $csData\n";
	}
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the colorset
definitions.

This tracker defines the following observables:

    "colorset changed"

NOT USABLE YET.

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    my $csTracker = $module->track("Colorsets");
    my $csHash = $csTracker->data;
    my $cs2_fg = $csHash->{2}->{fg} || 'black';
    my $cs5_bg = $csTracker->data(5)->{bg} || 'gray';

=head1 OVERRIDDEN METHODS

=over 4

=item B<data> [I<colorset-num>]

Returns either array ref of colorset hash refs, or one hash ref if
I<colorset-num> is given. The hash keys are not finalized yet.

=item B<dump> [I<colorset-num>]

Works similarly to B<data>, but returns debug line(s) for one or all colorsets.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
