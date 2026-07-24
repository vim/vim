#!/bin/bash

# An escaped ( in an array subscript must not swallow the array's
# closing ) and bleed the array-value highlighting into later lines.
declare -A bar=(
  [\(\)]="baz"
)
echo ${bar[\(\)]}

foo=([2]=10 [4]=100)
echo ${foo[4]}
