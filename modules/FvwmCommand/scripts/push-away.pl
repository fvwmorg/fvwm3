#! xPERLx
# FvwmCommand script
# Written by Toshi Isogai
#
# push-away
# push other windows back when they overlap
# usage: push-away <direction> <window name>
#   direction   - direction (down,up,left,right) to push away
#   window name - windows to be protected, name can be regular expression
# icons are ignored

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


$Dir = shift;
$Wn = shift;

if( $Dir =~ /down/i ) {
	$Dir = 1;
}elsif( $Dir =~ /up/i ) {
	$Dir = 2;
}elsif( $Dir =~ /left/i ) {
	$Dir = 3;
}elsif( $Dir =~ /right/i ) {
	$Dir = 4;
}

if( $Dir == 0 || $Wn eq '' ) {
	print STDERR "push other windows back when they overlap\n";
	print STDERR "usage: push-away <direction> <window name>\n";
	print STDERR "  direction   - direction (down,up,left,right) to push away\n";
	print STDERR "  window name - windows to be protected\n";
	print STDERR "                name can be regular expression\n";
	exit(1);
}


# start a dedicated server
$fifo = "$ENV{'HOME'}/.FCMpb";
system( "FvwmCommand 'FvwmCommandS $fifo'");

# larger number for slow machine
select(undef,undef,undef,0.5);

# we need this to run this script in background job
$SIG{'TTIN'} = "IGNORE";

# start monitoring (-m option ) all fvwm transaction (-i3 option )
open( FCM, "FvwmCommand -f $fifo -m -i3 |" ) || die "FCM $fifo";

# send command through the new fifo which is "$fifo" + "C"
open( FCC, ">${fifo}C" ) || die "FCC $fifo" ;

# non blocking outputs
select( FCC ); $| = 1;
select( STDOUT ); $| = 1;


# some delay for slow one
select(undef,undef,undef,0.1);
print FCC "send_windowlist\n";
# yet some more delay for slow one
select(undef,undef,undef,0.1);

$endlist = 0;
while( <FCM> ) {

	if( /^(0x\S+) frame\s+x (-?\d+), y (-?\d+), width (\d+), height (\d+)/ ) {
		$id = $1;
		$Config{$id}{'x'} = $2;
		$Config{$id}{'y'} = $3;
		$Config{$id}{'w'} = $4;
		$Config{$id}{'h'} = $5;

		next if ! $endlist ;

		# move other windows if necessary
		if( $Config{$id}{'protect'} ) {
			foreach $w (keys %Config) {
				if( $id ne $w ) {
					move_if_overlap( $w, $id );
				}
			}
		}else{
			foreach $w (keys %Config) {
				if( $Config{$w}{'protect'} ) {
					move_if_overlap( $id, $w );
				}
			}
		}

	}elsif( /^(0x\S+) desktop +(-?\d+)/ ) {
		$Config{$1}{'desk'} = $2;

	}elsif( /^(0x\S+) Iconified +(yes|no)/ ) {
		$Config{$1}{'Iconified'} = $2;

	}elsif( /^(0x\S+) window +(.*)/ ) {
		$id = $1;
		$window = $2;

		if( $window =~ /$Wn/ ) {
			$Config{$id}{'protect'} = 1;
		}

	}elsif( /end windowlist/ ) {
		$endlist = 1;
		foreach $id (keys %Config) {
			if( $Config{$id}{'protect'} ) {
				foreach $w (keys %Config) {
					if( $id ne $w ) {
						move_if_overlap( $w, $id );
					}
				}
			}
		}

	}elsif( /^(0x\S+) destroy/ ) {
		delete $Config{$1};

	}
}


sub move_if_overlap {
	my($id1, $id2) = @_;
	my($ov);
	my($c1xl,$c1xh,$c1yl,$c1yh);
	my($c2xl,$c2xh,$c2yl,$c2yh);



	if( $Config{$id1}{'desk'} != $Config{$id2}{'desk'}
		|| $Config{$id1}{'Iconified'} eq 'yes'
		|| $Config{$id2}{'Iconified'} eq 'yes' ) {
		return;
	}


	$ov = 0;

	$c1xl = $Config{$id1}{'x'};
	$c1yl = $Config{$id1}{'y'};
	$c1xh = $Config{$id1}{'x'}+$Config{$id1}{'w'};
	$c1yh = $Config{$id1}{'y'}+$Config{$id1}{'h'};

	$c2xl = $Config{$id2}{'x'};
	$c2yl = $Config{$id2}{'y'};
	$c2xh = $Config{$id2}{'x'}+$Config{$id2}{'w'};
	$c2yh = $Config{$id2}{'y'}+$Config{$id2}{'h'};


		if( $c2xl >= $c1xl && $c2xl <= $c1xh
			|| $c2xh >= $c1xl && $c2xh <= $c1xh ) {
			if($c2yl >= $c1yl && $c2yl <= $c1yh
			   || $c2yh >= $c1yl && $c2yh <= $c1yh  ) {
				$ov = 1;
			}
		}elsif( $c1xl >= $c2xl && $c1xl <= $c2xh
				|| $c1xh >= $c2xl && $c1xh <= $c2xh ) {
			if($c1yl >= $c2yl && $c1yl <= $c2yh
			   || $c1yh >= $c2yl && $c1yh <= $c2yh  ) {
				$ov = 1;
			}
		}

	if( $ov ) {
		$x = $c1xl;
		$y = $c1yl;
		if( $Dir==1 ) {
			$y = $c2yh+1;
		}elsif( $Dir==2 ) {
			$y = $c2yl-($c1yh-$c1yl)-1;
		}elsif( $Dir==3 ) {
			$x = $c2xl-($c1xh-$c1xl)-1;
		}elsif( $Dir==4 ) {
			$x = $c2xh+1;
		}


		print FCC "windowid $id1 move ${x}p ${y}p\n";

		# ignore - pixel info is the last info for move
		while(<FCM>) {
			last if /^0x\S+ pixel/;
		}

		select(undef,undef,undef,0.1);
	}
}
