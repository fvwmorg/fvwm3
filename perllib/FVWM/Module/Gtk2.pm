# Copyright (c) 2003, Mikhael Goikhman
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

package FVWM::Module::Gtk2;

use 5.004;
use strict;

use FVWM::Module::Toolkit qw(base Gtk2 Gtk2::Helper);

sub eventLoop ($@) {
	my $self = shift;
	my @params = @_;

	$self->eventLoopPrepared(@params);
	Gtk2::Helper->add_watch(
		$self->{istream}->fileno, 'in',
		sub ($$$) {
			#my ($socket, $fd, $flags) = @_;
			#return 0 unless $flags->{'read'};
			unless ($self->processPacket($self->readPacket)) {
				Gtk2->main_quit;
			}
			$self->eventLoopPrepared(@params);
			return 1;
		}
	);
	Gtk2->main;
	$self->eventLoopFinished(@params);
}

sub showError ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Error");

	my $dialog = new Gtk2::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk2::Label $msg;
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
	my $msg = shift;
	my $title = shift || ($self->name . " Message");

	my $dialog = new Gtk2::Dialog;
	$dialog->set_title($title);
	$dialog->set_border_width(4);

	my $label = new Gtk2::Label $msg;
	$dialog->vbox->pack_start($label, 0, 1, 10);

	my $button = new Gtk2::Button "Close";
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
		$self->{gtkDebugString} ||= "";

		$dialog = new Gtk2::Dialog;
		$dialog->set_title($title);
		$dialog->set_border_width(4);
		$dialog->set_default_size(540, 400);

		my $scroll = Gtk2::ScrolledWindow->new;
		$scroll->set_policy('automatic', 'automatic');
		$scroll->set_shadow_type('in');
		my $text = Gtk2::TextBuffer->new(undef);
		$text->insert($text->get_iter_at_offset(0), $self->{gtkDebugString});
		my $view = Gtk2::TextView->new;
		$view->set_buffer($text);
		$view->set_editable(0);
		$view->set_cursor_visible(0);
		$view->set_wrap_mode('word');
		$view->set_pixels_above_lines(2);
		$view->set_pixels_below_lines(2);
		$scroll->add($view);
		$dialog->vbox->pack_start($scroll, 1, 1, 4);

		my $button = new Gtk2::Button "Close";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub { $dialog->destroy; });

		$button = new Gtk2::Button "Clear";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub {
			$text->delete($text->get_bounds);
			$self->{gtkDebugString} = "";
		});

		$button = new Gtk2::Button "Save";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub {
			my $fileDialog = new Gtk2::FileSelection("Save $title");
			my $fileName = "$ENV{FVWM_USERDIR}/";
			$fileName .= $self->name . "-debug.txt";
			$fileDialog->set_filename($fileName);
			$fileDialog->ok_button->signal_connect("clicked", sub {
				$fileName = $fileDialog->get_filename;
				require General::FileSystem;
				my $text = \$self->{gtkDebugString};
				General::FileSystem::saveFile($fileName, $text)
					if $fileName;
				$fileDialog->destroy;
			});
			$fileDialog->cancel_button->signal_connect("clicked", sub {
				$fileDialog->destroy;
			});
			$fileDialog->show;
		});

		$dialog->signal_connect('destroy', sub {
			$self->{gtkDebugDialog} = undef;
			$self->{gtkDebugTextWg} = undef;
		});
		$dialog->show_all;

		$self->{gtkDebugDialog} = $dialog;
		$self->{gtkDebugTextWg} = $text;
	}

	my $text = $self->{gtkDebugTextWg};
	$text->insert(($text->get_bounds)[1], "$msg\n");
	$self->{gtkDebugString} .= "$msg\n";
}

1;

__END__

=head1 NAME

FVWM::Module::Gtk2 - FVWM::Module with the GTK+ v2 widget library attached

=head1 SYNOPSIS

Name this module TestModuleGtk2, make it executable and place in ModulePath:

    #!/usr/bin/perl -w

    use lib `fvwm-perllib dir`;
    use FVWM::Module::Gtk2;
    use Gtk2;  # preferably in this order

    my $module = new FVWM::Module::Gtk2(
        Debug => 2,
    );

    init Gtk2;
    my $dialog = new Gtk2::Dialog;
    my $id = $dialog->window->XWINDOW();
    $dialog->signal_connect("destroy", sub { Gtk2->main_quit; });
    $dialog->set_title("Simple Test");
    my $button = new Gtk2::Button "Close";
    $button->signal_connect("clicked", sub { $dialog->destroy; });
    $dialog->action_area->pack_start($button, 1, 1, 0);
    $dialog->show_all;

    $module->addDefaultErrorHandler;
    $module->addHandler(M_ICONIFY, sub {
        my $id0 = $_[1]->_win_id;
        $module->send("Iconify off", $id) if $id0 == $id;
    });
    $module->track('Scheduler')->schedule(60, sub {
        $module->showMessage("You run this module for 1 minute")
    });

    $module->send('Style "Simple Test" Sticky');
    $module->eventLoop;

=head1 DESCRIPTION

The B<FVWM::Module::Gtk2> class is a sub-class of B<FVWM::Module::Toolkit>
that overloads the methods B<eventLoop>, B<showError>, B<showMessage> and
B<showDebug> to manage GTK+ version 2 objects as well.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only overloaded or new methods are covered here:

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

Useful for diagnostics of a GTK+ based module.

=item B<showMessage> I<msg> [I<title>]

Creates a message window with one "Close" button.

Useful for notices by a GTK+ based module.

=item B<showDebug> I<msg> [I<title>]

Creates a persistent debug window with 3 buttons "Close", "Clear" and "Save".
All new debug messages are added to this window (i.e. the existing debug
window is reused if found).

"Close" withdraws the window until the next debug message arrives.

"Clear" erases the current contents of the debug window.

"Save" dumps the current contents of the debug window to the selected file.

Useful for debugging a GTK+ based module.

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
