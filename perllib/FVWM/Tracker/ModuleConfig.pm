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

package FVWM::Tracker::ModuleConfig;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"config line added",
	];
}

sub new ($$;$) {
	my $class = shift;
	my $module = shift;
	my $params = shift || {};

	my $self = $class->FVWM::Tracker::new($module);

	$self->{isHash} = ($params->{ConfigType} || 'hash') eq 'hash';
	$self->{moduleName} = $params->{ModuleName} || $self->{module}->name;
	# asis, spacefree, lowerkeys, upperkeys
	$self->{filter} = $params->{LineFilter} || 'spacefree';

	return $self;
}


sub start ($) {
	my $self = shift;

	$self->{data} = $self->{isHash}? {}: [];
	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	$self->requestConfigInfoEvents;

	my $result = $self->SUPER::start;

	$self->deleteHandlers;

	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		my $info = $self->calculateInternals($event->args);
		return unless defined $info;
		$self->notify("config line added", $info);
	});

	return $result;
}

sub calculateInternals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	my $text = $args->{text};
	$self->internalDie("No 'text' arg in M_CONFIG_INFO")
		unless defined $text;
	return undef unless $text =~ /^\*$self->{moduleName}(\s*[^\s].*)$/i;

	my $line = $1;
	$line =~ s/^\s+// unless $self->{filter} eq 'asis';
	$line =~ s/\s+$// unless $self->{filter} eq 'asis';

	if ($self->{isHash}) {
		my ($key, $value) = $line =~ /([^\s]+)\s*(.*)/;
		$key = lc($key) if $self->{filter} eq 'lowerkeys';
		$key = uc($key) if $self->{filter} eq 'upperkeys';
		$self->{data}->{$key} = $value;
		return $key;
	} else {
		push @{$self->{data}}, $line;
		return @{$self->{data}} - 1;
	}
}

sub data ($;$) { 
	my $self = shift;
	my $info = shift;
	my $data = $self->{data};
	return $data unless defined $info;
	return $data->{$info} if $self->{isHash};
	return $data->[$info];
}

sub dump ($;$) { 
	my $self = shift;
	my $info = shift;
	my $data = $self->{data};
	my @infos = defined $info? ($info):
		$self->{isHash}? sort keys %$data: (0 .. @$data - 1);

	my $string = "";
	foreach (@infos) {
		my $lineData = $self->{isHash}? "$_ $data->{$_}": $data->[$_];
		$string .= "\*$self->{moduleName}: $lineData\n";
	}
	return $string;
}

1;
