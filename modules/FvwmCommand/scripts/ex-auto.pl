#! xPERLx
# FvwmCommand example - auto raise small windows
# arg1 - size ( w * h in pixel)  (default 60000)
# arg2 - delay (second) (default 1)

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

open( FCM, "FvwmCommand -m -i3 send_windowlist |" ) || die "FCM";
open( FCC, ">$ENV{'HOME'}/.FvwmCommandC" ) || die "FCC";

select( FCC ); $| = 1;
select( STDOUT ); $| = 1;


$Size = shift;
if( $Size <= 0 ) {
	$Size = 60000;
}

if( $#ARGV >= 0)  {
	$Delay = shift;
}else{
	$Delay = 1;
}


LOOP1:while( <FCM> ) {
	if( /^0x(\S+) frame .*width (\d+), height (\d+)/ ) {
		$Config{$1}{'area'} = $2 * $3;
	}elsif( /^0x(\S+) (focus change|end windowlist)/ ) {

		if( $1 != 0 ) {
			# delay longer than FvwmAuto
			select(undef,undef,undef,$Delay);

			foreach $w (keys %Config) {
				if( $Config{$w}{'area'} < $Size ) {
					print FCC "windowid 0x$w Raise\n";

					# ignore
					while(<FCM>) {
						last if /^0x$w raise/;
                        redo LOOP1 if /^0x\S+ focus change/;
					}
					select(undef,undef,undef,0.1);
				}
			}
		}
	}
}



