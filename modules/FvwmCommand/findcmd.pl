#!/usr/bin/perl
# Find fvwm commands from function.c struct functions
# Written by Toshi Isogai

sub getcmd {
	$/ = "\n\n";
	while(<FF>) {
		#find "struct functions func_config[] =" in various spacing
		if (s/struct\s+functions.*\[\]\s+=(\s|\n)+\{// ) {
			return listcmd();
		}
	}
	print stderr "Can't find struct functions\n";
	exit 1;
}

sub listcmd {
my($cmd,@cmd);
    while( /"(.*)"/g ) {
		$cmd = $1;
		next if $cmd =~ /^\+?$/;
		push @cmd, $cmd;
	}
@cmd;
}
	
sub create_pm {
	my(@cmd,$i,@ln);
        my( $fvwmdir ) = $ARGV[0];
	my( $fc);
	my( $pm ) = "FvwmCommand.pm";

	$fc = "../../fvwm/functions.c";
	
	open(FF, $fc) || die "$fc $!";
	@cmd = getcmd();
	close(FF);
	open(FPL,">$pm") || die "$pm: $!";
	
	print FPL "# $pm\n";
	print FPL "# Collection of fvwm2 builtin commands for FvwmCommand\n";
	print FPL "package FvwmCommand;\nuse Exporter;\n";
	print FPL "\@ISA=qw(Exporter);\n\@EXPORT=qw(";
	for( $i=0; $i<=$#cmd; $i++) {
		if( $i % 5 == 0 ) {
			print FPL "\n  $cmd[$i]";
		}else{
			print FPL " $cmd[$i]";
		}
	}
	print FPL "\n);\n";
	print FPL "\nsub FvwmCommand { system \"$fvwmdir/FvwmCommand '\@_'\"}\n\n";
	foreach $i (@cmd) {
		print FPL "sub $i { FvwmCommand \"$i \@_ \" }\n";
	}
	print FPL "sub AM { FvwmCommand \"+ \@_ \" }\n";
	print FPL "1;\n";
	print "$pm created\n";
}

create_pm();
