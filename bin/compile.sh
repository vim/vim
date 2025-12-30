#!/usr/bin/env fish
# Compiles the program with clang.

cd (status dirname)/..

export CC=clang
./configure

cd src/
make
cd -
