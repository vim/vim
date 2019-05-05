" Test WinBar

if !has('menu')
  finish
endif

source shared.vim

func Test_add_remove_menu()
  new
  amenu 1.10 WinBar.Next :let g:did_next = 11<CR>
  amenu 1.20 WinBar.Cont :let g:did_cont = 12<CR>
  redraw
  call assert_match('Next    Cont', Screenline(1))

  emenu WinBar.Next
  call assert_equal(11, g:did_next)
  emenu WinBar.Cont
  call assert_equal(12, g:did_cont)

  wincmd w
  call assert_fails('emenu WinBar.Next', 'E334')
  wincmd p

  aunmenu WinBar.Next
  aunmenu WinBar.Cont
  close
endfunc

" Create a WinBar with three buttons.
" Columns of the button edges:
" _Next_  _Cont_  _Close_
" 2    7  10  15  18   24
func SetupWinbar()
  amenu 1.10 WinBar.Next :let g:did_next = 11<CR>
  amenu 1.20 WinBar.Cont :let g:did_cont = 12<CR>
  amenu 1.30 WinBar.Close :close<CR>
  redraw
  call assert_match('Next    Cont    Close', Screenline(1))
endfunc

func Test_click_in_winbar()
  new
  call SetupWinbar()
  let save_mouse = &mouse
  set mouse=a

  let g:did_next = 0
  let g:did_cont = 0
  for col in [1, 8, 9, 16, 17, 25, 26]
    call test_setmouse(1, col)
    call feedkeys("\<LeftMouse>", "xt")
    call assert_equal(0, g:did_next, 'col ' .. col)
    call assert_equal(0, g:did_cont, 'col ' .. col)
  endfor

  for col in range(2, 7)
    let g:did_next = 0
    call test_setmouse(1, col)
    call feedkeys("\<LeftMouse>", "xt")
    call assert_equal(11, g:did_next, 'col ' .. col)
  endfor

  for col in range(10, 15)
    let g:did_cont = 0
    call test_setmouse(1, col)
    call feedkeys("\<LeftMouse>", "xt")
    call assert_equal(12, g:did_cont, 'col ' .. col)
  endfor

  let wincount = winnr('$')
  call test_setmouse(1, 20)
  call feedkeys("\<LeftMouse>", "xt")
  call assert_equal(wincount - 1, winnr('$'))

  let &mouse = save_mouse
endfunc

func Test_click_in_other_winbar()
  new
  call SetupWinbar()
  let save_mouse = &mouse
  set mouse=a
  let winid = win_getid()

  split
  let [row, col] = win_screenpos(winid)

  " Click on Next button in other window
  let g:did_next = 0
  call test_setmouse(row, 5)
  call feedkeys("\<LeftMouse>", "xt")
  call assert_equal(11, g:did_next)

  " Click on Cont button in other window from Visual mode
  let g:did_cont = 0
  call setline(1, 'select XYZ here')
  call test_setmouse(row, 12)
  call feedkeys("0fXvfZ\<LeftMouse>x", "xt")
  call assert_equal(12, g:did_cont)
  call assert_equal('select  here', getline(1))

  " Click on Close button in other window
  let wincount = winnr('$')
  let winid = win_getid()
  call test_setmouse(row, 20)
  call feedkeys("\<LeftMouse>", "xt")
  call assert_equal(wincount - 1, winnr('$'))
  call assert_equal(winid, win_getid())

  bwipe!
endfunc
