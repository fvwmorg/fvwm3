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

# Function pushes all the buttons:
Operate () {
  FC 'Next (FButt) WarpToWindow 20p 20p'
  sleep `expr 1 \* $slow`
  FC 'FakeClick Depth 4 press 1 wait 3000 release 1'
  sleep `expr 1 \* $slow`
  FC 'CursorMove 32p 0'
  FC 'FakeClick Depth 4 press 1 wait 3000 release 1'
  sleep `expr 1 \* $slow`
  FC 'CursorMove 0 32p'
  FC 'FakeClick Depth 4 press 1 wait 3000 release 1'
  sleep `expr 1 \* $slow`
  FC 'CursorMove -32p 0'
  FC 'FakeClick Depth 4 press 1 wait 3000 release 1'
}

talk=true
slow=1
while getopts dtos arg ; do
  case $arg in
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

# Create shortcut function to create a window
# arg 0, window name
FC DestroyFunc W
FC AddToFunc W I Exec xterm -name '\$0' -g 10x10-20000-20000

# Create a button box with 4 panels, one for each direction
FC destroymoduleconfig FButt: \'*\'
# Somewhere near the middle of the screen:
FC '*FButt: Geometry +300+300'
FC '*FButt: ButtonGeometry 32x32'
FC '*FButt: Back gray'
FC '*FButt: Fore white'
FC '*FButt: BoxSize smart'
FC '*FButt: Columns 2'
FC '*FButt: Rows 2'
FC '*FButt: Font 9x15bold'
FC '*FButt: (Panel(up,    indicator) bu "W bu")'
FC '*FButt: (Panel(right, indicator) br "W br")'
FC '*FButt: (Panel(left,  indicator) bl "W bl")'
FC '*FButt: (Panel(down,  indicator) bd "W bd")'
#style("FButt CirculateHit, neverfocus, mousefocusclickraisesoff");
FC Module FvwmButtons FButt

sleep `expr 2 \* $slow`
Operate
sleep `expr 2 \* $slow`
Operate

FC KillModule FvwmButtons FButt

# Create a button box with 4 panels, one for each direction
FC destroymoduleconfig FButt: \'*\'
FC '*FButt: Geometry +300+300'
FC '*FButt: ButtonGeometry 32x32'
FC '*FButt: Back gray'
FC '*FButt: Fore white'
FC '*FButt: BoxSize smart'
FC '*FButt: Columns 2'
FC '*FButt: Rows 2'
FC '*FButt: Font 9x15bold'
FC '*FButt: (Panel(up,    indicator, delay 3000, smooth) bu "W bu")'
FC '*FButt: (Panel(right, indicator, hints)              br "W br")'
FC '*FButt: (Panel(left,  indicator 16) bl "W bl")'
FC '*FButt: (Panel(down,  indicator, steps 1) bd "W bd")'
FC Module FvwmButtons FButt

sleep `expr 2 \* $slow`
Operate
sleep `expr 2 \* $slow`
Operate

FC KillModule FvwmButtons FButt

