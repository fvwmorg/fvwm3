# Copyright (c) 2003-2009, Mikhael Goikhman
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

package FVWM::Tracker;

use 5.004;
use strict;

use FVWM::Constants;

sub import ($@) {
	my $class = shift;
	my $caller = caller;

	while (@_) {
		my $name = shift;
		if ($name eq 'base') {
			next if UNIVERSAL::isa($caller, __PACKAGE__);
			eval "
				package $caller;
				use vars qw(\@ISA);
				use FVWM::Constants;
				\@ISA = qw(FVWM::Tracker);
			";
			if ($@) {
				die "Internal error:\n$@";
			}
		}
	}
}

sub new ($$) {
	my $class = shift;
	my $module = shift;
	die "$class: no FVWM::Module object given in constructor\n"
		unless UNIVERSAL::isa($module, "FVWM::Module");
	my $self = {
		module => $module,
		data   => undef,
		active => 0,
		handler_types => {},
		handler_ids => {},
		observers => {},
	};
	bless $self, $class;

	return $self;
}

sub masks ($) {
	my $self = shift;
	my $mask = 0;
	my $xmask = 0;
	while (my ($id, $type) = each %{$self->{handler_types}}) {
		(($type & M_EXTENDED_MSG) ? $xmask : $mask) |= $type;
	}
	$self->internal_die("Inactive mask is not zero")
		unless $self->{active} || !$mask && !$xmask;
	my @list = ($mask, $xmask);
	return wantarray ? @list : \@list;
}

sub add_handler ($$$) {
	my $self = shift;
	my $type = shift;
	my $handler = shift;

	my $handler_id = $self->{module}->add_handler($type, $handler, 1);
	$self->{handler_types}->{$handler_id} = $type;
	$self->{handler_ids}->{$handler_id} = $handler_id;
	return $handler_id;
}

sub delete_handlers ($;$) {
	my $self = shift;
	my $handler_ids = ref($_[0]) eq 'ARRAY'
		? shift() : [ keys %{$self->{handler_ids}} ];

	foreach (@$handler_ids) {
		next unless defined delete $self->{handler_types}->{$_};
		my $handler_id = delete $self->{handler_ids}->{$_}
			or die "Internal #1";
		if ($self->{module}) {
			$self->{module}->delete_handler($handler_id)
				or die "Internal #2";
		}
	}
}

sub observe ($$;$) {
	my $self = shift;
	my $observable = ref($_[0]) eq "" ? shift : "main";
	my $callback = shift;

	my $observables = $self->observables;
	$observable = $observables->[0] if $observable eq "main";

	$self->{module}->debug(qq(observe "$observable"), 3);
	# TODO: check observable existence

	$self->{observers}->{$observable} ||= [];
	push @{$self->{observers}->{$observable}}, $callback;
	
	return [ $observable, @{$self->{observers}->{$observable}} - 1 ];
}

sub unobserve ($;$$) {
	my $self = shift;
	my $observable = ref($_) eq "" ? shift : "*";
	my $observer_id = shift || "*";

	### TODO
	#$self->{observers}->{$observable} = [];
}

sub notify ($$@) {
	my $self = shift;
	my $observable = shift;

	my $observables = $self->observables;
	$observable = $observables->[0] if $observable eq "main";

	$self->{module}->debug(qq(notify "$observable"), 3);
	# TODO: check observable existence

	my @callbacks = ();
	push @callbacks, @{$self->{observers}->{$observable}}
		if exists $self->{observers}->{$observable};
	push @callbacks, @{$self->{observers}->{'all'}}
		if exists $self->{observers}->{'all'} && $observable ne 'all';

	foreach (@callbacks) {
		$_->($self->{module}, $self, $self->data, @_);
	}
}

sub start ($) {
	my $self = shift;
	return if $self->{active};

	$self->{active} = 1;

	$self->{module}->FVWM::Module::event_loop(1)
		if %{$self->{handler_ids}};

	return $self->data;
}

sub stop ($) {
	my $self = shift;
	return unless $self->{active};
	$self->delete_handlers;
	$self->{active} = 0;
}

sub restart ($) {
	my $self = shift;
	$self->stop;
	$self->start;
}

sub to_be_disconnected ($) {
}

sub data ($) {
	my $self = shift;
	return $self->{data};
}

sub dump ($) {
	my $self = shift;
	return "";
}

sub request_windowlist_events ($) {
	my $self = shift;
	my $module = $self->{module};
	warn "request_windowlist_events() called after start()" if $self->{active};

	$self->add_handler(M_END_WINDOWLIST, sub { $_[0]->terminate; });
	$module->emulate_event(M_END_WINDOWLIST, []) if $module->is_dummy;
	$module->postpone_send("Send_WindowList");
}

sub request_configinfo_events ($;$) {
	my $self = shift;
	my $name = shift;
	my $module = $self->{module};
	warn "request_configinfo_events() called after start()" if $self->{active};

	$self->add_handler(M_END_CONFIG_INFO, sub { $_[0]->terminate; });
	$module->emulate_event(M_END_CONFIG_INFO, []) if $module->is_dummy;
	$module->postpone_send("Send_ConfigInfo" . ($name ? " *$name" : ""));
}

sub internal_die ($$) {
	my $self = shift;
	my $msg = shift;
	my $class = ref($self);
	$self->{module}->internal_die("$class: $msg")
}

sub DESTROY ($) {
	my $self = shift;
	$self->stop;
}

# class method, should be overwritten
sub observables ($) {
	return [];
}

use vars qw($AUTOLOAD);

# support old API, like addHandler, dispatch to add_handler
sub AUTOLOAD ($;@) {
	my $self = shift;
	my @params = @_; 

	my $autoload_method = $AUTOLOAD;
	my $method = $autoload_method;  

	# remove the package name
	$method =~ s/.*://g;

	$method =~ s/XMask/Xmask/;
	$method =~ s/([a-z])([A-Z])/${1}_\L$2/g;

	die "No method $method in $self as guessed from $autoload_method"
		unless $self->can($method);

	$self->$method(@params);
}

1;

__END__

=head1 DESCRIPTION

Tracker is an object that automatically listens to certain fvwm events and
gathers an information in the background.

When a tracker is created it may enter its own event loop to gather an
initial data, so the returned tracker object already has the initial data.
It also continues to update the data automatically until it is stopped.

This package is a superclass for the concrete tracker implementations.
It defines the common Tracker API, including a way to access the tracked data
and to define high level events for the tracker caller to observe.

=head1 SYNOPSYS

Using B<FVWM::Module> $module object:

    my $tracker = $module->track("TrackerName", @params);
    my $initial_data = $tracker->data;
    $tracker->observe("observable1", sub { shift->data });
    $tracker->observe("observable2", sub { shift->stop });

In the future this syntax will probably work too:

    my $tracker = new FVWM::Tracker::TrackerName($module, @params);
    my $initial_data = $tracker->start;
    $tracker->observe("observable1", sub { shift->data });
    $tracker->observe("observable2", sub { shift->stop });

=head1 PUBLIC METHODS

=over 4

=item B<start>

Makes the tracker actually work, i.e. listen to I<fvwm> events,
gather data and forms high level events, so called observables.

This method is usually automatically called when the tracker is created
unless specifically asked not to.

=item B<stop>

Stops the tracker activity. The corresponding I<fvwm> events are not listened,
data is not updated and no observers called.

To return the tracker to the normal activity, call B<start> method.

=item B<restart>

This is a shortcut method to B<stop> and then B<start> the tracker.
The following scenatio is possible. You start the tracker, read its
data and immediately stop it (to reduce event tracker to the module).
At some point you may want to read the updated data, so you restart the
tracker and optionally stop it again.

Note that no observers are removed during B<stop>, so the tracker
theoretically may be restarted without any side effect even if some
observers are defined.

=item B<observe> [I<observable>] I<observer-callback>

Defines an observer that will be called every time the tracker I<observable>
happens. The I<observer-callback> is a CODE reference that gets the
following parameters: $module (B<FVWM::Module> object), $tracker (this object),
$data (the same as returned by B<data> method) and optional observable
parameters that are specific to this I<observable>.

A special I<observable> value "main" means the first observable defined
in the tracker, it is the default value when no I<observable> is given.

=item B<unobserve> [I<observable> [I<observer-id>]]

Stops an observing using the I<observer-id> that is returned by B<observe>
method.

A special I<observable> value "main" means the first observable defined
in the tracker. A special I<observable> value "*" means all defined
observables.

=item B<data>

Returns the whole data collected by the tracker.

Usually subclasses add an optional parameter I<key> that limits the whole
data to the given key.

=item B<dump>

Returns the string representing the whole tracker data in the human readable
form, useful for debugging.

Usually subclasses add an optional parameter I<key> that limits the whole
data to the given key.

=back

=head1 METHODS FOR SUBCLASSES

=over 4

=item B<observables>

A subclass should define a list of observables that a caller may listen to
using B<observe> method. It is the subclass responsiblity to actually signal
every observable listed using B<notify> method.

Returns a reference to a string array.

=item B<new> I<module> I<param-hash>

This superclass method should be called by subclasses.
Please do not use this class method in programs, use the first syntax shown
in the I<SYNOPSYS> section instead.
 
I<module> is an B<FVWM::Module> instance.
I<param-hash> is specific to the concrete Tracker class.

=item B<add_handler> I<type> I<handler>

A wrapper to B<FVWM::Module>::B<add_handler>, has the same syntax, but stores
all handlers so they may be deleted at once using B<delete_handlers>.

=item B<delete_handlers> [I<handler-ids>]

Deletes all handlers defined using add_handler or the ones specified
using an optional I<handler-ids> array ref.

=item B<notify> I<observable> [I<observable-params>]

Notifies all listeners that were defined using B<observe>, by calling their
observer function with the following parameters: $module, $tracker, $data,
I<observable-params>.

=item B<request_windowlist_events>

Subclasses that work using I<fvwm> events sent in responce to
B<Send_WindowList> command should call this shortcut method.
Automatically sends the needed command (after the tracker event mask is
counted) and defines a handler that terminates the initial tracker event
loop in response to I<M_END_WINDOWLIST> event.

=item B<request_configinfo_events>

Subclasses that work using I<fvwm> events sent in responce to
B<Send_ConfigInfo> command should call this shortcut method.
Automatically sends the needed command (after a tracker event mask is
counted) and defines a handler that terminates the initial tracker event
loop in response to I<M_END_CONFIG_INFO> event.

=item B<internal_die>

Subclasses may call this method when something wrong happens.
This is a wrapper to B<FVWM::Module>::B<internal_die>.

=item B<to_be_disconnected>

Does nothing by default. Subclasses may implement this method if something
should be sent to I<fvwm> just before the module disconnects itself.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and the concrete tracker
implementations: L<FVWM::Tracker::Colorsets>, L<FVWM::Tracker::GlobalConfig>,
L<FVWM::Tracker::ModuleConfig>, L<FVWM::Tracker::PageInfo>,
L<FVWM::Tracker::Scheduler>, L<FVWM::Tracker::WindowList>.

=cut
