#!/bin/sh
# FvwmCommand test
# arg1  b  to invoke FvwmButtons
#       kb to kill FvwmButtons
#       r  to change button rows
#       c  to change button columns

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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

