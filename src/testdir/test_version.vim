" Test :version Ex command

func Test_version()
  " version should always return the same string.
  let v1 = execute('version')
  let v2 = execute('version')
  call assert_equal(v1, v2)

  call assert_match("^\n\nVIM - Vi IMproved .*", v1)
endfunc

func Test_version_redirect()
  CheckNotGui
  CheckCanRunGui
  CheckUnix

  call RunVim([], [], '--clean -g --version >Xversion 2>&1')
  call assert_match('Features included', readfile('Xversion')->join())

  call delete('Xversion')
endfunc

" The features[] array in version.c must stay sorted.
func Test_features_sorted()
  let lines = readfile('../version.c')
  let start = match(lines, '^static char \*(features\[\]) =')
  call assert_notequal(-1, start)
  let end = match(lines, '^};', start)
  call assert_notequal(-1, end)

  " Alternatives for a single slot (#if/#elif/#else chains): only one of
  " each group is ever compiled, so they are not separate list positions.
  let ignore = ['dialog_con', 'dialog_gui', 'dialog', 'multi_byte', 'xpm', 'xsmp']

  let names = []
  for line in lines[start : end]
    let name = matchstr(line, '"[+-]\zs[A-Za-z_0-9]\+\ze\%(/dyn\)\=",')
    if name == '' || index(ignore, name) >= 0
      continue
    endif
    if empty(names) || names[-1] !=# name
      call add(names, name)
    endif
  endfor

  call assert_true(len(names) > 100)
  call assert_equal(sort(copy(names), 'i'), names)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
