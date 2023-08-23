#!/bin/bash
# fetch-vim.bat: Fetch vim if necessary
# For use in the editorconfig-vim Appveyor build
# Copyright (c) 2018--2019 Chris White.  All rights reserved.
# Licensed Apache 2.0, or any later version, at your option.

# Debugging
set -x
set -o nounset
#set -o errexit

# Basic system info
uname -a
pwd
ls -l

echo "VIM_EXE: $VIM_EXE"
set

# If it's already been loaded from the cache, we're done
if [[ -x "$VIM_EXE" ]]; then
    echo Vim found in cache at "$VIM_EXE"
    exit 0
fi

# Otherwise, clone and build it
WHITHER="$APPVEYOR_BUILD_FOLDER/vim"

git clone https://github.com/vim/vim-appimage.git
cd vim-appimage
git submodule update --init --recursive

cd vim/src
./configure --with-features=huge --prefix="$WHITHER" --enable-fail-if-missing
make -j2    # Free tier provides two cores
make install
./vim --version
cd $APPVEYOR_BUILD_FOLDER
find . -type f -name vim -exec ls -l {} +

echo Done fetching and installing vim
