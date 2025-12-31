#!/bin/sh
# Compiles the program with clang.

cd $(dirname "$0")/../

CC=clang

export CC_OVERRIDE=$CC
./configure

cd src/
make
cd - 2 > /dev/null
