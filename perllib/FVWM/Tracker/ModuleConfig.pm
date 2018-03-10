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

package FVWM::Tracker::ModuleConfig;

use strict;

use FVWM::Tracker qw(base);

sub observables ($) {
	return [
		"config line added",
	];
}

sub new ($$%) {
	my $class = shift;
	my $module = shift;
	my %params = @_;

	my $self = $class->FVWM::Tracker::new($module);

	$self->{is_hash} = 1;
	$self->{module_name} = $self->{module}->name;
	$self->{filter} = 'spacefree';
	$self->{default_config} = {};

	return $self->init(%params);
}

sub init ($%) {
	my $self = shift;
	my %params = @_;

	if ($params{ConfigType}) {
		$self->{is_hash} = $params{ConfigType} eq 'hash';
		$self->{default_config} = $self->{is_hash} ? {} : [];
	}
	$self->{module_name} = $params{ModuleName} if $params{ModuleName};
	$self->{filter} = $params{LineFilter} if $params{LineFilter};
	$self->{default_config} = $params{DefaultConfig}
		if ref($self->{default_config}) eq ref($params{DefaultConfig});

	return $self;
}

sub start ($) {
	my $self = shift;

	my $default = $self->{default_config};
	$self->{data} = $self->{is_hash} ? { %$default } : [ @$default ];

	$self->add_handler(M_CONFIG_INFO, sub {
		my $event = $_[1];
		$self->calculate_internals($event->args);
	});

	$self->request_configinfo_events($self->{module_name});

	my $result = $self->SUPER::start;

	$self->delete_handlers;

	$self->add_handler(M_CONFIG_INFO | M_SENDCONFIG, sub {
		my $event = $_[1];
		return unless $event->type == M_CONFIG_INFO;
		my $info = $self->calculate_internals($event->args);
		return unless defined $info;
		$self->notify("config line added", $info);
	});

	return $result;
}

sub calculate_internals ($$) {
	my $self = shift;
	my $args = shift;
	my $data = $self->{data};

	my $text = $args->{text};
	$self->internal_die("No 'text' arg in M_CONFIG_INFO")
		unless defined $text;
	return undef unless $text =~ /^\*$self->{module_name}(\s*[^\s].*)$/i;

	my $line = $1;
	$line =~ s/^\s+// unless $self->{filter} eq 'asis';
	$line =~ s/\s+$// unless $self->{filter} eq 'asis';

	if ($self->{is_hash}) {
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
	return $data->{$info} if $self->{is_hash};
	return $data->[$info];
}

sub dump ($;$) { 
	my $self = shift;
	my $info = shift;
	my $data = $self->{data};
	my @infos = defined $info? ($info):
		$self->{is_hash} ? sort keys %$data : (0 .. @$data - 1);

	my $string = "";
	foreach (@infos) {
		my $line_data = $self->{is_hash} ? "$_ $data->{$_}" : $data->[$_];
		$string .= "\*$self->{module_name}: $line_data\n";
	}
	return $string;
}

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the module
configuration.  The module configuration is usually represented using a hash,
but it may be represented as array of lines too.

This tracker defines the following observables:

    "config line added"

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    my $config_tracker = $module->track("ModuleConfig");
    my $config_hash = $config_tracker->data;
    my $font = $config_hash->{Font} || 'fixed';

or:

    my $config_tracker = $module->track(
        "ModuleConfig", DefaultConfig => { Font => 'fixed' } );
    my $font = $config_tracker->data('Font');

=head1 NEW METHODS

=over 4

=item B<init> I<params>

Makes it possible to change the parameters of this tracker on the fly.
Use with caution. See B<new> method for the description of the I<params> hash.

=back

=head1 OVERRIDDEN METHODS

=over 4

=item B<new> I<module> I<params>

It is possible to specify additional parameters that this tracker understands.

    ConfigType     - string "hash" (default) or "array"
    ModuleName     - module to query, the default is $module->name
    LineFilter     - "asis", "spacefree" (default), "lowerkeys", "upperkeys"
    DefaultConfig  - the config hash/array of config to initially use

=item B<data> [I<line-key-or-number>]

Returns either array ref of configuration hash refs, or one hash ref if
I<line-key-or-number> is given.

Returns I<undef> if the configuration line for I<line-key-or-number> is not
defined.

=item B<dump> [I<line-key-or-number>]

Works similarly to B<data>, but returns one or many debug lines.

Returns an empty string if no module configuration is defined.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
