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

package FVWM::Tracker::Scheduler;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"scheduled alarm",
	];
}

sub new ($$%) {
	my $class = shift;
	my $module = shift;
	my %params = @_;

	my $self = $class->FVWM::Tracker::new($module);

	$self->{randomBaseNumber} = 8000000 + int(rand(900000));
	$self->{sentStringPrefix} = "FVWM::Tracker::Scheduler alarm ";
	$self->{moduleName} = $params{ModuleName} || $self->{module}->name;
	$self->{useAlarm} = (exists $params{UseAlarm}?
		$params{UseAlarm}: $module->isDummy)? 1: 0;

	return $self;
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};

	my $result = $self->SUPER::start;

	$self->{internalObserverId} ||= $self->observe(sub {
		my $scheduleId = $_[3];
		my $scheduleData = $self->data($scheduleId);
		$self->{rescheduledSeconds} = -1;

		&{$scheduleData->{callback}}($scheduleData, @{$scheduleData->{params}});

		my $newSeconds = $self->{rescheduledSeconds};
		if ($newSeconds >= 0) {
			$scheduleData->{seconds} = $newSeconds if $newSeconds;
			$self->sendSchedule($scheduleId);
		} else {
			$self->deschedule($scheduleId);
		}
	});

	$self->addHandler(M_STRING, sub {
		my $event = $_[1];
		my $text = $event->_text;
		return unless $text =~ /^$self->{sentStringPrefix}(\d+)/;
		$self->notify("main", $1);
	});

	return $result;
}

sub stop ($) {
	my $self = shift;

	$self->SUPER::stop;
	$self->descheduleAll;
}

sub sendSchedule ($$) {
	my $self = shift;
	my $scheduleId = shift;
	my $sd = $self->{data}->{$scheduleId};

	my $string = "$self->{sentStringPrefix}$scheduleId";
	if ($self->{useAlarm}) {
		$SIG{ALRM} = sub {
			$self->{module}->emulateEvent(M_STRING, [ 0, 0, 0, $string ]);
		};
		alarm($sd->{seconds});
	} else {
		my $mSeconds = $sd->{seconds} * 1000;
		$self->{module}->send("Schedule $mSeconds $sd->{fvwmId} "
			. "SendToModule $self->{moduleName} $string");
	}
	$sd->{sentTime} = time();
}

sub schedule ($$$;$) {
	my $self = shift;
	my $seconds = shift || $self->internalDie("schedule: no seconds");
	my $callback = shift || $self->internalDie("schedule: no callback");

	my $scheduleId = ++$self->{lastScheduleNum};
	my $fvwmId = $self->{randomBaseNumber} + $scheduleId;
	my $scheduleData = {
		sentTime => 0,
		seconds  => $seconds,
		fvwmId   => $fvwmId,
		callback => $callback,
		params   => [ @_ ],
	};

	$self->{data}->{$scheduleId} = $scheduleData;
	$self->sendSchedule($scheduleId);

	return $scheduleId;
}

sub deschedule ($$) {
	my $self = shift;
	my $scheduleId = shift;
	my $data = $self->{data};
	next unless exists $data->{$scheduleId};
	my $fvwmId = $data->{$scheduleId}->{fvwmId};

	$self->{module}->send("Deschedule $fvwmId")
		if defined $self->{module};  # ready for DESTROY
	delete $data->{$scheduleId};
}

sub reschedule ($;$) {
	my $self = shift;
	my $seconds = shift || 0;
	$self->{rescheduledSeconds} = $seconds;
}

sub descheduleAll ($) {
	my $self = shift;
	my $data = $self->{data};
	foreach (keys %$data) {
		$self->deschedule($_);
	}
}

sub toBeDisconnected ($) {
	my $self = shift;
	$self->descheduleAll();
}

sub data ($;$) { 
	my $self = shift;
	my $scheduleId = shift;
	my $data = $self->{data};
	return $data unless defined $scheduleId;
	return $data->{$scheduleId};
}

sub dump ($;$) { 
	my $self = shift;
	my $scheduleId = shift;
	my $data = $self->{data};
	my @ids = $scheduleId? $scheduleId: sort { $a <=> $b } keys %$data;

	my $string = "";
	foreach (@ids) {
		my $sd = $data->{$_};
		my $timeStr = localtime($sd->{sentTime});
		$timeStr = "$5-$2-$3 $4" if $timeStr =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/;

		$string .= "Schedule $_: $timeStr + $sd->{seconds} sec, use 'Deschedule $sd->{fvwmId}'\n";
	}
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to define alarm callback,
several at the same time.

This tracker defines the following observables:

    "scheduled alarm"

But it is suggested not to use the usual tracker B<observe>/B<unobserve> API,
but to use the B<schedule>/B<deschedule>/B<reschedule> API instead.

This tracker uses the I<fvwm> command B<Schedule> to get a notification.
It is possible to use the perl I<alarm>. This is the default if the module
is run in the dummy mode. To set it explicitly, pass I<UseAlarm> boolean
value when the tracker is created. Note that alarm signals are not relable
in some perl versions, on some systems and with some kind of applications,
don't expect the alarm method to work at all.

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    # beep every 40 seconds; exit in 100 seconds

    my $scheduler = $module->track("Scheduler");
    $scheduler->schedule(40,
        sub { $module->send("Beep"); $scheduler->reschedule; });
    $scheduler->schedule(100, sub { $module->terminate; });

=head1 PUBLIC METHODS

=over 4

=item B<schedule> I<seconds> I<callback> [I<params>]

Sets the alarm I<callback> that is called in about I<seconds> seconds.

The I<callback> parameters are: hash as returned using B<data> when called
with the I<scheduled-id> parameter, and optional I<params>.

Returns I<scheduled-id> that may be used in B<deschedule> or B<data>.

=item B<deschedule> I<scheduled-id>

Removes the scheduled callback identified by I<scheduled-id>.

=item B<reschedule> [I<seconds>]

When the scheduled callback is called, it is possible to reinitialize the
same callback using the same or different number of I<seconds>.

This may be useful when defining a periodical callback that should be,
say, called every 10 minutes (600 seconds).

=item descheduleAll

Removes all previously scheduled callbacks.

=back

=head1 OVERRIDDEN METHODS

=over 4

=item toBeDisconnected

Calls B<descheduleAll>.

=item B<data> [I<sheduled-id>]

Returns either array ref of hash refs, or one hash ref if
I<sheduled-id> is given. The hash keys are:

    sentTime   - unix time (seconds since 1970)
    seconds    - requested alarm seconds
    fvwmId     - internal I<fvwm>'s Schedule id
    callback   - alarm callback, CODE ref
    params     - ARRAY ref of optional callback parameters

=item B<dump> [I<sheduled-id>]

Works similarly to B<data>, but returns one or many debug lines (one line
per scheduled alarm).

If no scheduled callbacks are active, the empty string is returned as expected.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
