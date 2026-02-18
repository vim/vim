#!/bin/sh
# Read stdin if no arguments were given and stdin was redirected.

if [ $# -eq 0 ] && [ -t 0 ]; then
  echo "Missing filename" 1>&2
  exit
fi

if [ -t 1 ]; then
  [ $# -eq 0 ] && set -- "-"
  exec vim --cmd 'let no_plugin_maps=1' -c 'runtime! macros/less.vim' --not-a-term "$@"
else  # Output is not a terminal.
  exec cat "$@"
fi
