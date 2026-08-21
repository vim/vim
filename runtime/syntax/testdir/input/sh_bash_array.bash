#!/bin/bash

# Issue #18712

declare -A bar=(
  [\(\)]="baz"
)
echo ${bar[\(\)]}

foo=(a \( b)
foo=(\${foo[1]})

foo=([2]=10 [4]=100)
echo ${foo[4]}
