#!/usr/bin/zsh

emulate zsh
setopt magicequalsubst

strings=(`strings =fvwm | grep -i -v restart | grep -i -v quit | grep -i -v wait | grep -i -v piperead`)
nstr=${#strings}
seps=("+" "*" " " " " " " " " " " "\\" "\"" "\`" "'" "	" "	" "	" "\n")
nsep=${#seps}

while true; do
  N=$RANDOM
  if [[ ! $[$RANDOM % 4] = 1 ]]; then
    # add a separator
    I=$[1 + $N % $nsep]
    builtin echo -n "$seps[$I]"
  else
    # add a string
    I=$[1 + $N % $nstr]
    builtin echo -n "$strings[$I]"
  fi
done

