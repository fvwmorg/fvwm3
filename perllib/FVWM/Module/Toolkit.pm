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

package FVWM::Module::Toolkit;

use 5.004;
use strict;
use vars qw($VERSION @ISA);

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
			eval "
				package $caller;
				use $name;
			";
			if ($@) {
				$error = 1;
				last;
			}
		}
	}
	if ($error) {
		my $scriptName = $0; $scriptName =~ s|.*/||;
		my $errorTitle = 'FVWM Perl library error';
		my $errorMsg = "$scriptName requires Perl package $name to be installed";
		print STDERR "[$errorTitle]: $errorMsg\n$@";
		if (-x '/usr/bin/gdialog' || -x '/usr/bin/X11/gdialog') {
			system("gdialog --title '$errorTitle' --msgbox '$errorMsg' 500 100");
		} elsif (-x '/usr/bin/gtk-shell' || -x '/usr/bin/X11/gtk-shell') {
			system("gtk-shell --size 500 100 --title '$errorTitle' --label '$errorMsg' --button Close");
		} elsif (-x '/usr/bin/xmessage' || -x '/usr/bin/X11/xmessage') {
			system("xmessage -name '$errorTitle' '$errorMsg'");
		}
		exit(1);
	}
}

sub showError ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Error");

	print STDERR "[$title]: $msg\n";
}

sub showMessage ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Message");

	print STDERR "[$title]: $msg\n";
}

sub showDebug ($$;$) {
	my $self = shift;
	my $msg = shift;
	my $title = shift || ($self->name . " Debug");

	print STDERR "[$title]: $msg\n";
}

sub addDefaultErrorHandler ($) {
	my $self = shift;

	$self->addHandler(M_ERROR, sub {
		my ($self, $event) = @_;
		$self->showError($event->_text, "FVWM Error");
	});
}

1;

__END__

=head1 NAME

FVWM::Module::Toolkit - FVWM::Module with abstract widget toolkit attached

=head1 SYNOPSIS

Only to be uses to implement concrete toolkit subclasses.

    package FVWM::Module::SomeToolkit;
    # this automatically sets the base class and tries "use SomeToolkit;"
    use FVWM::Module::Toolkit qw(base SomeToolkit);

    sub showError ($$;$) {
        my ($self, $error, $title) = @_;
        $title ||= $self->name . " Error";

        # create a dialog box using SomeToolkit widgets
        SomeToolkit->Dialog(
            -title => $title,
            -text => $error,
            -buttons => ['Close'],
        );
    }

    sub eventLoop ($$) {
        my ($self, $event) = @_;

        # enter the SomeToolkit event loop with hooking $self->{istream}
        fileevent($self->{istream},
            read => sub {
                unless ($self->processPacket($self->readPacket)) {
                    $self->disconnect;
                    $top->destroy;
                }
            }
        );
        SomeToolkit->MainLoop;
    }

=head1 DESCRIPTION

The B<FVWM::Module::Toolkit> package is a sub-class of B<FVWM::Module> that
is intended to be uses as the base of sub-classes that attach widget
toolkit library, like Perl/Tk or Gtk-Perl. It does some common work to load
widget toolkit libraries and to show an error in the external window like
xmessage if the needed libraries are not available.

This class overloads one method B<addDefaultErrorHandler> and expects
sub-classes to overload the methods B<showError> and B<showMessage> to use
native widgets (these 2 methods in this class by themselves overwrite the
superclass versions by adding a title parameter).

This manual page details only those differences. For details on the
API itself, see L<FVWM::Module>.

=head1 METHODS

Only those methods that are not available in B<FVWM::Module>, or are overloaded
are covered here:

=over 8

=item B<showError> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog box
using the corresponding widgets. The default fall back implementation is
to print an error message (with a title that is the module name by default)
to STDERR.

May be good for module diagnostics or any other purpose.

=item B<showMessage> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog box
using the corresponding widgets. The default fall back implementation is
to print a message (with a title that is the module name by default)
to STDERR.

May be good for module debugging or any other purpose.

=item B<showDebug> I<msg> [I<title>]

This method is intended to be overridden in subclasses to create a dialog box
using the corresponding widgets. The default fall back implementation is
to print a message (with a title that is the module name by default)
to STDERR.

May be good for module debugging or any other purpose.

=item B<addDefaultErrorHandler>

This methods adds a M_ERROR handler to automatically notify you that an error
has been reported by FVWM. The M_ERROR handler then calls C<showError()>
with the received error text as a parameter to show it in a window.

=back

=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>.

=head1 SEE ALSO

For more information, see L<fvwm>, L<FVWM::Module>, L<FVWM::Module::Gtk>,
L<FVWM::Module::Gtk2>, L<FVWM::Module::Tk>.

=cut
