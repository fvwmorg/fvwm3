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

package FVWM::Module::Gtk;

use 5.004;
use strict;

use FVWM::Module::Toolkit qw(base Gtk);

sub eventLoop ($) {
	my $self = shift;

	Gtk::Gdk->input_add(
		$self->{istream}->fileno, ['read'],
		sub ($$$) {
			#my ($socket, $fd, $flags) = @_;
			#return 0 unless $flags->{'read'};
			unless ($self->processPacket($self->readPacket)) {
				Gtk->main_quit;
			}
			return 1;
		}
	);
	Gtk->main;
	$self->debug("exited Gtk event loop", 3);
	$self->disconnect;
}

sub showError ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Error");

	my $dialog = new Gtk::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk::Label $msg;
	$dialog->vbox->pack_start($label, 0, 1, 10);

	my $button = new Gtk::Button "Close";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { $dialog->destroy; });

	$button = new Gtk::Button "Close All Errors";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked",
		sub { $self->send("All ('$title') Close"); });

	$button = new Gtk::Button "Exit Module";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { Gtk->main_quit; });

	$dialog->show_all;
}

sub showMessage ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Message");

	my $dialog = new Gtk::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk::Label $msg;
	$dialog->vbox->pack_start($label, 0, 1, 10);

	my $button = new Gtk::Button "Close";
	$dialog->action_area->pack_start($button, 1, 1, 0);
	$button->signal_connect("clicked", sub { $dialog->destroy; });

	$dialog->show_all;
}

sub showDebug ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Debug");

	my $dialog = $self->{gtkDebugDialog};

	if (!$dialog) {
		$dialog = new Gtk::Dialog;
		$dialog->set_title($title);
		$dialog->set_border_width(4);

		my $text = $self->{gtkDebugText} || new Gtk::Text(undef, undef);
		$text->set_editable(0);
		$self->{gtkDebugString} ||= "";
		$text->insert(undef, undef, undef, $self->{gtkDebugString});
		$dialog->vbox->pack_start($text, 0, 1, 10);

		my $button = new Gtk::Button "Close";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub { $dialog->destroy; });

		$button = new Gtk::Button "Clear";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub {
			$text->backward_delete($text->get_length);
			$self->{gtkDebugString} = "";
		});

		$dialog->signal_connect('destroy', sub {
			$self->{gtkDebugDialog} = undef;
			$self->{gtkDebugText} = undef;
		});
		$dialog->show_all;

		$self->{gtkDebugDialog} = $dialog;
		$self->{gtkDebugText} = $text;
	}

	my $text = $self->{gtkDebugText};
	#$text->set_point($text->get_length);
	$text->insert(undef, undef, undef, "$msg\n");
	$self->{gtkDebugString} .= "$msg\n";
}

1;

__END__

=head1 NAME

FVWM::Module::Gtk - FVWM::Module with the GTK+ v1 widget library attached

=head1 SYNOPSIS

  use lib `fvwm-perllib dir`;
  use FVWM::Module::Gtk;
  use Gtk;

  my $module = new FVWM::Module::Gtk;

  init Gtk;
  my $window = new Gtk::Window -toplevel;;
  my $label = new Gtk::Label "Hello, world";
  $window->add($label);
  $window->show_all;

  $module->addHandler(M_CONFIGURE_WINDOW, \&configure_toplevel);
  $module->addHandler(M_CONFIG_INFO, \&some_other_sub);

  $module->eventLoop;

=head1 DESCRIPTION

The B<FVWM::Module::Gtk> package is a sub-class of B<FVWM::Module> that
overloads the methods B<eventLoop>, B<showError> and B<showMessage>
to manage GTK+ version 1 objects as well.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only methods that are not available in B<FVWM::Module> or the overloaded ones
are covered here:

=over 8

=item B<eventLoop>

From outward appearances, this methods operates just as the parent
B<eventLoop> does. It is worth mentioning, however, that this version
enters into the B<Gtk>->B<main> subroutine, ostensibly not to return.

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

=item B<showDebug> I<msg> [I<title>]

Creates a persistent debug window with one "Close" button.
All new debug messages are added to this window (i.e. the existing
debug window is reused if found).

Good for debugging a GTK+ based module.

=head1 BUGS

Awaiting for your reporting.

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 THANKS TO

Randy J. Ray <randy@byz.org>.

Kenneth Albanowski, Paolo Molaro for Gtk/Perl extension.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module> and L<Gtk>.

See also L<FVWM::Module::Gtk2> for use with GTK+ version 2.

=cut
