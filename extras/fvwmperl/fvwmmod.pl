# fvwmmod.pl 1.1 - perl routines for implementing fvwm modules
# (c)1996 Dan Astoorian <djast@utopia.csas.com>

########################################################################
# Global values:

########################################################################
# $M_...: fvwm-to-module packet types:
#
$M_NEW_PAGE =		(1);
$M_NEW_DESK =		(1<<1);
$M_ADD_WINDOW =		(1<<2);
$M_RAISE_WINDOW =	(1<<3);
$M_LOWER_WINDOW =	(1<<4);
$M_CONFIGURE_WINDOW =	(1<<5);
$M_FOCUS_CHANGE =	(1<<6);
$M_DESTROY_WINDOW =	(1<<7);
$M_ICONIFY =		(1<<8);
$M_DEICONIFY =		(1<<9);
$M_WINDOW_NAME =	(1<<10);
$M_ICON_NAME =		(1<<11);
$M_RES_CLASS =		(1<<12);
$M_RES_NAME =		(1<<13);
$M_END_WINDOWLIST =	(1<<14);
$M_ICON_LOCATION =	(1<<15);
$M_MAP =		(1<<16);
$M_ERROR =		(1<<17);
$M_CONFIG_INFO =	(1<<18);
$M_END_CONFIG_INFO =	(1<<19);
$M_ICON_FILE =		(1<<20);
$M_DEFAULTICON =	(1<<21);
$M_STRING =		(1<<22);
$M_MINI_ICON =		(1<<23);
$M_WINDOWSHADE =	(1<<24);
$M_DEWINDOWSHADE =	(1<<25);

########################################################################
# Related constants
$START_FLAG =		0xffffffff;
$MAX_MESSAGES =		26;
$MAX_MASK =		((1<< $MAX_MESSAGES)-1);
$HEADER_SIZE =		4;
$MAX_BODY_SIZE =	24;
$MAX_PACKET_SIZE =	( $HEADER_SIZE+ $MAX_BODY_SIZE );

########################################################################
# $C_...: fvwm-to-module contexts:
$C_NO_CONTEXT =	0;
$C_WINDOW =	1;
$C_TITLE =	2;
$C_ICON =	4;
$C_ROOT =	8;
$C_FRAME =	16;
$C_SIDEBAR =	32;
$C_L1 =		64;
$C_L2 =		128;
$C_L3 =		256;
$C_L4 =		512;
$C_L5 =		1024;
$C_R1 =		2048;
$C_R2 =		4096;
$C_R3 =		8192;
$C_R4 =		16384;
$C_R5 =		32768;
$C_LALL =	($C_L1|$C_L2|$C_L3|$C_L4|$C_L5);
$C_RALL =	($C_R1|$C_R2|$C_R3|$C_R4|$C_R5);
$C_ALL =	($C_WINDOW|$C_TITLE|$C_ICON|$C_ROOT|$C_FRAME|$C_SIDEBAR|
                 $C_L1|$C_L2|$C_L3|$C_L4|$C_L5|$C_R1|$C_R2|$C_R3|$C_R4|$C_R5);

########################################################################
# $P_...: fvwmperl specific constants
$P_STRIP_NEWLINES =	(1);
$P_PACKET_PASSALL =	(1<<1);

########################################################################
# Local parameters:

$FvwmMod'txtTypes = $M_ERROR | $M_CONFIG_INFO | $M_STRING;
$FvwmMod'varTypes =  $M_WINDOW_NAME | $M_ICON_NAME | $M_RES_CLASS |
		     $M_RES_NAME | $M_ICON_FILE | $M_DEFAULTICON;
@FvwmMod'handlerList = ();
$FvwmMod'didInit = 0;
$FvwmMod'options = $P_STRIP_NEWLINES;

%FvwmMod'packetTypes = (
    $M_NEW_PAGE,	"l5",
    $M_NEW_DESK,	"l",
    $M_ADD_WINDOW,	"l24",
    $M_CONFIGURE_WINDOW,"l24",

    $M_LOWER_WINDOW,	"l3",
    $M_RAISE_WINDOW,	"l3",
    $M_DESTROY_WINDOW,	"l3",
    $M_FOCUS_CHANGE,	"l5",
    $M_ICONIFY,		"l7",
    $M_ICON_LOCATION,	"l7",
    $M_DEICONIFY,	"l3",
    $M_MAP,		"l3",

    $M_WINDOW_NAME,	"l3a*",
    $M_ICON_NAME,	"l3a*",
    $M_RES_CLASS,	"l3a*",
    $M_RES_NAME,	"l3a*",

    $M_END_WINDOWLIST,	"",
    $M_ERROR,		"l3a*",
    $M_CONFIG_INFO,	"l3a*",
    $M_END_CONFIG_INFO,	"",

    $M_ICON_FILE,	"l3a*",
    $M_DEFAULTICON,	"l3a*",
    $M_STRING,		"l3a*",
    $M_MINI_ICON,	"l6",
    $M_WINDOWSHADE,	"l1",
    $M_DEWINDOWSHADE,	"l1",
);

sub InitModule {
    local($hwinId, $sendFd, $recvFd, $oldfh, @argv);
    local($fvwmWinId, $fvwmContxt, $fvwmRcfile);
    local(@initMask) = @_;

    @argv=@ARGV;

    @argv >= 5 || die "$0 should only be run from fvwm/fvwm2.\n";
    ($outFd, $inFd, $fvwmRcfile, $hwinId, $fvwmContxt) = 
	splice(@argv,0,5);
    $fvwmWinId  = hex($hwinId);

    open(FvwmMod'FVWMOUT, ">&$outFd");
    open(FvwmMod'FVWMIN,  "<&$inFd");
    $oldfh = select(FvwmMod'FVWMIN); $| = 1; select(FvwmMod'FVWMOUT); $| = 1; select($oldfh);

    @FvwmMod'handlerList = ();
    $FvwmMod'sentEndPkt = 0;
    $FvwmMod'didInit = 1;

    if (@initMask) {
	&SendInfo(0, "Set_Mask $initMask[0]");
    } else {
	# Send a dummy packet, so fvwm will start broadcasting events
	# to us
	&SendInfo(0, "Nop");
    }

    ($fvwmWinId, $fvwmContxt, @argv);
}

sub SendInfo {
    local($len, $cont);
    &InitModule unless ($FvwmMod'didInit);

    die "Wrong # args to SendInfo" if (@_ < 2 || @_ > 3);
    $len = length($_[1]);

    $cont = (@_ > 2) ? $_[2] : 1;
    $FvwmMod'sentEndPkt = 1 unless ($cont);
    print FvwmMod'FVWMOUT pack("lla${len}l", $_[0], $len, $_[1], $cont);
    1;
}

sub ReadPacket {
    local ($got, $header, $packet) = ("","","");
    local ($magic, $type, $len, $timestmp);
    &InitModule unless ($FvwmMod'didInit);

    # header is sizeof(int) * HEADER_SIZE bytes long:
    $got = read(FvwmMod'FVWMIN, $header, 4 * $HEADER_SIZE) || return(-1);
    return(-1) unless ($got == 4 * $HEADER_SIZE);

    ($magic, $type, $len, $timestmp) = unpack("L$HEADER_SIZE", $header);

    die "Bad magic number $magic" unless $magic == $START_FLAG;

    # $len is # words in packet, including header;
    # we need this as number of bytes.
    $len -= $HEADER_SIZE;
    $len *= 4;

    if ($len > 0) {
	read(FvwmMod'FVWMIN, $packet, $len) || return (-1);
    }
    ($len, $packet, $type);
}

sub EndModule {
    if ($FvwmMod'didInit) {
	&SendInfo(0, "Nop", 0) unless ($FvwmMod'sentEndPkt);
	close(FvwmMod'FVWMIN);
	close(FvwmMod'FVWMOUT);
    }
}

sub EventLoop {
    local ($len, $packet, $type, $nlpat);
    &InitModule unless ($FvwmMod'didInit);

    # Is Tk being used?
    if (!defined($INC{'Tk.pm'})) {
	# Tk not in use; use our own event loop
	for(;;) {
	    &ProcessPacket;
	    last if ($FvwmMod'closing);
	}
    } else {
	# This eval is so that the syntax will be valid in Perl4 
	eval <<'PERL5'
	    Tk::fileevent(FvwmMod'FVWMIN, 'readable', \&ProcessPacket);
	    for (;;) {
		DoOneEvent;
		if ($FvwmMod'closing) {
		    last;
		}
	    }
PERL5
    }
}

sub ProcessPacket {
    ($len, $packet, $type) = &ReadPacket;
    if ($len < 0) {
	$FvwmMod'closing = 1;
	return 0;
    };

    $type &= $MAX_MASK;
    @args = unpack($FvwmMod'packetTypes{$type}, $packet);
    # If packet is text-based, strip everything past the first null
    # byte (or newline)
    if ($type & $FvwmMod'txtTypes) {
	$nlpat = ($FvwmMod'options & $P_STRIP_NEWLINES) ? "\n+" : "";
	$args[3] =~ s/$nlpat\0.*//g;
	if (! ($FvwmMod'options & $P_PACKET_PASSALL) ) {
	    splice(@args, 0, 3);
	}
    } elsif ($type & $FvwmMod'varTypes) {
	$args[3] =~ s/\0.*//g;
    }
    if (!eval "@FvwmMod'handlerList ; 1") {
	$FvwmMod'closing = 1;
    }
}

####################################
# Handler maintenance routines:
#
sub AddHandler {
    local($htype, $handler) = @_;
    local($onfail);

    if (defined(&$handler)) {
	++$id;
	$FvwmMod'handlerTable{$id} = 
	    "if(\$type & $htype) { &$handler(\$type, \@args) || return 0 }";
	&GenHandlerList;
	$id;
    } else {
	0;
    }
}

sub GenHandlerList {
    @FvwmMod'handlerList = ();
    foreach (sort {$a<=>$b} keys(%FvwmMod'handlerTable)) {
	push(@FvwmMod'handlerList, $FvwmMod'handlerTable{$_});
    }
}

sub DeleteHandler {
    local($which) = @_;

    if ($which eq "*") {
	&ClearAllHandlers;
    } else {
	delete($FvwmMod'handlerTable{$which}) ? 1 : 0;
	&GenHandlerList;
    }
}

sub ClearAllHandlers {
    undef %FvwmMod'handlerTable;
    undef @FvwmMod'handlerList;
}

sub ListHandlers {
    foreach (sort {$a<=>$b} keys(%FvwmMod'handlerTable)) {
	print "$_:\n\t$FvwmMod'handlerTable{$_}";
    }
}

sub SetModOptions {
    $_[2] = $FvwmMod'options if (defined $_[2]);
    $FvwmMod'options &= $_[0];
    $FvwmMod'options |= $_[1];
}

####################################
# Convenience functions

sub GetConfigInfo {
    local($name, $value, @args);

    &SendInfo(0, "Send_ConfigInfo");

    while (1) {
	($len, $packet, $type) = &ReadPacket;
	last if ($type & $M_END_CONFIG_INFO);
	last if ($len < 0);
	next if (!($type & $M_CONFIG_INFO));

	@args = unpack($FvwmMod'packetTypes{$type}, $packet);
	$_ = $args[3];

	$nlpat = ($FvwmMod'options & $P_STRIP_NEWLINES) ? "\n+" : "";
	($name, $value) = /^\*(\w+)(.*)/;
	$value =~ s/\0*//;
	print "$name :- $value\n";
	$FvwmModOption{$name} = $value;
    }
}


1;
