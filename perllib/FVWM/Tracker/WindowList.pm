# Copyright (c) 2004 Mikhael Goikhman, Scott Smedley
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package FVWM::Tracker::WindowList;

use strict;

use FVWM::Tracker qw(base);

my $windowEvents = M_ADD_WINDOW | M_CONFIGURE_WINDOW | M_DESTROY_WINDOW |
	M_ICONIFY | M_DEICONIFY;
my $nameEvents = M_RES_NAME | M_RES_CLASS | M_WINDOW_NAME | M_VISIBLE_NAME |
	M_ICON_NAME;
my $nameXEvents = MX_VISIBLE_ICON_NAME;
my $stackEvents = M_RESTACK | M_RAISE_WINDOW | M_LOWER_WINDOW;
my $iconEvents = M_ICON_LOCATION | M_ICON_FILE | M_DEFAULTICON | M_MINI_ICON;

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

sub addRequestedInfoHandlers ($$) {
	my $self = shift;
	my $handler = shift;

	my $useWInfo = 1;
	my $useNames = 1;
	my $useStack = 0;
	my $useIcons = 0;
	foreach (@{$self->{options}}) {  
		/^(\!?)winfo$/ and $useWInfo = $1 ne '!';
		/^(\!?)names$/ and $useNames = $1 ne '!';
		/^(\!?)stack$/ and $useStack = $1 ne '!';
		/^(\!?)icons$/ and $useIcons = $1 ne '!';
	}
	my $mask  = 0;
	$mask |= $windowEvents if $useWInfo;
	$mask |= $nameEvents if $useNames;
	$mask |= $stackEvents if $useStack;  
	$mask |= $iconEvents if $useIcons;

	my $xmask = 0;
	$xmask |= $nameXEvents if $useNames;

	$self->addHandler($mask, $handler) if $mask;
	$self->addHandler($xmask, $handler) if $xmask;
	$self->addHandler(M_NEW_PAGE | M_NEW_DESK, sub {
		$self->handlerPageInfo($_[1]);
	}) if $useWInfo;
}

sub start ($) {
	my $self = shift;

	$self->{data} = {};

	$self->addRequestedInfoHandlers(sub {
		my $event = $_[1];
		$self->calculateInternals($event);
	});

	$self->requestWindowListEvents;
	my $result = $self->SUPER::start;
	$self->deleteHandlers;

	$self->addRequestedInfoHandlers(sub {
		my $event = $_[1];
		my ($winId, $oldHash) = $self->calculateInternals($event);
		return unless defined $winId;
		my $type = $event->type();
		my $observable = undef;

		if ($self->{module}->isEventExtended($type)) {
			if ($type & $nameXEvents) {
				$observable = "window name updated";
			}
		} elsif ($type & M_ADD_WINDOW) {
			$observable = "window added";
		} elsif ($type & M_CONFIGURE_WINDOW) {
			$observable = "window properties updated";

			# this observable is too broad, try to narrow
			if ($oldHash) {
				my $win = $self->{data}->{$winId};

				$self->notify("window resized", $winId, $oldHash)
					if $win->{width} != $oldHash->{width}
					|| $win->{height} != $oldHash->{height};

				$self->notify("window moved", $winId, $oldHash)
					if $win->{desk} != $oldHash->{desk}
					|| $win->{x} != $oldHash->{x}
					|| $win->{y} != $oldHash->{y};
			}
		} elsif ($type & M_DESTROY_WINDOW) {
			$observable = "window deleted";
		} elsif ($type & M_ICONIFY) {
			$observable = "window iconified";
		} elsif ($type & M_DEICONIFY) {
			$observable = "window deiconified";
		} elsif ($type & $nameEvents) {
			$observable = "window name updated";
		} elsif ($type & $stackEvents) {
			$observable = "window stack updated";
		} elsif ($type & $iconEvents) {
			$observable = "window icon updated";
		}
		$self->notify($observable, $winId, $oldHash) if $observable;
	});

	return $result;
}

sub calculateInternals ($$) {
	my $self = shift;
	my $event = shift;
	my $args = $event->args;
	my $data = $self->{data};
	my $winId = $args->{win_id};

	my $oldHash = undef;
	$oldHash = { %{$data->{$winId}} } if defined $data->{$winId};

	my $window = $data->{$winId} ||=
		bless { id => $winId, iconified => 0, _tracker => $self },
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
	if (!$self->{module}->isEventExtended($type)) {
		if ($type & M_DEICONIFY) {
			$window->{iconified} = 0;
		} elsif ($type & M_ICONIFY) {
			$window->{iconified} = 1;
		} elsif ($type & M_DESTROY_WINDOW) {
			delete $data->{$winId};
		}
	}

	return wantarray ? ($winId, $oldHash) : $winId;
}

sub handlerPageInfo ($$) {
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

sub pageInfo ($;$) {
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
			my $page = $self->{_tracker}->pageInfo;
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
	my $string = "Window $_\n";
	my $window = $data->{$_};
	foreach my $prop (sort keys %$window) {
		next if $prop =~ /^_/;
		$string .= "\t$prop:\t[$window->{$prop}]\n";
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
    while (my ($winId, $window) = each %$data) {
        next unless $window->match("CurrentPage, Iconified");
        $module->send("Iconify off", $winId);
    }

Default $options string is: "!stack !icons names winfo"

=head1 OVERRIDDEN METHODS

=over 4

=item B<new> I<module> I<params>

It is possible the kind of window list.

To be written.

=item B<data> [I<window-id>]

Returns array ref of window hash refs. or one window hash ref if
I<window-id> is given.

=item B<dump> [I<window-id>]

Works similarly to B<data>, but returns debug lines for one or all windows.

=back

=head1 METHODS

=over 4

=item B<pageInfo> [I<field>]

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
