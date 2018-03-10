# Copyright (c) 2004-2009 Mikhael Goikhman, Scott Smedley
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

package FVWM::Tracker::WindowList;

use strict;

use FVWM::Tracker qw(base);

my $window_events = M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW |
	M_ICONIFY | M_DEICONIFY;
my $name_events = M_RES_NAME | M_RES_CLASS | M_WINDOW_NAME | M_VISIBLE_NAME |
	M_ICON_NAME;
my $name_xevents = MX_VISIBLE_ICON_NAME;
my $stack_events = M_RESTACK | M_RAISE_WINDOW | M_LOWER_WINDOW;
my $icon_events = M_ICON_LOCATION | M_ICON_FILE | M_DEFAULTICON | M_MINI_ICON;

sub observables ($) {
	return [
		"window added",
		"window deleted",
		"window properties updated",
		"window moved",
		"window resized",
		"window iconified",
		"window deiconified",
		"window name updated",
		"window stack updated",
		"window icon updated",
	];
}

sub new ($$;$) {
	my $class = shift;
	my $module = shift;
	my @options = split(/ /, shift || "");

	my $self = $class->FVWM::Tracker::new($module);

	$self->{options} = [ @options ];
	return $self;
}

sub add_requested_info_handlers ($$) {
	my $self = shift;
	my $handler = shift;

	my $use_winfo = 1;
	my $use_names = 1;
	my $use_stack = 0;
	my $use_icons = 0;
	foreach (@{$self->{options}}) {  
		/^(\!?)winfo$/ and $use_winfo = $1 ne '!';
		/^(\!?)names$/ and $use_names = $1 ne '!';
		/^(\!?)stack$/ and $use_stack = $1 ne '!';
		/^(\!?)icons$/ and $use_icons = $1 ne '!';
	}
	my $mask  = 0;
	$mask |= $window_events if $use_winfo;
	$mask |= $name_events   if $use_names;
	$mask |= $stack_events  if $use_stack;  
	$mask |= $icon_events   if $use_icons;

	my $xmask = 0;
	$xmask |= $name_xevents if $use_names;

	$self->add_handler($mask, $handler) if $mask;
	$self->add_handler($xmask, $handler) if $xmask;
	$self->add_handler(M_NEW_PAGE | M_NEW_DESK, sub {
		$self->handler_page_info($_[1]);
	}) if $use_winfo;
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};

	$self->add_requested_info_handlers(sub {
		my $event = $_[1];
		$self->calculate_internals($event);
	});

	$self->request_windowlist_events;
	my $result = $self->SUPER::start;
	$self->delete_handlers;

	$self->add_requested_info_handlers(sub {
		my $event = $_[1];
		my ($win_id, $old_hash) = $self->calculate_internals($event);
		return unless defined $win_id;
		my $type = $event->type();
		my $observable = undef;

		if ($self->{module}->is_event_extended($type)) {
			if ($type & $name_xevents) {
				$observable = "window name updated";
			}
		} elsif ($type & M_ADD_WINDOW) {
			$observable = "window added";
		} elsif ($type & M_CONFIGURE_WINDOW) {
			$observable = "window properties updated";

			# this observable is too broad, try to narrow
			if ($old_hash) {
				my $win = $self->{data}->{$win_id};

				$self->notify("window resized", $win_id, $old_hash)
					if $win->{width} != $old_hash->{width}
					|| $win->{height} != $old_hash->{height};

				$self->notify("window moved", $win_id, $old_hash)
					if $win->{desk} != $old_hash->{desk}
					|| $win->{x} != $old_hash->{x}
					|| $win->{y} != $old_hash->{y};
			}
		} elsif ($type & M_DESTROY_WINDOW) {
			$observable = "window deleted";
		} elsif ($type & M_ICONIFY) {
			$observable = "window iconified";
		} elsif ($type & M_DEICONIFY) {
			$observable = "window deiconified";
		} elsif ($type & $name_events) {
			$observable = "window name updated";
		} elsif ($type & $stack_events) {
			$observable = "window stack updated";
		} elsif ($type & $icon_events) {
			$observable = "window icon updated";
		}
		$self->notify($observable, $win_id, $old_hash) if $observable;
	});

	return $result;
}

sub calculate_internals ($$) {
	my $self = shift;
	my $event = shift;
	my $args = $event->args;
	my $data = $self->{data};
	my $win_id = $args->{win_id};

	my $old_hash = undef;
	$old_hash = { %{$data->{$win_id}} } if defined $data->{$win_id};

	my $window = $data->{$win_id} ||=
		bless { id => $win_id, iconified => 0, _tracker => $self },
		"FVWM::Window";

	# There are some fields that are not unique to all events. To ensure
	# we don't clobber them, we rename some fields. For example, the 'name'
	# field of M_MINI_ICON events is renamed to 'mini_icon_name'.
	foreach ('name', 'x', 'y', 'width', 'height') {
		if (defined $args->{$_}) {
			(my $name = lc($event->name())) =~ s/^.*?_//;
			$name .= "_$_" if ($name !~ /$_$/);
			$args->{$name} = $args->{$_};
			delete $args->{$_};
		}
	}

	#print $event->dump;

	$args->{name} = delete $args->{window_name} if exists $args->{window_name};
	@$window{keys %$args} = values %$args;

	if (defined $args->{frame_x}) {
		# frame_x & frame_y are _relative_ coords of the window to the
		# current page - calculate the _absolute_ coords - x & y.
		$window->{x} = delete $window->{frame_x};
		$window->{y} = delete $window->{frame_y};
		$window->{width}  = delete $window->{frame_width};
		$window->{height} = delete $window->{frame_height};
		my $page = $self->{page_info};
		if (defined $page) {
			$window->{X} = $page->{vp_x} + $window->{x};
			$window->{Y} = $page->{vp_y} + $window->{y};
			$window->{page_nx} = int($window->{X} / $page->{vp_width});
			$window->{page_ny} = int($window->{Y} / $page->{vp_height});
		}
	}

	my $type = $event->type();
	if (!$self->{module}->is_event_extended($type)) {
		if ($type & M_DEICONIFY) {
			$window->{iconified} = 0;
		} elsif ($type & M_ICONIFY) {
			$window->{iconified} = 1;
		} elsif ($type & M_DESTROY_WINDOW) {
			delete $data->{$win_id};
		}
	}

	return wantarray ? ($win_id, $old_hash) : $win_id;
}

sub handler_page_info ($$) {
	my $self = shift;
	my $event = shift;
	my $args = $event->args;
	my $data = $self->{page_info} ||= {};

	@$data{keys %$args} = values %$args;
	if ($event->type & M_NEW_PAGE) {
		$data->{page_nx} = int($data->{vp_x} / $data->{vp_width});
		$data->{page_ny} = int($data->{vp_y} / $data->{vp_height});
	}
}

sub page_info ($;$) {
	my $self = shift;
	my $id = shift;
	my $data = $self->{page_info};
	return $data unless defined $id;
	return $data->{$id};
}

sub data ($;$) { 
	my $self = shift;
	my $id = shift;
	my $data = $self->{data};
	return $data unless defined $id;
	return $data->{$id};
}

sub dump ($;$) { 
	my $self = shift;
	my $id = shift;
	my $data = $self->{data};
	my @ids = defined $id? ($id): sort { $a <=> $b } keys %$data;

	my $string = "";
	foreach (@ids) {
		my $window = $data->{$_};
		$string .= $window->dump;
	}
	return $string;
}

sub windows ($) {
	my $self = shift;
	my @windows = values %{$self->data};
	return wantarray? @windows: \@windows;
}

# ----------------------------------------------------------------------------

package FVWM::Window;

sub match ($$) {
	my $self = shift;
	my $condition = shift;
	my @conditions = split(/[,\s]+/, $condition);

	my $match = 1;
	foreach (@conditions) {
		my $opposite = s/^!//;
		if (/^iconified$/i) {
			return 0 unless $opposite ^ $self->{iconified};
		} elsif (/^current(page|desk)$/i) {
			my $page = $self->{_tracker}->page_info;
			return 0 unless $opposite ^ ($self->{desk} == $page->{desk_n});
			next if lc($1) eq "desk";
			return 0 unless $opposite ^ (
				$self->{x} + $self->{width} > $page->{vp_x} &&
				$self->{x} < $page->{vp_x} + $page->{vp_width} &&
				$self->{y} + $self->{height} > $page->{vp_y} &&
				$self->{y} < $page->{vp_y} + $page->{vp_height}
			);
		}
	}
	return $match;
}

sub dump ($) {
	my $self = shift;
	my $id = $self->{win_id};
	my $string = "Window $id\n";
	foreach my $prop (sort keys %$self) {
		next if $prop =~ /^_/;
		$string .= "\t$prop:\t[$self->{$prop}]\n";
	}
	return $string;
}

# ----------------------------------------------------------------------------

1;

__END__

=head1 DESCRIPTION

This is a subclass of B<FVWM::Tracker> that enables to read the window
information.

This tracker defines the following observables:

    "window added",
    "window deleted",
    "window properties updated",
    "window moved",
    "window resized",
    "window iconified",
    "window deiconified",
    "window name updated",
    "window stack updated",
    "window icon updated",

=head1 SYNOPSYS
 
Using B<FVWM::Module> $module object:

    my $tracker = $module->track("WindowList");
    my @windows = $tracker->windows;
    foreach my $window ($tracker->windows) {
        print "+$window->{x}+$window->{y}, $window->{name}\n";
    }

or:

    my $tracker = $module->track("WindowList", "winfo");
    my $x = $tracker->data("0x230002a")->{x};

or:

    my $tracker = $module->track("WindowList", $options);
    my $data = $tracker->data;
    while (my ($win_id, $window) = each %$data) {
        next unless $window->match("CurrentPage, Iconified");
        $module->send("Iconify off", $win_id);
    }

Default $options string is: "!stack !icons names winfo"

=head1 OVERRIDDEN METHODS

=over 4

=item B<new> I<module> I<params>

It is possible the kind of window list.

To be written.

=item B<data> [I<window-id>]

Returns hash ref of window hash refs. or one window hash ref if
I<window-id> is given.

=item B<dump> [I<window-id>]

Works similarly to B<data>, but returns debug lines for one or all windows.

=back

=head1 METHODS

=over 4

=item B<page_info> [I<field>]

Returns hash ref of page/desk info, or actual hash value using B<field> as a key (if specified).

=back

=head1 AUTHORS

=over 4

=item Mikhael Goikhman <migo@homemail.com>

=item Scott Smedley

=back

=head1 SEE ALSO

For more information, see L<FVWM::Module> and L<FVWM::Tracker>.

=cut
