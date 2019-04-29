" Tests for decoding escape sequences sent by the terminal.

" This only works for Unix in a terminal
if has('gui_running') || !has('unix')
  finish
endif

source shared.vim

" Helper function to emit a terminal escape code.
func TerminalEscapeCode(code_xterm, code_sgr, row, col, m)
  if &ttymouse ==# 'xterm2'
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

func MouseMiddleClick(row, col)
  call TerminalEscapeCode(0x21, 1, a:row, a:col, 'M')
endfunc

func MouseLeftRelease(row, col)
  call TerminalEscapeCode(0x23, 3, a:row, a:col, 'm')
endfunc

func MouseMiddleRelease(row, col)
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

func Test_xterm_mouse_left_click()
  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  call setline(1, ['line 1', 'line 2', 'line 3 is a bit longer'])

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    go
    call assert_equal([0, 1, 1, 0], getpos('.'), msg)
    let row = 2
    let col = 6
    call MouseLeftClick(row, col)
    call MouseLeftRelease(row, col)
    call assert_equal([0, 2, 6, 0], getpos('.'), msg)
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  bwipe!
endfunc

func Test_xterm_mouse_middle_click()
  if !WorkingClipboard()
    throw 'Skipped: No working clipboard'
  endif

  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  let save_quotestar = @*
  let @* = 'abc'
  set mouse=a term=xterm

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    call setline(1, ['123456789', '123456789'])

    " Middle-click in the middle of the line pastes text where clicked.
    let row = 1
    let col = 6
    call MouseMiddleClick(row, col)
    call MouseMiddleRelease(row, col)
    call assert_equal(['12345abc6789', '123456789'], getline(1, '$'), msg)

    " Middle-click beyond end of the line pastes text at the end of the line.
    let col = 20
    call MouseMiddleClick(row, col)
    call MouseMiddleRelease(row, col)
    call assert_equal(['12345abc6789abc', '123456789'], getline(1, '$'), msg)

    " Middle-click beyond the last line pastes in the last line.
    let row = 5
    let col = 3
    call MouseMiddleClick(row, col)
    call MouseMiddleRelease(row, col)
    call assert_equal(['12345abc6789abc', '12abc3456789'], getline(1, '$'), msg)
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  let @* = save_quotestar
  bwipe!
endfunc

func Test_xterm_mouse_wheel()
  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  call setline(1, range(1, 100))

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    go
    call assert_equal(1, line('w0'), msg)
    call assert_equal([0, 1, 1, 0], getpos('.'), msg)

    call MouseWheelDown(1, 1)
    call assert_equal(4, line('w0'), msg)
    call assert_equal([0, 4, 1, 0], getpos('.'), msg)

    call MouseWheelDown(1, 1)
    call assert_equal(7, line('w0'), msg)
    call assert_equal([0, 7, 1, 0], getpos('.'), msg)

    call MouseWheelUp(1, 1)
    call assert_equal(4, line('w0'), msg)
    call assert_equal([0, 7, 1, 0], getpos('.'), msg)

    call MouseWheelUp(1, 1)
    call assert_equal(1, line('w0'), msg)
    call assert_equal([0, 7, 1, 0], getpos('.'), msg)
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

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val

    " Split horizontally and test dragging the horizontal window separator.
    split
    let rowseparator = winheight(0) + 1
    let row = rowseparator
    let col = 1

    " When 'ttymouse' is 'xterm2', row/col bigger than 223 are not supported.
    if ttymouse_val !=# 'xterm2' || row <= 223
      call MouseLeftClick(row, col)
      let row -= 1
      call MouseLeftDrag(row, col)
      call assert_equal(rowseparator - 1, winheight(0) + 1, msg)
      let row += 1
      call MouseLeftDrag(row, col)
      call assert_equal(rowseparator, winheight(0) + 1, msg)
      call MouseLeftRelease(row, col)
      call assert_equal(rowseparator, winheight(0) + 1, msg)
    endif
    bwipe!

    " Split vertically and test dragging the vertical window separator.
    vsplit
    let colseparator = winwidth(0) + 1
    let row = 1
    let col = colseparator

    " When 'ttymouse' is 'xterm2', row/col bigger than 223 are not supported.
    if ttymouse_val !=# 'xterm2' || col <= 223
      call MouseLeftClick(row, col)
      let col -= 1
      call MouseLeftDrag(row, col)
      call assert_equal(colseparator - 1, winwidth(0) + 1, msg)
      let col += 1
      call MouseLeftDrag(row, col)
      call assert_equal(colseparator, winwidth(0) + 1, msg)
      call MouseLeftRelease(row, col)
      call assert_equal(colseparator, winwidth(0) + 1, msg)
    endif
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
  let save_laststatus = &laststatus
  set mouse=a term=xterm laststatus=2

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val

    call assert_equal(1, &cmdheight, msg)
    let rowstatusline = winheight(0) + 1
    let row = rowstatusline
    let col = 1

    if ttymouse_val ==# 'xterm2' && row > 223
      " When 'ttymouse' is 'xterm2', row/col bigger than 223 are not supported.
      continue
    endif

    call MouseLeftClick(row, col)
    let row -= 1
    call MouseLeftDrag(row, col)
    call assert_equal(2, &cmdheight, msg)
    call assert_equal(rowstatusline - 1, winheight(0) + 1, msg)
    let row += 1
    call MouseLeftDrag(row, col)
    call assert_equal(1, &cmdheight, msg)
    call assert_equal(rowstatusline, winheight(0) + 1, msg)
    call MouseLeftRelease(row, col)
    call assert_equal(1, &cmdheight, msg)
    call assert_equal(rowstatusline, winheight(0) + 1, msg)
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  let &laststatus = save_laststatus
endfunc

func Test_xterm_mouse_click_tab()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  let row = 1

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    e Xfoo
    tabnew Xbar

    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xfoo',
        \              'Tab page 2',
        \              '>   Xbar'], a, msg)

    " Test clicking on tab names in the tabline at the top.
    let col = 2
    redraw
    call MouseLeftClick(row, col)
    call MouseLeftRelease(row, col)
    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '>   Xfoo',
        \              'Tab page 2',
        \              '    Xbar'], a, msg)

    let col = 9
    call MouseLeftClick(row, col)
    call MouseLeftRelease(row, col)
    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xfoo',
        \              'Tab page 2',
        \              '>   Xbar'], a, msg)

    %bwipe!
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
endfunc

func Test_xterm_mouse_click_X_to_close_tab()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  set mouse=a term=xterm
  let row = 1
  let col = &columns

  for ttymouse_val in ['xterm2', 'sgr']
    if ttymouse_val ==# 'xterm2' && col > 223
      " When 'ttymouse' is 'xterm2', row/col bigger than 223 are not supported.
      continue
    endif
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    e Xtab1
    tabnew Xtab2
    tabnew Xtab3
    tabn 2

    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xtab1',
        \              'Tab page 2',
        \              '>   Xtab2',
        \              'Tab page 3',
        \              '    Xtab3'], a, msg)

    " Click on "X" in tabline to close current tab i.e. Xtab2.
    redraw
    call MouseLeftClick(row, col)
    call MouseLeftRelease(row, col)
    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xtab1',
        \              'Tab page 2',
        \              '>   Xtab3'], a, msg)

    %bwipe!
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
endfunc

func Test_xterm_mouse_drag_to_move_tab()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  " Set 'mousetime' to 1 to avoid recognizing a double-click in the loop
  set mouse=a term=xterm mousetime=1
  let row = 1

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    e Xtab1
    tabnew Xtab2

    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xtab1',
        \              'Tab page 2',
        \              '>   Xtab2'], a, msg)
    redraw

    " Click in tab2 and drag it to tab1.
    " Check getcharmod() to verify that click is not
    " interpreted as a spurious double-click.
    call MouseLeftClick(row, 10)
    call assert_equal(0, getcharmod(), msg)
    for col in [9, 8, 7, 6]
      call MouseLeftDrag(row, col)
    endfor
    call MouseLeftRelease(row, col)
    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '>   Xtab2',
        \              'Tab page 2',
        \              '    Xtab1'], a, msg)

    " brief sleep to avoid causing a double-click
    sleep 20m
    %bwipe!
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  set mousetime&
endfunc

func Test_xterm_mouse_double_click_to_create_tab()
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  " Set 'mousetime' to a small value, so that double-click works but we don't
  " have to wait long to avoid a triple-click.
  set mouse=a term=xterm mousetime=100
  let row = 1
  let col = 10

  for ttymouse_val in ['xterm2', 'sgr']
    let msg = 'ttymouse=' .. ttymouse_val
    exe 'set ttymouse=' . ttymouse_val
    e Xtab1
    tabnew Xtab2

    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xtab1',
        \              'Tab page 2',
        \              '>   Xtab2'], a, msg)

    redraw
    call MouseLeftClick(row, col)
    " Check getcharmod() to verify that first click is not
    " interpreted as a spurious double-click.
    call assert_equal(0, getcharmod(), msg)
    call MouseLeftRelease(row, col)
    call MouseLeftClick(row, col)
    call assert_equal(32, getcharmod(), msg) " double-click
    call MouseLeftRelease(row, col)
    let a = split(execute(':tabs'), "\n")
    call assert_equal(['Tab page 1',
        \              '    Xtab1',
        \              'Tab page 2',
        \              '>   [No Name]',
        \              'Tab page 3',
        \              '    Xtab2'], a, msg)

    if ttymouse_val !=# 'sgr'
      " We need to sleep, or else MouseLeftClick() in next loop
      " iteration will be interpreted as a spurious triple-click.
      sleep 100m
    endif
    %bwipe!
  endfor

  let &mouse = save_mouse
  let &term = save_term
  let &ttymouse = save_ttymouse
  set mousetime&
endfunc

func Test_xterm_mouse_click_in_fold_columns()
  new
  let save_mouse = &mouse
  let save_term = &term
  let save_ttymouse = &ttymouse
  let save_foldcolumn = &foldcolumn
  set mouse=a term=xterm foldcolumn=3 ttymouse=xterm2

  " Create 2 nested folds.
  call setline(1, range(1, 7))
  2,6fold
  norm! zR
  4,5fold
  call assert_equal([-1, -1, -1, 4, 4, -1, -1],
        \           map(range(1, 7), 'foldclosed(v:val)'))

  " Click in "+" of inner fold in foldcolumn should open it.
  redraw
  let row = 4
  let col = 2
  call MouseLeftClick(row, col)
  call MouseLeftRelease(row, col)
  call assert_equal([-1, -1, -1, -1, -1, -1, -1],
        \           map(range(1, 7), 'foldclosed(v:val)'))

  " Click in "-" of outer fold in foldcolumn should close it.
  redraw
  let row = 2
  let col = 1
  call MouseLeftClick(row, col)
  call MouseLeftRelease(row, col)
  call assert_equal([-1, 2, 2, 2, 2, 2, -1],
        \           map(range(1, 7), 'foldclosed(v:val)'))
  norm! zR

  " Click in "|" of inner fold in foldcolumn should close it.
  redraw
  let row = 5
  let col = 2
  call MouseLeftClick(row, col)
  call MouseLeftRelease(row, col)
  call assert_equal([-1, -1, -1, 4, 4, -1, -1],
        \           map(range(1, 7), 'foldclosed(v:val)'))

  let &foldcolumn = save_foldcolumn
  let &ttymouse = save_ttymouse
  let &term = save_term
  let &mouse = save_mouse
  bwipe!
endfunc
