#!/usr/bin/perl

# Usage:
#   perl FormFvwmRootCursor.pl >FormFvwmRootCursor.

$CHOICES_IN_LINE = 5;
$CURSORFONT_H = "/usr/X11/include/X11/cursorfont.h";

die "Can't find cursorfont.h, tried '$CURSORFONT_H'.\n"
  unless -r $CURSORFONT_H;

open(CFH, "<$CURSORFONT_H");
$cfContent = join('', <CFH>);
close(CFH);

@cursorFonts = $cfContent =~ /XC_([^\s]*)\s/sg;
@cursorFonts = grep(!/num_glyphs/, @cursorFonts);

$len = 0; map { $len = length if $len < length } @cursorFonts;
$include = "";
$count = $CHOICES_IN_LINE;

foreach (@cursorFonts) {
  if ($count == $CHOICES_IN_LINE) {
    $include .= "*FormFvwmRootCursor.Line   expand\n"; $count = 0;
  }
  $include .= sprintf("*FormFvwmRootCursor.Choice cursor_font %-${len}s off \"%-${len}s\"\n", $_, $_);
  $count++;
}
$include =~ s/(left_ptr\s+)off/$1on /s;  # set default cursor

$_ = join('', <DATA>);
s/\@INCLUDE_STANDARD_CURSOR_FONTS@\n/$include/s;
print $_;

# ============================================================================
# Here is the actual FormFvwmRootCursor. template goes
__DATA__
# Front end to xsetroot, sets root cursor font and color.

DestroyModuleConfig FormFvwmRootCursor.*
*FormFvwmRootCursor.WarpPointer
#*FormFvwmRootCursor.Font	fixed
*FormFvwmRootCursor.ButtonFont	6x13
#*FormFvwmRootCursor.Fore	yellow
#*FormFvwmRootCursor.Back	navy
#*FormFvwmRootCursor.ItemFore	black
#*FormFvwmRootCursor.ItemBack	#20D020

*FormFvwmRootCursor.Text	"Cursor font"
*FormFvwmRootCursor.Selection	cursor_font single

@INCLUDE_STANDARD_CURSOR_FONTS@

*FormFvwmRootCursor.Line	expand
*FormFvwmRootCursor.Text	"Cursor inner color"
*FormFvwmRootCursor.Line	expand

*FormFvwmRootCursor.Selection	cursor_fg single
*FormFvwmRootCursor.Line   center
*FormFvwmRootCursor.Choice cursor_fg black    on  "  black   "
*FormFvwmRootCursor.Choice cursor_fg red      off "  red     "
*FormFvwmRootCursor.Choice cursor_fg green    off "  green   "
*FormFvwmRootCursor.Choice cursor_fg blue     off "  blue    "
*FormFvwmRootCursor.Choice cursor_fg bisque   off "  bisque  "
*FormFvwmRootCursor.Choice cursor_fg brown    off "  brown   "
*FormFvwmRootCursor.Choice cursor_fg gray     off "  gray    "
*FormFvwmRootCursor.Line   center
*FormFvwmRootCursor.Choice cursor_fg cyan     off "  cyan    "
*FormFvwmRootCursor.Choice cursor_fg violet   off "  violet  "
*FormFvwmRootCursor.Choice cursor_fg seagreen off "  seagreen"
*FormFvwmRootCursor.Choice cursor_fg navy     off "  navy    "
*FormFvwmRootCursor.Choice cursor_fg gold     off "  gold    "
*FormFvwmRootCursor.Choice cursor_fg yellow   off "  yellow  "
*FormFvwmRootCursor.Choice cursor_fg white    off "  white   "

*FormFvwmRootCursor.Line	expand
*FormFvwmRootCursor.Text	"Cursor outer color"
*FormFvwmRootCursor.Line	expand

*FormFvwmRootCursor.Selection	cursor_bg single
*FormFvwmRootCursor.Line   center
*FormFvwmRootCursor.Choice cursor_bg black    off "  black   "
*FormFvwmRootCursor.Choice cursor_bg red      off "  red     "
*FormFvwmRootCursor.Choice cursor_bg green    off "  green   "
*FormFvwmRootCursor.Choice cursor_bg blue     off "  blue    "
*FormFvwmRootCursor.Choice cursor_bg bisque   off "  bisque  "
*FormFvwmRootCursor.Choice cursor_bg brown    off "  brown   "
*FormFvwmRootCursor.Choice cursor_bg gray     off "  gray    "
*FormFvwmRootCursor.Line   center
*FormFvwmRootCursor.Choice cursor_bg cyan     off "  cyan    "
*FormFvwmRootCursor.Choice cursor_bg violet   off "  violet  "
*FormFvwmRootCursor.Choice cursor_bg seagreen off "  seagreen"
*FormFvwmRootCursor.Choice cursor_bg navy     off "  navy    "
*FormFvwmRootCursor.Choice cursor_bg gold     off "  gold    "
*FormFvwmRootCursor.Choice cursor_bg yellow   off "  yellow  "
*FormFvwmRootCursor.Choice cursor_bg white    on  "  white   "

*FormFvwmRootCursor.Line	expand
*FormFvwmRootCursor.Line	expand

*FormFvwmRootCursor.Button	continue "   Set Root Cursor   "
*FormFvwmRootCursor.Command	CursorStyle ROOT $(cursor_font!none) $(cursor_fg) $(cursor_bg)
*FormFvwmRootCursor.Button	quit "   Finish   " ^[
*FormFvwmRootCursor.Command	Nop
