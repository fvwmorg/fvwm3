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
		handlerIds => {},
		observers => {},
	};
	bless $self, $class;

	return $self;
}

sub masks ($) {
	my $self = shift;
	my $mask = 0;
	my $xmask = 0;
	while (my ($id, $type) = each %{$self->{handlerIds}}) {
		(($type & M_EXTENDED_MSG)? $xmask: $mask) |= $type;
	}
	$self->internalDie("Inactive mask is not zero")
		unless $self->{active} || !$mask && !$xmask;
	my @list = ($mask, $xmask);
	return wantarray? @list: \@list;
}

sub addHandler ($$$) {
	my $self = shift;
	my $type = shift;
	my $handler = shift;

	my $handlerId = $self->{module}->addHandler($type, $handler);
	$self->{handlerIds}->{$handlerId} = $type;
	return $handlerId;
}

sub deleteHandlers ($;$) {
	my $self = shift;
	my $handlerIds = ref($_[0]) eq 'ARRAY'?
		shift(): [ keys %{$self->{handlerIds}} ];

	foreach (@$handlerIds) {
		next unless defined delete $self->{handlerIds}->{$_};
		$self->{module}->deleteHandler($_) if $self->{module};
	}
}

sub observe ($$;$) {
	my $self = shift;
	my $observable = ref($_[0]) eq ""? shift: "main";
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
	my $observable = ref($_) eq ""? shift: "*";
	my $observerId = shift || "*";

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

	$self->{module}->FVWM::Module::eventLoop(1)
		if %{$self->{handlerIds}};

	return $self->data;
}

sub stop ($) {
	my $self = shift;
	return unless $self->{active};
	$self->deleteHandlers;
	$self->{active} = 0;
}

sub restart ($) {
	my $self = shift;
	$self->stop;
	$self->start;
}

sub data ($) {
	my $self = shift;
	return $self->{data};
}

sub dump ($) {
	my $self = shift;
	return "";
}

sub requestWindowListEvents ($) {
	my $self = shift;
	warn "requestWindowListEvents() called after start()" if $self->{active};
	$self->addHandler(M_END_WINDOWLIST, sub { $_[0]->terminate; });
	$self->{module}->postponeSend("Send_WindowList");
}

sub requestConfigInfoEvents ($) {
	my $self = shift;
	warn "requestConfigInfoEvents() called after start()" if $self->{active};
	$self->addHandler(M_END_CONFIG_INFO, sub { $_[0]->terminate; });
	$self->{module}->postponeSend("Send_ConfigInfo");
}

sub internalDie ($$) {
	my $self = shift;
	my $msg = shift;
	my $class = ref($self);
	$self->{module}->internalDie("$class: $msg")
}

sub DESTROY ($) {
	my $self = shift;
	$self->stop;
}

# class method, should be overwritten
sub observables ($) {
	return [];
}

1;

__END__

=head1 DESCRIPTION

Tracker is an object that automatically listens to certain fvwm events and
gathers an information dynamically until it is stopped.

It also defines high level events for a caller to observe.

=head1 SYNOPSYS

Using B<FVWM::Module> $module object (preferably):

  my $tracker = $module->track("TrackerName", @params);
  my $initialData = $tracker->data;
  $tracker->observe("HighLevelTrackerEvent", sub { shift->data });
  $tracker->observe("HighLevelTrackerEvent2", sub { shift->stop });

Or directly (B<FVWM::Module> $module object is still needed):

  my $tracker = new FVWM::Tracker::TrackerName($module, @params);
  my $initialData = $tracker->start;
  $tracker->observe("HighLevelTrackerEvent", sub { shift->data });
  $tracker->observe("HighLevelTrackerEvent2", sub { shift->stop });

=cut
