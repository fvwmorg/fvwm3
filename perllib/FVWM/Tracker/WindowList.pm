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

	# Adding MX_VISIBLE_ICON_NAME to $nameEvents does not work.
	my $xmask = 0;
	$xmask |= MX_VISIBLE_ICON_NAME if $useNames;

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
		if ($type & M_ADD_WINDOW) {
			$self->notify("window added", $winId);
		} elsif ($type & M_CONFIGURE_WINDOW) {
			$self->notify("window properties updated", $winId, $oldHash);
			# The "window properties updated" observable is very broad &
			# occurs as a result of many different operations - here we
			# try & determine if some common operations occur & invoke more
			# specific observables. This may reduce the amount of parsing
			# external modules have to do.
			my $p = $self->{data}->{$winId};
			if ($p->{frame_width} != $oldHash->{frame_width} ||
				$p->{frame_height} != $oldHash->{frame_height})
			{
				$self->notify("window resized", $winId, $oldHash);
			}
			elsif ($p->{desk} != $oldHash->{desk} ||
				$p->{x} != $oldHash->{x} ||
				$p->{y} != $oldHash->{y})
			{
				$self->notify("window moved", $winId, $oldHash);
			}
			# We can easily add other specific observables here as required.
		} elsif ($type & M_DESTROY_WINDOW) {
			$self->notify("window deleted", $winId, $oldHash);
		} elsif ($type & M_ICONIFY) {
			$self->notify("window iconified", $winId, $oldHash);
		} elsif ($type & M_DEICONIFY) {
			$self->notify("window deiconified", $winId, $oldHash);
		} elsif ($type & $nameEvents || $type & MX_VISIBLE_ICON_NAME) {
			$self->notify("window name updated", $winId, $oldHash);
		} elsif ($type & $stackEvents) {
			$self->notify("window stack updated", $winId, $oldHash);
		} elsif ($type & $iconEvents) {
			$self->notify("window icon updated", $winId, $oldHash);
		}
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
	if (defined $data->{$winId}) {
		# make a copy of the hash cos we are about to modify it.
		my %tmp = %{$data->{$winId}};
		$oldHash = \%tmp;
	}

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

	# Please leave this (commented-out) code here. I often enable it
	# when debugging -- SS.
#	print(STDERR "Got " . $event->name() . " event\n");
#	foreach (sort(keys %{$args})) {
#		print(STDERR "    $_=" . $args->{$_} . "\n");
#	}

	@{$data->{$winId}}{keys %{$args}} = values %{$args};

	if (defined $args->{frame_x} && defined $self->{page_info}) {
		# frame_x & frame_y are _relative_ coords of the window to the
		# current page - calculate the _absolute_ coords - x & y.
		my $pW = $data->{$winId};
		$pW->{x} = $self->pageInfo('vp_x') + $args->{frame_x};
		$pW->{y} = $self->pageInfo('vp_y') + $args->{frame_y};
		$pW->{page_x} = int($pW->{x} / $self->pageInfo('vp_width'));
		$pW->{page_y} = int($pW->{y} / $self->pageInfo('vp_height'));
	}

	my $type = $event->type();
	if ($type & M_ADD_WINDOW || $type & M_DEICONIFY) {
		$data->{$winId}->{iconified} = 0;
	} elsif ($type & M_ICONIFY) {
		$data->{$winId}->{iconified} = 1;
	} elsif ($type & M_DESTROY_WINDOW) {
		delete $data->{$winId};
	}

	return wantarray ? ($winId, $oldHash) : $winId;
}

sub handlerPageInfo ($$) {
	my $self = shift;
	my $event = shift;
	my $args = $event->args;

	@{$self->{page_info}}{keys %{$args}} = values %{$args};

	if ($event->type() & M_NEW_PAGE) {
		$self->{page_info}->{page_x} = int($args->{vp_x} / $args->{vp_width});
		$self->{page_info}->{page_y} = int($args->{vp_y} / $args->{vp_height});
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
		my $winData = $data->{$_};
		$string .= "Window $_\n";
		while (my ($prop, $value) = %$winData) {
			$string .= "\t$prop:\t[$value]\n";
		}
	}
	return $string;
}

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
 
Using B<FVWM::Module> $module object (preferably):

	my $windowsTracker = $module->track("WindowList", $options);
	my $windows = $windowsTracker->data;
	my $windowSizeX = $windows->{$winId}->{'x'};

or:

	my $windowsTracker = $module->track("WindowList", $options);
	my $windowSizeX = $windowsTracker->data($winId)->{'x'};

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
