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

	my $eventInfo = $FVWM::EventNames::EVENTS_INFO->
		{ $isSpecial? "faked": $type };
	die "Unsupported event type (" . $class->typeToBinary($type) . ")"
		unless defined $eventInfo;

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

sub argNames ($) {
	my $self = shift;
	return $self->{'eventInfo'}->{'names'};
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

=item B<propagationAllowed> [bool]

Sets or returns a boolean value that indicates enabling or disabling of
this event propagation.

=item B<_>I<name>

This is a shortcut for $event->args->{'name'}. Returns the named event
argument. See L<FVWM::EventNames> for names of all event argument names.

=cut

1;
