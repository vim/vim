" Test for number and relativenumber
"
source view_util.vim

function! s:screen_lines(start, end) abort
  return ScreenLines([a:start, a:end], 8)
endfunction

function! s:compare_lines(expect, actual)
  call assert_equal(a:expect, a:actual)
endfunction

function! s:test_windows(h, w) abort
  call NewWindow(a:h, a:w)
endfunction

function! s:close_windows() abort
  call CloseWindow()
endfunction

function! s:validate_cursor() abort
  " update skipcol.
  " wincol():
  "   f_wincol
  "     -> validate_cursor
  "          -> curs_columns
  call wincol()
endfunction

func Test_number()
  call s:test_windows(10, 20)
  call setline(1, ["abcdefghij", "klmnopqrst", "uvwxyzABCD", "EFGHIJKLMN", "OPQRSTUVWX", "YZ"])
  setl number
  let lines = s:screen_lines(1, 4)
  let expect = [
\ "  1 abcd",
\ "  2 klmn",
\ "  3 uvwx",
\ "  4 EFGH",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

func Test_relativenumber()
  call s:test_windows(10, 20)
  call setline(1, ["abcdefghij", "klmnopqrst", "uvwxyzABCD", "EFGHIJKLMN", "OPQRSTUVWX", "YZ"])
  3
  setl relativenumber
  let lines = s:screen_lines(1, 6)
  let expect = [
\ "  2 abcd",
\ "  1 klmn",
\ "  0 uvwx",
\ "  1 EFGH",
\ "  2 OPQR",
\ "  3 YZ  ",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

func Test_number_with_relativenumber()
  call s:test_windows(10, 20)
  call setline(1, ["abcdefghij", "klmnopqrst", "uvwxyzABCD", "EFGHIJKLMN", "OPQRSTUVWX", "YZ"])
  4
  setl number relativenumber
  let lines = s:screen_lines(1, 6)
  let expect = [
\ "  3 abcd",
\ "  2 klmn",
\ "  1 uvwx",
\ "4   EFGH",
\ "  1 OPQR",
\ "  2 YZ  ",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

func Test_number_with_linewrap1()
  call s:test_windows(3, 20)
  normal! 61ia
  setl number wrap
  call s:validate_cursor()
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "--1 aaaa",
\ "    aaaa",
\ "    aaaa",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

" Pending: https://groups.google.com/forum/#!topic/vim_dev/tzNKP7EDWYI
func XTest_number_with_linewrap2()
  call s:test_windows(3, 20)
  normal! 61ia
  setl number wrap
  call s:validate_cursor()
  0
  call s:validate_cursor()
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "  1 aaaa",
\ "    aaaa",
\ "    aaaa",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

" Pending: https://groups.google.com/forum/#!topic/vim_dev/tzNKP7EDWYI
func XTest_number_with_linewrap3()
  call s:test_windows(4, 20)
  normal! 81ia
  setl number wrap
  call s:validate_cursor()
  setl nonumber
  call s:validate_cursor()
  let lines = s:screen_lines(1, 4)
  let expect = [
\ "aaaaaaaa",
\ "aaaaaaaa",
\ "aaaaaaaa",
\ "a       ",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc

func Test_numberwidth()
  call s:test_windows(10, 20)
  call setline(1, repeat(['aaaa'], 10))
  setl number numberwidth=6
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "    1 aa",
\ "    2 aa",
\ "    3 aa",
\ ]
  call s:compare_lines(expect, lines)

  set relativenumber
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "1     aa",
\ "    1 aa",
\ "    2 aa",
\ ]
  call s:compare_lines(expect, lines)

  set nonumber
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "    0 aa",
\ "    1 aa",
\ "    2 aa",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunction

func Test_numberwidth_adjusted()
  call s:test_windows(10, 20)
  call setline(1, repeat(['aaaa'], 10000))
  setl number numberwidth=4
  let lines = s:screen_lines(1, 3)
  let expect = [
\ "    1 aa",
\ "    2 aa",
\ "    3 aa",
\ ]
  call s:compare_lines(expect, lines)

  $
  let lines = s:screen_lines(8, 10)
  let expect = [
\ " 9998 aa",
\ " 9999 aa",
\ "10000 aa",
\ ]
  call s:compare_lines(expect, lines)

  setl relativenumber
  let lines = s:screen_lines(8, 10)
  let expect = [
\ "    2 aa",
\ "    1 aa",
\ "10000 aa",
\ ]
  call s:compare_lines(expect, lines)

  setl nonumber
  let lines = s:screen_lines(8, 10)
  let expect = [
\ "  2 aaaa",
\ "  1 aaaa",
\ "  0 aaaa",
\ ]
  call s:compare_lines(expect, lines)
  call s:close_windows()
endfunc
