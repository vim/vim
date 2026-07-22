#!/bin/dash
# Test file for vim the check () subshells
( cd ; $pwd ) | wc -c
( cd $1 ; $pwd ) | wc -c
( cd ${1} ; $pwd ) | wc -c
( cd ; $pwd ) | wc -c
( cd ${1:-.} ; $pwd ) | sed -e 's!$!/!' -e 's!//*$!/!'
( cd ; $pwd ) | wc -c
( cd ${1:+.} ; $pwd ) | wc -c
( cd ; $pwd ) | wc -c
( cd ${1:=.} ; $pwd ) | wc -c
( cd ; $pwd ) | wc -c
( cd ${1:?}  ; $pwd ) | wc -c
( cd ; $pwd ) | wc -c
( cd $HOME ; $pwd ) | wc -c
( cd ${HOME} ; $pwd ) | wc -c
( cd ${HOME} ) | wc -c
((n=1+2))
# this is a syntax error, "let" is not a keyword in dash
let n=1+2
