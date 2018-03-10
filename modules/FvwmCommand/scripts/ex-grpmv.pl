#! xPERLx
# FvwmCommand example - move a group of windows

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


ask();


open( FCM, "FvwmCommand -m -i3 |" ) || die "$! FvwmCommand";
open( FCC, ">$ENV{'HOME'}/.FvwmCommandC" ) || die "FCC";
select( FCC ); $| = 1;
select( STDOUT ); $| = 1;

while( <FCM> ) {
    if( /^0x(\S+) frame +x (\d+), y (\d+)/ ) {
		# check if a member window is moved
		($id,$x,$y) = ($1,$2,$3);

		if( defined $Config{$id} ) {
			foreach $w (keys %Config) {
				$newx = $x + $Config{$w}{'x'}-$Config{$id}{'x'};
				$newy = $y + $Config{$w}{'y'}-$Config{$id}{'y'};
				if( $newx < 0 ) {
					$newx = 0;
				}
				if( $newy < 0 ) {
					$newy = 0;
				}
				print FCC "windowid 0x$w move ${newx}p ${newy}p\n";

				# ignore - pixel info is the last info for move
				while(<FCM>) {
					last if /^0x$w pixel/;
				}
				select(undef,undef,undef,0.1);
			}
		}
	}
}



sub ask {
    my($k,$c,$w, $mark);

	# list up windows
	open( FCM, "FvwmCommand -i2 send_windowlist |" ) || die "$! FvwmCommand";
	while( <FCM> ) {
		if( /^0x(\S+) (id|window) +(.*)/ ) {
			$Config{$1}{$2} = $3;
		}elsif( /^0x(\S+) frame +x (\d+), y (\d+)/ ) {
			$Config{$1}{'x'} = $2;
			$Config{$1}{'y'} = $3;
		}
	}
	close( FCM );

    $k = 'a';
    foreach $w (keys %Config) {
	$win{$k++} = $w;
	$Config{$w}{'grp'} = 0;
    }
    while(1) {
		print "\nWhich windwow to group (* for all, ENTER to Start)?\n";
		foreach $k (keys %win) {
			$mark = $Config{$win{$k}}{'grp'} ? 'X':' ';
			print "($k) ($mark) $Config{$win{$k}}{'window'}\n";
		}

		system "stty -icanon min 1 time 0";
		$c = getc();
		system "stty icanon";

		if( $c eq '*' ) {
			foreach(keys %win) {
				$Config{$_}{'grp'} = 1;
			}
		}elsif( $c eq "\n" ) {
			last;
		}elsif( !grep( /$c/,keys %win) ) {
			print "\aUndefine process\n";
		}else{
			$Config{$win{$c}}{'grp'} = ! $Config{$win{$c}}{'grp'};
		}
    }

	print "Group move on effect\n";
	foreach $w (keys %Config) {
		delete $Config{$w} if $Config{$w}{'grp'}!=1;
	}
}
