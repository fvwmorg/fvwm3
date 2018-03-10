#! xPERLx
# FvwmCommand script
# Written by Toshi Isogai
#
# 1. auto focus Netscape dialog when opened
# 2. move download/upload window to right edge of the screen

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

# screen width
if( `xwininfo -root` =~ /Width: (\d+)/ ) {
	$SW = $1;
}else{
	# some resonable number if xwininfo doesn't work
	$SW = 1024;
}

# start a dedicated server
$fifo = "$ENV{'HOME'}/.FCMfocus";
system( "FvwmCommand 'FvwmCommandS $fifo'");
#for slow machine
select(undef,undef,undef,1);

# we need this to run this script in background job
$SIG{'TTIN'} = "IGNORE";

# start monitoring (-m option ) all fvwm transaction (-i3 option )
open( FCM, "FvwmCommand -f $fifo -m -i3 |" ) || die "FCM $fifo";

# send command through the new fifo which is "$fifo" + "C"
open( FCC, ">${fifo}C" ) || die "FCC $fifo" ;

# appearantly, it has be unbuffered
select( FCC ); $| = 1;
select( STDOUT ); $| = 1;


LOOP1: while( <FCM> ) {
	if( /^(0x[\da-f]+) add/ ) {
		$id = $1;

		while( <FCM> ) {

			# keep window frame
			if( /^$id frame\s+x -?\d+, y (-?\d+), width (\d+)/ ) {
				$y = $1;
				$width = $2;

				# search for class  line
			}elsif( /^$id class/ ) {

				if( !/\sNetscape/ ) {
					# not Netscape
					last;
				}

				# the next line should be resource line
				$_ = <FCM>;

				# resource line tells what the window is
				if( /^$id resource/ ) {

					# search for Netscape popups
					if( /\s+\w+popup/ ) {

						# fvwm doesn't like commands from modules too fast
						select(undef,undef,undef, 0.4 );

						# focus it
						print FCC "windowid $id focus\n";

					}
					# search for Netscape download or upload window
					elsif( /\s+(Down|Up)load/ ) {
						select(undef,undef,undef, 0.4 );

						# move to the right edge, keep the whole window in screen
						$x = $SW - $width;
						print FCC "windowid $id move ${x}p ${y}p\n";
					}
					last;
				}
			}
		}
	}
}
print "end\n";




