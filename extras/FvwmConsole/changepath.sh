#!/bin/sh
pp=`which perl`
if [ $? = 0 ] ; then
  sed -e "s-^#!.*-#!$pp-" FvwmConsoleC.pl > tmp1
  mv tmp1 FvwmConsoleC.pl
  chmod +x FvwmConsoleC.pl
fi
