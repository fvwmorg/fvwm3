#!/bin/sh
# FvwmCommand test
# arg1  b  to invoke FvwmButtons
#       kb to kill FvwmButtons
#       r  to change button rows
#       c  to change button columns

. ./FvwmCommand.sh

if [ "$1" = 'b' ] ; then
  Module FvwmButtons;
elif [ "$1" = 'kb' ] ; then
  KillModule FvwmButtons;
elif [ "$1" = 'r' ] ; then
  n=$2
  DestroyModuleConfig '*FvwmButtonsRows'
  AddModuleConfig "*FvwmButtonsRows $n"
elif [ "$1" = 'c' ] ; then
  n=$2
  DestroyModuleConfig '*FvwmButtonsColumns'
  AddModuleConfig "*FvwmButtonsColumns $n"
else
 echo " arg1  b  to invoke FvwmButtons"
 echo "       kb to kill FvwmButtons"
 ehco "       r  to change button rows"
 echo "       c  to change button columns"

fi

