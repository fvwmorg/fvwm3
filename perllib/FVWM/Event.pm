# Copyright (c) 2003 Mikhael Goikhman
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

package FVWM::Event;

use strict;
use FVWM::EventNames;

sub new ($$$) {
	my $class = shift;
	my $type = shift;
	my $argValues = shift;

	my $isSpecial = defined $argValues? 0: 1;
	my $isExtended = $type & M_EXTENDED_MSG? 1: 0;

	$argValues ||= [];
	$argValues = eventArgValues($isSpecial? "faked": $type, $argValues)
		unless ref($argValues);

	my $self = {
		type => $type,
		args => undef,  # lazy hash of event arguments
		argValues => $argValues,
		propagationAllowed => 1,
		isSpecial => $isSpecial,
		isExtended => $isExtended,
	};

	bless $self, $class;

	return $self;
}

sub type ($) {
	my $self = shift;
	return $self->{'type'};
}

sub argValues ($) {
	my $self = shift;
	return $self->{'argValues'};
}

sub argNames ($) {
	my $self = shift;
	return eventArgNames($self->type, $self->argValues);
}

sub argTypes ($) {
	my $self = shift;
	return eventArgTypes($self->type, $self->argValues);
}

sub loopArgNames ($) {
	my $self = shift;
	return eventLoopArgNames($self->type, $self->argValues);
}

sub loopArgTypes ($) {
	my $self = shift;
	return eventLoopArgTypes($self->type, $self->argValues);
}

sub args ($) {
	my $self = shift;
	$self->{'args'} ||= eventArgs($self->type, $self->argValues);
	return $self->{'args'};
}

sub isExtended ($) {
	my $self = shift;
	return $self->{'isExtended'};
}

sub name ($) {
	my $self = shift;
	return eventName($self->type);
}

sub propagationAllowed ($;$) {
	my $self = shift;
	my $value = shift;
	$self->{'propagationAllowed'} = $value if defined $value;

	return $self->{'propagationAllowed'};
}

sub dump ($) {
	my $self = shift;
	my $args = $self->args;
	my $string = $self->name . "\n";

	my @argNames  = @{$self->argNames};
	my @argTypes  = @{$self->argTypes};
	my @argValues = @{$self->argValues};

	while (@argNames) {
		my $name  = shift @argNames;
		my $type  = shift @argTypes;
		my $value = shift @argValues;

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
			my $loopArgNames = $self->loopArgNames;
			my $loopArgTypes = $self->loopArgTypes;
			my $j = 0;
			while ($j < @$value) {
				my $k = 0;
				foreach (@$loopArgNames) {
					my $i = int($j / @$loopArgNames) + 1;
					push @argNames, "[$i] $_";
					push @argTypes, $loopArgTypes->[$k];
					push @argValues, $value->[$j];
					$j++; $k++;
				}
			}
			$text = sprintf("(%d)", @$value / @$loopArgNames);
		} elsif ($type == FVWM::EventNames::wflags) {
			$text = qq([window flags are not supported yet]);
		} else {
			$text = qq([unsupported arg type $type] "$value");
		}

		my $nameLen = 12;
		$nameLen = int((length($name) + 5) / 6) * 6
			if length($name) > $nameLen;
		$string .= sprintf "\t%-${nameLen}s %s\n", $name, $text;
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
		my $argValue = $self->args->{$method};
		return $argValue if defined $argValue;

		my $alias = eventArgAliases($self->type)->{$method} || '*none*';
		$argValue = $self->args->{$alias};
		return $argValue if defined $argValue;

		die "Unknown argument $method for event " . $self->name . "\n";
	}

	die "Unknown method $method on $self called\n";
}

# ----------------------------------------------------------------------------

=head1 NAME

FVWM::Event - the FVWM event object passed to event handlers

=head1 SYNOPSIS

  use lib `fvwm-perllib dir`;
  use FVWM::Module;

  my $module = new FVWM::Module(Mask => M_FOCUS_CHANGE);

  # auto-raise all windows
  sub autoRaise ($$) {
      my ($module, $event) = @_;
      $module->debug("Got " . $event->name . "\n");
      $module->debug("\t$_: " . $event->args->{$_} . "\n")
          foreach sort keys %{$event->args};
      $module->send("Raise", $event->_win_id);
  }
  $module->addHandler(M_FOCUS_CHANGE, \&autoRaise);

  $module->eventLoop;

=head1 DESCRIPTION

To be written.

=head1 METHODS

=over 4

=item B<new> I<type> I<argValues>

Constructs event object of the given I<type>.
I<argValues> is either an array ref of event's arguments (every event type
has its own argument list, see L<FVWM::EventNames>) or a packed string of
these arguments as received from the I<fvwm> pipe.

=item B<type>

Returns event's type (usually long integer).

=item B<argNames>

Returns an array ref of the event argument names.

    print "$_ " foreach @{$event->argNames});

Note that this array of names is statical for any given event type.

=item B<argTypes>

Returns an array ref of the event argument types.

    print "$_ " foreach @{$event->argTypes});

Note that this array of types is statical for any given event type.

=item B<loopArgNames>

Returns an array ref of the looped argument names of the event (or undef).

=item B<loopArgTypes>

Returns an array ref of the looped argument types of the event (or undef).

=item B<argValues>

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

=item B<isExtended>

For technical reasons there are 2 categories of FVWM events, regular and
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

=item B<propagationAllowed> [I<bool>]

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
