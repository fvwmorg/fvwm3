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

use strict;

package FVWM::TrackerFactory;

=head1 To be added to FVWM::Module API

Tracker is an object that automatically listens to the corresponding
fvwm events and gathers an information dynamically until it is unloaded.

It is not clear yet whether everything should be assynchronous (non-blocking)
or some initial step should be synchronous (blocking).

  my $tracker = $module->loadTracker("TrackerName", @params);
  my $initialInfo = $tracker->info;
  $tracker->setHandler("HighLevelTrackerEvent", sub { shift->info });
  $tracker->setHandler("HighLevelTrackerEvent2", sub { shift->unload });

Grabber is just a one time Tracker, it should not be unloaded,
it unloads itself.

  my $grabber = $module->runGrabber("GrabberName", @params);
  my $info = $grabber->info;

=cut

my $trackers = {};
my $grabbers = {};

# for registering user defined trackers, as well as the ones in this file
sub registerTracker ($$$) {
	$trackers->{$_[1]} = $_[2];
}

sub registerGrabber ($$$) {
	$grabbers->{$_[1]} = $_[2];
}

# ----------------------------------------------------------------------------

package FVWM::Tracker;

use FVWM::Constants;

sub new ($$) {
	my $class = shift;
	my $module = shift;
	die "FVWM::Tracker: no module given in constructor"
		unless UNIVERSAL::isa($module, "FVWM::Module");
	my $self = {
		module => $module,
		info => undef,
	};
	bless $self, $class;

	my $callback = sub { $self->processEvent(@_); };
	$self->{handler1} = $module->addHandler(
		$self->additionalMask, $callback
	);
	$self->{handler2} = $module->addHandler(
		M_EXTENDED_MSG | $self->additionalXMask, $callback
	);
	return $self;
}

sub additionalMask ($) { 0 }
sub additionalXMask ($) { 0 }

sub processEvent ($$$) {
	my $self = shift;
	my $module = shift;
	my $event = shift;
	$self->{module}->internalDie("abstract method FVWM::Tracker::processEvent called");
}

sub unload ($) {
	my $self = shift;
	return unless defined $self->{module};
	$self->{module}->removeHandler($self->{handler1});
	$self->{module}->removeHandler($self->{handler2});
	$self->{module} = undef;
}

sub DESTROY ($) {
	my $self = shift;
	$self->unload;
}

# ----------------------------------------------------------------------------

package FVWM::Tracker::WindowList;

use base 'FVWM::Tracker';
use FVWM::Constants;

# options: "stack", "icons", "wininfo"
sub new ($$@) {
	my $class = shift;
	my $module = shift;
	my @params = @_;

	my $self = $class->SUPER::new($module);
	$self->{options} = ref($params[0]) eq 'ARRAY'? shift @params: [];
	$self->{info} = {};  # may be a hash of window structures
	return $self;
}

sub additionalMask ($) {
	my $self = shift;
	my $useWInfo = 1;
	my $useNames = 1;
	my $useStack = 0;
	my $useIcons = 0;
	foreach my $option (@{$self->{options}}) {
		/^(\!?)winfo$/ and $useWInfo = $1 ne '!';
		/^(\!?)names$/ and $useNames = $1 ne '!';
		/^(\!?)stack$/ and $useStack = $1 ne '!';
		/^(\!?)icons$/ and $useIcons = $1 ne '!';
	}
	my $mask = M_DESTROY_WINDOW;
	$mask |= M_ADD_WINDOW | M_CONFIGURE_WINDOW if $useWInfo;
	$mask |= M_RES_NAME | M_RES_CLASS | M_WINDOW_NAME | M_VISIBLE_NAME | M_ICON_NAME | MX_VISIBLE_ICON_NAME
		if $useNames;
	$mask |= M_RESTACK | M_RAISE_WINDOW | M_LOWER_WINDOW if $useStack;
	$mask |= M_ICON_LOCATION | M_ICON_FILE | M_DEFAULTICON | M_MINI_ICON
		if $useIcons;
	return $mask;
}

sub additionalXMask ($) {
	my $self = shift;
	my $useNames = 1;
	foreach my $option (@{$self->{options}}) {
		/^(\!?)names$/ and $useNames = $1 ne '!';
	}
	my $mask = 0;
	$mask |= MX_VISIBLE_ICON_NAME if $useNames;
	return $mask;
}

sub processHandler ($$$) {
	my $self = shift;
	my $module = shift;
	my $event = shift;
	# TODO
}

FVWM::TrackerFactory->registerTracker('WindowList', 'FVWM::Tracker::WindowList');

# ----------------------------------------------------------------------------

package FVWM::Tracker::Colorsets;
use base 'FVWM::Tracker';

FVWM::TrackerFactory->registerTracker('Colorsets', 'FVWM::Tracker::Colorsets');

# ----------------------------------------------------------------------------

package FVWM::Tracker::GlobalData;
use base 'FVWM::Tracker';

FVWM::TrackerFactory->registerTracker('GlobalData', 'FVWM::Tracker::GlobalData');

# ----------------------------------------------------------------------------

package FVWM::Tracker::ConfigInfo;
use base 'FVWM::Tracker';

FVWM::TrackerFactory->registerTracker('ConfigInfo', 'FVWM::Tracker::ConfigInfo');

# ----------------------------------------------------------------------------

package FVWM::Tracker::PageInfo;
use base 'FVWM::Tracker';

FVWM::TrackerFactory->registerTracker('PageInfo', 'FVWM::Tracker::PageInfo');

# ----------------------------------------------------------------------------

package FVWM::Grabber;
use base 'FVWM::Tracker';

# think about a configurable mechanism of auto unloading

# ----------------------------------------------------------------------------

package FVWM::Grabber::ConfigInfo;
use base 'FVWM::Grabber';

FVWM::TrackerFactory->registerGrabber('ConfigInfo', 'FVWM::Tracker::ConfigInfo');

1;
