#!/bin/sh
#
###
#  This script demonstrates the fvwm menu DynamicPopupAction functionality.
#  You can use a more configurable fvwm-menu-directory instead.
#  The line below almost exactly simulates this script:
#    fvwm-menu-directory --reuse --links --order 4 --name <menu name> \
#      --exec-file vi --exec-title - --special-dirs
###
#
#  Modification History
#
#  Created on 05/06/99 by Dominik Vogt (domivogt):
#
#  provides output to read in with PipeRead to build a menu
#  containing a directory listing.
#
#  You should put these lines into your fvwm configuration file to invoke
#  this script:
#
#  AddToMenu <menu name>
#  + DynamicPopupAction Piperead 'fvwm_make_browse_menu.sh <menu name>'

#
# configuration section
#

# the file containing the desired directory name
DIRFILE="$HOME/.fvwm_browse_menu_cwd"

# the name of our menu
MENU=`echo "$1" | $SED -e s:\"::g`

# the command to execute on plain files
ACTION=vi

# the terminal program to invoke
TERMINAL="xterm -e"

# you may use the absolute path here if you have an alias like ls="ls -F"
LS=ls

SED=sed

#
# you may customize this script below here.
#

# create a file containing the current working directory
if [ ! -f "$DIRFILE" ] ; then
    echo "$HOME" > "$DIRFILE"
fi

# get desired directory
DIR="`cat "$DIRFILE"`"
if [ ! -d "$DIR" ] ; then
    DIR="$HOME"
fi

# dump all menu items
echo DestroyMenu recreate \""$MENU"\"

# add a new title
echo AddToMenu \""$MENU"\" \"`cat "$DIRFILE"`\" Title

# add '..' entry
cd "$DIR"/..
echo AddToMenu \""$MENU"\" \"..\" PipeRead \'echo \""`/bin/pwd`"\" \> "$DIRFILE" ';' echo Menu \\\""$MENU"\\\" WarpTitle\'

# add separator
echo AddToMenu \""$MENU"\" \"\" Nop

# add $HOME entry
echo AddToMenu \""$MENU"\" \"~\" PipeRead \'echo \""$HOME"\" \> "$DIRFILE" ';' echo Menu \\\""$MENU"\\\" WarpTitle\'

# add separator
echo AddToMenu \""$MENU"\" \"\" Nop

# add directory contents
for i in `"$LS" "$DIR"` ; do
    if [ -d "$DIR/$i" ] ; then
        # it's a directory
        cd "$DIR/$i"
        # put new path in $DIRFILE and invoke the menu again
        echo AddToMenu \""$MENU"\" \""$i/"\" PipeRead \'echo \"`echo $DIR/$i|$SED -e s://:/:g`\" \> "$DIRFILE" ';' echo Menu \\\""$MENU"\\\" WarpTitle\'
    else
        # something else, apply $ACTION to it
        echo AddToMenu \""$MENU"\" \""$i"\" Exec $TERMINAL $ACTION \""$DIR/$i"\"
    fi
done
