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
use FVWM::Constants;
use FVWM::EventNames;

sub new ($$$) {
	my $class = shift;
	my $type = shift;
	my $argValues = shift;

	my $isSpecial = defined $argValues? 0: 1;
	my $isExtended = $type & M_EXTENDED_MSG? 1: 0;

	my $eventInfo = $class->eventTypeInfo($isSpecial? "faked": $type);

	$argValues ||= [];
	$argValues = [ unpack($eventInfo->{'format'}, $argValues) ]
		unless ref($argValues);

	my $self = {
		type => $type,
		args => undef,  # lazy hash of event arguments
		argValues => $argValues,
		propagationAllowed => 1,
		isSpecial => $isSpecial,
		isExtended => $isExtended,
		eventInfo => $eventInfo,
	};

	bless $self, $class;

   # strip everything past the first null (or newline) if needed
   $argValues->[@$argValues - 1] =~ s/\n*\0.*//s if $self->isTextBased;

	return $self;
}

sub type ($) {
	my $self = shift;
	return $self->{'type'};
}

sub typeToBinary ($;$) {
	my $this = shift;
	my $type = shift;
	$type = (ref($this)? $this->{'type'}: "no-event-type")
		unless defined $type;

	return $type unless $type =~ /^\d+$/;
	return sprintf("%b", $type);
}

# class method
sub eventTypeInfo ($$) {
	my $class = shift;
	my $type = shift || "no-event-type";
	my $eventInfo = $FVWM::EventNames::EVENTS_INFO->{$type};
	die "Unsupported event type (" . $class->typeToBinary($type) . ")"
		unless defined $eventInfo;
	return $eventInfo;
}

sub argNames ($;$) {
	my $this = shift;
	my $type = shift;
	my $eventInfo = ref($this)?
		$this->{'eventInfo'}: $this->eventTypeInfo($type);

	return $eventInfo->{'names'};
}

sub argValues ($) {
	my $self = shift;
	return $self->{'argValues'};
}

sub constructArgs ($) {
	my $self = shift;
	my $names = $self->argNames;
	my $values = $self->argValues;
	my $i = 0;
	my $suffix = "";
	$suffix = "1" if @$values > @$names;
	my %args = map {
		if ($i == $names) { $i = 0; $suffix++; }
		(($names->[$i++] || "unknown") . $suffix, $_)
	} @$values;
	$self->{'args'} = { %args };
}

sub args ($) {
	my $self = shift;
	$self->constructArgs unless defined $self->{'args'};
	return $self->{'args'};
}

sub isExtended ($) {
	my $self = shift;
	return $self->{'isExtended'};
}

sub isTextBased ($) {
	my $self = shift;
	return $self->{'eventInfo'}->{'format'} =~ /a\*$/? 1: 0;
}

sub name ($) {
	my $self = shift;
	my $type = $self->{'type'};
	if (!defined $FVWM::Event::EVENT_TYPE_NAMES) {
		$FVWM::Event::EVENT_TYPE_NAMES = {};
		foreach (@FVWM::Constants::EXPORT) {
			next unless /^M_/ || /^MX_/;
			no strict 'refs';
			next if /^MX_/ && !(&$_() & M_EXTENDED_MSG);
			$FVWM::Event::EVENT_TYPE_NAMES->{&$_()} = $_;
		}
	}
	my $name = $FVWM::Event::EVENT_TYPE_NAMES->{$type};
	$name = "*unknown-event-" . $self->typeToBinary . "*"
		unless defined $name;
	return $name;
}

sub propagationAllowed ($;$) {
	my $self = shift;
	my $value = shift;
	$self->{'propagationAllowed'} = $value if defined $value;

	return $self->{'propagationAllowed'};
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

=item B<typeToBinary> [I<type>]

Returns the binary representation of the event type. This may be used either
as class method (in which case additional parameter should be given) or as
object method.

=item B<argNames> [I<type>]

Returns an array ref of the event argument names.
May be also used as class method if additional parameter is given.

    print foreach @{$event->argNames});
    print join(', ', @{FVWM::Event->argNames(M_ADD_WINDOW)}), "\n";

Note that this array of names is statical and may be not synchronized
in some cases with the actual argument values described in the following
two methods.

=item B<argValues>

Returns an array ref of the event argument values.
In the previous versions of the library, all argument values were passed
to event handlers, now only one event method is passed. Calling this
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

=item B<isTextBased>

If the last argument of the event is text the event is text based. For
convenience, everything past the first null (or newline) is automatically
stripped from the text based packets, so the event handlers should not do
this every time.

=item B<name>

Returns a string representing the event name (like "M_ADD_WINDOW"), it is
the same as the corresponding C/Perl constant. May be (and in fact is)
used for debugging.

=item B<propagationAllowed> [I<bool>]

Sets or returns a boolean value that indicates enabling or disabling of
this event propagation.

=item B<_>I<name>

This is a shortcut for $event->args->{'name'}. Returns the named event
argument. See L<FVWM::EventNames> for names of all event argument names.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module>, L<FVWM::Constants> and
L<FVWM::EventNames>.

=cut

1;
