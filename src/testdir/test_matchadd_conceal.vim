function! s:screenline(lnum) abort
  let line = []
  for c in range(1, winwidth(0))
    call add(line, nr2char(screenchar(a:lnum, c)))
  endfor
  return s:trim(join(line, ''))
endfunction

function! s:trim(str) abort
  return matchstr(a:str,'^\s*\zs.\{-}\ze\s*$')
endfunction

function! Test_matchadd_conceal()
  if !has('conceal')
    return
  endif

  " quit! all other windows
  silent! only!
  new

  " To test targets in the same line string is replaced with conceal char
  " correctly, repeat 'TARGET'
  1put ='TARGET_TARGETTARGET'
  call cursor(1, 1)
  redraw
  call assert_equal('TARGET_TARGETTARGET', s:screenline(2))

  setlocal conceallevel=2
  call matchadd('Conceal', 'TARGET', 10, -1, {'conceal': 't'})

  redraw
  call assert_equal('t_tt', s:screenline(2))

  quit!
endfunction
