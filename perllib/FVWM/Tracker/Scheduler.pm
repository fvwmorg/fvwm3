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

sub new ($$;$) {
   my $class = shift;
   my $module = shift;
   my $params = shift || {};

   my $self = $class->FVWM::Tracker::new($module);

	$self->{randomBaseNumber} = 8000000 + int(rand(900000));
	$self->{sentStringPrefix} = "FVWM::Tracker::Scheduler alarm ";
   $self->{moduleName} = $params->{ModuleName} || $self->{module}->name;

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
	my $data = $self->{data};

	$self->SUPER::stop;
	foreach (keys %$data) {
		$self->deschedule($_);
	}
}

sub sendSchedule ($$) {
	my $self = shift;
	my $scheduleId = shift;
	my $sd = $self->{data}->{$scheduleId};
	my $mSeconds = $sd->{seconds} * 1000;

	$self->{module}->send("Schedule $mSeconds $sd->{fvwmId} SendToModule " .
		"$self->{moduleName} $self->{sentStringPrefix}$scheduleId");
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
