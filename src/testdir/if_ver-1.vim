" Print all interface versions and write the result into if_ver.txt.
" For Ubuntu. Part 1.

redir! > if_ver.txt
if 1
  echo "*** Interface versions ***"
  echo "\nLua:"
  lua print(_VERSION)
  " echo "\nLuaJIT:"
  " lua print(jit.version)
  if has('mzscheme')
    echo "\nMzScheme:"
    mzscheme (display (version))
  endif
  echo "\nPerl:"
  perl print $^V
  echo "\nRuby:"
  ruby print RUBY_VERSION
  if has('tcl')
    echo "\nTcl:"
    tcl puts [info patchlevel]
  endif
  echo "\nPython 2:"
  python import sys; print sys.version
endif
redir END
