#!/usr/bin/perl -w

# $Id$
#
# This script is used to generate HTML for the "allCommands.html" page.
# ./genAllCommands.pl cmds.txt

my $n=0;
my $bHTML = 1; # change me to 0 to set ASCII output.
my @obsolete = ('ColorLimit', 'GlobalOpts', 'HilightColor', 'HilightColorset', 'IconFont', 'IconPath', 'PixmapPath', 'Recapture', 'RecaptureWindow', 'WindowShadeAnimate', 'WindowFont', 'WindowsDesk');
# @deprecated is not currently used.
my @deprecated = ('AddToDecor', 'ChangeDecor', 'DestroyDecor', 'UpdateDecor');
while (<>)
{
	if (/^--$/)
	{
		$n++;
	}
	else
	{
		chop();
		push(@{$a[$n]}, $_);
	}
}

my $maxRows = -1;
for (my $i = 0; $i <= $n; $i++)
{
	my $rows = scalar(@{$a[$i]});
	$maxRows = $rows if ($rows > $maxRows);
	printf("column $i has $rows items.\n");
	foreach (@{$a[$i]})
	{
# printf(" $_\n");
	}
}
printf("maxRows=$maxRows\n");

for (my $r = 0; $r < $maxRows; $r++)
{
	printf("<tr>\n") if ($bHTML);
	for (my $i = 0; $i <= $n; $i++)
	{
		my ($cmd, $prefix, $eek, $bObsolete) = ('n/a', '', '', 0);
		if (defined $a[$i][$r])
		{
			$cmd = $a[$i][$r];
			my $l = substr($cmd, 0, 1);
			$bObsolete = grep(/^$cmd$/, @obsolete);
			my $c = ($bObsolete ? "<font class=\"obsolete\">$cmd</font>" : $cmd);
			$eek = "><a href=\"commands/$cmd.html\">$c</a";
			if (!defined $letter{$l})
			{
				if ($bHTML)
				{
					$prefix = " align=\"right\"><b>$l&nbsp;</b";
				}
				else
				{
					$prefix = $l
				}
			}
			$letter{$l} = 1;
		}


		if (!$bHTML)
		{
			$cmd .= '*' if ($bObsolete);
			printf("%-2s%-21s ", $prefix, $cmd);
		}
		else
		{
			printf("<td$prefix></td><td$eek></td>\n");
		}
	}
	printf("\n") if (!$bHTML);
	printf("</tr>\n") if ($bHTML);
}



