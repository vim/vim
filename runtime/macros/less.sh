#!/bin/sh
# Shell script to start Vim with less.vim.
# Read stdin if no arguments were given.

if test -t 1; then
 if test $# = 0; then
   vim --cmd 'let no_plugin_maps = 1' -c 'runtime! macros/less.vim' -
  else
   vim --cmd 'let no_plugin_maps = 1' -c 'runtime! macros/less.vim' "$@"
  fi
else
  # Output is not a terminal, cat arguments or stdin
  if test $# = 0; then
    cat
  else
    cat "$@"
  fi
fi
