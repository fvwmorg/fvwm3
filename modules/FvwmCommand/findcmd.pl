#!/usr/bin/perl
# Find fvwm commands from functions.c struct functions
# Written by Toshi Isogai

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

sub getcmd {
  $/ = "\n\n";
  while(<STDIN>) {
    #find "func_t func_config[] =" in various spacing
    if (s/func_t.*\[\]\s*=(\s|\n)*\{// ) {
      return listcmd();
    }
  }
  print stderr "Can't find func_t\n";
  exit 1;
}

sub listcmd {
  my($cmd,@cmd);

  while (/CMD_ENTRY/) {
    $old = $_;
    s/.*CMD_ENTRY[^,]*,\s*CMD_([^,]+).*/\1/s;
    s/\s*$//;
    if (!/^\+\s*$/) {
      push @cmd, $_;
    }
    $_ = $old;
    s/(.*)CMD_ENTRY.*/\1/s;
  }
  @cmd;
}

sub create_pm {
  my( @cmd, $i, @ln );
  my( $fvwmdir ) = $ARGV[0];
  my( $pm ) = "FvwmCommand.pm";

  @cmd = getcmd();

  if ($ARGV[1] eq "-sh") {
    print "# FvwmCommand.sh\n";
    print "# Collection of fvwm builtin commands for FvwmCommand\n";
    print "#\n";
    print "alias FvwmCommand=\'$fvwmdir/FvwmCommand\'\n";

    for( $i=0; $i<=$#cmd; $i++) {
      print  "$cmd[$i] () {\n";
      print  "\tFvwmCommand \"$cmd[$i] \$\*\"\n";
      print  "}\n";
    }

    print "AM () { \n";
    print "	FvwmCommand \"+ \$\*\"\n";
    print "}\n";
  }
  else {
    print  "# $pm\n";
    print  "# Collection of fvwm builtin commands for FvwmCommand\n";
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
}

create_pm();
