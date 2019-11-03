" Print all interface versions and write the result into if_ver.txt.
" For Ubuntu.

if 1
  echo "*** Interface versions ***"

  const s:interfaces = #{
        \ lua: #{name: 'Lua', cmd: 'lua print(_VERSION)'},
        \ mzscheme: #{name: 'MzScheme', cmd: 'mzscheme (display (version))'},
        \ perl: #{name: 'Perl', cmd: 'perl print $^V'},
        \ python: #{name: 'Python 2', cmd: 'python print sys.version'},
        \ python3: #{name: 'Python 3', cmd: 'python3 print(sys.version)'},
        \ ruby: #{name: 'Ruby', cmd: 'ruby print RUBY_VERSION'},
        \ tcl: #{name: 'Tcl', cmd: 'tcl puts [info patchlevel]'},
        \ }

  for s:lang in sort(keys(s:interfaces))
    let s:item = s:interfaces[s:lang]
    echo "\n" .. s:item.name .. ':'
    if has(s:lang)
      exec s:item.cmd
    else
      echo 'N/A'
    endif
  endfor

  echo "\n"
endif
