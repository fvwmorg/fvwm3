#! /usr/bin/perl
#    $0 - Front end of FvwmConsole
#    FvwmConsole server must be running

# Copyright 1997, Toshi Isogai
# You may use this code for any purpose, as long as the original
# copyright remains in the source code and all documentation

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

# If you change this, be sure to change the test in configure.in
require 5.002;

use Socket;

$ESC = "\e";
$HISTFILE = "$ENV{FVWM_USERDIR}/.FvwmConsole-History0";
$SOCKET_NAME = "$ENV{FVWM_USERDIR}/.FvwmConsole-Socket";
#$VERSION = '1.2';
$VERSION = '1.0.1';



if (-c "/dev/console" && -w "/dev/console") {
	close STDERR;
	if (!open (STDERR, ">/dev/console")) {
		print "Can't redirect STDERR to /dev/console: $!\n";
		sleep 3;
		exit;
	}
}

($Filename = $0) =~ s@.*/@@;
($Sname = $Filename) =~ s/C(\.pl)?$//;


$tty = `tty`;
$tty =~ s/\n//;
$org_stty = &stty('-g');

@Hist = ();
@Histall = ();
$HIST_SIZE = 100;
$MAX_COMMAND_SIZE = 1000;

main();
exit;


sub main::default_key {
#------------ default key bindings ----------
# these can be overidden by config lines
#
# It may need these lines in .Xdefault to make home and end key work
# FvwmConsole*VT100*Translations: #override \n \
#	<Key> Home: string(0x1b) string("[1~" ) \n \
#	<Key> Delete:  string(0x1b) string("[3~" ) \n
#	<Key> End:  string(0x1b) string("[4~" ) \n

	package User;

	$ESC = $main::ESC;
	$Key{"$ESC\[1~"} = 'bol';  #home Key
	$Key{"$ESC\[3~"} = 'del_char';
	$Key{"$ESC\[4~"} = 'eol';  #end key
	$Key{"$ESC\[A"}= 'prev_line'; #up
	$Key{"$ESC\[B"}= 'next_line'; #down
	$Key{"$ESC\[C"}= 'next_char'; #right
	$Key{"$ESC\[D"}= 'prev_char'; #left
	$Key{"${ESC}f"}= 'next_word';
	$Key{"${ESC}b"} = 'prev_word';

	$Key{"$ESC"} = 'prefix';
	$Key{"\cD"} = 'del_char';
	$Key{"\c?"} = 'del_char';
	$Key{"\cH"} = 'bs';
	$Key{"\cq"} = 'quote';
	$Key{"\cU"} = 'del_line';
	$Key{"\cs"} = 'search';
	$Key{"\cR"} = 'search_rev';
	$Key{"\cK"} = 'del_forw_line';
	$Key{"\ca"} = 'bol';
	$Key{"\ce"} = 'eol';
	$Key{"\cp"} = 'prev_line';
	$Key{"\cn"} = 'next_line';
	$Key{"\cf"} = 'next_char';
	$Key{"\cb"} = 'prev_char';
	$Key{"\cx"} = 'prefix';
	$Key{"\cx\cb"} = 'bind';
	$Key{"\cx\ck"} = 'cancel';
	$Key{"\cw"} = 'del_back_word';
	$Key{"\x8d"} = 'enter_wo_subst'; # alt enter
	$Key{"\n"} = 'enter';
	$Key{"\ci"} = 'ins_char (" ")';
	$Key{"\xE4"} = 'del_forw_word'; # alt_d
	$Key{"\xE6"} = 'next_word'; # alt_f
	$Key{"\xEB"} = 'bind'; # alt_k
	$Key{"\xEC"} = 'list_func'; # alt_k
	$Key{"\xF3"} = 'subst'; # alt_s
	$Key{"\xF4"} = 'termsize'; # alt_t
	$Key{"\xE2"} = 'prev_word'; # alt_b
	$Key{"\xb1"} = 'ins_nth_word(1)';
	$Key{"\xb2"} = 'ins_nth_word(2)';
	$Key{"\xb3"} = 'ins_nth_word(3)';
	$Key{"\xb4"} = 'ins_nth_word(4)';
	$Key{"\xb5"} = 'ins_nth_word(5)';
	$Key{"\xb6"} = 'ins_nth_word(6)';
	$Key{"\xb7"} = 'ins_nth_word(7)';
	$Key{"\xb8"} = 'ins_nth_word(8)';
	$Key{"\xb9"} = 'ins_nth_word(9)';
	$Key{"${ESC}b"} = 'prev_word'; # esc_b
	$Key{"${ESC}f"} = 'next_word'; # esc_f
	$Key{"${ESC}>"} = 'eoh_ign_mode'; # end of history, ignore mode
	$Key{"${ESC}<"} = 'boh_ign_mode'; # begining of history, ignore mode
	$Key{"${ESC}."} = 'ins_last_word';

	$Key{EOF} = "\cD"; #eof work only when line is empty
    $Subst{'^#.*'} = ''; # ignore comments
#---------------- end of key binding -----------------

#---------------- Terminal control -------------------
	$TERM_EEOL = "$ESC\[K";		# erase to end of line
	$TERM_RIGHT = "$ESC\[C";      # move cursor right
	$TERM_LEFT = "$ESC\[D";      # move cursor left
	$TERM_DOWN = "$ESC\[B";        # move cursor up
	$TERM_UP = "$ESC\[A";        # move cursor up
}

sub read_config {
	my( $hash,@keys,$key,@vals,$val);
	while(<SH>) {
		last if $_ eq "_C_Config_Line_End_\n";
		next if !s/^\*${Sname}//;
	    ($hash,@keys[0..3],@vals) =
			(/
			 ^(\w+)\s+    #hash name
			 ('([^\']*)'|"([^\"]*)"|(\S+))  #key quoted or bare word
			 (\s+('([^\']*)'|"([^\"]*)"|(\S+)))? #value
			 /x);
	    $key = $keys[1].$keys[2].$keys[3];
	    $val = $vals[2].$vals[3].$vals[4];

	    if( defined %{$User::{$hash}} ) {
            User::bind( $hash, $key, $val );
		  }
    }
}

sub main {
	my($sin, $cmd);
	my($name, $ppid, $cpid);

	socket(SH, PF_UNIX, SOCK_STREAM, 0) || die "$!\n";
	$sun = sockaddr_un($SOCKET_NAME);
	connect(SH, $sun) || die "$sun: $!\n";
	print "$Filename $VERSION\n";
	default_key();
	read_config();  #must be done before forking

	$ppid = $$;
	if( $cpid = fork()  ) {
		&input_open($tty,$tty,$HISTFILE,1);
		while( $cmd = &input('','',1) ) {
			next if $cmd =~/^\s*$/;
			last if $cmd eq "\0";
			if( length($cmd) > $MAX_COMMMAND_SIZE ) {
				print User::OUT "\a";
			}
			send( SH, $cmd."\0", 0 );
		}
		dokill( $cpid );
	} else {
		#child handles output
		while(<SH>) {
			last if $_ eq '';
			if( $_  eq "_C_Socket_Close_\n" ) {
				dokill( $ppid );
			}
			print;
		}
		dokill( $ppid );
	}

}

sub dokill {
	my($pid) = @_;
	unlink SH;
	kill -9,$pid if $pid;
	exit;
}

sub input_open {
	# arg0 input device
	# arg1 output device
	# arg2 history file
	# arg3 key selection - bit0
	#                      bit1
	#                      bit2 return undef esc code as it is

	($Dev_in,$Dev_out,$File,$Ksel) = @_;
	if( !$Dev_in ) {$Dev_in = $tty;}
	elsif( $Dev_in eq "not a tty" ) { $Dev_in = $ENV{'TTY'};}
	if( !$Dev_out ) {$Dev_out = $tty;}
	if( !$File ) { $File = '/tmp/input.tmp';}
	open(User::IN,"<$Dev_in") || die "open in at input_open '$Dev_in' $!\n";
	open(User::OUT,">$Dev_out") || die "can't open input at 'input_open' $!\n";
	select((select(User::OUT), $| = 1)[0]); # unbuffer pipe
	if( defined $File ) {
		if( open(INITF,"$File") ) {
			do "$File";
			@Histall=<INITF>; close(INITF); $#Histall--;
		}else{
			print STDERR "Can't open history file $File\n";
		}
	}
}

sub input_close  {
	close(User::IN);
	close(User::OUT);
}

sub getchar  {
	# get char from input
    # if esc , check for more char
    my($c,$s,$rin,$rout);
	sysread(User::IN, $c, 1);
	if( $c ne $ESC ) {
		$s = $c;
	}else {
		$rin = '';
		vec( $rin, fileno(User::IN),1) = 1;
		$n= select( $rout=$rin, undef, undef, 0.1 );
		$s = $ESC;
		if($n) {
			while($n= select( $rout=$rin, undef, undef, 0.1 ) ) {
				sysread( User::IN, $c, 1 );
				$s .= $c;
				last if $c =~ /[A-Dz~]/;     # end of escape seq
			}
		}
	}
    $s;
}

sub insert_char {
	local($c,*len,*ix,*hist) =@_;
	local($clen);
	$clen = length $c;
	if( $init_in ) {
		$len = $ix = $clen; # new hist - clear old one
		$hist[$#hist] = $c;
	}else{
		substr($hist[$#hist],$ix,0) = $c;   #insert char
		$len += $clen;
		$ix += $clen;
	}
}
sub stty    {
	my($arg) = @_;
	`/bin/stty $arg <$tty 2>&1`;
#	if( -x "/usr/5bin/stty" ) {
#		`/usr/5bin/stty $arg <$tty`;
#	}elsif( -x "/usr/bin/stty" ) {
#		`/usr/bin/stty $arg `;
#	}else {
#		`/bin/stty $arg `;
#	}
}

sub add_hist {
  # add input into history file
	local($type,*cmd) = @_;	#not my
	my( $t )= sprintf("%s",$type);
	my($h) = $cmd[$#cmd];
	return if !defined $File;
	if( $#cmd ==0 || $h ne $cmd[$#cmd-1] )        {
		$h =~ s/([\"@\$\\])/\\$1/g;
		$t =~ s/^\*//;
		push(@Histall, "push  (\@$t, \"$h\");\n" );
		@Histall = splice( @Histall, -$HIST_SIZE, $HIST_SIZE ); # take last HIST_SIZE commands
		if( open( FILE, ">$File" ) ){
			print FILE @Histall;
			print FILE "1;\n";
			close(FILE);
		}
	}else {
		$#cmd--;
	}
}

#----------------
# print mini help
#----------------
sub usage_error {
	open( THIS, "$0");
    while(<THIS>) {
		s/\$0/$Filename/;
		if( /^\#/ ) {
			print STDERR $_;
		}else{
			last;
		}
	}
	close THIS;
sleep 3;
exit 1;
}

sub search_mode {
	local(*c, *s, *prompt, *mode, *isp, *hist ) =@_;
	my($p_save, $isp_cur);
	if($c eq "\n"){
		$prompt = $p_save;
		$mode = 'normal';
		last IN_STACK;
	}
	$isp_cur = $isp;
	if( $User::Key{$c} =~ /^search/ ) {
		#search furthur
		$mode = $User::Key{$c};
		while(1) {
			if( $mode eq 'search_rev' && --$isp<0 ||
			   $mode eq 'search' && ++$isp>$#hist-1 ) {
				print User::OUT "\a"; # couldn't find one
				$isp = $isp_cur;
				last;
			}
			last if( index($hist[$isp],$s) >=0);
		}
	}elsif( $User::Key{$c} eq 'bs' ) {
		$s =~ s/.$//;
	}elsif( ord($c) < 32 ) {
		#non-printable char, get back to normal mode
		print User::OUT "\a";
		$prompt = $p_save;
		$mode = 'normal';
		return;
	}else{
		$s .= $c;
		while(1) {
			last if (index($hist[$isp],$s) >=0);
			if( $mode eq 'search_rev' && --$isp<0 ||
			   $mode eq 'search' && ++$isp>$#hist ) {
				print User::OUT "\a";   #couldn't find one
				chop($s);
				$isp = $isp_cur;
				last;
			}
		}
	}
	$prompt = "($mode)'$s':";
}

sub calcxy {
	my( $mode, $prompt, $len, $ix, $off, $wd ) = @_;
	my($plen);
	my(	$y_len, $y_ix, $col);
	my($adjust);  # 1 when the last char is on right edge

	$plen = length($prompt);
	$y_len = int (($plen+$len+$off) / $wd );
	$adjust = ( (($plen+$len+$off) % $wd == 0) && ($y_len > 0 )) ? 1:0;
	if( $mode =~ /^search/ ) {
		#move cursor to search string
		$y_ix = int (($plen-2+$off) / $wd );
		$col = ($plen-2+$off) % $wd;
	}else{
		#normal mode - move cursor back to $ix
		$y_ix = int (($plen+$ix+$off) / $wd );
		$col = ($plen+$ix+$off) % $wd;
	}
	($y_len, $y_ix, $col, $adjust);
}

package User;

sub move_cursor {
	my($x,$y, $x_prev,$y_prev) = @_;
	my($termcode);

	$termcode = '';
	if($y > $y_prev ) {
		$termcode = $TERM_DOWN x ($y-$y_prev);
	}elsif( $y < $y_prev ) {
		$termcode = $TERM_UP x ($y_prev-$y);
	}
	if( $x > $x_prev ) {
		$termcode .= $TERM_RIGHT x ($x-$x_prev);
	}elsif( $x < $x_prev ) {
		$termcode .= $TERM_LEFT x ($x_prev-$x);
	}
	print OUT $termcode;
}

sub another_line {
	$init_in = 1-$app;
	($hist[$#hist] = $hist[$isp]) =~ s/\n//;
	$ix = length($hist[$#hist]);
}

sub main::input {
	# arg0 - prompt
	# arg1 - input stack
	# arg2 - append input to command if 1
	# arg3 - # of column offset
	local($prompt,*hist,$app,$off) = @_;
	local($len,$ix);
	local($c,$isp,$s,$wisp);
	local($mode);
	local(%lastop);

	local($init_in);
	local($print_line);  #0-none, 1-whole, 2-from cursor
	my($y_ix,$y_ix0,$y_len,$wd,$ht,$col,$col0);
	my($term);
	my($init_in,$op);

	$off = 0 if( !defined $off );
	*hist = *main::Hist if( ! defined @hist );
	$isp = ++$#hist ;
	$wisp = $isp;
	if( -f "/vmunix" ) {
		&main::stty("-echo -icanon min 1 time 0 stop ''");
	}else {
		&main::stty(" -echo -icanon eol \001 stop ''");
	}
	($ht,$wd) = &termsize();
	$y_ix = $y_len = 0;
	$mode = 'normal';
	another_line();
	$print_line = 1;

	IN_STACK:while(1){

		  if( $print_line==0 ) {
			  #just move cursor
			  ($y_len,$y_ix,$col,$adjust) =
				&main::calcxy($mode,$prompt,$len,$ix,$off,$wd);
			  move_cursor( $col,$y_ix, $col0,$y_ix0);

		  }elsif($print_line==2 || $print_line==3 ) {
			  # delete - print cursor to eol
			  $len = length($hist[$#hist]);
			  ($y_len,$y_ix,$col,$adjust) =
				&main::calcxy($mode,$prompt,$len,$ix,$off,$wd);

			  if( $print_line==3 ) {
				  # delete backward
				  move_cursor( $col,$y_ix, $col0,$y_ix0);
			  }

			  if( $y_len0 > $y_ix && ($adjust || $y_len0 > $y_len) ) {
				  print( OUT "\n$TERM_EEOL" x ($y_len0-$y_ix),
						 $TERM_UP x ($y_len0-$y_ix),
						 "\r", $TERM_RIGHT x $col,  );
			  }
			  print( OUT substr("$prompt$hist[$#hist]", $ix),
					 $adjust ? '':$TERM_EEOL,
					 "\r", $TERM_RIGHT x $col,
					 $TERM_UP x ($y_len-$y_ix) ,
					 ($adjust && $ix!=$len)? $TERM_DOWN : '' );


		  }elsif($print_line==4) {
			  # insert
			  $len = length($hist[$#hist]);
			  ($y_len,$y_ix,$col,$adjust) =
				&main::calcxy($mode,$prompt,$len,$ix,$off,$wd);

			  print( OUT substr("$prompt$hist[$#hist]", $ix),
					 $TERM_UP x ($y_len-$y_ix) ,"\r", $TERM_RIGHT x $col,
					 $TERM_DOWN x $adjust );

		  }else{
			  # print whole line
			  $len = length($hist[$#hist]);
			  #move cursor to bol on screen, erase prev printout
			  print (OUT $TERM_DOWN x ($y_len-$y_ix),
					 "\r$TERM_EEOL$TERM_UP" x ($y_len),
					 "\r$TERM_EEOL\r",
					 $TERM_RIGHT x $off,"$prompt$hist[$#hist]");
			  ($y_len,$y_ix,$col,$adjust) =
				&main::calcxy($mode,$prompt,$len,$ix,$off,$wd);

			  #mv cursor to cur pos
			  print( OUT $TERM_UP x ($y_len-$y_ix) ,"\r", $TERM_RIGHT x $col,
					 $TERM_DOWN x $adjust);
		  }


    	GETC:{
			  ($col0, $y_ix0, $y_len0) = ($col, $y_ix, $y_len);
			  $print_line=1;

			  $c = main::getchar();
			  while($Key{$c} eq "prefix" ) {
				  $c .= main::getchar();
			  }

			  ($op = $Key{$c}) =~ s/(.*)\s*[\(;].*/$1/;
			  $op =~ /(\w+)$/;
			  $op = $1;

			if( $Key{$c} =~ /ign_mode/ ) {
				# ignore mode and execute command
				eval "&{$Key{$c}}";
			}elsif( $mode =~ /^search/ ) {
			  main::search_mode(*c,*s,*prompt,*mode,*isp, *hist);
				another_line();
			}elsif( $c eq $Key{EOF} && $len==0 ) {
				return '';	# eof return null
			}elsif( defined $Key{$c} ) {
				eval "&{$Key{$c}}";
				$lastop{op} = $op;
			}elsif( ord ($c) < 32 ) {
				#undefined control char
				print OUT "\a";
				$print_line = 0;
			}else {
				$lastop{op} = 'ins_char';
				&ins_char( $c );
				print OUT $c;
			}
			$init_in = 0;
		}
	}

    if( $y_ix != $y_len ) {
		print OUT "\n" x ($y_len-$y_ix);
    }
    &main::stty($org_stty);

	print OUT "\n";
	if( $hist[$#hist] eq '' ) {
		pop(@hist);
		return "\n";
	}
	if( $#hist>0 && $hist[$#hist] eq $hist[$#hist-1] ) {
		pop(@hist);    # if it is the same, delete
	}else{
		&main::add_hist( *hist, *hist );
	}
	$hist[$#hist]."\n";
}

#-----------------------------
# editinig command functions
#
#   functions must be below here to be listed by list_func
#
#   the variables below are local to sub input
#	$prompt,$hist,$app,$off
#	$len,$ix
#	$c,$isp,$wisp,$s
#   $mode
#-----------------------------
sub prefix { } # it's only here to be listed by list_func
sub boh {
	$isp = 0;
	another_line();
}
sub boh_ign_mode {
	boh();
}
sub bol {
	$ix = 0 ;
	$print_line=0;
}
sub bs  {
	my($l) = @_;
	$l = 1 if $l eq '';
	if( $len && $ix ) {
		$ix-=$l;	# mv left
		substr($hist[$#hist],$ix,$l) = "";   # del char
	}
	$print_line = 3;
}
sub del_back_line {
	substr($hist[$#hist],0,$ix) = "";
	$ix = 0;
	$print_line = 3;
}
sub del_forw_line {
	substr($hist[$#hist],$ix) = "";
	$print_line = 2;
}
sub del_char {
	my($l) = @_;
	$l = 1 if $l eq '';
	if( $len > $ix ) {
		substr($hist[$#hist],$ix,$l) = "";   # del char
	}
	$print_line = 2;
}
sub del_line {
	$ix = 0;
	$hist[$#hist] = "";
	$print_line = 3;
}
sub del_back_word {
	my($tmp);
	$tmp = substr($hist[$#hist],0,$ix);
	$tmp =~ s/(^|\S+)\s*$//;
	$tmp = length $tmp;
	substr($hist[$#hist],$tmp,$ix-$tmp) = "";
	$ix = $tmp;
	$print_line = 3;
}
sub del_forw_word {
	$hist[$#hist] =~ s/^(.{$ix})\s*\S+/$1/;
	$print_line = 2;
}
sub enter {
	subst();
	enter_wo_subst();
}
sub eoh          {
	if( $isp==$#hist ) {
		print OUT "\a";
	}else{
		$hist[$#hist] = ''
	}
	$isp = $#hist;
	another_line();
	$print_line = 1;
}
sub eoh_ign_mode {
	eoh();
	$print_line = 1;
}
sub eol {
	$ix = $len;
	$print_line=0;
}
sub execute {
    eval "$hist[$#hist]";
	if( $#hist>0 && $hist[$#hist] eq $hist[$#hist-1] ) {
		pop(@hist);    # if it is the same, delete
	}else{
		&main::add_hist( *hist, *hist );
	}
	push( @hist, ''); # deceive 'input' it is an empty line
	last IN_STACK;
}
sub ins_char {
	my($c) = @_;
	&main::insert_char($c,*len,*ix,*hist);
	$print_line = 4;
}
sub ins_last_word {
	if( $lastop{op} =~ /^ins_(nth|last)_word/ ) {
		return if $wisp < 1;
		#delete last last_word
		bs(length $lastop{word});
	}else {
		$wisp = $#hist;
		return if $wisp < 1;
	}
	$hist[--$wisp] =~ /(\S+)\s*$/;
	$lastop{word} = $1;
	ins_char($lastop{word});
}
sub ins_nth_word {
	my($n) = @_;
	if( $lastop{op} =~ /^ins_(nth|last)_word/ ) {
		return if $wisp < 1;
		#delete last last_word
		bs(length $lastop{word});
	}else {
		$wisp = $#hist;
		return if $wisp < 1;
	}
	$hist[--$wisp] =~ /((\S+)\s*){1,$n}/;
	$lastop{word} = $2;
	ins_char($lastop{word});
}
sub list_func {
	my( $s, @cmds, $cmd, $func);
	$func = 0;
	open( THIS, "$0" ) || return; #shouldn't occur
	while( $s = <THIS> ) {
		if( $s =~ /^\s*sub\s+main::input\s*\{/ ) {
			$func = 1;
			next;
		}
		next if !$func;
		if( $s =~ s/^\s*sub\s+// ) {
			$s =~ s/\s*[\{].*//;
			push @cmds,$s;
		}
	}
	close THIS;
	foreach $cmd (sort @cmds) {
		print OUT $cmd;
	}
}

sub bind {
	# bind Key or Subst
    # if there is no arguments, then list them
	my($hash,$key,$val) = @_;
	my( $mod,$chr,$v2,$k,$cnt );
	if( defined %{$hash} ) {
		$k = $key;
	    if( $hash eq "Key" ) {
			($v2 = $val) =~ s/\s*[\(;].*//;
			if( !defined &{$v2} ) {
				print STDERR "Unknown function $v2\n";
				return;
			}
			$mod = 0; $cnt =0; $k = '';
			for( $i=0; $i<length $key; $i++ ) {
				$chr = substr($key,$i,1);
				if( $chr eq "\\" ) {
					$chr = substr($key,++$i,1);
					if( $chr=~/m/i ) {
						$mod = 0x80;
					}elsif( $chr=~/c/i ) {
						$cnt = 1;
					}elsif( $chr=~/e/i ) {
						$chr = $ESC;
						$chr = pack("c",ord($chr)+$mod);
						$mod = 0 ; $cnt = 0;
						$k .= $chr;
					}else {
						print "Unknown char $key\n";
					}
				}else {
					if( $cnt ) {
						eval "\$chr = \"\\c$chr\" ";
					}
					$chr = pack("c",ord($chr)+$mod);
					$mod = 0 ; $cnt = 0;
					$k .= $chr;
				}
			}
		}
		if( $val eq '' ) {
			delete ${$hash}{$k};
	    }else {
			${$hash}{$k} = $val;
        }


    }else {
		foreach $key (sort(keys(%Key) )){
			$val = $Key{$key};
			$mod = '';
			while( $key =~ s/(.|\s)// ) {
				$chr = $1;
				if( ord($chr) >= 0x80 ) {
					$mod .= '\M';
					$chr = pack("c", ord($chr)-0x80);
				}
				if( $chr eq $ESC ) {
					$chr = '\E';
				}elsif( ord($chr) < 0x20 ) {
					$mod .= '\C';
					$chr = pack("c", ord($chr)+0x40);
				}elsif( ord($chr) == 0x7f ) {
					$chr = '\C?';
				}
				$mod .= $chr;
			}
			if( ord($val) < 0x20 ) {
				$val = '\C'.pack("c", ord($val)+0x40);
			}
			print OUT "Key $mod   $val\n";
		}
        while( ($key,$val) = each(%Subst) ) {
		    print OUT "Subst $key   $val\n";
        }
	}
}
sub next_char {
	$ix++ if ($ix<$len);
	$print_line=0;
}

sub next_line {
	if($isp<$#hist) {
		$isp++;
		if( $isp==$#hist ) {
			$hist[$isp] = '';
		}
	}else {
		$isp = $#hist;
		print OUT "\a";
	}
	another_line();
}

sub next_word {
	$hist[$#hist] =~ /^(.{$ix}\S*(\s+|$))/;
	$ix = length($1);
	$print_line=0;
}

sub enter_wo_subst {
	last IN_STACK;
}

sub prev_char {
	$ix-- if $ix>0;
	$print_line=0;
}

sub prev_line {
	if($isp>0) {
		$isp--;
	}else {
		$isp = 0;
		print OUT "\a";
	}
	another_line();
}

sub prev_word {
	my($tmp);
	$tmp = substr($hist[$#hist],0,$ix);
	$tmp =~ s/(^|\S+)\s*$//;
	$ix = length($tmp);
	$print_line=0;
}

sub cancel {
	$hist[$#hist] = "";
	$len = 0;
	last IN_STACK;
}
sub quote {
	my($c);
	sysread(IN, $c, 1);
#	$c = getc(IN);
	ins_char($c);
}

sub search_rev {
	$s = '';
	$mode = 'search_rev';
	$p_save = $prompt;
	$prompt = "($mode)'$s':";
	$hist[$#hist] = $hist[$isp];
	another_line();
}

sub search {
	$s = '';
	$mode = 'search';
	$p_save = $prompt;
	$prompt = "($mode)'$s':";
	$hist[$#hist] = $hist[$isp];
	another_line();
}

sub subst {
	my($key,$val);
	$done = 0;
	while( ($key,$val) = each(%Subst) ) {
		last if( eval "\$hist[\$#hist] =~ s\$key$val" ) ;
	}
	$ix = $len = length($hist[$#hist]);
}

sub termsize {
	my($row, $col,$s);
	if( -f "/vmunix" ) {
		$s =&main::stty ("everything");
		($row,$col) = ($s =~ /(\d+)\s+rows[,\s]+(\d+)\s+columns/ );
	} else {
		$s =&main::stty ("-a");
		($row,$col) = ($s =~ /rows[=\s]+(\d+)[,;\s]+columns[=\s]+(\d+)/ );
	}
	($row,$col);
}

