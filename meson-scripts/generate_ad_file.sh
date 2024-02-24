#!/bin/sh

cat $2 | \
    grep -A 1000000 -- "^// BEGIN '$1'" | \
    grep -B 1000000 -- "^// END '$1'" | \
    grep -v "^// .* '$1'"
