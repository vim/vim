" Tests for parsing the modeline.

func Test_modeline_invalid()
  " This was reading before allocated memory.
  call writefile(['vi:0', 'nothing'], 'Xmodeline')
  let modeline=&modeline
  set modeline
  call assert_fails('split Xmodeline', 'E518:')
  let &modeline=modeline
  bwipe!
  call delete('Xmodeline')
endfunc
