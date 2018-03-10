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

	$self->{random_base_number} = 8000000 + int(rand(900000));
	$self->{sent_string_prefix} = "FVWM::Tracker::Scheduler alarm ";
	$self->{module_name} = $params{ModuleName} || $self->{module}->name;
	$self->{use_alarm} = (exists $params{UseAlarm}
		? $params{UseAlarm} : $module->is_dummy) ? 1 : 0;

	return $self;
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};

	my $result = $self->SUPER::start;

	$self->{internal_observer_id} ||= $self->observe(sub {
		my $schedule_id = $_[3];
		my $schedule_data = $self->data($schedule_id);
		$self->{rescheduled_seconds} = -1;

		&{$schedule_data->{callback}}($schedule_data, @{$schedule_data->{params}});

		my $new_seconds = $self->{rescheduled_seconds};
		if ($new_seconds >= 0) {
			$schedule_data->{seconds} = $new_seconds if $new_seconds;
			$self->send_schedule($schedule_id);
		} else {
			$self->deschedule($schedule_id);
		}
	});

	$self->add_handler(M_STRING, sub {
		my $event = $_[1];
		my $text = $event->_text;
		return unless $text =~ /^$self->{sent_string_prefix}(\d+)/;
		$self->notify("main", $1);
	});

	return $result;
}

sub stop ($) {
	my $self = shift;

	$self->SUPER::stop;
	$self->deschedule_all;
}

sub send_schedule ($$) {
	my $self = shift;
	my $schedule_id = shift;
	my $sd = $self->{data}->{$schedule_id};

	my $string = "$self->{sent_string_prefix}$schedule_id";
	if ($self->{use_alarm}) {
		$SIG{ALRM} = sub {
			$self->{module}->emulate_event(M_STRING, [ 0, 0, 0, $string ]);
		};
		alarm($sd->{seconds});
	} else {
		my $mseconds = $sd->{seconds} * 1000;
		$self->{module}->send("Schedule $mseconds $sd->{fvwm_id} "
			. "SendToModule $self->{module_name} $string");
	}
	$sd->{time_sent} = time();
}

sub schedule ($$$;$) {
	my $self = shift;
	my $seconds = shift || $self->internal_die("schedule: no seconds");
	my $callback = shift || $self->internal_die("schedule: no callback");

	my $schedule_id = ++$self->{last_schedule_num};
	my $fvwm_id = $self->{random_base_number} + $schedule_id;
	my $schedule_data = {
		time_sent => 0,
		seconds   => $seconds,
		fvwm_id   => $fvwm_id,
		callback  => $callback,
		params    => [ @_ ],
	};

	$self->{data}->{$schedule_id} = $schedule_data;
	$self->send_schedule($schedule_id);

	return $schedule_id;
}

sub deschedule ($$) {
	my $self = shift;
	my $schedule_id = shift;
	my $data = $self->{data};
	next unless exists $data->{$schedule_id};
	my $fvwm_id = $data->{$schedule_id}->{fvwm_id};

	$self->{module}->send("Deschedule $fvwm_id")
		if defined $self->{module};  # ready for DESTROY
	delete $data->{$schedule_id};
}

sub reschedule ($;$) {
	my $self = shift;
	my $seconds = shift || 0;
	$self->{rescheduled_seconds} = $seconds;
}

sub deschedule_all ($) {
	my $self = shift;
	my $data = $self->{data};
	foreach (keys %$data) {
		$self->deschedule($_);
	}
}

sub to_be_disconnected ($) {
	my $self = shift;
	$self->deschedule_all();
}

sub data ($;$) { 
	my $self = shift;
	my $schedule_id = shift;
	my $data = $self->{data};
	return $data unless defined $schedule_id;
	return $data->{$schedule_id};
}

sub dump ($;$) { 
	my $self = shift;
	my $schedule_id = shift;
	my $data = $self->{data};
	my @ids = $schedule_id ? $schedule_id : sort { $a <=> $b } keys %$data;

	my $string = "";
	foreach (@ids) {
		my $sd = $data->{$_};
		my $time_str = localtime($sd->{time_sent});
		$time_str = "$5-$2-$3 $4" if $time_str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/;

		$string .= "Schedule $_: $time_str + $sd->{seconds} sec, use 'Deschedule $sd->{fvwm_id}'\n";
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

=item deschedule_all

Removes all previously scheduled callbacks.

=back

=head1 OVERRIDDEN METHODS

=over 4

=item to_be_disconnected

Calls B<deschedule_all>.

=item B<data> [I<sheduled-id>]

Returns either array ref of hash refs, or one hash ref if
I<sheduled-id> is given. The hash keys are:

    time_sent  - unix time (seconds since 1970)
    seconds    - requested alarm seconds
    fvwm_id    - internal I<fvwm>'s Schedule id
    callback   - alarm callback, CODE ref
    params     - ARRAY ref of optional callback parameters

=item B<dump> [I<sheduled-id>]

Works similarly to B<data>, but returns one or many debug lines (one line
per scheduled alarm).

If no scheduled callbacks are active, the empty string is returned as expected.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
