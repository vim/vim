#!/bin/bash

# Issue #962

arr=(
  1 2 3 4
) # ok

if true; then

  arr=(1 2 3 4) # ok

  arr=( 1 2 3 4 ) # ok

  arr=(
    1 2 3 4
  ) # paren error!

fi

