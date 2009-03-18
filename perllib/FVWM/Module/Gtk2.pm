# Copyright (c) 2003-2009, Mikhael Goikhman
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

sub event_loop ($@) {
	my $self = shift;
	my @params = @_;

	$self->event_loop_prepared(@params);
	Gtk2::Helper->add_watch(
		$self->{istream}->fileno, 'in',
		sub ($$$) {
			#my ($socket, $fd, $flags) = @_;
			#return 0 unless $flags->{'read'};
			unless ($self->process_packet($self->read_packet)) {
				Gtk2->main_quit;
			}
			$self->event_loop_prepared(@params);
			return 1;
		}
	);
	Gtk2->main;
	$self->event_loop_finished(@params);
}

sub show_error ($$;$) {
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

sub show_message ($$;$) {
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

sub show_debug ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Debug");

	my $dialog = $self->{gtk_debug_dialog};

	if (!$dialog) {
		$self->{gtk_debug_string} ||= "";

		$dialog = new Gtk2::Dialog;
		$dialog->set_title($title);
		$dialog->set_border_width(4);
		$dialog->set_default_size(540, 400);

		my $scroll = Gtk2::ScrolledWindow->new;
		$scroll->set_policy('automatic', 'automatic');
		$scroll->set_shadow_type('in');
		my $text = Gtk2::TextBuffer->new(undef);
		$text->insert($text->get_iter_at_offset(0), $self->{gtk_debug_string});
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
			$self->{gtk_debug_string} = "";
		});

		$button = new Gtk2::Button "Save";
		$dialog->action_area->pack_start($button, 1, 1, 0);
		$button->signal_connect("clicked", sub {
			my $file_dialog = new Gtk2::FileSelection("Save $title");
			my $filename = "$ENV{FVWM_USERDIR}/";
			$filename .= $self->name . "-debug.txt";
			$file_dialog->set_filename($filename);
			$file_dialog->ok_button->signal_connect("clicked", sub {
				$filename = $file_dialog->get_filename;
				require General::FileSystem;
				my $text = \$self->{gtk_debug_string};
				General::FileSystem::save_file($filename, $text)
					if $filename;
				$file_dialog->destroy;
			});
			$file_dialog->cancel_button->signal_connect("clicked", sub {
				$file_dialog->destroy;
			});
			$file_dialog->show;
		});

		$dialog->signal_connect('destroy', sub {
			$self->{gtk_debug_dialog} = undef;
			$self->{gtk_debug_text_wg} = undef;
		});
		$dialog->show_all;

		$self->{gtk_debug_dialog} = $dialog;
		$self->{gtk_debug_text_wg} = $text;
	}

	my $text = $self->{gtk_debug_text_wg};
	$text->insert(($text->get_bounds)[1], "$msg\n");
	$self->{gtk_debug_string} .= "$msg\n";
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
    Gtk2->init;

    my $module = new FVWM::Module::Gtk2(
        Debug => 2,
    );

    my $dialog = new Gtk2::Dialog;
    $dialog->signal_connect("destroy", sub { Gtk2->main_quit; });
    $dialog->set_title("Simple Test");
    my $button = new Gtk2::Button "Close";
    $button->signal_connect("clicked", sub { $dialog->destroy; });
    $dialog->action_area->pack_start($button, 1, 1, 0);
    $dialog->show_all;
    my $id = $dialog->window->XWINDOW();

    $module->add_default_error_handler;
    $module->add_handler(M_ICONIFY, sub {
        my $id0 = $_[1]->_win_id;
        $module->send("Iconify off", $id) if $id0 == $id;
    });
    $module->track('Scheduler')->schedule(60, sub {
        $module->show_message("You run this module for 1 minute")
    });

    $module->send('Style "Simple Test" Sticky');
    $module->event_loop;

=head1 DESCRIPTION

The B<FVWM::Module::Gtk2> class is a sub-class of B<FVWM::Module::Toolkit>
that overloads the methods B<event_loop>, B<show_error>, B<show_message> and
B<show_debug> to manage GTK+ version 2 objects as well.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only overloaded or new methods are covered here:

=over 8

=item B<event_loop>

From outward appearances, this methods operates just as the parent
B<event_loop> does. It is worth mentioning, however, that this version
enters into the B<Gtk2>->B<main> subroutine, ostensibly not to return.

=item B<show_error> I<msg> [I<title>]

This method creates a dialog box using the GTK+ widgets. The dialog has
three buttons labeled "Close", "Close All Errors" and "Exit Module".
Selecting the "Close" button closes the dialog. "Close All Errors" closes
all error dialogs that may be open on the screen at that time.
"Exit Module" terminates your entire module.

Useful for diagnostics of a GTK+ based module.

=item B<show_message> I<msg> [I<title>]

Creates a message window with one "Close" button.

Useful for notices by a GTK+ based module.

=item B<show_debug> I<msg> [I<title>]

Creates a persistent debug window with 3 buttons "Close", "Clear" and "Save".
All new debug messages are added to this window (i.e. the existing debug
window is reused if found).

"Close" withdraws the window until the next debug message arrives.

"Clear" erases the current contents of the debug window.

"Save" dumps the current contents of the debug window to the selected file.

Useful for debugging a GTK+ based module.

=back

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
