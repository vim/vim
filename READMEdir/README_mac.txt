README_mac.txt for version 8.0 of Vim: Vi IMproved.

This file explains the installation of Vim on Macintosh systems.
See "README.txt" for general information about Vim.


To build from sources, like on Unix

1. Get the build tools: "clang" and "make".  These can be installed with the
   "CommandLineTools" package.   If you don't have one, do
	xcode-select --install
   Just like for any software development with OS X.

2. Get the source code.  Best is to use git (which you need to install first),
   see http://www.vim.org/git.php
   Or you can download and unpack the Unix tar archive, see
   http://www.vim.org/download.php

3. Go to the top directory of the source tree, do
	make
	sudo make install
  A newly built vim will be installed under "/usr/local".


If you can't manage to make this work, there is a fallback using Homebrew:

1. Install Homebrew from http://brew.sh/
2. Install latest Vim with:  brew install vim
