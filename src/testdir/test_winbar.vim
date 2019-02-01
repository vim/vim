" Test WinBar

if !has('menu')
  finish
endif

func Test_add_remove_menu()
  new
  amenu 1.10 WinBar.Next :let g:did_next = 11<CR>
  amenu 1.20 WinBar.Cont :let g:did_cont = 12<CR>
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
