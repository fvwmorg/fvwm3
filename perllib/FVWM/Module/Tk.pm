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
				$self->invokeHandler(ON_EXIT);
				$self->destroy;
				$top->destroy;
			}
		}
	);
	MainLoop;
}

sub addDefaultErrorHandler ($) {
	my $self = shift;

	$self->addHandler(M_ERROR, sub {
		my ($self, $type, @args) = @_;

		my $top = $self->{topLevel};

		my $err = $top->Dialog(
			-title => 'FVWM Error',
			-bitmap => 'error',
			-default_button => 'Dismiss',
			-text => $args[3],
			-buttons => ['Dismiss', 'Exit']
		);
		my $btn = $err->Show(-global);

		die "quit" if $btn eq 'Exit';
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
  use FVWM::Module::Tk;

  my $top = new MainWindow;
  my $module = new FVWM::Module::Tk $top;

  $module->addHandler(M_CONFIGURE_WINDOW, \&configure_toplevel);
  $module->addHandler(M_CONFIG_INFO, \&some_other_sub);

  $module->eventLoop;

=head1 DESCRIPTION

The B<FVWM::Module::Tk> package is a sub-class of B<FVWM::Module> that
overloads the methods B<new>, B<eventLoop> and B<addDefaultErrorHandler>
to manage Tk objects as well.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only those methods that are not available in B<FVWM::Module>, or are overloaded
are covered here:

=over 8

=item new

$self = new FVWM::Module::Tk $top, %params

Create and return an object of the B<FVWM::Module::Tk> class. The return value is
the blessed reference. This B<new> method is identical to the parent class
method, with the exception that a Tk top-level of some sort (MainWindow,
TopLevel, Frame, etc.) must be passed before the hash of options. The options
themselves are as specified in L<FVWM::Module>.

=item eventLoop 

From outward appearances, this methods operates just as the parent
B<eventLoop> does. It is worth mentioning, however, that this version
enters into the Tk B<MainLoop> subroutine, ostensibly not to return.

=item addDefaultErrorHandler

This methods adds a M_ERROR handler that creates a dialog box using the Tk
widgets to notify you that an error has been reported by FVWM. The dialog
has two buttons, labelled "Exit" and "Dismiss". Selecting the "Dismiss"
button closes the dialog, and allows your application to continue (the
handler grabs the pointer until the dialog is closed). Choosing the "Exit"
button causes the termination of the running module. After exiting that
window, the application will continue as if the "Dismiss" button had been
pressed.

=item topLevel

Returns the Tk toplevel that this object was created with.

=item winId

A shortcut for C<$self->topLevel->id>, exists for efficiency reasons.

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

Randy J. Ray <randy@byz.org>, original B<X11::Fvwm> and B<X11::Fvwm::Tk>.

=head1 THANKS TO

Nick Ing-Simmons <Nick.Ing-Simmons@tiuk.ti.com> for Tk Perl extension.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module> and L<Tk>.

=cut
