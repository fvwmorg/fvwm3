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

package FVWM::Module::Gtk2;

use 5.004;
use strict;

use FVWM::Module::Toolkit qw(base Gtk2);

sub eventLoop ($) {
	my $self = shift;

	$self->eventLoopPrepared(@_);
	Gtk2::Gdk->input_add(
		$self->{istream}->fileno, ['read'],
		sub ($$$) {
			#my ($socket, $fd, $flags) = @_;
			#return 0 unless $flags->{'read'};
			unless ($self->processPacket($self->readPacket)) {
				Gtk2->main_quit;
			}
			$self->eventLoopPrepared(@_);
			return 1;
		}
	);
	Gtk2->main;
	$self->eventLoopFinished(@_);
}

sub showError ($$;$) {
	my $self = shift;
	my $error = shift;
	my $title = shift || ($self->name . " Error");

	my $dialog = new Gtk2::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk2::Label $error;
	$dialog->vbox->pack_start($label, 0, 1, 10);

	my $button = new Gtk2::Button "Close";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { $dialog->destroy; });

	$button = new Gtk2::Button "Close All Errors";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked",
		sub { $self->send("All ('$title') Close"); });

	$button = new Gtk2::Button "Exit Module";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { Gtk2->main_quit; });

	$dialog->show_all;
}

sub showMessage ($$;$) {
	my $self = shift;
	my $message = shift;
	my $title = shift || ($self->name . " Message");

	my $dialog = new Gtk2::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk2::Label $message;
	$dialog->vbox->pack_start($label, 0, 1, 10);

	my $button = new Gtk2::Button "Close";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { $dialog->destroy; });

	$dialog->show_all;
}

1;

__END__

=head1 NAME

FVWM::Module::Gtk2 - FVWM::Module with the GTK+ v2 widget library attached

=head1 SYNOPSIS

  use lib `fvwm-perllib dir`;
  use FVWM::Module::Gtk2;
  use Gtk2;

  my $module = new FVWM::Module::Gtk2;

  init Gtk2;
  my $window = new Gtk2::Window -toplevel;;
  my $label = new Gtk2::Label "Hello, world";
  $window->add($label);
  $window->show_all;

  $module->addHandler(M_CONFIGURE_WINDOW, \&configure_toplevel);
  $module->addHandler(M_CONFIG_INFO, \&some_other_sub);

  $module->eventLoop;

=head1 DESCRIPTION

The B<FVWM::Module::Gtk2> package is a sub-class of B<FVWM::Module> that
overloads the methods B<eventLoop>, B<showError> and B<showMessage>
to manage GTK+ version 2 objects as well.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only methods that are not available in B<FVWM::Module> or the overloaded ones
are covered here:

=over 8

=item B<eventLoop>

From outward appearances, this methods operates just as the parent
B<eventLoop> does. It is worth mentioning, however, that this version
enters into the B<Gtk2>->B<main> subroutine, ostensibly not to return.

=item B<showError> I<msg> [I<title>]

This method creates a dialog box using the GTK+ widgets. The dialog has
three buttons labeled "Close", "Close All Errors" and "Exit Module".
Selecting the "Close" button closes the dialog. "Close All Errors" closes
all error dialogs that may be open on the screen at that time.
"Exit Module" terminates your entire module.

Good for diagnostics of a GTK+ based module.

=item B<showMessage> I<msg> [I<title>]

Creates a message window with one "Close" button.

Good for debugging a GTK+ based module.

=head1 BUGS

Awaiting for your reporting.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 THANKS TO

gtk2-perl.sf.net team for Gtk2-Perl extension.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module> and L<Gtk2>.

See also L<FVWM::Module::Gtk> for use with GTK+ version 1.

=cut
