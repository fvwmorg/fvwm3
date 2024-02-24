#!/bin/bash

# This script is used to generate the FvwmScript syntax

LEX='flex'
# check for bison, prefer that if found else fall back to yacc.
YACC=$(command -v bison >/dev/null 2>&1) && YACC="$(which bison) -y" || YACC=yacc

rm -f lex.yy.c y.tab.c y.tab.h script.tab.c script.tab.h || true

if [ "$(command -v LEX >/dev/null 2>&1)" ]; then
  LEX="$LEX --nounput"
fi

$LEX scanner.l && mv lex.yy.c scanner.c

$YACC script.y && mv *.tab.c script.c

# Check and update header file if needed
if [ -f y.tab.h -o -f script.tab.h ]; then
  if [ "$(cmp -s *.tab.h script.h)" = 0 ]; then
    rm -f y.tab.h script.tab.h
  else
    mv *.tab.h script.h
  fi
fi
