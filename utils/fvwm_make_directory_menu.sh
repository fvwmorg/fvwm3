#!/bin/sh
#
#  Modification History
#
#  Created on 06/07/99 by Dominik Vogt (domivogt):
#
#  provides output to read in with PipeRead to build a menu
#  containing a directory listing.
#
#  You should put these lines into your fvwm configuration file to invoke
#  this script:
#
#  AddToFunc MakeMissingDirectoryMenu
#  + i piperead 'fvwm_make_directory_menu.sh '$0
#
#  And put these lines in the menu from which you want to pop up the
#  directory menus:
#
#  AddToMenu SomeMenu MissingSubmenuFunction MakeMissingDirectoryMenu
#  + "my directory item" Popup /whatever/path/you/like
#
#  Note: please use absolute path names.

#
# configuration section
#

# If you want to optimize for speed, unset this variable. Warning: speed
# optimization takes up a lot of memory that is never free'd again while fvwm
# is running.
OPTIMIZE_SPACE=yes

# you may use the absolute path here if you have an alias like ls="ls -F"
LS="/bin/ls -LF"
SED=sed

# the name of our menu
MENU=`echo $1 | $SED -e 's://*:/:g'`

# the item title
ITEM='basename $MENU'

# the command to execute on plain files
ACTION=vi

# the terminal program to invoke
TERMINAL="xterm -e"
TERMINALSHELL="xterm"

#
# you may customize this script below here.
#

# dump all menu items
echo DestroyMenu recreate \"$MENU\"

# add a new title
echo AddToMenu \"$MENU\" \"$MENU\" Exec "echo cd $MENU\; $TERMINALSHELL | $SHELL"

# add a separator
echo AddToMenu \"$MENU\" \"\" nop

# destroy the menu after it is popped down
if [ "$OPTIMIZE_SPACE" = yes ] ; then
  echo AddToMenu \"$MENU\" DynamicPopDownAction DestroyMenu \"$MENU\"
fi

# set the 'missing submenu function'
echo AddToMenu \"$MENU\" MissingSubmenuFunction MakeMissingDirectoryMenu

# add directory contents
$LS "$MENU" 2> /dev/null | $SED -n '
  /\/$/ bdirectory
  s:[=*@|]$::1
  s:^\(.*\)$:AddToMenu "'$MENU'" "\1" Exec '"$TERMINAL $ACTION"' "'"$MENU"'/\1":1
  bnext
  :directory
  s:^\(.*\)/$:AddToMenu "'$MENU'" "\1" Popup "'"$MENU"'/\1" item +100 c:1
  :next
  s://*:/:g
  p
'
