#!/usr/bin/perl
# FvwmCommand example - auto raise small windows
# arg1 - size ( w * h in pixel)  (default 60000)
# arg2 - delay (second) (default 1)

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


	
