#!/bin/sh

# Test shell for FvwmButtons.

# If you don't have the FvwmCommand server running, first start it:

#    Module FvwmCommandS

# Then, run this shell.

# Args are:
# -d debug by echoing commands instead of issuing them.
# -t trace exection
# -s run slowly.  Needed on very slow connections.
# -o just operate the 4 button setup once.

#  Modification History

#  Created on 03/02/01 by Dan Espen (dane):
#  - Tests for FvwmButtons.


# Function for running an Fvwm command:
FC () {
  $debug FvwmCommand "$*"
  $talk "`date` FvwmCommand $*"
}

# Draw attention to the pointer
Pointer_Attn () {
  # Can't use FC here, tracing destroys the effect.
  x=1
  while [ $x -lt 40 ] ; do
   FvwmCommand "WindowId root WarpToWindow $x $x"
    x=`expr $x + 4`
  done
  while [ $x -gt 10 ] ; do
   FvwmCommand "WindowId root WarpToWindow $x $x"
    x=`expr $x - 2`
  done
}

button_size=32

# Function pushes all the buttons:
Operate () {
  Pointer_Attn
  FC 'Next (FButt) WarpToWindow 20p 20p'
  sleep `expr 2 \* $slow`
  FC 'FakeClick Depth 4 press 1 wait 30 release 1'
  sleep `expr 2 \* $slow`
  FC 'CursorMove '"${button_size}"'p 0'
  FC 'FakeClick Depth 4 press 1 wait 30 release 1'
  sleep `expr 2 \* $slow`
  FC 'CursorMove 0 '"${button_size}"'p'
  FC 'FakeClick Depth 4 press 1 wait 30 release 1'
  sleep `expr 2 \* $slow`
  FC 'CursorMove -'"${button_size}"'p 0'
  FC 'FakeClick Depth 4 press 1 wait 30 release 1'
  sleep `expr 3 \* $slow`
}

talk=true
slow=1
dosome=n
while getopts dtos12 arg ; do
  case $arg in
    1) 
      do1=y
      dosome=y;;
    2) 
      do2=y
      dosome=y;; 
   d) 
      debug=echo;;
    t)
      talk=echo;;
    s)
      slow=10;;
    o)
     Operate
     exit 0;;
  esac
done

# Test 1 issues:
# UseTitle doesn't put anything in button, button doesn't operate
# Padding makes indicator disappear
# Noborder test needs a window with no border.

# Create shortcut function to create a window
# arg 0, window name
FC DestroyFunc W
FC AddToFunc W I Exec xterm -name '\$0' -g 10x10-20000-20000

if [ "$dosome" = "n" -o "$do1" = "y" ] ; then
  # Create a button box with 4 panels, one for each direction
  FC destroymoduleconfig FButt: \'*\'
  # Somewhere near the middle of the screen:
  FC '*FButt: Geometry +300+300'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back gray'
  FC '*FButt: Fore white'
  FC '*FButt: BoxSize smart'
  FC '*FButt: Columns 2'
  FC '*FButt: Rows 2'
  FC '*FButt: Font 9x15bold'
  FC '*FButt: (Panel(up,    indicator) bu "W bu")'
  FC '*FButt: (Panel(right, indicator, noborder) br "W br")'
  FC '*FButt: (Panel(left, UseTitle) bl "W bl")'
  FC '*FButt: (Panel(down,  indicator, padding 2) bd "W bd")'
  #style("FButt CirculateHit, neverfocus, mousefocusclickraisesoff");
  FC Module FvwmButtons FButt

  Operate
  Operate
  # Need a long delay here, or the window just goes away
  sleep `expr 2 \* $slow`
  FC Beep
  FC KillModule FvwmButtons FButt
fi


# The "down" button with 1 step doesn't draw anything.

if [ "$dosome" = "n" -o "$do2" = "y" ] ; then
  button_size=64
  # Create a button box with 4 panels, one for each direction
  FC destroymoduleconfig FButt: \'*\'
  FC '*FButt: Geometry +400+400'
  FC '*FButt: ButtonGeometry '"${button_size}x${button_size}"
  FC '*FButt: Back cornflowerblue'
  FC '*FButt: Fore white'
  FC '*FButt: BoxSize fixed'
  FC '*FButt: Columns 2'
  FC '*FButt: Rows 2'
  FC '*FButt: Font 9x15bold'
  FC '*FButt: (Panel(up,    indicator, delay 3000, smooth, steps 3) bu "W bu")'
  FC '*FButt: (Panel(right, indicator, hints)              br "W br")'
  FC '*FButt: (Panel(left,  indicator 16) bl "W bl")'
  FC '*FButt: (Panel(down,  indicator, steps 1) bd "W bd")'
  FC Module FvwmButtons FButt

  Operate
  Operate
  # Need a long delay here, or the window just goes away
  sleep `expr 2 \* $slow`
  FC Beep
  FC KillModule FvwmButtons FButt
fi
