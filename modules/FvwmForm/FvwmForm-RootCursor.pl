#!/usr/bin/perl

# Usage:
#   perl FvwmForm-RootCursor.pl >FvwmForm-RootCursor

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
		$include .= "*FvwmForm-RootCursor: Line   expand\n"; $count = 0;
	}
	$include .= sprintf("*FvwmForm-RootCursor: Choice cursor_font %-${len}s off \"%-${len}s\"\n", $_, $_);
	$count++;
}
$include =~ s/(left_ptr\s+)off/$1on /s;  # set default cursor

$_ = join('', <DATA>);
s/\@INCLUDE_STANDARD_CURSOR_FONTS@\n/$include/s;
print $_;

# ****************************************************************************
# Here is the actual FvwmForm-RootCursor template goes
__DATA__
# This form changes the root cursor font and color.

DestroyModuleConfig FvwmForm-RootCursor: *
*FvwmForm-RootCursor: WarpPointer
*FvwmForm-RootCursor: Font	fixed
*FvwmForm-RootCursor: ButtonFont	6x13

*FvwmForm-RootCursor: Text	"Cursor font"
*FvwmForm-RootCursor: Selection	cursor_font single

@INCLUDE_STANDARD_CURSOR_FONTS@

*FvwmForm-RootCursor: Line	expand
*FvwmForm-RootCursor: Text	"Cursor inner color"
*FvwmForm-RootCursor: Line	expand

*FvwmForm-RootCursor: Selection	cursor_fg single
*FvwmForm-RootCursor: Line   center
*FvwmForm-RootCursor: Choice cursor_fg black    on  "  black   "
*FvwmForm-RootCursor: Choice cursor_fg red      off "  red     "
*FvwmForm-RootCursor: Choice cursor_fg green    off "  green   "
*FvwmForm-RootCursor: Choice cursor_fg blue     off "  blue    "
*FvwmForm-RootCursor: Choice cursor_fg bisque   off "  bisque  "
*FvwmForm-RootCursor: Choice cursor_fg brown    off "  brown   "
*FvwmForm-RootCursor: Choice cursor_fg gray     off "  gray    "
*FvwmForm-RootCursor: Line   center
*FvwmForm-RootCursor: Choice cursor_fg cyan     off "  cyan    "
*FvwmForm-RootCursor: Choice cursor_fg violet   off "  violet  "
*FvwmForm-RootCursor: Choice cursor_fg seagreen off "  seagreen"
*FvwmForm-RootCursor: Choice cursor_fg navy     off "  navy    "
*FvwmForm-RootCursor: Choice cursor_fg gold     off "  gold    "
*FvwmForm-RootCursor: Choice cursor_fg yellow   off "  yellow  "
*FvwmForm-RootCursor: Choice cursor_fg white    off "  white   "

*FvwmForm-RootCursor: Line	expand
*FvwmForm-RootCursor: Text	"Cursor outer color"
*FvwmForm-RootCursor: Line	expand

*FvwmForm-RootCursor: Selection	cursor_bg single
*FvwmForm-RootCursor: Line   center
*FvwmForm-RootCursor: Choice cursor_bg black    off "  black   "
*FvwmForm-RootCursor: Choice cursor_bg red      off "  red     "
*FvwmForm-RootCursor: Choice cursor_bg green    off "  green   "
*FvwmForm-RootCursor: Choice cursor_bg blue     off "  blue    "
*FvwmForm-RootCursor: Choice cursor_bg bisque   off "  bisque  "
*FvwmForm-RootCursor: Choice cursor_bg brown    off "  brown   "
*FvwmForm-RootCursor: Choice cursor_bg gray     off "  gray    "
*FvwmForm-RootCursor: Line   center
*FvwmForm-RootCursor: Choice cursor_bg cyan     off "  cyan    "
*FvwmForm-RootCursor: Choice cursor_bg violet   off "  violet  "
*FvwmForm-RootCursor: Choice cursor_bg seagreen off "  seagreen"
*FvwmForm-RootCursor: Choice cursor_bg navy     off "  navy    "
*FvwmForm-RootCursor: Choice cursor_bg gold     off "  gold    "
*FvwmForm-RootCursor: Choice cursor_bg yellow   off "  yellow  "
*FvwmForm-RootCursor: Choice cursor_bg white    on  "  white   "

*FvwmForm-RootCursor: Line	expand
*FvwmForm-RootCursor: Line	expand

*FvwmForm-RootCursor: Button	continue "   Set Root Cursor   "
*FvwmForm-RootCursor: Command	CursorStyle ROOT $(cursor_font!none) $(cursor_fg) $(cursor_bg)
*FvwmForm-RootCursor: Button	quit "   Finish   " ^[
*FvwmForm-RootCursor: Command	Nop
