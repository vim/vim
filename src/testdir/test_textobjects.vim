" Test for textobjects

if !has('textobjects')
  finish
endif

set belloff=all
func CpoM(line, useM, expected)
  new

  if a:useM
    set cpoptions+=M
  else
    set cpoptions-=M
  endif

  call setline(1, a:line)

  call setreg('"', '')
  normal! ggfrmavi)y
  call assert_equal(getreg('"'), a:expected[0])

  call setreg('"', '')
  normal! `afbmavi)y
  call assert_equal(getreg('"'), a:expected[1])

  call setreg('"', '')
  normal! `afgmavi)y
  call assert_equal(getreg('"'), a:expected[2])

  q!
endfunc

func Test_inner_block_without_cpo_M()
  call CpoM('(red \(blue) green)', 0, ['red \(blue', 'red \(blue', ''])
endfunc

func Test_inner_block_with_cpo_M_left_backslash()
  call CpoM('(red \(blue) green)', 1, ['red \(blue) green', 'blue', 'red \(blue) green'])
endfunc

func Test_inner_block_with_cpo_M_right_backslash()
  call CpoM('(red (blue\) green)', 1, ['red (blue\) green', 'blue\', 'red (blue\) green'])
endfunc

func Test_quote_selection_selection_exclusive()
  new
  call setline(1, "a 'bcde' f")
  set selection=exclusive
  exe "norm! fdvhi'y"
  call assert_equal('bcde', @")
  set selection&vim
  bw!
endfunc
