#!/bin/sh

# Find all Go source files in the current directory and its subdirectories.
# That is all

find "$(pwd)" -type f -name '*.go'
