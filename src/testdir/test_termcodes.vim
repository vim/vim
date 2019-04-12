" Tests for decoding escape sequences sent by the terminal.

" This only works for Unix in a terminal
if has('gui_running') || !has('unix')
  finish
endif

" Helper function to emit a terminal escape code.
func TerminalEscapeCode(code_xterm, code_sgr, row, col, m)
  if &ttymouse ==# 'xterm'
    " need to use byte encoding here.
    let str = list2str([a:code_xterm, a:col + 0x20, a:row + 0x20])
    if has('iconv')
      let bytes = iconv(str, 'utf-8', 'latin1')
    else
      " Hopefully the numbers are not too big.
      let bytes = str
    endif
    call feedkeys("\<Esc>[M" .. bytes, 'Lx!')
  elseif &ttymouse ==# 'sgr'
    call feedkeys(printf("\<Esc>[<%d;%d;%d%s", a:code_sgr, a:col, a:row, a:m), 'Lx!')
  endif
endfunc

func MouseLeftClick(row, col)
  call TerminalEscapeCode(0x20, 0, a:row, a:col, 'M')
endfunc

func MouseLeftRelease(row, col)
  call TerminalEscapeCode(0x23, 3, a:row, a:col, 'm')
endfunc

func MouseLeftDrag(row, col)
  call TerminalEscapeCode(0x43, 0x20, a:row, a:col, 'M')
endfunc

func MouseWheelUp(row, col)
  call TerminalEscapeCode(0x40, 0x40, a:row, a:col, 'M')
endfunc

func MouseWheelDown(row, col)
  call TerminalEscapeCode(0x41, 0x41, a:row, a:col, 'M')
endfunc

func Test_xterm_mouse_click()
  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  call setline(1, ['line 1', 'line 2', 'line 3 is a bit longer'])

  for ttymouse_val in ['xterm', 'sgr']
    exe 'set ttymouse=' . ttymouse_val
    go
    call assert_equal([0, 1, 1, 0], getpos('.'))
    let row = 2
    let col = 6
    call MouseLeftClick(row, col)
    call MouseLeftRelease(row, col)
    call assert_equal([0, 2, 6, 0], getpos('.'))
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  bwipe!
endfunc

func Test_xterm_mouse_wheel()
  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  call setline(1, range(1, 100))

  for ttymouse_val in ['xterm', 'sgr']
    exe 'set ttymouse=' . ttymouse_val
    go
    call assert_equal(1, line('w0'))
    call assert_equal([0, 1, 1, 0], getpos('.'))

    call MouseWheelDown(1, 1)
    call assert_equal(4, line('w0'))
    call assert_equal([0, 4, 1, 0], getpos('.'))

    call MouseWheelDown(1, 1)
    call assert_equal(7, line('w0'))
    call assert_equal([0, 7, 1, 0], getpos('.'))

    call MouseWheelUp(1, 1)
    call assert_equal(4, line('w0'))
    call assert_equal([0, 7, 1, 0], getpos('.'))

    call MouseWheelUp(1, 1)
    call assert_equal(1, line('w0'))
    call assert_equal([0, 7, 1, 0], getpos('.'))
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  bwipe!
endfunc

func Test_xterm_mouse_drag_window_separator()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm

  for ttymouse_val in ['xterm', 'sgr']
    exe 'set ttymouse=' . ttymouse_val

    " Split horizontally and test dragging the horizontal window separator.
    split
    let rowseparator = winheight(0) + 1
    let row = rowseparator
    let col = 1

    if ttymouse_val ==# 'xterm' && row > 223
      " When 'ttymouse' is 'xterm', row/col bigger than 223 are not supported.
      continue
    endif

    call MouseLeftClick(row, col)

    let row -= 1
    call MouseLeftDrag(row, col)
    call assert_equal(rowseparator - 1, winheight(0) + 1)
    let row += 1
    call MouseLeftDrag(row, col)
    call assert_equal(rowseparator, winheight(0) + 1)
    call MouseLeftRelease(row, col)
    call assert_equal(rowseparator, winheight(0) + 1)

    bwipe!

    " Split vertically and test dragging the vertical window separator.
    vsplit
    let colseparator = winwidth(0) + 1

    let row = 1
    let col = colseparator
    call MouseLeftClick(row, col)
    let col -= 1
    call MouseLeftDrag(row, col)
    call assert_equal(colseparator - 1, winwidth(0) + 1)
    let col += 1
    call MouseLeftDrag(row, col)
    call assert_equal(colseparator, winwidth(0) + 1)
    call MouseLeftRelease(row, col)
    call assert_equal(colseparator, winwidth(0) + 1)

    bwipe!
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
endfunc

func Test_xterm_mouse_drag_statusline()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm

  for ttymouse_val in ['xterm', 'sgr']
    exe 'set ttymouse=' . ttymouse_val

    call assert_equal(1, &cmdheight)
    let rowstatusline = winheight(0) + 1
    let row = rowstatusline
    let col = 1

    if ttymouse_val ==# 'xterm' && row > 223
      " When 'ttymouse' is 'xterm', row/col bigger than 223 are not supported.
      continue
    endif

    call MouseLeftClick(row, col)
    let row -= 1
    call MouseLeftDrag(row, col)
    call assert_equal(2, &cmdheight)
    call assert_equal(rowstatusline - 1, winheight(0) + 1)
    let row += 1
    call MouseLeftDrag(row, col)
    call assert_equal(1, &cmdheight)
    call assert_equal(rowstatusline, winheight(0) + 1)
    call MouseLeftRelease(row, col)
    call assert_equal(1, &cmdheight)
    call assert_equal(rowstatusline, winheight(0) + 1)
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
endfunc
