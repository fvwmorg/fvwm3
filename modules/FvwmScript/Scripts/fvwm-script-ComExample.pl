# should be call by FvwmScript-ComExample as:
#    perl [-w] fvwm-script-ComExample --com-name ComExample-pid
# where pid is the process id of FvwmScript-ComExample

use strict;
use Getopt::Long;

my $comName = "";
my $userHome = $ENV{'HOME'} || "./.";
my $userDir = $ENV{'FVWM_USERDIR'} || "$userHome/.fvwm";
my $scriptName = ($0 =~ m:([^/]+)$:, $1);

GetOptions(
   "help|h"      => \&showHelp,
   "com-name=s"  => \$comName,
) || showHelp();

showHelp() if ($comName eq "");

# the fifos for the communication with FvwmScript
my $outFifo = ".tmp-com-out-" . $comName;
my $inFifo = ".tmp-com-in-"  . $comName;

# extract the the pid of FvwmScript-ComExample
my $comPid = $comName;
$comPid =~ s/ComExample-//;
showHelp() if ($comPid !~ /^\d+$/);

# startup stuff:
# nothing :o)

# go to the communication loop (never return)
comLoop();

#-------------------------------------------------------------------------------
# usage
sub showHelp {
	print "\n";
	print "$scriptName must be run by FvwmScript-ComExample as follow:\n\n";
	print "\tperl $scriptName --com-name ComExample-pid\n\n";
	print "where pid is the process id of FvwmScript-ComExample\n";
	print "\n";
	exit 0;
}

#-------------------------------------------------------------------------------
# the communication loop
sub comLoop {
	my $command = "";
	my $return = "";
	my $count=0;

	chdir($userDir) || die "No FvwmConfigHome $userDir";
	# paranoia
	unlink($outFifo) if -p "$outFifo";
	unlink($inFifo) if -p "$inFifo";

	while(1) {
		# read the command and check every 10 sec if FvwmScript-ComExemple
		# is still alive
		myMakeFifo($inFifo) if ! -p "$inFifo";
		eval {
			local $SIG{ALRM} = \&checkScript;
			alarm(10);
			# block unless FvwmScript write on $inFifo
			open(IN,"$inFifo") || die "cannot open $inFifo";
			alarm(0);
			# read the message
			($command)=(<IN>);
			chomp($command);
			close(IN);
		};
		if ($@ =~ /^cannot/) {
			print STDERR "$comName: cannot read in fifo $inFifo\n";
			unlink("$inFifo") if -p "$inFifo";
			exit(1);
		}
		if ($@ =~ /^NoScript/) {
			print STDERR "$comName: No more FvwmScript-ComExample: exit!\n";
			unlink("$inFifo") if -p "$inFifo";
			exit(0);
		}
		if ($@ =~ /^Script/) {
			next;
		}

		unlink($inFifo);

		# build the answer
		$return = "";
		if ($command eq "startup")
		{
			$return = "fvwm-script-ComExample.pl perl script is up";
		}
		elsif ($command eq "count")
		{
			$count++;
			$return = "Get $count count messages";
		}
		elsif ($command eq "multilines")
		{
			# return an answer ready for the FvwmScript Parse function 
			my @r = ();
			$r[0] = "line1 (" . int(rand 20) . ")";
			$r[1] = "Second Line (" . int(rand 20) . ")";
			$r[2] = "An other Line (" . int(rand 20) . ")";
			$return = buildAnswerForParse(@r);
		}
		# ...etc.
		# --------------------------	
		elsif ($command eq "exit") {
			exit(0);
		}
		else {
			print STDERR "$comName: unknown command $command\n";
			$return = "0";
		}
		
		# send the answer to FvwmScript, we wait 10 sec
		myMakeFifo($outFifo);
		eval {
			local $SIG{ALRM} = sub { die "Timeout" };
			alarm(10);
			# this line block until FvwmScript take the answer
			open(OUT,">$outFifo") || die "$comName: cannot write on fifo $outFifo";
			alarm(0);
			print OUT $return;
			close(OUT);
		};
		if ($@ =~ /cannot/) {
			print STDERR "$comName: cannot write on fifo $outFifo\n";
			unlink($outFifo);
			exit(1);
		}
		if ($@ =~ /Timeout/) {
			print STDERR "$comName: FvwmScript do not read my answer!\n";
		}
		unlink($outFifo);
	} # while
}

#-----------------------------------------------------------------------------
# multi-lines to one line for the Parse FvwmScript function
sub buildAnswerForParse {
	my @r = @_;
	my $out = "";
	my $l = 0;

	foreach (@r) {
		$l = length($_);
		$out .= "0" x (4 - length($l)) . "$l" . $_;
	}
	return $out;
}

#-----------------------------------------------------------------------------
# make a fifo
sub myMakeFifo {
	my ($fifo) = @_;
	system("mkfifo '$fifo'");
}

#----------------------------------------------------------------------------
# An alarm handler to check if FvwmScript-ComExample is still alive
sub checkScript {

	die "Script" unless ($comPid);

	my $test = 0;

	$test = 1 if kill 0 => $comPid;

	if ($test) { die "Script"; }
	else {
		unlink($outFifo) if -p "$outFifo";
		unlink($inFifo) if -p "$inFifo";
		die "NoScript";
	}
}

#-----------------------------------------------------------------------------
# For killing the FvwmScript-ComExample if an error happen in this script
END {
	if ($?) {
		my $message = "$scriptName: internal error $?\n";
		$message .= "\tOS error: $!\n" if $!;
		# actually the following is never executed on unix
		$message .= "\tOS error 2: $^E\n" if $^E && !($! && $^E eq $!);

		unlink($outFifo) if -p "$outFifo";
		unlink($inFifo) if -p "$inFifo";
		if ($comPid) {
			kill(9, $comPid);
			$message .= "\tkilling FvwmScript-ComExample";
		}
		print STDERR "$message\n";
	}
}
