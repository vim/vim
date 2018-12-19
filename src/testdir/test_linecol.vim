" Tests for position expressions: '.', '$', 'w0', 'w$', etc.

func Test_position_expr_with_offset()
  5new
  call setline(1, map(range(1, 20), 'printf("#line%02d#", v:val)'))

  " line() and getline()
  normal! 10gg

  " . : the cursor position
  call assert_equal(10, line('.'))
  call assert_equal('#line10#', getline('.'))
  call assert_equal(5, line('.-5'))
  call assert_equal('#line05#', getline('.-5'))
  call assert_equal(13, line('.+3'))
  call assert_equal('#line13#', getline('.+3'))

  " $ : the last line in the current buffer
  call assert_equal(20, line('$'))
  call assert_equal('#line20#', getline('$'))
  call assert_equal(18, line('$-2'))
  call assert_equal('#line18#', getline('$-2'))
  call assert_equal(21, line('$+1'))
  call assert_equal('', getline('$+1'))

  " 'x : position of mark x (if the mark is not set, 0 is returned)
  mark a
  normal! gg
  call assert_equal(10, line("'a"))
  call assert_equal('#line10#', getline("'a"))
  call assert_equal(6, line("'a-4"))
  call assert_equal('#line06#', getline("'a-4"))
  call assert_equal(14, line("'a+4"))
  call assert_equal('#line14#', getline("'a+4"))
  normal! 'a
  delmarks a

  " w0 : first line visible in current window
  call assert_equal(8, line('w0'))
  call assert_equal('#line08#', getline('w0'))
  call assert_equal(5, line('w0-3'))
  call assert_equal('#line05#', getline('w0-3'))
  call assert_equal(9, line('w0+1'))
  call assert_equal('#line09#', getline('w0+1'))

  " w$ : last line visible in current window
  call assert_equal(12, line('w$'))
  call assert_equal('#line12#', getline('w$'))
  call assert_equal(9, line('w$-3'))
  call assert_equal('#line09#', getline('w$-3'))
  call assert_equal(14, line('w$+2'))
  call assert_equal('#line14#', getline('w$+2'))

  " v : In Visual mode: the start of the Visual area
  call assert_equal(10, line('v'))
  call assert_equal('#line10#', getline('v'))
  normal! kv4j
  call assert_equal(9, line('v'))
  call assert_equal('#line09#', getline('v'))
  " when using 'v', offset suffix is invalid
  call assert_equal(0, line('v+1'))
  call assert_equal('', getline('v+1'))

  " confirm invalid suffix has no effect
  call assert_equal(13, line('.@@@'))
  call assert_equal('#line13#', getline('.@@@'))

  " col()
  normal! gg3l

  " . : the cursor position
  call assert_equal(4, col('.'))
  call assert_equal(2, col('.-2'))
  call assert_equal(7, col('.+3'))

  " $ : the end of the cursor line
  call assert_equal(9, col('$'))
  call assert_equal(6, col('$-3'))
  call assert_equal(10, col('$+1'))

  bwipeout!
endfunc
