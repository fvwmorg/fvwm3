# Copyright (C) 2002, Mikhael Goikhman
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

package General::Parse;
use strict;

use vars qw(@ISA @EXPORT);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(
	getToken cutToken eqi nei
);

# currently backslashes are ignored and this is a bit buggy
sub getToken ($) {
	my $line = shift;
	my $token;

	$line =~ s/^\s+//;
	my $quote = '\s';  
	$quote = substr($line, 0, 1) if $line =~ /^["'`]/;

	$line =~ s/^$quote//;
	if ($line =~ /^(.*?)$quote\s*(.*)$/) {
		$token = $1;
		$line = $2;
	} else {
		$token = $line;
		$line = "";
	}
	return wantarray? ($token, $line): $token;
}

# returns the next quoted token, modifies the parameter (ref to line)
sub cutToken ($) {
	my $line = shift;
	my ($token, $rest) = getToken($$line);
	$$line = $rest;
	return $token;
}

sub eqi ($$) {
	my ($a, $b) = @_;
	return lc($a) eq lc($b);
}
 
sub nei ($$) {
	my ($a, $b) = @_;
	return lc($a) ne lc($b);
}

1;
