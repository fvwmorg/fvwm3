# Copyright (c) 2003-2009 Mikhael Goikhman
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

package FVWM::Module::Toolkit;

use 5.004;
use strict;
use vars qw($VERSION @ISA $_dialog_tool);

use FVWM::Module;

BEGIN {
	$VERSION = $FVWM::Module::VERSION;
	@ISA = qw(FVWM::Module);
}

sub import ($@) {
	my $class = shift;
	my $caller = caller;
	my $error = 0;
	my $name = "*undefined*";

	while (@_) {
		$name = shift;
		if ($name eq 'base') {
			next if UNIVERSAL::isa($caller, __PACKAGE__);
			my $caller2 = (caller(1))[0];
			eval "
				package $caller2;
				use FVWM::Constants;

				package $caller;
				use vars qw(\$VERSION \@ISA);
				use FVWM::Constants;
				\$VERSION = \$FVWM::Module::Toolkit::VERSION;
				\@ISA = qw(FVWM::Module::Toolkit);
			";
			if ($@) {
				die "Internal error:\n$@";
			}
		} else {
			my ($name0, $args) = split(/>?=/, $name, 2);
			my $mod = $args? "$name0 split(/,/, q{$args})": $name;
			eval "
				package $caller;
				use $mod;
			";
			if ($@) {
				$error = 1;
				last;
			}
		}
	}
	if ($error) {
		my $script_name = $0; $script_name =~ s|.*/||;
		my $error_title = 'FVWM Perl library error';
		my $error_msg = "$script_name requires Perl package $name to be installed.\n\n";
		$error_msg .= "You may either find it as a binary package for your distribution\n";
		$error_msg .= "or download it from CPAN, http://cpan.org/modules/by-module/ .\n";
		$class->show_message($error_msg, $error_title, 1);
		print STDERR "[$error_title]: $error_msg\n$@";
		exit(1);
	}
}

sub show_error ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Error");

	$self->show_message($msg, $title, 1);
	print STDERR "[$title]: $msg\n";
}

sub show_message ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Message");
	my $no_stderr = shift || 0;  # for private usage only

	unless ($_dialog_tool) {
		my @dirs = split(':', $ENV{PATH});
		# kdialog is last because at least v0.9 ignores --title
		TOOL_CANDIDATE:
		foreach (qw(gdialog Xdialog zenity message kdialog)) {
			foreach my $dir (@dirs) {
				my $file = "$dir/$_";
				if (-x $file) {
					$_dialog_tool = $_;
					last TOOL_CANDIDATE;
				}
			}
		}
	}
	my $tool = $_dialog_tool || "xterm";

	$msg =~ s/'/'"'"'/sg;
	$title =~ s/'/'"'"'/sg;
	if ($tool eq "gdialog" || $tool eq "Xdialog" || $tool eq "kdialog") {
		system("$tool --title '$title' --msgbox '$msg' 500 100 &");
	} elsif ($tool eq "zenity") {
		system("zenity --title '$title' --info --text '$msg' --no-wrap &");
	} elsif ($tool eq "xmessage") {
		system("xmessage -name '$title' '$msg' &");
	} else {
		$msg =~ s/"/\\"/sg;
		$msg =~ s/\n/\\n/sg;
		system("xterm -g 70x10 -T '$title' -e \"echo '$msg'; sleep 600000\" &");
	}
	print STDERR "[$title]: $msg\n" if $! && !$no_stderr;
}

sub show_debug ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Debug");

	print STDERR "[$title]: $msg\n";
}

sub add_default_error_handler ($) {
	my $self = shift;

	$self->add_handler(M_ERROR, sub {
		my ($self, $event) = @_;
		$self->show_error($event->_text, "fvwm error");
	});
}

1;

__END__

=head1 NAME

FVWM::Module::Toolkit - FVWM::Module with abstract widget toolkit attached

=head1 SYNOPSIS

1) May be used anywhere to require external Perl classes and report error in
the nice dialog if absent:

    use FVWM::Module::Toolkit qw(Tk X11::Protocol Tk::Balloon);

    use FVWM::Module::Toolkit qw(Tk=804.024,catch X11::Protocol>=0.52);

There is the same syntactic sugar as in "perl -M", with an addition
of ">=" being fully equivalent to "=". The ">=" form may look better for
the user in the error message. If the required Perl class is absent,
FVWM::Module::Toolkit->show_message() is used to show the dialog and the
application dies.

2) This class should be uses to implement concrete toolkit subclasses.
A new toolkit subclass implementation may look like this:

    package FVWM::Module::SomeToolkit;
    # this automatically sets the base class and tries "use SomeToolkit;"
    use FVWM::Module::Toolkit qw(base SomeToolkit);

    sub show_error ($$;$) {
        my ($self, $error, $title) = @_;
        $title ||= $self->name . " Error";

        # create a dialog box using SomeToolkit widgets
        SomeToolkit->Dialog(
            -title => $title,
            -text => $error,
            -buttons => ['Close'],
        );
    }

    sub event_loop ($$) {
        my $self = shift;
        my @params = @_;

        # enter the SomeToolkit event loop with hooking $self->{istream}
        $self->event_loop_prepared(@params);
        fileevent($self->{istream},
            read => sub {
                unless ($self->process_packet($self->read_packet)) {
                    $self->disconnect;
                    $top->destroy;
                }
                $self->event_loop_prepared(@params);
            }
        );
        SomeToolkit->MainLoop;
        $self->event_loop_finished(@params);
    }

=head1 DESCRIPTION

The B<FVWM::Module::Toolkit> package is a sub-class of B<FVWM::Module> that
is intended to be uses as the base of sub-classes that attach widget
toolkit library, like Perl/Tk. It does some common work to load
widget toolkit libraries and to show an error in the external window like
xmessage if the required libraries are not available.

This class overloads one method B<add_default_error_handler> and expects
sub-classes to overload the methods B<show_error>, B<show_message> and
B<show_debug> to use native widgets. These 3 methods are implemented in this
class, they extend the superclass versions by adding a title parameter and
using an external dialog tool to show error/message.

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only overloaded or new methods are covered here:

=over 8

=item B<show_error> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog box
using the corresponding widgets. The default fall back implementation is
similar to B<show_message>, but the error message (with title) is also always
printed to STDERR.

May be good for module diagnostics or any other purpose.

=item B<show_message> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog
box using the corresponding widgets. The default fall back implementation is
to find a system message application to show the message. The potentially
used applications are I<gdialog>, I<Xdialog>, I<zenity>,
I<xmessage>, I<kdialog>, or I<xterm> as the last resort. If not given,
I<title> is based on the module name.

May be good for module debugging or any other purpose.

=item B<show_debug> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog box
using the corresponding widgets. The default fall back implementation is
to print a message (with a title that is the module name by default)
to STDERR.

May be good for module debugging or any other purpose.

=item B<add_default_error_handler>

This methods adds a M_ERROR handler to automatically notify you that an error
has been reported by fvwm. The M_ERROR handler then calls C<show_error()>
with the received error text as a parameter to show it in a window.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module>, L<FVWM::Module::Tk>.

=cut
