if 1 " This prevents it from being run in tiny versions
  func s:print_ver(lang, ...)
    if has(a:lang)
      exec a:lang join(a:000)
    else
      echo 'N/A'
    endif
    echo ''
  endfunc

  command -nargs=+ PrintVer call <SID>print_ver(<f-args>)

  " Print scripting interface versions
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

  echo 'Python 3:'
  PrintVer python3 print(sys.version)

  " Check for required features
  if exists("g:required")
    for feature in g:required
      if !has(feature)
        echo "Error: Feature '" .. feature .. "' not found"
        echo ''
        cquit
      endif
    endfor
    echo "\nChecked features: " .. string(g:required)
    echo ''
  endif
endif
" vim: sts=2 sw=2 et
