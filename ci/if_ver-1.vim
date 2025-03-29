" Print all interface versions. Part 1.

if 1 " This prevents it from being run in tiny versions
  execute 'source' expand('<sfile>:h') .. '/if_ver-cmd.vim'

  echo "*** Interface versions ***\n"

  echo 'Lua:'
  PrintVer lua print(vim.lua_version, jit and "(LuaJIT)" or "")

  echo 'MzScheme:'
  PrintVer mzscheme (display (version))

  echo 'Perl:'
  PrintVer perl print $^V

  echo 'Ruby:'
  PrintVer ruby print RUBY_VERSION

  echo 'Tcl:'
  PrintVer tcl puts [info patchlevel]

  echo 'Python 2:'
  PrintVer python print sys.version
endif
