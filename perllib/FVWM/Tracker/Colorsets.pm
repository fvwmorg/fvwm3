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

package FVWM::Tracker::Colorsets;

use strict;

use FVWM::Tracker qw(base);
use General::Parse qw(get_tokens);

use constant CS_FIELDS => qw(
	fg bg hilite shadow fgsh tint icon_tint
	pixmap shape_mask fg_alpha_percent width height
	pixmap_type shape_width shape_height shape_type
	tint_percent do_dither_icon icon_tint_percent icon_alpha_percent
);

use constant PIXMAP_TYPES => qw(
	PIXMAP_TILED PIXMAP_STRETCH_X PIXMAP_STRETCH_Y
	PIXMAP_STRETCH PIXMAP_STRETCH_ASPECT
	PIXMAP_ROOT_PIXMAP_PURE PIXMAP_ROOT_PIXMAP_TRAN
);

sub observables ($) {
	return [
		"colorset changed",
	];
}

sub start ($) {
	my $self = shift;

	# just an extra debug service for developers
	$self->{module}->emulate_event(M_CONFIG_INFO, [
		0, 0, 0,
		"Colorset 0 0 00c0c0 00e0e0 00a0a0 007070 0 0 0 0 64 0 0 0 0 0 0 0 0 0 64"
	])	if $self->{module}->is_dummy;

	$self->{data} = {};
	$self->add_handler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		$self->calculate_internals($event->args);
	});

	$self->request_configinfo_events;

	my $result = $self->SUPER::start;

	$self->delete_handlers;

	$self->add_handler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		my ($num, $old_hash) = $self->calculate_internals($event->args);
		return unless defined $num;
		$self->notify("colorset changed", $num, $old_hash);
	});

	return $result;
}

sub calculate_internals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	my $text = $args->{text};
	$self->internal_die("No 'text' arg in M_CONFIG_INFO")
		unless defined $text;
	return undef if $text !~ /^colorset ([\da-f]+) ([\da-f ]+)$/i;

	my $num = hex($1);
	my @numbers = get_tokens($2);
	return undef if @numbers != CS_FIELDS;

	# memory used for keys may be optimized later
	my $new_hash = {};
	my $i = 0;
	foreach (CS_FIELDS) {
		$new_hash->{$_} = hex($numbers[$i++]);
	}

	my $old_hash = $data->{$num};
	$data->{$num} = $new_hash;

	return wantarray? ($num, $old_hash): $num;
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
	return "Colorset $num is unknown\n" if defined $num && !$data->{$num};
	my @nums = defined $num? ($num):
		sort { "$a$b" =~ /^\d+$/? $a <=> $b: $a cmp $b } keys %$data;

	my $string = "";
	foreach (@nums) {
		my $cs_hash = $data->{$_};
		$string .= "Colorset $_";
		my $i = 0;
		foreach (CS_FIELDS) {
			my $value = $cs_hash->{$_};
			$i++;
			next if $i > 5 && ($value == 0 || /alpha_percent$/ && $value == 100);
			$value = sprintf("#%06lx", $value) if $i <= 7;
			$value = sprintf("0x%07lx", $value) if /pixmap$/;
			$string .= " $_=$value";
		}
		$string .= "\n";
	}
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the colorset
definitions.

This tracker defines the following observable:

    "colorset changed"

that is notified using 2 additional parameters: colorset number and
old colorset data hash ref.

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    my $cs_tracker = $module->track("Colorsets");
    my $cs_hash = $cs_tracker->data;
    my $cs2_fg = $cs_hash->{2}->{fg} || 'black';
    my $cs5_bg = $cs_tracker->data(5)->{bg} || 'gray';

    $cs_tracker->observe(sub {
        my ($module, $tracker, $data, $num, $old_hash) = @_;
        my $new_hash = $data->{$num};

        if ($old_hash->{pixmap} == 0 && $new_hash->{pixmap}) {
            my $pixmap_type = $new_hash->{pixmap_type};
            my $pixmap_name = ($tracker->PIXMAP_TYPES)[$pixmap_type];
            $module->debug("Colorset: $num, Pixmap type: $pixmap_name");
        }
    };

=head1 OVERRIDDEN METHODS

=over 4

=item B<data> [I<colorset-num>]

Returns either array ref of colorset hash refs, or one hash ref if
I<colorset-num> is given. The hash keys are listed in CS_FIELDS, the
constant of this class.

=item B<dump> [I<colorset-num>]

Works similarly to B<data>, but returns debug line(s) for one or all colorsets.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
