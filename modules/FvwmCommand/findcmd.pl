#!/usr/bin/perl
# Find fvwm commands from function.c struct functions
# Written by Toshi Isogai

sub getcmd {
    $/ = "\n\n";
    while(<STDIN>) {
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
    my( @cmd, $i, @ln );
    my( $fvwmdir ) = $ARGV[0];
    my( $pm ) = "FvwmCommand.pm";
    
    @cmd = getcmd();

    print  "# $pm\n";
    print  "# Collection of fvwm2 builtin commands for FvwmCommand\n";
    print  "package FvwmCommand;\nuse Exporter;\n";
    print  "\@ISA=qw(Exporter);\n\@EXPORT=qw(";
    for( $i=0; $i<=$#cmd; $i++) {
	if( $i % 5 == 0 ) {
	    print  "\n  $cmd[$i]";
	}else{
	    print  " $cmd[$i]";
	}
    }
    print  "\n);\n";
    print  "\nsub FvwmCommand { system \"$fvwmdir/FvwmCommand '\@_'\"}\n\n";
    foreach $i (@cmd) {
	print  "sub $i { FvwmCommand \"$i \@_ \" }\n";
    }
    print  "sub AM { FvwmCommand \"+ \@_ \" }\n";
    print  "1;\n";
}

create_pm();
