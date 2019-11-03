" Print all interface versions and write the result into if_ver.txt.
" For Ubuntu.

if 1
  echo "*** Interface versions ***\n"

  func s:print_ver(lang, ...)
    if has(a:lang)
      exec a:lang join(a:000)
    else
      echo 'N/A'
    endif
    echo ''
  endfunc
  command -nargs=+ PrintVer call s:print_ver(<f-args>)

  echo 'Lua:'
  PrintVer lua print(_VERSION)

  echo 'MzScheme:'
  PrintVer mzscheme (display (version))

  echo 'Perl:'
  PrintVer perl print $^V

  echo 'Python 2:'
  PrintVer python print sys.version

  echo 'Python 3:'
  PrintVer python3 print(sys.version)

  echo 'Ruby:'
  PrintVer ruby print RUBY_VERSION

  echo 'Tcl:'
  PrintVer tcl puts [info patchlevel]

  delcommand PrintVer
endif
