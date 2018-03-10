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

package FVWM::Event;

use strict;
use FVWM::EventNames;

sub new ($$$) {
	my $class = shift;
	my $type = shift;
	my $arg_values = shift;

	my $is_special = defined $arg_values ? 0 : 1;
	my $is_extended = $type & M_EXTENDED_MSG ? 1 : 0;

	$arg_values ||= [];
	$arg_values = event_arg_values($is_special ? "faked" : $type, $arg_values)
		unless ref($arg_values);

	my $self = {
		type => $type,
		args => undef,  # lazy hash of event arguments
		arg_values => $arg_values,
		propagation_allowed => 1,
		is_special => $is_special,
		is_extended => $is_extended,
	};

	bless $self, $class;

	return $self;
}

sub type ($) {
	my $self = shift;
	return $self->{'type'};
}

sub arg_values ($) {
	my $self = shift;
	return $self->{'arg_values'};
}

sub arg_names ($) {
	my $self = shift;
	return event_arg_names($self->type, $self->arg_values);
}

sub arg_types ($) {
	my $self = shift;
	return event_arg_types($self->type, $self->arg_values);
}

sub loop_arg_names ($) {
	my $self = shift;
	return event_loop_arg_names($self->type, $self->arg_values);
}

sub loop_arg_types ($) {
	my $self = shift;
	return event_loop_arg_types($self->type, $self->arg_values);
}

sub args ($) {
	my $self = shift;
	$self->{'args'} ||= event_args($self->type, $self->arg_values);
	return $self->{'args'};
}

sub is_extended ($) {
	my $self = shift;
	return $self->{'is_extended'};
}

sub name ($) {
	my $self = shift;
	return event_name($self->type);
}

sub propagation_allowed ($;$) {
	my $self = shift;
	my $value = shift;
	$self->{'propagation_allowed'} = $value if defined $value;

	return $self->{'propagation_allowed'};
}

sub dump ($) {
	my $self = shift;
	my $args = $self->args;
	my $string = $self->name . "\n";

	my @arg_names  = @{$self->arg_names};
	my @arg_types  = @{$self->arg_types};
	my @arg_values = @{$self->arg_values};

	while (@arg_names) {
		my $name  = shift @arg_names;
		my $type  = shift @arg_types;
		my $value = shift @arg_values;

		my $text;
		if ($type == FVWM::EventNames::number) {
			$text = $value;
			$text = "*undefined*" unless defined $value;
		} elsif ($type == FVWM::EventNames::bool) {
			$text = $value? "True": "False";
		} elsif ($type == FVWM::EventNames::window) {
			$text = sprintf("0x%07lx", $value);
		} elsif ($type == FVWM::EventNames::pixel) {
			$text = "rgb:" . join('/',
				sprintf("%06lx", $value) =~ /(..)(..)(..)/);
		} elsif ($type == FVWM::EventNames::string) {
			$value =~ s/"/\\"/g;
			$text = qq("$value");
		} elsif ($type == FVWM::EventNames::looped) {
			my $loop_arg_names = $self->loop_arg_names;
			my $loop_arg_types = $self->loop_arg_types;
			my $j = 0;
			while ($j < @$value) {
				my $k = 0;
				foreach (@$loop_arg_names) {
					my $i = int($j / @$loop_arg_names) + 1;
					push @arg_names, "[$i] $_";
					push @arg_types, $loop_arg_types->[$k];
					push @arg_values, $value->[$j];
					$j++; $k++;
				}
			}
			$text = sprintf("(%d)", @$value / @$loop_arg_names);
		} elsif ($type == FVWM::EventNames::wflags) {
			$text = qq([window flags are not supported yet]);
		} else {
			$text = qq([unsupported arg type $type] "$value");
		}

		my $name_len = 12;
		$name_len = int((length($name) + 5) / 6) * 6
			if length($name) > $name_len;
		$string .= sprintf "\t%-${name_len}s %s\n", $name, $text;
	}
	return $string;
}

sub AUTOLOAD ($;@) {
	my $self = shift;
	my @params = @_;

	my $method = $FVWM::Event::AUTOLOAD;

	# remove the package name
	$method =~ s/.*://g;
	# DESTROY messages should never be propagated
	return if $method eq 'DESTROY';

	if ($method =~ s/^_//) {
		my $arg_value = $self->args->{$method};
		return $arg_value if defined $arg_value;

		my $alias = event_arg_aliases($self->type)->{$method} || '*none*';
		$arg_value = $self->args->{$alias};
		return $arg_value if defined $arg_value;

		die "Unknown argument $method for event " . $self->name . "\n";
	}

	die "Unknown method $method on $self called\n";
}

# ----------------------------------------------------------------------------

=head1 NAME

FVWM::Event - the fvwm event object passed to event handlers

=head1 SYNOPSIS

  use lib `fvwm-perllib dir`;
  use FVWM::Module;

  my $module = new FVWM::Module(Mask => M_FOCUS_CHANGE);

  # auto-raise all windows
  sub auto_raise ($$) {
      my ($module, $event) = @_;
      $module->debug("Got " . $event->name . "\n");
      $module->debug("\t$_: " . $event->args->{$_} . "\n")
          foreach sort keys %{$event->args};
      $module->send("Raise", $event->_win_id);
  }
  $module->add_handler(M_FOCUS_CHANGE, \&auto_raise);

  $module->event_loop;

=head1 DESCRIPTION

To be written.

=head1 METHODS

=over 4

=item B<new> I<type> I<arg_values>

Constructs event object of the given I<type>.
I<arg_values> is either an array ref of event's arguments (every event type
has its own argument list, see L<FVWM::EventNames>) or a packed string of
these arguments as received from the I<fvwm> pipe.

=item B<type>

Returns event's type (usually long integer).

=item B<arg_names>

Returns an array ref of the event argument names.

    print "$_ " foreach @{$event->arg_names});

Note that this array of names is statical for any given event type.

=item B<arg_types>

Returns an array ref of the event argument types.

    print "$_ " foreach @{$event->arg_types});

Note that this array of types is statical for any given event type.

=item B<loop_arg_names>

Returns an array ref of the looped argument names of the event (or undef).

=item B<loop_arg_types>

Returns an array ref of the looped argument types of the event (or undef).

=item B<arg_values>

Returns an array ref of the event argument values.
In the previous versions of the library, all argument values were passed
to event handlers, now only one event object is passed. Calling this
method is the way to emulate the old behaviour.

Note that you should know the order of arguments, so the suggested way
is to use C<args> instead, although it is a bit slower.

=item B<args>

Returns hash ref of the named event argument values.

    print "[Debug] Got event ", $event->type, " with args:\n";
    while (($name, $value) = each %{$event->args})
        { print "\t$name: $value\n"; }

=item B<is_extended>

For technical reasons there are 2 categories of fvwm events, regular and
extended. This was done to enable more events. With introdution of the
extended event types (with the highest bit set) it is now possible to have
31+31=62 different event types rather than 32. This is a good point, the bad
point is that only event types of the same category may be masked (or-ed)
together. This method returns 1 or 0 depending on whether the event is
extended or not.

=item B<name>

Returns a string representing the event name (like "M_ADD_WINDOW"), it is
the same as the corresponding C/Perl constant. May be (and in fact is)
used for debugging.

=item B<propagation_allowed> [I<bool>]

Sets or returns a boolean value that indicates enabling or disabling of
this event propagation.

=item B<dump>

Returns a string representation of the event object, basically the event
name and all argument name=value lines.

=item B<_>I<name>

This is a shortcut for $event->args->{'I<name>'}. Returns the named event
argument. See L<FVWM::EventNames> for names of all event argument names.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module>, L<FVWM::Constants> and
L<FVWM::EventNames>.

=cut

1;
