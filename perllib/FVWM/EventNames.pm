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

package FVWM::EventNames;

use strict;
use FVWM::Constants;

use constant number => 0;
use constant bool   => 1;
use constant window => 2;
use constant pixel  => 3;
use constant string => 4;
use constant wflags => 5;
use constant looped => 6;

use vars qw($EVENTS_INFO);

# ----------------------------------------------------------------------------
# start of the would-be-generated part

$EVENTS_INFO = {

	&M_NEW_PAGE             => {
		format => "l!3L!4",
		fields => [
			vp_x         => number,
			vp_y         => number,
			desk_n       => number,
			vp_width     => number,
			vp_height    => number,
			desk_pages_x => number,
			desk_pages_y => number,
		],
		aliases => {
			desk => 'desk_n',
		},
	},

	&M_NEW_DESK             => {
		format => "l!",
		fields => [
			desk_n       => number,
		],
		aliases => {
			desk => 'desk_n',
		},
	},

#	&M_OLD_ADD_WINDOW       => {
#	},

	&M_RAISE_WINDOW         => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_LOWER_WINDOW         => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

#	&M_OLD_CONFIGURE_WINDOW => {
#	},

	&M_FOCUS_CHANGE         => {
		format => "L!5",
		fields => [
			win_id       => window,
			frame_id     => window,
			flip         => bool,
			focus_fg     => pixel,
			focus_bg     => pixel,
		],
	},

	&M_DESTROY_WINDOW       => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_ICONIFY              => {
		format => "L!3l!8",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			icon_x       => number,
			icon_y       => number,
			icon_width   => number,
			icon_height  => number,
			frame_x      => number,
			frame_y      => number,
			frame_width  => number,
			frame_height => number,
		],
	},

	&M_DEICONIFY            => {
		format => "L!3l!8",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			icon_x       => number,
			icon_y       => number,
			icon_width   => number,
			icon_height  => number,
			frame_x      => number,
			frame_y      => number,
			frame_width  => number,
			frame_height => number,
		],
	},

	&M_WINDOW_NAME          => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_ICON_NAME            => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_RES_CLASS            => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_RES_NAME             => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_END_WINDOWLIST       => {
		format => "",
		fields => [
		],
	},

	&M_ICON_LOCATION        => {
		format => "L!3l!4",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			x            => number,
			y            => number,
			width        => number,
			height       => number,
		],
	},

	&M_MAP                  => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_ERROR                => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			text         => string,
		],
	},

	&M_CONFIG_INFO          => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			text         => string,
		],
	},

	&M_END_CONFIG_INFO      => {
		format => "",
		fields => [
		],
	},

	&M_ICON_FILE            => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_DEFAULTICON          => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&M_STRING               => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			text         => string,
		],
	},

	&M_MINI_ICON            => {
		format => "L!9a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			width        => number,
			height       => number,
			depth        => number,
			icon_id      => window,
			mask         => number,
			alpha        => number,
			name         => string,
		],
	},

	&M_WINDOWSHADE          => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_DEWINDOWSHADE        => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_VISIBLE_NAME         => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

#	&M_SENDCONFIG           => {
#	},

	&M_RESTACK              => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			low_windows  => looped,
		],
		loop_format => "L!3a*",
		loop_fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&M_ADD_WINDOW           => {
		format => "L!3l!6l!8L!2l!L!2l!3S4a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			frame_x      => number,
			frame_y      => number,
			frame_width  => number,
			frame_height => number,
			desk         => number,
			layer        => number,
			win_width    => number,
			win_height   => number,
			resize_width_inc  => number,
			resize_height_inc => number,
			minimum_width     => number,
			minimum_height    => number,
			maximum_width     => number,
			maximum_height    => number,
			icon_title_id     => window,
			icon_image_id     => window,
			gravity      => number,
			fore_color   => pixel,
			back_color   => pixel,
			ewmh_layer   => number,
			ewmh_desktop => number,
			ewmh_window_type  => number,
			title_height => number,
			border_width => number,
			dummy_zero_1 => number,
			dummy_zero_2 => number,
			window_flags => wflags,
		],
	},

	&M_CONFIGURE_WINDOW     => {
		format => "L!3l!6l!8L!2l!L!2l!3S4a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			frame_x      => number,
			frame_y      => number,
			frame_width  => number,
			frame_height => number,
			desk         => number,
			layer        => number,
			win_width    => number,
			win_height   => number,
			resize_width_inc  => number,
			resize_height_inc => number,
			minimum_width     => number,
			minimum_height    => number,
			maximum_width     => number,
			maximum_height    => number,
			icon_title_id     => window,
			icon_image_id     => window,
			gravity      => number,
			fore_color   => pixel,
			back_color   => pixel,
			ewmh_layer   => number,
			ewmh_desktop => number,
			ewmh_window_type  => number,
			title_height => number,
			border_width => number,
			dummy_zero_1 => number,
			dummy_zero_2 => number,
			window_flags => wflags,
		],
	},

#	&M_EXTENDED_MSG         => {
#	},

	&MX_VISIBLE_ICON_NAME   => {
		format => "L!3a*",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
			name         => string,
		],
	},

	&MX_ENTER_WINDOW        => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&MX_LEAVE_WINDOW        => {
		format => "L!3",
		fields => [
			win_id       => window,
			frame_id     => window,
			ptr          => number,
		],
	},

	&MX_PROPERTY_CHANGE     => {
		format => "L!3a*",
		fields => [
			type         => number,
			value        => number,
			win_id       => window,
			text         => string,
		],
		info => {
			type_names => [ qw(NONE BACKGROUND SWALLOW) ],
		},
	},

	"faked"                 => {
		format => "",
		fields => [
		],
	},

};

# end of the would-be-generated part
# ----------------------------------------------------------------------------

use Exporter;
use vars qw(@EXPORT @ISA $EVENT_TYPES $EVENT_NAMES $EVENT_TYPE_NAMES);
@EXPORT = (
	@FVWM::Constants::EXPORT,
	qw(eventName eventArgNames eventArgTypes eventArgValues eventArgs),
	qw(eventLoopArgNames eventLoopArgTypes),
	qw(eventArgAliases allEventNames allEventTypes)
);
@ISA = qw(Exporter);

sub allEventTypeNames () {
	if (!defined $EVENT_TYPE_NAMES) {
		$EVENT_TYPES = [];
		$EVENT_NAMES = [];
		$EVENT_TYPE_NAMES = {};
		my ($type, $name);
		foreach $name (@FVWM::Constants::EXPORT) {
			next unless $name =~ /^MX?_/;
			next if $name eq 'M_EXTENDED_MSG';
			no strict 'refs';
			$type = &$name();
			next if $name =~ /^MX_/ && !($type & M_EXTENDED_MSG);
			push @$EVENT_TYPES, $type;
			push @$EVENT_NAMES, $name;
			$EVENT_TYPE_NAMES->{$type} = $name;
		}
	}
	return $EVENT_TYPE_NAMES;
}

sub allEventNames () {
	allEventTypeNames();
	return wantarray? @$EVENT_NAMES: $EVENT_NAMES;
}

sub allEventTypes () {
	allEventTypeNames();
	return wantarray? @$EVENT_TYPES: $EVENT_TYPES;
}

sub eventName ($) {
	my $type = shift;
	return allEventTypeNames()->{$type};
}

sub eventTypeToBinary ($) {
	my $type = shift || "no-event-type";

	return $type unless $type =~ /^\d+$/;
	return sprintf("%b", $type);
}

sub eventInfo ($) {
	my $type = shift;
	unless (defined $EVENTS_INFO->{$type}) {
		die "FVWM::EventNames: Unknown event type (" .
			eventTypeToBinary($type) . ")\n";
	}
	return $EVENTS_INFO->{$type};
}

sub calculateInternals ($) {
	my $type = shift;

	my $eventInfo = eventInfo($type);
	$eventInfo->{names} = [];
	$eventInfo->{types} = [];

	my $i = 0;
	foreach (@{$eventInfo->{fields}}) {
		push @{$eventInfo->{names}}, $_ if ($i % 2) == 0;
		push @{$eventInfo->{types}}, $_ if ($i % 2) == 1;
		$i++;
	}

	# handle loop args if any
	return unless exists $eventInfo->{loop_fields};
	$eventInfo->{loop_names} = [];
	$eventInfo->{loop_types} = [];

	$i = 0;
	foreach (@{$eventInfo->{loop_fields}}) {
		push @{$eventInfo->{loop_names}}, $_ if ($i % 2) == 0;
		push @{$eventInfo->{loop_types}}, $_ if ($i % 2) == 1;
		$i++;
	}
}

sub eventArgNames ($$) {
	my $type = shift;
	my $argValues = shift;

	my $eventInfo = eventInfo($type);
	my $argNames = $eventInfo->{names};
	return $argNames if defined $argNames;
	calculateInternals($type);
	return $eventInfo->{names};
}

sub eventArgTypes ($$) {
	my $type = shift;
	my $argValues = shift;

	my $eventInfo = eventInfo($type);
	my $argTypes = $eventInfo->{types};
	return $argTypes if defined $argTypes;
	calculateInternals($type);
	return $eventInfo->{types};
}

sub eventArgValues ($$) {
	my $type = shift;
	my $packedStr = shift;

	my $eventInfo = eventInfo($type);
	my @argValues = unpack($eventInfo->{format}, $packedStr);
	my $argFields = $eventInfo->{fields};
	push @argValues, (undef) x ((@$argFields / 2) - @argValues);

	# process looped args
	if (@$argFields && $argFields->[@$argFields - 1] == looped) {
		my @loopArgValues = ();

		my $restStr = pop @argValues;
		while ($restStr ne "") {
			my @newArgValues = unpack($eventInfo->{loop_format}, $restStr);
			die "Internal error, no loop args unpacked ($type)\n" unless @newArgValues > 1;
			$restStr = pop @newArgValues;
			push @loopArgValues, @newArgValues;
		}

		push @argValues, \@loopArgValues;
	}

	# strip everything past the first null (or newline) if needed
	if (@$argFields && $argFields->[-1] == string) {
		$argValues[-1] =~ s/\n*\0.*//s;
	}

	return \@argValues;
}

sub eventLoopArgNames ($$) {
	my $type = shift;
	my $argValues = shift;

	my $eventInfo = eventInfo($type);
	my $argNames = $eventInfo->{loop_names};
	return $argNames if defined $argNames;
	calculateInternals($type);
	return $eventInfo->{loop_names};
}

sub eventLoopArgTypes ($$) {
	my $type = shift;
	my $argValues = shift;

	my $eventInfo = eventInfo($type);
	my $argTypes = $eventInfo->{loop_types};
	return $argTypes if defined $argTypes;
	calculateInternals($type);
	return $eventInfo->{loop_types};
}

sub eventArgs ($$) {
	my $type = shift;
	my $argValues = shift;

	my $argNames = eventArgNames($type, $argValues);

	die sprintf "Internal error, event type %s (%d names, %d values)\n",
		eventTypeToBinary($type), scalar @$argNames, scalar @$argValues
		if @$argNames != @$argValues;

	my $loopArgNames = eventLoopArgNames($type, $argValues);

	die sprintf "Internal error, event type %s (%d loop names, non array)\n",
		eventTypeToBinary($type), scalar @$loopArgNames
		if $loopArgNames && ref($loopArgNames) ne 'ARRAY'
			&& !@$loopArgNames && ref($argValues->[-1]) ne 'ARRAY';

	my $i = 0;
	my %args = map {
		my $value = $_;
		$argNames->[$i++], ref($value) ne 'ARRAY'? $value: do {
			my $loopValue = [];
			my $j = 0;
			while ($j < @$value) {
				my %loopHash = map { $_, $value->[$j++] } @$loopArgNames;
				push @$loopValue, \%loopHash;
			}
			$loopValue
		}
	} @$argValues;
	return \%args;
}

sub eventArgAliases ($) {
	my $type = shift;

	return eventInfo($type)->{aliases} || {};
}

# ----------------------------------------------------------------------------

=head1 NAME

FVWM::EventNames - names and types of all FVWM event arguments

=head1 SYNOPSIS

  use FVWM::EventNames;

  print "All event names: ", join(", ", @{allEventNames()}), "\n";
  print "All event types: ", join(", ", @{allEventTypes()}), "\n";

  my $name      = eventName     (M_ICON_LOCATION);
  my $argValues = eventArgValues(M_ICON_LOCATION, $packedStr);
  my $argNames  = eventArgNames (M_ICON_LOCATION, $argValues);
  my $argTypes  = eventArgTypes (M_ICON_LOCATION, $argValues);
  my $args      = eventArgs     (M_ICON_LOCATION, $argValues);

=head1 DESCRIPTION

Every event send by I<fvwm> consist of arguments. The argument names and
types vary from one event type to another. For example, event of the
type B<M_NEW_DESK> consists of only one argument I<desk> of type I<number>.
B<M_NEW_PAGE> consists of 5 numeric arguments, B<M_MINI_ICON> consists of 10
arguments of different types.

This class provides information about all fvwm events. It provides such
services as listing all supported event types and their names,
converting event type to event name, listing the event argument names/types,
constructing event argument values from the plain packet data.

Usually you do not need to work with this class directly, but, instead, with
B<FVWM::Event> objects. Hovewer, you may need this class source as a
reference for the names of the event arguments and their types.

=head1 PUBLIC FUNCTIONS

=over 4

=item B<eventName> I<type>

Returns the string representation of the numeric event I<type> constant,
like I<M_RAISE_WINDOW> or I<MX_ENTER_WINDOW>.

=item B<eventArgValues> I<type> I<packedStr>

Constructs array ref of argument values for the event I<type>
from the I<packedStr> (as received from I<fvwm>).

If the last argument type of the event is string, for convenience,
everything past the first null (or newline) is automatically stripped
from the last argument value.

=item B<eventArgNames> I<type> I<argValues>

Returns array ref of argument names of the event type.

I<argValues> is either the real array ref of values (as returned by
B<eventArgValues>) or a number of actual values.
The returned array has the same number of elements.

=item B<eventArgTypes> I<type> I<argValues>

Returns array ref of argument types of the event type.

I<argValues> is either the real array ref of values (as returned by
B<eventArgValues>) or a number of actual values.
The returned array has the same number of elements.

=item B<eventLoopArgNames> I<type> I<argValues>

Returns array ref of looped argument names of the event type (or undef).

=item B<eventLoopArgTypes> I<type> I<argValues>

Returns array ref of looped argument types of the event type (or undef).

=item B<eventArgs> I<type> I<argValues>

Constructs hash ref of the named arguments for the event I<type>
from the I<argValues> array ref (as returned by B<eventArgValues>).

=item B<eventArgAliases> I<type>

This method is provided for backward compatibility when argument names
are changed. For example, in the past the argument name of I<M_NEW_DESK>
was B<desk>, but now it is B<desk_n>. Using this method it is possible
to make both names supported. Returns hash ref (old-name => new-name).

=item B<allEventNames>

Returns array ref of all known event names (strings).
In the list context returns list of these names.

=item B<allEventTypes>

Returns array ref of all known event types (numbers).
In the list context returns list of these types.

=item B<allEventTypeNames>

Returns hash ref of all known event names and types (type => name).

=back

=head1 SEE ALSO

L<FVWM::Event>.

=cut

1;
