" Test that CursorHold is NOT triggered at startup before a keypress

source check.vim
CheckCanRunGui

func Test_CursorHold_not_triggered_at_startup()
  call delete('Xcursorhold.log')
  call writefile([
        \ 'set updatetime=300',
        \ 'let g:cursorhold_triggered = 0',
        \ 'autocmd CursorHold * let g:cursorhold_triggered += 1 | call writefile(["CursorHold triggered"], "Xcursorhold.log", "a")',
        \ 'call timer_start(400, {-> execute(''call writefile(["g:cursorhold_triggered=" . g:cursorhold_triggered], "Xcursorhold.log", "a") | qa!'')})',
        \ ], 'Xcursorhold_test.vim')

  let vimcmd = v:progpath . ' -g -f -N -u NONE -i NONE -S Xcursorhold_test.vim'
  call system(vimcmd)

  let lines = filereadable('Xcursorhold.log') ? readfile('Xcursorhold.log') : []
  call delete('Xcursorhold.log')
  call delete('Xcursorhold_test.vim')

  " Assert that CursorHold did NOT trigger at startup
  call assert_false(index(lines, 'CursorHold triggered') != -1)
  let found = filter(copy(lines), 'v:val =~ "^g:cursorhold_triggered="')
  call assert_equal(['g:cursorhold_triggered=0'], found)
endfunc

" vim: shiftwidth=2 sts=2 expandtab
