#!/bin/sh

# Test shell for FvwmButtons.

# If you don't have the FvwmCommand server running, first start it:

#    Module FvwmCommandS

# Then, run this shell.

# Args are:
# -d debug by echoing commands instead of issuing them.
# -k do not kill FvwmButtons if script is interrupted with ctrl-c
# -t trace exection
# -s run slowly.  Needed on very slow connections.
# -S specify a delay between the test steps. -S 1 is the default, -S 20 is
#    equivalent to -s
# -o just operate the 4 button setup once.
# -c just create the window, don't operate it or kill it.

#  Modification History

#  Created on 03/02/01 by Dan Espen (dane):
#  - Tests for FvwmButtons.


# Base unit in seconds for delay
DEFAULT_DELAY=1
DEFAULT_SLOWNESS_FACTOR=20

# Function for running an Fvwm command:
FC () {
  $DEBUG FvwmCommand "$*"
  $TALK "`date '+%m/%d/%y %H:%M:%S'` FvwmCommand $*"
}

# This doesn't really do it for me...comment out for now
# was being used at the beginning of Operate.
# Draw attention to the pointer
# Pointer_Attn () {
#   # Can't use FC here, tracing destroys the effect.
#   x=1
#   while [ $x -lt 40 ] ; do
#    FvwmCommand "WindowId root WarpToWindow $x $x"
#     x=`expr $x + 4`
#   done
#   while [ $x -gt 10 ] ; do
#    FvwmCommand "WindowId root WarpToWindow $x $x"
#     x=`expr $x - 2`
#   done
# }

trap_sigint () {
  FC KillModule FvwmButtons FButt
  exit 0
}

# Function to insert a pause
Delay () {
  if [ x$1 = x ] ; then
    DELAY=$DEFAULT_DELAY
  else
    DELAY=$1
  fi
  sleep `expr $DELAY \* $SLOW`
}

# Push a button
# arg 1 row
# arg 2 col
# global button_size gives multiplier
Push () {
  Delay
  FC 'Next (FButt) WarpToWindow 20p 20p'
  FC "CursorMove `expr $button_size \* $1`p `expr $button_size \* $2`p"
  FC 'FakeClick Depth 4 press 1 wait 300 release 1'
}
# Push all the buttons,
# uses globals:
# button_size for distance to move
# rows for number of rows of buttons
# cols for number of cols of buttons
Operate () {
  rowc=0
  colc=0
  while [ $rowc -lt $rows -a $colc -lt $cols ] ; do
    Push $rowc $colc
    rowc=`expr $rowc + 1`
    if [ $rowc -eq $rows ] ; then
      colc=`expr $colc + 1`
      rowc=0
    fi
  done
}

TALK=true
SLOW=1
DO_SOME=n
KILL=1
CREATE_ONLY=n
while getopts cdkosS:t1234 arg ; do
  case $arg in
    1)
      DO_1=y
      DO_SOME=y;;
    2)
      DO_2=y
      DO_SOME=y;;
    3)
      DO_3=y
      DO_SOME=y;;
    4)
      DO_4=y
      DO_SOME=y;;
    c)
      CREATE_ONLY=y;;
    d)
      DEBUG=echo;;
    k)
      KILL=0;;
    o)
      Operate
      exit 0;;
    s)
      SLOW=$DEFAULT_SLOWNESS_FACTOR;;
    S)
      SLOW=$OPTARG;;
    t)
      TALK=echo;;
    *)
      # usage would be nice here...
      exit 1;;
  esac
done

if [ $KILL = 1 ] ; then
  trap trap_sigint 2
fi

# Create shortcut function to create a window
# arg 0, window name
FC DestroyFunc W
FC AddToFunc W I Exec xterm -font 10x20 -name '\$0' -g 10x10-20000-20000
# On my system, the window width is 104, the border width is 6
ww=120

# Setup window style for FvwmButtons
FC Style FButt CirculateHit, neverfocus, mousefocusclickraisesoff, HandleWidth 6
FC Style FButt NoTitle
# Setup window styles for the 4 slideout panels:
FC 'Style b* HandleWidth 6, NoTitle'

if [ "$DO_SOME" = "n" -o "$DO_1" = "y" ] ; then
  # Create a button box with 4 panels, one for each direction
  button_size=32
  rows=2
  cols=2
  FC destroymoduleconfig FButt: \'*\'
  # Somewhere near the middle of the screen:
  FC '*FButt: Geometry +300+300'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back gray'
  FC '*FButt: Fore white'
  FC '*FButt: BoxSize smart'
  FC '*FButt: Columns '"$cols"
  FC '*FButt: Rows '"$rows"
  FC '*FButt: Font 9x15bold'
  FC '*FButt: (Panel(up,    indicator) bu "W bu")'
  FC 'Style br HandleWidth 0'
  FC '*FButt: (Panel(right, indicator, noborder) br "W br")'
  FC '*FButt: (Title "<<", Panel(left) bl "W bl")'
  FC '*FButt: (Padding 1 1, Panel(down,  indicator) bd "W bd")'
  FC Module FvwmButtons FButt

  if [ $CREATE_ONLY != 'y' ] ; then
    Operate
    Operate
    # Need a long delay here, or the window just goes away
    Delay
    FC Beep
    FC KillModule FvwmButtons FButt
    FC 'Style br HandleWidth 6'
  fi
fi

# Test 2 issues:
# The test with 1 step doesn't work, the window doesn't appear, sometimes
# I see just piece of the frame.  2 steps works.

if [ "$DO_SOME" = "n" -o "$DO_2" = "y" ] ; then
  button_size=64
  rows=2
  cols=2
  # Create a button box with 4 panels, one for each direction
  FC destroymoduleconfig FButt: \'*\'
  FC '*FButt: Geometry +400+400'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back cornflowerblue'
  FC '*FButt: Fore white'
  FC '*FButt: BoxSize fixed'
  FC '*FButt: Columns '"$cols"
  FC '*FButt: Rows '"$rows"
  FC '*FButt: Font 9x15bold'
  FC '*FButt: (Panel(up,    indicator, delay 250, smooth, steps 3) bu "W bu")'
  FC '*FButt: (Panel(right, indicator, hints)              br "W br")'
  FC '*FButt: (Panel(left,  indicator 16, steps 2) bl "W bl")'
  FC '*FButt: (Panel(down,  indicator 33, steps 1) bd "W bd")'
  FC Module FvwmButtons FButt

  if [ $CREATE_ONLY != 'y' ] ; then
    Operate
    Operate
    # Need a long delay here, or the window just goes away
    Delay
    FC Beep
    FC KillModule FvwmButtons FButt
  fi
fi

# Test 3 issues:
# Panel bul doesn't seem to be where it should be, or is it panel bur?
# Panel brc, and the bottom panels seem to come out from the wrong place,
# but slide in OK.

# Test 3, use a 3x3 panel, test positioning.
# Make sure no panel is positioned over FvwmButtons itself, you can't push
# a button if somethings in the way.

if [ "$DO_SOME" = "n" -o "$DO_3" = "y" ] ; then
  button_size=48
  rows=3
  cols=3
  # Create a button box with 4 panels, one for each direction
  FC destroymoduleconfig FButt: \'*\'
  FC '*FButt: Geometry +500+500'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back wheat'
  FC '*FButt: Fore black'
  FC '*FButt: BoxSize fixed'
  FC '*FButt: Columns '"$cols"
  FC '*FButt: Rows '"$rows"
  FC '*FButt: Font 10x20'
  FC '*FButt: Padding 12'
  base='indicator, smooth, delay 30, steps 3, position'
  FC '*FButt: (Panel(up,    '"$base "'Module top -'"$ww"'p 0 mlr mtb) bul "W bul")'
  FC '*FButt: (Panel(up,    '"$base"' Button top center mlr mtb) buc "W buc")'
  FC '*FButt: (Panel(up,    '"$base"' Module top 100 0 mlr mtb) bur "W bur")'
  # Root right 0 50 put brl down a page in the middle. isn't that 100?
  # This goes to the bottom right.  why?
  FC '*FButt: (Panel(left,  '"$base"' Root tight 0 0) brl "W brl")'
  FC '*FButt: (Panel(right, '"$base"' Module right) brc "W brc")'
  # This ends up on the left bottom, -10p off screen, 20p up from the bottom
  FC '*FButt: (Panel(right, '"$base"' Root right -5p -100p) brr "W brr")'
  FC '*FButt: (Panel(down,  '"$base"' Module bottom) bdl "W bdl")'
  FC '*FButt: (Panel(down,  '"$base"' Button bottom) bdc "W bdc")'
  FC '*FButt: (Panel(down,  '"$base"' Module bottom) bdr "W bdr")'
  FC Module FvwmButtons FButt

  if [ $CREATE_ONLY != 'y' ] ; then
    Operate
    Operate
    # Need a long delay here, or the window just goes away
    Delay
    FC Beep
    FC KillModule FvwmButtons FButt
  fi
fi

# Test 4, 1 Button.
if [ "$DO_SOME" = "n" -o "$DO_4" = "y" ] ; then
  button_size=180
  rows=1
  cols=1
  # Create a button box with 1 button
  FC destroymoduleconfig FButt: \'*\'
  FC '*FButt: Geometry +600+600'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back lightblue'
  FC '*FButt: Fore black'
  FC '*FButt: BoxSize smart'
  FC '*FButt: Columns '"$cols"
  FC '*FButt: Rows '"$rows"
  FC '*FButt: Font 9x15bold'
  FC '*FButt: Padding 12'
  FC '*FButt: (Title "OneButton", Panel(up) bd "W bd")'
  FC 'Module FvwmButtons FButt'
  if [ $CREATE_ONLY != 'y' ] ; then
    Operate
    Operate
    # Need a long delay here, or the window just goes away
    Delay
    FC Beep
    FC KillModule FvwmButtons FButt
  fi
fi
