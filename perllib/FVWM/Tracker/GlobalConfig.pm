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

package FVWM::Tracker::GlobalConfig;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"value updated",
	];
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};
	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		$self->calculateInternals($event->args);
	});

	$self->requestConfigInfoEvents;

	my $result = $self->SUPER::start;

	$self->deleteHandlers;

	$self->addHandler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		my $name = $self->calculateInternals($event->args);
		return unless defined $name;
		$self->notify("value changed", $name);
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
	return undef if $text =~ /^(?:desktopsize|colorset|\*)/i;

	return undef unless $text =~ /^(desktopname \d+|[^\s]+) (.*)$/i;
	my $name = $1;
	my $value = $2;
	$self->{data}->{$name} = $value;

	return $name;
}

sub data ($;$) { 
	my $self = shift;
	my $name = shift;
	my $data = $self->{data};
	return $data unless defined $name;
	return $data->{$name};
}

sub dump ($;$) { 
	my $self = shift;
	my $name = shift;
	my $data = $self->{data};
	my @names = defined $name? ($name): sort keys %$data;

	my $string = "";
	foreach (@names) {
		my $value = $data->{$_};
		$string .= "$_ $value\n";
	}
	return $string;
}

1;
