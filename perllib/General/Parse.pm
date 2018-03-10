# Copyright (c) 2002-2009, Mikhael Goikhman
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

package General::Parse;
use strict;

use vars qw(@ISA @EXPORT);
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(
	get_token cut_token get_tokens cut_tokens eqi nei
);

# currently backslashes are ignored and this is a bit buggy
sub get_token ($) {
	my $line = shift;
	my $token;

	$line =~ s/^\s+//;
	my $quote = '\s';  
	$quote = substr($line, 0, 1) if $line =~ /^["'`]/;

	$line =~ s/^$quote//;
	if ($line =~ /^(.*?)$quote\s*(.*)$/) {
		$token = $1;
		$line = $2;
	} elsif ($line eq "") {
		$token = undef;
		$line = "";
	} else {
		$token = $line;
		$line = "";
	}
	return wantarray ? ($token, $line) : $token;
}

# returns the next quoted token, modifies the parameter (ref to line)
sub cut_token ($) {
	my $line_ref = shift;
	my ($token, $rest) = get_token($$line_ref);
	$$line_ref = $rest;
	return $token;
}

sub cut_tokens ($$) {
	my $line_ref = shift;
	my $limit = shift;
	my @tokens = ();

	$$line_ref =~ s/^\s+//;
	while ($$line_ref ne "" && $limit-- > 0) {
		push @tokens, cut_token($line_ref);
	}
	return wantarray ? @tokens : \@tokens;
}

sub get_tokens ($;$) {
	my $line = shift;
	my $limit = shift || 1000;

	return cut_tokens(\$line, $limit);
}

sub eqi ($$) {
	my ($a, $b) = @_;
	return lc($a) eq lc($b);
}
 
sub nei ($$) {
	my ($a, $b) = @_;
	return lc($a) ne lc($b);
}

# ---------------------------------------------------------------------------- 

=head1 NAME

General::Parse - parsing functions

=head1 SYNOPSIS

  use General::Parse;

  my $string = q{Some "not very long" string of 6 tokens.};
  my $token1 = cut_token(\$string);          # $token1 = "Some";
  my ($token2, $token3) = cut_tokens(\$string, 2);
  my @subtokens = get_tokens($token2);       # ("not", "very", "long")
  my $subtoken1 = get_token($token2);        # the same as $subtokens[0]
  my $ending_array_ref = get_tokens($string);  # ["of", "6", "tokens."]

=head1 DESCRIPTION

This package may be used for parsing a string into tokens (quoted words).

=head1 FUNCTIONS


=head2 get_token

=over 4

=item description

Returns the first token of the given string without changing the string itself.

If the string is empty or consists of only spaces the returned token is undef.

If the caller expects a scalar - the token is returned. If it expects an
array - 2 values returned, the token and the rest of the string.

=item parameters   

String (scalar) to be parsed.

=item returns

Token (scalar).
Or, in array context, array of 2 scalars: token and the rest of string.

=back


=head2 cut_token

=over 4

=item description

Returns the first token of the given string, the input string is cut to
contain tokens starting from the second one.

If the string is empty or consists of only spaces the returned token is undef
and the string is changed to an empty string.

=item parameters   

String (scalar) to be parsed.

=item returns

Token (scalar).

=back


=head2 get_tokens

=over 4

=item description

Returns all or the requested number of tokens of the given string without
changing the string itself.

The returned array may contain less tokens than requested if the string
is too short. Particularly, the returned array is empty if the string is empty
or consists of only spaces.

If the caller expects a scalar - a reference to the token array is returned.
If it expects an array - the token array is returned.

=item parameters   

String (scalar) to be parsed, and optional limit (integer) for number of
returned tokens.

=item returns

Tokens (array of scalars or array ref of scalars depending on context).

=back


=head2 cut_tokens

=over 4

=item description

Returns the requested number of tokens of the given string, the string is cut
to contain the tokens starting from the first non returned token.

The returned array may contain less tokens than requested if the string
is too short. Particularly, the returned array is empty if the string is empty
or consists of only spaces.

If the caller expects a scalar - a reference to the token array is returned.
If it expects an array - the token array is returned.

=item parameters   

String (scalar) to be parsed, and limit (integer) for number of
returned tokens.

=item returns

Tokens (array of scalars or array ref of scalars depending on context).

=back


=head2 eqi

=over 4

=item description

Similar to B<eq>, but case-insensitive, gets 2 strings, returns boolean.

=back


=head2 nei

=over 4

=item description

Similar to B<ne>, but case-insensitive, gets 2 strings, returns boolean.

=back


=head1 AUTHOR

Mikhael Goikhman <migo@homemail.com>

=cut
# ============================================================================

1;
