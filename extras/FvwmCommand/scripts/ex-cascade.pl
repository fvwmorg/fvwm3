#!/usr/bin/perl
# FvwmCommand example - cascade windows

open( F, "FvwmCommand -ri2 send_windowlist |" ) || die "command" ;
while( <F> ) {
	if( /^(0x[0-9a-f]+) frame *x (\d+), y (\d+), width (\d+), height (\d+)/ ) {
		$config{$1}{'area'} = $4 * $5;
		$config{$1}{'x'} = $2;
		$config{$1}{'y'} = $3;
	}elsif( /^(0x[0-9a-f]+) Sticky +(yes|no)$/  ) {
		$config{$1}{'sticky'} = $2;
	}elsif( /^(0x[0-9a-f]+) desktop +(\d+)/  ) {
		$config{$1}{'desktop'} = $2;
	}
}
close F;


$x = $y = 0;

$delay = 3000; #need bigger number for slower machine

foreach $i (keys %config) {
	next if( $config{$i}{'sticky'} =~ /yes/  ) ;
	system("FvwmCommand -w $delay 'windowid $i windowsdesk 0' " );
	system("FvwmCommand -w 0 'windowid $i move ${x}p ${y}p' " );
	system("FvwmCommand -w 0 'windowid $i raise' " );
	$x += 50;
	$y += 50;
}

print( 'hit return to move them back!' );
<>;
foreach $i (keys %config) {
	next if( $config{$i}{'sticky'} =~ /yes/  ) ;
	system("FvwmCommand -w 0 'windowid $i windowsdesk $config{$i}{desktop}'>/dev/null");
	system("FvwmCommand -w $delay 'gotopage 0 0' 'windowid $i move $config{$i}{x}p $config{$i}{y}p' >/dev/null" );
}



