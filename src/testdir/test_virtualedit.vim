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

" Tests for pasting at the beginning, end and middle of a tab character
" in virtual edit mode.
func Test_paste_in_tab()
  new
  call append(0, '')
  set virtualedit=all

  " Tests for pasting a register with characterwise mode type
  call setreg('"', 'xyz', 'c')

  " paste (p) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal p
  call assert_equal('a xyz      b', getline(1))

  " paste (P) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal P
  call assert_equal("axyz\tb", getline(1))

  " paste (p) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal p
  call assert_equal("a\txyzb", getline(1))

  " paste (P) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal P
  call assert_equal('a      xyz b', getline(1))

  " Tests for pasting a register with blockwise mode type
  call setreg('"', 'xyz', 'b')

  " paste (p) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal p
  call assert_equal('a xyz      b', getline(1))

  " paste (P) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal P
  call assert_equal("axyz\tb", getline(1))

  " paste (p) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal p
  call assert_equal("a\txyzb", getline(1))

  " paste (P) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal P
  call assert_equal('a      xyz b', getline(1))

  " Tests for pasting with gp and gP in virtual edit mode

  " paste (gp) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal gp
  call assert_equal('a xyz      b', getline(1))
  call assert_equal([0, 1, 12, 0, 12], getcurpos())

  " paste (gP) unnamed register at the beginning of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 0)
  normal gP
  call assert_equal("axyz\tb", getline(1))
  call assert_equal([0, 1, 5, 0, 5], getcurpos())

  " paste (gp) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal gp
  call assert_equal("a\txyzb", getline(1))
  call assert_equal([0, 1, 6, 0, 12], getcurpos())

  " paste (gP) unnamed register at the end of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 6)
  normal gP
  call assert_equal('a      xyz b', getline(1))
  call assert_equal([0, 1, 12, 0, 12], getcurpos())

  " Tests for pasting a named register
  let @r = 'xyz'

  " paste (gp) named register in the middle of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 2)
  normal "rgp
  call assert_equal('a   xyz    b', getline(1))
  call assert_equal([0, 1, 8, 0, 8], getcurpos())

  " paste (gP) named register in the middle of a tab
  call setline(1, "a\tb")
  call cursor(1, 2, 2)
  normal "rgP
  call assert_equal('a  xyz     b', getline(1))
  call assert_equal([0, 1, 7, 0, 7], getcurpos())

  bwipe!
  set virtualedit=
endfunc

" Test for yanking a few spaces within a tab to a register
func Test_yank_in_tab()
  new
  let @r = ''
  call setline(1, "a\tb")
  set virtualedit=all
  call cursor(1, 2, 2)
  normal "ry5l
  call assert_equal('     ', @r)

  bwipe!
  set virtualedit=
endfunc

" vim: shiftwidth=2 sts=2 expandtab
