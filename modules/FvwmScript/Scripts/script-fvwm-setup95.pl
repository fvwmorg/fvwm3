# This script must be run by ScriptFvwmSetup95 only!

use Getopt::Long;

my $uncomment=0;
my $comment=0;
my $prefapps=0;
my $gnomepath=0;

GetOptions(
	"help"         => \&wrongUsage,
   "configin=s"	=> \$ConfigIn,
   "configout=s"	=> \$ConfigOut,
	"fvwm=i"       => \$Fvwm,
	"km=i"			=> \$KdeMenu,
	"ksm=i"       	=> \$KdeSysMenu,
	"kum=i"			=> \$KdeUserMenu,
	"kfm=i"   		=> \$KfmIcons,
	"gmg=i"  		=> \$GnomeMenuGtk,
	"gsmg=i"   		=> \$GnomeSysMenuGtk,
	"gumg=i"       => \$GnomeUserMenuGtk,
	"grmg=i"      	=> \$GnomeRHMenuGtk,
	"gsmf=i"   		=> \$GnomeSysMenuFvwm,
	"gumf=i"       => \$GnomeUserMenuFvwm,
	"grmf=i"      	=> \$GnomeRHMenuFvwm,
	"gs=i"      	=> \$GnomeSession,
,	"gy=i"       	=> \$GnomeY,
,	"gpath=s"      => \$GnomePath,
	"stroke=i"     => \$Stroke,
	"lap=i"      	=> \$Laptop,
	"pp=i"  			=> \$PanelStaysPut,
	"editor=s" 		=> \$Editor,
	"reditor=s" 	=> \$Reditor,
	"file=s" 		=> \$FileMgr,
	"term=s" 		=> \$Term,
	"spl=s"			=> \$SoundPlayer,
	"spa=s"			=> \$SoundPath,
	"i=i"				=> \$IP,
	"ip=s"			=> \$ImagePath,
	"dip=s"			=> \$DefImagePath
) || wrongUsage();

if ($ConfigIn eq "" || $ConfigOut eq "" || $Fvwm eq "" ||
		$KdeMenu eq "" || $KdeSysMenu eq "" || $KdeUserMenu eq "" ||  
		$KfmIcons eq "" ||  $GnomeMenuGtk eq "" || 
		$GnomeSysMenuGtk eq "" || $GnomeUserMenuGtk eq "" ||
		$GnomeRHMenuGtk eq "" || $GnomeSysMenuFvwm eq "" || 
		$GnomeUserMenuFvwm eq "" || $GnomeRHMenuFvwm eq "" ||
		$GnomeSession eq "" || $GnomeY eq "" || $GnomePath eq "" ||
		$Stroke eq "" || $Laptop eq "" || $PanelStaysPut eq "" || 
		$Editor eq "" || $Reditor eq "" || $FileMgr eq "" || 
		$Term eq "" || $SoundPlayer eq "" || $SoundPath eq "" ||
		$IP eq "" || $ImagePath	eq "" || $DefImagePath	eq "") {
	wrongUsage();
}

if (! $SoundPath =~ /\/$/) { $SoundPath = "$SoundPath/"; }

open(IN,"$ConfigIn");
open(OUT,">$ConfigOut");
while(<IN>) {
	$line=$_;
	chomp($line);

	$line =~ s/^\#// if $uncomment;
   $line = "\#$line" if $comment;
   $uncomment-- if ($uncomment > 0);
   $comment-- if ($comment > 0);

   if (/^\#WIN/ && $Fvwm) {
		@l=split(' ',$line);
		$comment=$l[1];
	}

	if(($line =~ /^\#FVWM/ && $Fvwm) || 
			($line =~ /^\#K_M/ && $KdeMenu) || 
			($line =~ /^\#K_SM/&& $KdeSysMenu) || 
			($line =~ /^\#K_UM/ && $KdeUserMenu) ||
			($line =~ /^\#KFM_ICONS/ && $KfmIcons) || 
			($line =~ /^\#G_M_G/ && $GnomeMenuGtk) ||
			($line =~ /^\#G_SM_G/ && $GnomeSysMenuGtk) || 
			($line =~ /^\#G_UM_G/ && $GnomeUserMenuGtk) || 
			($line =~ /^\#G_RHM_G/ && $GnomeRHMenuGtk) || 
			($line =~ /^\#G_SM_F/ && $GnomeSysMenuFvwm) || 
			($line =~ /^\#G_UM_F/ && $GnomeUserMenuFvwm) || 
			($line =~ /^\#G_RHM_F/ && $GnomeRHMenuFvwm) || 
			($line =~ /^\#G_SESSION/ && $GnomeSession) || 
			($line =~ /^\#G_ST/ && $GnomeY) || 
			($line =~ /^\#STROKE/ && $Stroke) ||
			($line =~ /^\#LAPTOP/ && $Laptop) || 
			($line =~ /^\#PANEL_PUT/ && $PanelStaysPut)) {
		@l=split(' ',$line);
		$uncomment=$l[1];
	}

	if ($prefapps) {
		$line =~ s# emacs# $Editor#;
		$line =~ s#\^emacs#\^$Editor#;
		$line =~ s#\*macs#$Reditor#;
		$line =~ s#xfm#$FileMgr#;
		$line =~ s#xterm#$Term#;
	}
	$prefapps-- if $prefapps;
	if ($line =~ /^\#PREF/) {
		@l=split(' ',$line);
		$prefapps=$l[1];
	}

	$line =~ s#rplay#$SoundPlayer# if ($line =~ /Event/);
	$line =~ s#/usr/share/sounds/#$SoundPath# if ($line =~ /Event/);

	$line =~ s#/usr#$GnomePath# if($gnomepath);
	$gnomepath-- if $gnomepath;
	$gnomepath=1 if ($line =~ /^\#G/);

	if ($line =~ /add_dir_to_fvwm_image_path/ && $IP) {
		$line =~ s/^\#//;
		$line =~ s#add_dir_to_fvwm_image_path#$ImagePath#;
	}
	$line =~ s#/usr/include/X11/bitmap:/usr/include/X11/pixmaps#$DefImagePath# 
		if ($line =~ /\/usr\/include\/X11\/bitmap:\/usr\/include\/X11\/pixmaps/);

	print OUT "$line\n";
}

close(IN);
close(OUT);

print "Echo END\n";

sub wrongUsage {
	print STDERR "This script must be run by ScriptFvwmSetup95 only!\n";
	exit -1;
}
