# This script must be run by FvwmScript-Setup95 only!

use Getopt::Long;

my $uncomment=0;
my $comment=0;
my $prefapps=0;
my $gnomepath=0;
my $mainOut="";
my $currentOut="";
my @PATH_DIRS = split(':',$ENV{PATH});
my $date=`date +%Y%m%d-%H%M%S`;
$date=~ s/\s//g;
my $backupDir="";

GetOptions(
	"help"         => \&wrongUsage,
	"configin=s"	=> \$ConfigIn,
	"dirout=s"		=> \$DirOut,
	"fvwm=i"       => \$Fvwm,
	"sxs=i"			=> \$Slow,
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
	"gy=i"       	=> \$GnomeY,
	"gpath=s"      => \$GnomePath,
	"cde=i"			=> \$Cde,
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

if ($ConfigIn eq "" || $DirOut eq "" || $Fvwm eq "" || $Slow eq "" ||
		$KdeMenu eq "" || $KdeSysMenu eq "" || $KdeUserMenu eq "" ||  
		$KfmIcons eq "" ||  $GnomeMenuGtk eq "" || 
		$GnomeSysMenuGtk eq "" || $GnomeUserMenuGtk eq "" ||
		$GnomeRHMenuGtk eq "" || $GnomeSysMenuFvwm eq "" || 
		$GnomeUserMenuFvwm eq "" || $GnomeRHMenuFvwm eq "" ||
		$GnomeSession eq "" || $GnomeY eq "" || $GnomePath eq "" ||
		$Stroke eq "" || $Laptop eq "" || $PanelStaysPut eq "" || 
		$Editor eq "" || $Reditor eq "" || $FileMgr eq "" || 
		$Term eq "" || $SoundPlayer eq "" || $SoundPath eq "" ||
		$IP eq "" || $ImagePath	eq "" || $DefImagePath	eq "" ||
		$Cde eq "") {
	wrongUsage();
}

if (! $SoundPath =~ /\/$/) { $SoundPath = "$SoundPath/"; }
$fileOut=".fvwm2rc";
$mainOut="$DirOut/$fileOut";
$currentOut=$mainOut;
$backupDir="$DirOut/backup";

open(IN,"$ConfigIn") || die "cannot read $ConfigIn";

if (-f "$mainOut") {
	system("/bin/mkdir -p '$backupDir'");
	$backup="$backupDir/$fileOut-$date";
	system("/bin/mv '$mainOut' '$backup'");
	print "Echo backup $mainOut in $backup\n"
}
open(MAINOUT,">$mainOut") || die "cannot write on $mainOut";

while (<IN>) {
	$line = $_;
	chomp($line);

	next if ($line =~ /^\#\!D/);

	$line =~ s/^\#// if $uncomment;
	$line = "\#$line" if $comment;
	$uncomment-- if ($uncomment > 0);
	$comment-- if ($comment > 0);

	if ((/^\#WIN/ && $Fvwm)||(/^\#FAST/ && $Slow)) {
		@l=split(' ',$line);
		$comment=$l[1];
	}
	

	if (($line =~ /^\#FVWM/ && $Fvwm) ||
			($line =~ /^\#SLOW/ && $Slow) ||
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
			($line =~ /^\#PANEL_PUT/ && $PanelStaysPut) ||
			($line =~ /^\#CDE/ && $Cde))
	{
		@l=split(' ', $line);
		$uncomment = $l[1];
	}

	if ($prefapps) {
		$line =~ s# emacs# $Editor#;
		$line =~ s#\^"emacs"#\^"$Editor"#;
		$line =~ s#\*macs#$Reditor#;
		$line =~ s# xfm# $FileMgr#;
		$line =~ s# xterm# $Term#;
	}
	$prefapps-- if $prefapps;
	if ($line =~ /^\#PREF/) {
		@l=split(' ',$line);
		$prefapps=$l[1];
	}

	if ($currentOut =~ /menus/ && $line =~ /^\+/) {
		$line = menuCheck($line);
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
	
	if ($line =~ /^(.*?)\s+#!([Ee])\s+(.*?)\s*$/) {
		$line = $1;
		my $isApp = $2 eq "E";
		my @files = split(':', $3);
		my $found = 0;
		foreach (@files) { $found = 1 if $isApp? checkApp($_): -e; }
		$line = "#$line" unless $found;
	}

	if ($line =~ /^\#SEG/) {
		$line =~ s/^\#SEG//;
		$line =~ s/\s//g;
		$currentOut=$line;
		if ($line eq "END") {
			$currentOut=$mainOut;
			close(OUT);
		} else {
			print MAINOUT "Read $line\n\n";
			close(OUT) if ($currentOut ne $mainOut);
			$currentOut="$DirOut/$line";
			$fileOut=$line;
			if (-f "$currentOut") {
				system("/bin/mkdir -p '$backupDir'");
				$backup="$backupDir/$fileOut-$date";
				system("/bin/mv '$currentOut' '$backup'");
				print "Echo backup $currentOut in $backup\n";
			}
			open(OUT,">$currentOut") || die "cannot write on $currentOut";
		}
	} else {
		if ($currentOut ne $mainOut) {
			print OUT "$line\n";
		} else {
			print MAINOUT "$line\n";
		}
	}
}

close(IN);
close(MAINOUT);

print "Echo END\n";

#--------------------------------------------------------------------------
# menu check

sub menuCheck {
	my ($menuline) =  @_;
	my $path = "";

	return $menuline unless $menuline =~ /([Ee]xec|"\s+Restart)\s+(.+)$/;
	my $action = $2;
	
	if ($menuline =~ /Exec\s+cd/) {
		$path = substr($menuline,rindex($menuline,'Exec cd'));
		$path =~  s/^\s*Exec\s+cd\s*//;
		$path = substr($action,0,index($path,';'));
		$path =~ s/\s//g;
		$path =~ s/\/$//;
	}
	$action =~ s/^exec\s+//;
	$action =~ s/^killall\s+//;
	my $tact = $action;
	$tact = substr($tact,0,index($tact,' ')) if ($tact =~ /\s/);
	if ($tact eq "xterm" && $menuline =~ /\-e/) {
		$action = substr($action,index($action,'-e'));
		$action =~ s/^\-e\s+//;
		$action = substr($action,0,index($action,' ')) if ($action =~ /\s/);
	} else {
		$action = $tact;
	}
	$action =~ s/^\s*\.\///;
	$action = "$path/$action" if ($path ne "");
	$menuline = "#".$menuline if (!checkApp($action));
	return $menuline;
}

sub checkApp {
	my($app) = @_;
	my $dir ="";
	if ($app =~ /^\//) {
		if (-x $app ) { return 1 }
	} else {
		foreach $dir (@PATH_DIRS) {
			if ( -x "$dir/$app" ) { return 1 }
		}
	}
	return 0;
}

sub wrongUsage {
	print STDERR "This script must be run by FvwmScript-Setup95 only!\n";
	exit -1;
}
