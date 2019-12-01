" Tests for 'virtualedit'.

func Test_yank_move_change()
  new
  call setline(1, [
	\ "func foo() error {",
	\ "\tif n, err := bar();",
	\ "\terr != nil {",
	\ "\t\treturn err",
	\ "\t}",
	\ "\tn = n * n",
	\ ])
  set virtualedit=all
  set ts=4
  function! MoveSelectionDown(count) abort
    normal! m`
    silent! exe "'<,'>move'>+".a:count
    norm! ``
  endfunction

  xmap ]e :<C-U>call MoveSelectionDown(v:count1)<CR>
  2
  normal 2gg
  normal J
  normal jVj
  normal ]e
  normal ce
  bwipe!
  set virtualedit=
  set ts=8
endfunc

func Test_paste_end_of_line()
  new
  set virtualedit=all
  call setline(1, ['456', '123'])
  normal! gg0"ay$
  exe "normal! 2G$lllA\<C-O>:normal! \"agP\r"
  call assert_equal('123456', getline(2))

  bwipe!
  set virtualedit=
endfunc

func Test_replace_end_of_line()
  new
  set virtualedit=all
  call setline(1, range(20))
  exe "normal! gg2jv10lr-"
  call assert_equal(["1", "-----------", "3"], getline(2,4))
  call setline(1, range(20))
  exe "normal! gg2jv10lr\<c-k>hh"
  call assert_equal(["1", "───────────", "3"], getline(2,4))

  bwipe!
  set virtualedit=
endfunc

func Test_edit_CTRL_G()
  new
  set virtualedit=insert
  call setline(1, ['123', '1', '12'])
  exe "normal! ggA\<c-g>jx\<c-g>jx"
  call assert_equal(['123', '1  x', '12 x'], getline(1,'$'))

  set virtualedit=all
  %d_
  call setline(1, ['1', '12'])
  exe "normal! ggllix\<c-g>jx"
  call assert_equal(['1 x', '12x'], getline(1,'$'))


  bwipe!
  set virtualedit=
endfunc

func Test_edit_change()
  new
  set virtualedit=all
  call setline(1, "\t⒌")
  normal Cx
  call assert_equal('x', getline(1))
  bwipe!
  set virtualedit=
endfunc

" Test for pasting before and after a tab character
func Test_paste_in_tab()
  new
  let @" = 'xyz'
  set virtualedit=all
  call append(0, "a\tb")
  call cursor(1, 2, 6)
  normal p
  " Behavior is different when 'vartabstop' is not supported.
  if has('vartabs')
    call assert_equal("a\txyzb", getline(1))
  else
    call assert_equal("a       xyzb", getline(1))
  endif
  call setline(1, "a\tb")
  call cursor(1, 2)
  normal P
  call assert_equal("axyz\tb", getline(1))

  " Test for virtual block paste
  call setreg('"', 'xyz', 'b')
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal p
  call assert_equal("a\txyzb", getline(1))
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal P
  call assert_equal("a      xyz b", getline(1))

  " Test for virtual block paste with gp and gP
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal gp
  call assert_equal("a\txyzb", getline(1))
  call assert_equal([0, 1, 6, 0, 12], getcurpos())
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal gP
  call assert_equal("a      xyz b", getline(1))
  call assert_equal([0, 1, 12, 0 ,12], getcurpos())

  bwipe!
  set virtualedit=
endfunc
