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

package FVWM::Module::Tk;

require 5.003;

use strict;
use vars qw($VERSION @ISA @EXPORT);
use Exporter;

use FVWM::Module;
use Tk;
use Tk::Dialog;

@ISA = qw(FVWM::Module Exporter);
@EXPORT = @FVWM::Module::EXPORT;

$VERSION = "1.1";

sub new ($$@) {
	my $class = shift;
	$class = ref($class) || $class;
	my $top = shift;
	my %params = @_;

	my $self = $class->SUPER::new(%params);
	bless $self, $class;

	$self->internalDie("No Tk::Toplevel given in constructor")
		unless UNIVERSAL::isa($top, "Tk::Toplevel");

	$self->{topLevel} = $top;
	$self->{winId} = $top->id;

	return $self;
}

sub eventLoop ($) {
	my $self = shift;

	my $top = $self->{topLevel};
	$top->fileevent($self->{istream},
		readable => sub {
			unless ($self->processPacket($self->readPacket)) {
				$self->disconnect;
				$top->destroy;
			}
		}
	);
	MainLoop;
}

sub openErrorDialog ($$;$) {
	my $self = shift;
	my $error = shift;
	my $title = shift || ($self->name . " Error");

	my $top = $self->{topLevel};

	my $dialog = $top->Dialog(
		-title => $title,
		-bitmap => 'error',
		-default_button => 'Close',
		-text => $error,
		-buttons => ['Close', 'Close All Errors', 'Exit Module']
	);
	my $btn = $dialog->Show;

	$self->terminate if $btn eq 'Exit Module';
	$self->send("All ('$title') Close") if $btn eq 'Close All Errors';
}

sub addDefaultErrorHandler ($) {
	my $self = shift;

	$self->addHandler(M_ERROR, sub {
		my ($self, $event) = @_;
		$self->openErrorDialog($event->_text, "FVWM Error");
	});
}

sub topLevel ($) {
	return shift->{topLevel};
}

sub winId ($) {
	return shift->{winId};
}

1;

__END__

=head1 NAME

FVWM::Module::Tk - FVWM::Module with the Tk widget library attached

=head1 SYNOPSIS

  use Tk;
  use lib `fvwm-perllib dir`;
  use FVWM::Module::Tk;

  my $top = new MainWindow;
  my $module = new FVWM::Module::Tk $top;

  $module->addHandler(M_CONFIGURE_WINDOW, \&configure_toplevel);
  $module->addHandler(M_CONFIG_INFO, \&some_other_sub);

  $module->eventLoop;

=head1 DESCRIPTION

The B<FVWM::Module::Tk> package is a sub-class of B<FVWM::Module> that
overloads the methods B<new>, B<eventLoop> and B<addDefaultErrorHandler>
to manage Tk objects as well. It also adds new methods B<openErrorDialog>,
B<topLevel> and B<winId>.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only those methods that are not available in B<FVWM::Module>, or are overloaded
are covered here:

=over 8

=item B<new>

$module = new FVWM::Module::Tk $top, %params

Create and return an object of the B<FVWM::Module::Tk> class. The return value is
the blessed reference. This B<new> method is identical to the parent class
method, with the exception that a Tk top-level of some sort (MainWindow,
TopLevel, Frame, etc.) must be passed before the hash of options. The options
themselves are as specified in L<FVWM::Module>.

=item B<eventLoop>

From outward appearances, this methods operates just as the parent
B<eventLoop> does. It is worth mentioning, however, that this version
enters into the Tk B<MainLoop> subroutine, ostensibly not to return.

=item B<openErrorDialog> I<error> [I<title>]

This method creates a dialog box using the Tk widgets. The dialog has
three buttons labeled "Close", "Close All Errors" and "Exit Module".
Selecting the "Close" button closes the dialog. "Close All Errors" closes
all error dialogs that may be open on the screen at that time.
"Exit Module" terminates your entire module.

Good for debugging a Tk based module.

=item B<addDefaultErrorHandler>

This methods adds a M_ERROR handler to automatically notify you that an error
has been reported by FVWM. The M_ERROR handler then calls C<openErrorDialog()>
with the received error text as a parameter to show it in a window.

=item B<topLevel>

Returns the Tk toplevel that this object was created with.

=item B<winId>

A shortcut for $self->topLevel->id, exists for efficiency reasons.

=back

=head1 BUGS

Would not surprise me in the least.

=head1 CAVEATS

In keeping with the UNIX philosophy, B<FVWM::Module> does not keep you from
doing stupid things, as that would also keep you from doing clever things.
What this means is that there are several areas with which you can hang your
module or even royally confuse your running I<fvwm> process. This is due to
flexibility, not bugs.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

Randy J. Ray <randy@byz.org>, author of the old classes
B<X11::Fvwm> and B<X11::Fvwm::Tk>.

=head1 THANKS TO

Nick Ing-Simmons <Nick.Ing-Simmons@tiuk.ti.com> for Tk Perl extension.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module> and L<Tk>.

=cut
